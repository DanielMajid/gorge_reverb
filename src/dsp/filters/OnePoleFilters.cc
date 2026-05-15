#include "OnePoleFilters.hpp"
#include <cassert>

OnePoleLPFilter::OnePoleLPFilter(float cutoffFreq, float initSampleRate) {
    setSampleRate(initSampleRate);
    setCutoffFreq(cutoffFreq);
}

float OnePoleLPFilter::process() {
    _z =  _a * input + _z * _b;
    output = _z;
    return output;
}

void OnePoleLPFilter::clear() {
    input = 0.0f;
    _z = 0.0f;
    output = 0.0f;
}

void OnePoleLPFilter::setSampleRate(float sampleRate) {
    assert(sampleRate > 0.0f);

    _sampleRate = sampleRate;
    _1_sampleRate = 1.0f / sampleRate;
    _maxCutoffFreq = sampleRate / 2.0f - 1.0f;

    // During object construction _cutoffFreq may still be 0.0f.
    // Defer coefficient update until a valid cutoff is provided.
    if (_cutoffFreq > 0.0f) {
        setCutoffFreq(_cutoffFreq);
    }
}

void OnePoleLPFilter::setCutoffFreq(float cutoffFreq) {
    if (cutoffFreq == _cutoffFreq) {
        return;
    }

    assert(cutoffFreq > 0.0f);
    assert(cutoffFreq <= _maxCutoffFreq);

    _cutoffFreq = cutoffFreq;
    _b = expf(-_2M_PI * _cutoffFreq * _1_sampleRate);
    _a = 1.0f - _b;
}

float OnePoleLPFilter::getMaxCutoffFreq() const {
    return _maxCutoffFreq;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

OnePoleHPFilter::OnePoleHPFilter(float initCutoffFreq, float initSampleRate) {
    setSampleRate(initSampleRate);
    setCutoffFreq(initCutoffFreq);
    clear();
}

float OnePoleHPFilter::process() {
    _x0 = input;
    _y0 = _a0 * _x0 + _a1 * _x1 + _b1 * _y1;
    _y1 = _y0;
    _x1 = _x0;
    output = _y0;
    return _y0;
}

void OnePoleHPFilter::clear() {
    input = 0.0f;
    output = 0.0f;
    _x0 = 0.0f;
    _x1 = 0.0f;
    _y0 = 0.0f;
    _y1 = 0.0f;
}

void OnePoleHPFilter::setCutoffFreq(float cutoffFreq) {
    if (cutoffFreq == _cutoffFreq) {
        return;
    }

    assert(cutoffFreq > 0.0f);
    assert(cutoffFreq <= _maxCutoffFreq);

    _cutoffFreq = cutoffFreq;
    _b1 = expf(-_2M_PI * _cutoffFreq * _1_sampleRate);
    _a0 = (1.0f + _b1) / 2.0f;
    _a1 = -_a0;
}

void OnePoleHPFilter::setSampleRate(float sampleRate) {
    assert(sampleRate > 0.0f);

    _sampleRate = sampleRate;
    _1_sampleRate = 1.0f / _sampleRate;
    _maxCutoffFreq = sampleRate / 2.0f - 1.0f;

    // During object construction _cutoffFreq may still be 0.0f.
    // Defer coefficient update until a valid cutoff is provided.
    if (_cutoffFreq > 0.0f) {
        setCutoffFreq(_cutoffFreq);
    }

    clear();
}

DCBlocker::DCBlocker() {
    setSampleRate(44100.0f);
    setCutoffFreq(20.f);
    clear();
}

DCBlocker::DCBlocker(float cutoffFreq) {
    setSampleRate(44100.0f);
    setCutoffFreq(cutoffFreq);
    clear();
}

float DCBlocker::process(float input) {
    output = input - _z + _b * output;
    _z = input;
    return output;
}

void DCBlocker::clear() {
    _z = 0.0f;
    output = 0.0f;
}

void DCBlocker::setSampleRate(float sampleRate) {
    _sampleRate = sampleRate;
    _maxCutoffFreq = sampleRate / 2.0f;
    setCutoffFreq(_cutoffFreq);
}

void DCBlocker::setCutoffFreq(float cutoffFreq) {
    _cutoffFreq = cutoffFreq;
    _b = 0.999f;
}

float DCBlocker::getMaxCutoffFreq() const {
    return _maxCutoffFreq;
}
