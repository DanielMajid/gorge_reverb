#pragma once
#include <cmath>
#include <cstdint>

#define _1_FACT_2 0.5f
#define _1_FACT_3 0.1666666667f
#define _1_FACT_4 0.04166666667f
#define _1_FACT_5 0.008333333333f
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define _2M_PI (2.0f * M_PI)

template<typename T>
T fastexp(T x) {
    T xx = x * x;
    T x3 = x * xx;
    T x4 = xx * xx;
    T x5 = x4 * x;
    x = 1 + x + (xx * _1_FACT_2) + (x3 * _1_FACT_3) + (x4 * _1_FACT_4);
    return x + (x5 * _1_FACT_5);
}

class OnePoleLPFilter {
public:
    OnePoleLPFilter(float cutoffFreq = 22049.0f, float initSampleRate = 44100.0f);
    float process();
    void clear();
    void setCutoffFreq(float cutoffFreq);
    void setSampleRate(float sampleRate);
    float getMaxCutoffFreq() const;
    float input = 0.0f;
    float output = 0.0f;
private:
    float _sampleRate = 44100.0f;
    float _1_sampleRate = 1.0f / _sampleRate;
    float _cutoffFreq = 0.0f;
    float _maxCutoffFreq = _sampleRate / 2.0f;
    float _a = 0.0f;
    float _b = 0.0f;
    float _z = 0.0f;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

class OnePoleHPFilter {
public:
    OnePoleHPFilter(float initCutoffFreq = 10.0f, float initSampleRate = 44100.0f);
    float process();
    void clear();
    void setCutoffFreq(float cutoffFreq);
    void setSampleRate(float sampleRate);
    float input = 0.0f;
    float output = 0.0f;
private:
    float _sampleRate = 0.0f;
    float _1_sampleRate = 0.0f;
    float _cutoffFreq = 0.0f;
    float _maxCutoffFreq = _sampleRate / 2.0f - 1.0f;
    float _y0 = 0.0f;
    float _y1 = 0.0f;
    float _x0 = 0.0f;
    float _x1 = 0.0f;
    float _a0 = 0.0f;
    float _a1 = 0.0f;
    float _b1 = 0.0f;
};


