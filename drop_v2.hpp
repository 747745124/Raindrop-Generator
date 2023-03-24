#pragma once
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <random>
#include "utility.hpp"

class Drop_v2
{
public:
    float time = 0.f;
    float t_init = 0.001f;
    float delta_t_1 = 0.002f;
    float delta_t_2 = 0.006f;
    float delta_t_3 = 0.012f;

    float A0 = 1.0f;
    float A1 = 20.0f;
    float k = 3.0f;
    float m = 6.f;
    float f = 50.0f;

    Drop_v2(float t_init = 0.001, float delta_t_1 = 0.002, float delta_t_2 = 0.006, float delta_t_3 = 0.012, float A0 = 1.0, float A1 = 1.20f, float k = 3.0, float m = 6.0, float f = 1500.0)
    {
        this->t_init = t_init;
        this->delta_t_1 = delta_t_1;
        this->delta_t_2 = delta_t_2;
        this->delta_t_3 = delta_t_3;
        this->A0 = A0;
        this->A1 = A1;
        this->k = k;
        this->m = m;
        this->f = f;
    }

    void reset(float end_time, float interval_coeff, float freq_coeff)
    {
        this->t_init = rand_num_new(end_time);
        this->delta_t_1 = interval_coeff * rand_num_new(0.002);
        this->delta_t_2 = 0.002 + interval_coeff * rand_num_new(0.004);
        this->delta_t_3 = 0.006 + interval_coeff * rand_num_new(0.006);
        this->A0 = 1.0f;
        this->A1 = 1.2f;
        this->k = 3.0f;
        this->m = 3 + freq_coeff * rand_num_new(12);
        this->f = 1000 + freq_coeff * rand_num_new(1000);
        this->time = 0.f;
    };

    float operator()()
    {
        float value = 0.f;

        if (time < t_init)
        {
            time += 1.0f / 44100.0f;
            return value;
        }
        if (time < (t_init + delta_t_1))
        {
            // t range from -1 to 1
            time += 1.0f / 44100.0f;
            float t = 2 * (time - t_init) / delta_t_1 - 1;
            return A0 * fast_acos(t * t) * 2 / M_PI;
        }

        if (time < (t_init + delta_t_2))
        {
            time += 1.0f / 44100.0f;
            return value;
        }
        if (time < (t_init + delta_t_3))
        {
            time += 1.0f / 44100.0f;
            value = exp(-m * (time - t_init - delta_t_2) / (delta_t_3 - delta_t_2)) * A1 * sin(2 * M_PI * f * (time - t_init - delta_t_2));
            return value;
        }

        time += 1.0f / 44100.0f;
        return value;
    }
};