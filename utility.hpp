#include <random>
#include <cmath>
#pragma once
float soft_clip(float x)
{
    if (x > 1.f)
        return 1.f;
    if (x < -1.f)
        return -1.f;
    return 3.f * x / 2.f - x * x * x / 2.f;
}

template <typename T>
T dbtoa(T db)
{
    return pow(T(10), db / T(20));
}

float mix(float a, float b, float t)
{
    return a * (1 - t) + b * t;
}

// random number from 0 to 1
float rand_num()
{
    return ((double)rand() / (RAND_MAX));
}

// random number from 0 to 1
float rand_num_new()
{
    std::random_device rd;  // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
    std::uniform_real_distribution<> dist(0, 1);
    return dist(gen);
}

// random number from 0 to end_time
float rand_num_new(float end_time)
{
    std::random_device rd;  // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
    std::uniform_real_distribution<> dist(0, end_time);
    return dist(gen);
}
// random number from start_time to end_time
float rand_num_new(float start_time,float end_time)
{
    std::random_device rd;  // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
    std::uniform_real_distribution<> dist(start_time, end_time);
    return dist(gen);
}

float Fast_InvSqrt(float number)
{
    long i;
    float x2, y;
    const float threehalfs = 1.5f;

    x2 = number * 0.5f;
    y = number;
    i = *(long *)&y;           // Floating point bit hack
    i = 0x5f3759df - (i >> 1); // Magic number
    y = *(float *)&i;
    y = y * (threehalfs - (x2 * y * y)); // Newton 1st iteration
                                         //  y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration (disabled)

    return y;
}

// valid when -1 <= x <= 1
float fast_acos(float x)
{
    if (x < -1)
        return 0.f;
    if (x > 1)
        return 0.f;

    float negate = float(x < 0);
    x = abs(x);
    float ret = -0.0187293;
    ret = ret * x;
    ret = ret + 0.0742610;
    ret = ret * x;
    ret = ret - 0.2121144;
    ret = ret * x;
    ret = ret + 1.5707288;
    ret = ret * sqrt(1.0 - x);
    ret = ret - 2 * negate * ret;
    return negate * 3.14159265358979 + ret;
}