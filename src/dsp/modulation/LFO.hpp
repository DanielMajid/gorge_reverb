#ifndef DSJ_LFO_HPP
#define DSJ_LFO_HPP
#include <vector>
#include <cmath>
#include <cstdint>

class LFO {
public:
    float output = 0.0f;
    float phase = 0.0f;

    LFO() {
        _frequency = 1.0f;
        _sampleRate = 44100.0f;
        _stepSize = _frequency * (float)kTableLength / _sampleRate;
        for(auto i = 0; i < kTableLength; ++i) {
            _sine.push_back(std::sin(2.0f * (float)M_PI * (float)i / (float)kTableLength));
        }
        _phasor = 0.0f;
    }

    float process() {
        _plusPhase = _phasor + phase * kTableLength;
        if(_plusPhase < 0.0f) {
            _plusPhase += kTableLength;
        }
        else if(_plusPhase >= kTableLength) {
            _plusPhase -= kTableLength;
        }

        _a = (long)_plusPhase;
        _frac = _plusPhase - _a;
        _b = _a + 1;
        _b %= kTableLength;
        output = _sine[_a] * (1.0f - _frac) + _sine[_b] * _frac;

        _phasor += _stepSize;
        if(_phasor >= kTableLength) {
            _phasor -= kTableLength;
        }
        return output;
    }

    void setFrequency(float frequency) {
        _frequency = frequency;
        calcStepSize();
    }
    void setSamplerate(float sampleRate) {
        _sampleRate = sampleRate;
        calcStepSize();
    }
private:
    float _frequency;
    float _sampleRate;
    float _stepSize;
    float _phasor;
    float _plusPhase;
    long _a, _b;
    float _frac;
    const long kTableLength = 4096;
    std::vector<float> _sine;

    void calcStepSize() {
        _stepSize = _frequency * (float)kTableLength / _sampleRate;
    }
};

class TriSawLFO {
public:
    TriSawLFO(float sampleRate = 44100.0f, float frequency = 1.0f) {
        phase = 0.0f;
        _output = 0.0f;
        _sampleRate = sampleRate;
        _step = 0.0f;
        _rising = true;
        setFrequency(frequency);
        setRevPoint(0.5f);
    }

    float process() {
        if(_step > 1.0f) {
            _step -= 1.0f;
            _rising = true;
        }

        if(_step >= _revPoint) {
            _rising = false;
        }

        if(_rising) {
            _output = _step * _riseRate;
        }
        else {
            _output = _step * _fallRate - _fallRate;
        }

        _step += _stepSize;
        _output *= 2.0f;
        _output -= 1.0f;
        return _output;
    }

    void setFrequency(float frequency) {
        if (frequency == _frequency) {
            return;
        }
        _frequency = frequency;
        calcStepSize();
    }

    void setRevPoint(float revPoint) {
        _revPoint = revPoint;
        if(_revPoint < 0.0001) {
            _revPoint = 0.0001;
        }
        if(_revPoint > 0.999) {
            _revPoint = 0.999;
        }

        _riseRate = 1.0f / _revPoint;
        _fallRate = -1.0f / (1.0f - _revPoint);
    }

    void setSamplerate(float sampleRate) {
        _sampleRate = sampleRate;
        calcStepSize();
    }

    float getOutput() const {
        return _output;
    }

    float phase;

private:
    float _output;
    float _sampleRate;
    float _frequency = 0.0f;
    float _revPoint;
    float _riseRate;
    float _fallRate;
    float _step;
    float _stepSize;
    bool _rising;

    void calcStepSize() {
        _stepSize = _frequency / _sampleRate;
    }
};

#endif
