#include "drops_v2.hpp"
#include "plugin_processor.hpp"
#include <mutex>
#include <thread>

using namespace juce;
struct Raindrops : public AudioProcessor
{
  MonoChain leftChain, rightChain;
  using BlockType = juce::AudioBuffer<float>;
  using Filter = juce::dsp::IIR::Filter<float>;
  using Coefficients = Filter::CoefficientsPtr;
  using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
  using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

  SingleChannelSampleFifo<BlockType> leftChannelFifo{Channel::Left};
  SingleChannelSampleFifo<BlockType> rightChannelFifo{Channel::Right};

  AudioParameterFloat *noise_level;
  AudioParameterFloat *gain;
  AudioParameterFloat *freq_coeff;
  AudioParameterFloat *density;
  AudioParameterFloat *single_drop_interval;

  AudioParameterBool *HPF_enabled;
  AudioParameterFloat *HPF_freq;
  AudioParameterBool *LPF_enabled;
  AudioParameterFloat *LPF_freq;

  std::unique_ptr<Drops_v2> drops = std::make_unique<Drops_v2>(0.5, 100, 1.0f);
  float running_max = -20.f;

  Raindrops()
      : AudioProcessor(BusesProperties()
                           .withInput("Input", AudioChannelSet::stereo())
                           .withOutput("Output", AudioChannelSet::stereo()))
  {
    addParameter(gain = new AudioParameterFloat(
                     {"gain", 1}, "Gain",
                     NormalisableRange<float>(-65.f, -1.f, 0.01f), -65.f));
    addParameter(density = new AudioParameterFloat(
                     {"density", 1}, "Density",
                     NormalisableRange<float>(1.f, 400.f, 1.f), 10.f));
    addParameter(freq_coeff = new AudioParameterFloat(
                     {"randomness", 1}, "Freq Coeff",
                     NormalisableRange<float>(0.1f, 4.0f, 0.1f), 4.0f));
    addParameter(single_drop_interval = new AudioParameterFloat(
                     {"interval_coeff", 1}, "Interval Coeff",
                     NormalisableRange<float>(0.1f, 4.0f, 0.1f), 1.0f));
    addParameter(noise_level = new AudioParameterFloat(
                     {"noise_level", 1}, "Noise Level",
                     NormalisableRange<float>(0.00f, 0.01f, 0.001f), 0.0f));
    addParameter(HPF_freq = new AudioParameterFloat(
                     {"HPF Freq", 1}, "HPF Freq",
                     NormalisableRange<float>(1000.f, 20000.0f, 100.f), 100.0f));
    addParameter(LPF_freq = new AudioParameterFloat(
                     {"LPF Freq", 1}, "LPF Freq",
                     NormalisableRange<float>(100.f, 20000.0f, 100.f), 100.0f));
    addParameter(HPF_enabled = new AudioParameterBool(
                     {"HPF Enabled", 1}, "HPF Enabled", true));
    addParameter(LPF_enabled = new AudioParameterBool(
                     {"LPF Enabled", 1}, "LPF Enabled", true));
  }

  /// this function handles the audio ///////////////////////////////////////
  void processBlock(AudioBuffer<float> &buffer, MidiBuffer &) override
  {

    updateFilters();
    auto left = buffer.getWritePointer(0, 0);
    auto right = buffer.getWritePointer(1, 0);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {

      float res = 0.0f;
      res = drops->operator()();
      for (int k = 0; k < density->get(); ++k)
      {
        if (drops->drops[k].time > 1.012f)
          drops->drops[k].reset(1.0f, single_drop_interval->get(), freq_coeff->get());
      }

      if (fabs(res) > fabs(running_max))
      {
        running_max = res;
      }

      float noise = rand_num_new(-1.f, 1.f);
      left[i] = soft_clip(res * dbtoa(gain->get()) / fabs(running_max) + noise_level->get() * noise);
      right[i] = left[i];
    }

    // 1.wrap the buffer with audio block
    juce::dsp::AudioBlock<float> block(buffer);
    // 2.extract individual data
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);
    // 3.wrap the block to a context which the process chain could use
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
    // 4.pass the context to filter chains
    leftChain.process(leftContext);
    rightChain.process(rightContext);
    // no effects till now since there's no efficient set
    leftChannelFifo.update(buffer);
    rightChannelFifo.update(buffer);
  }

  /// start and shutdown callbacks///////////////////////////////////////////
  void prepareToPlay(double sampleRate, int samplesPerBlock) override
  {
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;
    // monoChain so numChannel to be 1

    leftChain.prepare(spec);
    rightChain.prepare(spec);
    updateFilters();

    // prepare fifo
    leftChannelFifo.prepare(samplesPerBlock);
    rightChannelFifo.prepare(samplesPerBlock);
  }

  void updateFilters()
  {
    auto chainSettings = getChainSettings();
    updateLowCutFilter(chainSettings);
    updateHighCutFilter(chainSettings);
  };

  void updateLowCutFilter(const ChainSettings &chainSettings)
  {

    auto lowCutCoefficients = makeLowCutFilter(chainSettings, getSampleRate());

    auto &leftLowCut = leftChain.get<ChainPositions::LowCut>();
    auto &rightLowCut = rightChain.get<ChainPositions::LowCut>();

    leftChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);
    rightChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);

    updateCutFilter(leftLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(rightLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
  }

  void updateHighCutFilter(const ChainSettings &chainSettings)
  {

    auto highCutCoefficients = makeHighCutFilter(chainSettings, getSampleRate());

    auto &leftHighCut = leftChain.get<ChainPositions::HighCut>();
    auto &rightHighCut = rightChain.get<ChainPositions::HighCut>();

    leftChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);
    rightChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);

    updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
    updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);
  }

  ChainSettings getChainSettings()
  {
    ChainSettings settings;
    settings.lowCutFreq = HPF_freq->get();
    settings.highCutFreq = LPF_freq->get();
    settings.highCutSlope = Slope::Slope_12;
    settings.lowCutSlope = Slope::Slope_12;
    settings.lowCutBypassed = !HPF_enabled->get();
    settings.highCutBypassed = !LPF_enabled->get();
    return settings;
  };

  void releaseResources() override
  {
  }

  /// maintaining persistant state on suspend ///////////////////////////////
  void getStateInformation(MemoryBlock &destData) override
  {
    MemoryOutputStream(destData, true).writeFloat(*gain);
    MemoryOutputStream(destData, true).writeFloat(*density);
    MemoryOutputStream(destData, true).writeFloat(*freq_coeff);
    MemoryOutputStream(destData, true).writeFloat(*single_drop_interval);
    /// add parameters here /////////////////////////////////////////////////
  }

  void setStateInformation(const void *data, int sizeInBytes) override
  {
    gain->setValueNotifyingHost(
        MemoryInputStream(data, static_cast<size_t>(sizeInBytes), false)
            .readFloat());
    density->setValueNotifyingHost(
        MemoryInputStream(data, static_cast<size_t>(sizeInBytes), false)
            .readFloat());
    freq_coeff->setValueNotifyingHost(
        MemoryInputStream(data, static_cast<size_t>(sizeInBytes), false)
            .readFloat());
    single_drop_interval->setValueNotifyingHost(
        MemoryInputStream(data, static_cast<size_t>(sizeInBytes), false)
            .readFloat());
    /// add parameters here /////////////////////////////////////////////////
  }

  /// do not change anything below this line, probably //////////////////////

  /// general configuration /////////////////////////////////////////////////
  const String getName() const override { return "Raindrops"; }
  double getTailLengthSeconds() const override { return 0; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }

  /// for handling presets //////////////////////////////////////////////////
  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const String getProgramName(int) override { return "None"; }
  void changeProgramName(int, const String &) override {}

  /// ?????? ////////////////////////////////////////////////////////////////
  bool isBusesLayoutSupported(const BusesLayout &layouts) const override
  {
    const auto &mainInLayout = layouts.getChannelSet(true, 0);
    const auto &mainOutLayout = layouts.getChannelSet(false, 0);

    return (mainInLayout == mainOutLayout && (!mainInLayout.isDisabled()));
  }

  /// automagic user interface //////////////////////////////////////////////
  AudioProcessorEditor *createEditor() override
  {
    // return new AudioPluginAudioProcessorEditor(*this);
    return new GenericAudioProcessorEditor(*this);
  }
  bool hasEditor() const override { return true; }

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Raindrops)
};

AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
  return new Raindrops();
}