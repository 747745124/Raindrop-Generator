#include "utility.hpp"
#include "drop_v2.hpp"

class Drops_v2
{
public:
    std::vector<Drop_v2> drops;
    uint num_drops;
    uint counter;

    // randomness is a number between 0 and 1
    // this generates a sampler between 0 to 4 seconds
    Drops_v2(float randomness = 0.5, uint density = 1000, float end_time = 1.f, float single_drop_interval = 1.0f)
    {
        this->num_drops = density * end_time;
        this->counter = 0;
        for (int _ = 0; _ < num_drops; _++)
        {
            drops.push_back(Drop_v2(rand_num_new(end_time), single_drop_interval * rand_num_new(0.002), 0.002 + single_drop_interval * rand_num_new(0.004), 0.006 + single_drop_interval * rand_num_new(0.006), 1.0f, 1.2f, 3.0, 3 + randomness * rand_num_new(12), 1000 + randomness * rand_num_new(1000)));
        }
    }

    float operator()()
    {
        float res = 0.0f;
        for (int i = 0; i < drops.size(); i++)
        {
            res += drops[i]();
        }
        return res;
    }
};