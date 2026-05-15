#include "Dattorro.hpp"
#include <cassert>
#include <algorithm>
#include <cmath>

static inline float fast_exp2(float x)
{
    // 5th-order exp approximation on ln-domain. Accurate enough for control-rate pitch mapping.
    constexpr float kLn2 = 0.69314718056f;
    const float t = x * kLn2;
    const float t2 = t * t;
    const float t3 = t2 * t;
    const float t4 = t2 * t2;
    const float t5 = t4 * t;
    return 1.0f + t + 0.5f * t2 + 0.16666667f * t3 + 0.041666667f * t4 + 0.008333333f * t5;
}

static inline float pitch_to_hz(float pitch)
{
    float ratio = fast_exp2(pitch - 5.0f);

    // Polynomial approximation can undershoot below zero for low inputs.
    // Fall back to accurate exp2f to keep filter cutoff domains valid.
    if (ratio <= 0.0f) {
        ratio = exp2f(pitch - 5.0f);
    }

    const float hz = 440.0f * ratio;
    return hz > 1.0e-3f ? hz : 1.0e-3f;
}

Dattorro1997Tank::Dattorro1997Tank(const float initSampleRate,
                                   const float initMaxLfoDepth,
                                   const float initMaxTimeScale) :
    maxTimeScale(initMaxTimeScale) 
{
    timePadding = initMaxLfoDepth;
    setSampleRate(initSampleRate);

    leftOutDCBlock.setCutoffFreq(20.0);
    rightOutDCBlock.setCutoffFreq(20.0);

    lfo1.setFrequency(lfo1Freq);
    lfo2.setFrequency(lfo2Freq);
    lfo3.setFrequency(lfo3Freq);
    lfo4.setFrequency(lfo4Freq);

    lfo1.setRevPoint(0.5);
    lfo2.setRevPoint(0.5);
    lfo3.setRevPoint(0.5);
    lfo4.setRevPoint(0.5);
}

void Dattorro1997Tank::process(const float leftIn, const float rightIn,
                               float* leftOut, float* rightOut) {
    tickApfModulation();

    decay = frozen ? 1.0f : decayParam;

    leftSum += leftIn;
    rightSum += rightIn;

    leftApf1.input = leftSum;
    leftDelay1.input = leftApf1.process();
    leftDelay1.process();
    leftHighCutFilter.input = leftDelay1.output;
    leftLowCutFilter.input = leftHighCutFilter.process();
    leftApf2.input = (leftDelay1.output * (1.0f - fade) + leftLowCutFilter.process() * fade) * decay;
    leftDelay2.input = leftApf2.process();
    leftDelay2.process();

    rightApf1.input = rightSum;
    rightDelay1.input = rightApf1.process();
    rightDelay1.process();
    rightHighCutFilter.input = rightDelay1.output;
    rightLowCutFilter.input =  rightHighCutFilter.process();
    rightApf2.input = (rightDelay1.output * (1.0f - fade) + rightLowCutFilter.process() * fade) * decay;
    rightDelay2.input = rightApf2.process();
    rightDelay2.process();

    rightSum = leftDelay2.output * decay;
    leftSum = rightDelay2.output * decay;

    leftOutDCBlock.input = leftApf1.output;
    leftOutDCBlock.input += leftDelay1.tap(scaledOutputTaps[L_DELAY_1_L_TAP_1]);
    leftOutDCBlock.input += leftDelay1.tap(scaledOutputTaps[L_DELAY_1_L_TAP_2]);
    leftOutDCBlock.input -= leftApf2.delay.tap(scaledOutputTaps[L_APF_2_L_TAP]);
    leftOutDCBlock.input += leftDelay2.tap(scaledOutputTaps[L_DELAY_2_L_TAP]);
    leftOutDCBlock.input -= rightDelay1.tap(scaledOutputTaps[R_DELAY_1_L_TAP]);
    leftOutDCBlock.input -= rightApf2.delay.tap(scaledOutputTaps[R_APF_2_L_TAP]);
    leftOutDCBlock.input -= rightDelay2.tap(scaledOutputTaps[R_DELAY_2_L_TAP]);

    rightOutDCBlock.input = rightApf1.output;
    rightOutDCBlock.input += rightDelay1.tap(scaledOutputTaps[R_DELAY_1_R_TAP_1]);
    rightOutDCBlock.input += rightDelay1.tap(scaledOutputTaps[R_DELAY_1_R_TAP_2]);
    rightOutDCBlock.input -= rightApf2.delay.tap(scaledOutputTaps[R_APF_2_R_TAP]);
    rightOutDCBlock.input += rightDelay2.tap(scaledOutputTaps[R_DELAY_2_R_TAP]);
    rightOutDCBlock.input -= leftDelay1.tap(scaledOutputTaps[L_DELAY_1_R_TAP]);
    rightOutDCBlock.input -= leftApf2.delay.tap(scaledOutputTaps[L_APF_2_R_TAP]);
    rightOutDCBlock.input -= leftDelay2.tap(scaledOutputTaps[L_DELAY_2_R_TAP]);

    *leftOut = leftOutDCBlock.process() * 0.5f;
    *rightOut = rightOutDCBlock.process() * 0.5f;

    fade += fadeStep * fadeDir;
    fade = (fade < 0.0f) ? 0.0f : ((fade > 1.0f) ? 1.0f : fade);

    assert(fade >= 0.0);
    assert(fade <= 1.0);
}

void Dattorro1997Tank::freeze(bool freezeFlag) {
    frozen = freezeFlag;
    if (frozen) {
        fadeDir = -1.0f;
        decay = 1.0f;
    }
    else {
        fadeDir = 1.0f;
        decay = decayParam;
    }
}

void Dattorro1997Tank::setSampleRate(const float newSampleRate) {
    sampleRate = newSampleRate;
    sampleRate = sampleRate > maxSampleRate ? maxSampleRate : sampleRate;
    sampleRate = sampleRate < 1.f ? 1.f : sampleRate;
    sampleRateScale = sampleRate / dattorroSampleRate;

    fadeStep = 1.0f / sampleRate;

    leftOutDCBlock.setSampleRate(sampleRate);
    rightOutDCBlock.setSampleRate(sampleRate);

    rescaleTapTimes();
    setTimeScale(timeScale);
    initialiseDelaysAndApfs();
    clear();
}

void Dattorro1997Tank::setTimeScale(const float newTimeScale) {
    timeScale = newTimeScale;
    timeScale = timeScale < minTimeScale ? minTimeScale : timeScale;

    rescaleApfAndDelayTimes();
}

void Dattorro1997Tank::setDecay(const float newDecay) {
    decayParam = (newDecay > 1.0f ? 1.0f :
                 (newDecay < 0.0f ? 0.0f : newDecay));
}

void Dattorro1997Tank::setModSpeed(const float newModSpeed) {
    lfo1.setFrequency(lfo1Freq * newModSpeed);
    lfo2.setFrequency(lfo2Freq * newModSpeed);
    lfo3.setFrequency(lfo3Freq * newModSpeed);
    lfo4.setFrequency(lfo4Freq * newModSpeed);
}

void Dattorro1997Tank::setModDepth(const float newModDepth) {
    lfoExcursion = newModDepth * lfoMaxExcursion * sampleRateScale;
}

void Dattorro1997Tank::setModShape(const float shape) {
    lfo1.setRevPoint(shape);
    lfo2.setRevPoint(shape);
    lfo3.setRevPoint(shape);
    lfo4.setRevPoint(shape);
}

void Dattorro1997Tank::setHighCutFrequency(const float frequency) {
    leftHighCutFilter.setCutoffFreq(frequency);
    rightHighCutFilter.setCutoffFreq(frequency);
}

void Dattorro1997Tank::setLowCutFrequency(const float frequency) {
    leftLowCutFilter.setCutoffFreq(frequency);
    rightLowCutFilter.setCutoffFreq(frequency);
}

void Dattorro1997Tank::setDiffusion(const float diffusion) {
    assert(diffusion >= 0.0f && diffusion <= 10.0f);

    float diffusion1 = (diffusion / 10.0f) * maxDiffusion1;
    float diffusion2 = (diffusion / 10.0f) * maxDiffusion2;

    leftApf1.setGain(-diffusion1);
    leftApf2.setGain(diffusion2);
    rightApf1.setGain(-diffusion1);
    rightApf2.setGain(diffusion2);
}

void Dattorro1997Tank::clear() {
    leftApf1.clear();
    leftDelay1.clear();
    leftHighCutFilter.clear();
    leftLowCutFilter.clear();
    leftApf2.clear();
    leftDelay2.clear();

    rightApf1.clear();
    rightDelay1.clear();
    rightHighCutFilter.clear();;
    rightLowCutFilter.clear();
    rightApf2.clear();
    rightDelay2.clear();

    leftOutDCBlock.clear();
    rightOutDCBlock.clear();

    leftSum = 0.0f;
    rightSum = 0.0f;
}

void Dattorro1997Tank::initialiseDelaysAndApfs() {
    auto maxScaledOutputTap = *std::max_element(scaledOutputTaps.begin(),
                                                scaledOutputTaps.end());
    auto calcMaxTime = [&](float delayTime) -> long {
        return (long)(sampleRateScale * (delayTime * maxTimeScale + 
                                         maxScaledOutputTap + timePadding));
    };

    const long kLeftApf1MaxTime = calcMaxTime(leftApf1Time);
    const long kLeftDelay1MaxTime = calcMaxTime(leftDelay1Time);
    const long kLeftApf2MaxTime = calcMaxTime(leftApf2Time);
    const long kLeftDelay2MaxTime = calcMaxTime(leftDelay2Time);
    const long kRightApf1MaxTime = calcMaxTime(rightApf1Time);
    const long kRightDelay1MaxTime = calcMaxTime(rightDelay1Time);
    const long kRightApf2MaxTime = calcMaxTime(rightApf2Time);
    const long kRightDelay2MaxTime = calcMaxTime(rightDelay2Time);

    leftApf1 = AllpassFilter<float>(kLeftApf1MaxTime);
    leftDelay1 = InterpDelay<float>(kLeftDelay1MaxTime);
    leftApf2 = AllpassFilter<float>(kLeftApf2MaxTime);
    leftDelay2 = InterpDelay<float>(kLeftDelay2MaxTime);
    rightApf1 = AllpassFilter<float>(kRightApf1MaxTime);
    rightDelay1 = InterpDelay<float>(kRightDelay1MaxTime);
    rightApf2 = AllpassFilter<float>(kRightApf2MaxTime);
    rightDelay2 = InterpDelay<float>(kRightDelay2MaxTime);
}

void Dattorro1997Tank::tickApfModulation() {
    leftApf1.delay.setDelayTime(lfo1.process() * lfoExcursion + scaledLeftApf1Time);
    leftApf2.delay.setDelayTime(lfo2.process() * lfoExcursion + scaledLeftApf2Time);
    rightApf1.delay.setDelayTime(lfo3.process() * lfoExcursion + scaledRightApf1Time);
    rightApf2.delay.setDelayTime(lfo4.process() * lfoExcursion + scaledRightApf2Time);
}

void Dattorro1997Tank::rescaleApfAndDelayTimes() {
    float scaleFactor = timeScale * sampleRateScale;

    scaledLeftApf1Time = leftApf1Time * scaleFactor;
    scaledLeftDelay1Time = leftDelay1Time * scaleFactor;
    scaledLeftApf2Time = leftApf2Time * scaleFactor;
    scaledLeftDelay2Time = leftDelay2Time * scaleFactor;

    scaledRightApf1Time = rightApf1Time * scaleFactor;
    scaledRightDelay1Time = rightDelay1Time * scaleFactor;
    scaledRightApf2Time = rightApf2Time * scaleFactor;
    scaledRightDelay2Time = rightDelay2Time * scaleFactor;

    leftDelay1.setDelayTime(scaledLeftDelay1Time);
    leftDelay2.setDelayTime(scaledLeftDelay2Time);
    rightDelay1.setDelayTime(scaledRightDelay1Time);
    rightDelay2.setDelayTime(scaledRightDelay2Time);
}

void Dattorro1997Tank::rescaleTapTimes() {
    static const long kOutputTaps[7] = {266, 2974, 1913, 1996, 1990, 187, 1066};
    for (size_t i = 0; i < scaledOutputTaps.size(); ++i) {
        scaledOutputTaps[i] = (long)((float)kOutputTaps[i] * sampleRateScale);
    }
}

bool Dattorro1997Tank::setDelayMemory(float *&cursor, size_t &remaining, InterpDelay<float> &delay) {
    const size_t len = static_cast<size_t>(delay.getMaxLength());
    if (remaining < len) {
        return false;
    }
    delay.setMemory(cursor, len);
    cursor += len;
    remaining -= len;
    return true;
}

size_t Dattorro1997Tank::getBufferSize() const {
    size_t total = 0;
    total += static_cast<size_t>(leftApf1.delay.getMaxLength());
    total += static_cast<size_t>(leftDelay1.getMaxLength());
    total += static_cast<size_t>(leftApf2.delay.getMaxLength());
    total += static_cast<size_t>(leftDelay2.getMaxLength());
    total += static_cast<size_t>(rightApf1.delay.getMaxLength());
    total += static_cast<size_t>(rightDelay1.getMaxLength());
    total += static_cast<size_t>(rightApf2.delay.getMaxLength());
    total += static_cast<size_t>(rightDelay2.getMaxLength());
    return total;
}

bool Dattorro1997Tank::setBuffer(float *&cursor, size_t &remaining) {
    if (!setDelayMemory(cursor, remaining, leftApf1.delay)) return false;
    if (!setDelayMemory(cursor, remaining, leftDelay1)) return false;
    if (!setDelayMemory(cursor, remaining, leftApf2.delay)) return false;
    if (!setDelayMemory(cursor, remaining, leftDelay2)) return false;
    if (!setDelayMemory(cursor, remaining, rightApf1.delay)) return false;
    if (!setDelayMemory(cursor, remaining, rightDelay1)) return false;
    if (!setDelayMemory(cursor, remaining, rightApf2.delay)) return false;
    if (!setDelayMemory(cursor, remaining, rightDelay2)) return false;
    return true;
}

Dattorro::Dattorro(const float initMaxSampleRate,
                   const float initMaxLfoDepth,
                   const float initMaxTimeScale)
    : tank(initMaxSampleRate, initMaxLfoDepth, initMaxTimeScale)
{
    sampleRate = initMaxSampleRate;
    dattorroScaleFactor = sampleRate / dattorroSampleRate;

    preDelay = InterpDelay<float>(192010, 0);

    inputLpf = OnePoleLPFilter(22000.0);
    inputHpf = OnePoleHPFilter(13.75);

    inApf1 = AllpassFilter<float>(dattorroScale(8 * kInApf1Time), dattorroScale(kInApf1Time), inputDiffusion1);
    inApf2 = AllpassFilter<float>(dattorroScale(8 * kInApf2Time), dattorroScale(kInApf2Time), inputDiffusion1);
    inApf3 = AllpassFilter<float>(dattorroScale(8 * kInApf3Time), dattorroScale(kInApf3Time), inputDiffusion2);
    inApf4 = AllpassFilter<float>(dattorroScale(8 * kInApf4Time), dattorroScale(kInApf4Time), inputDiffusion2);

    leftInputDCBlock.setCutoffFreq(20.0);
    rightInputDCBlock.setCutoffFreq(20.0);
}

void Dattorro::process(float leftInput, float rightInput) {
    constexpr float kStereoSideInjection = 0.5f;

    leftInputDCBlock.input = leftInput;
    rightInputDCBlock.input = rightInput;
    inputLpf.setCutoffFreq(inputHighCut);
    inputHpf.setCutoffFreq(inputLowCut);

    const float leftDc = leftInputDCBlock.process();
    const float rightDc = rightInputDCBlock.process();
    const float mid = 0.5f * (leftDc + rightDc);
    const float side = 0.5f * (leftDc - rightDc);

    inputLpf.input = mid;
    inputHpf.input = inputLpf.process();
    inputHpf.process();
    preDelay.input = inputHpf.output;
    preDelay.process();
    inApf1.input = preDelay.output;
    inApf2.input = inApf1.process();
    inApf3.input = inApf2.process();
    inApf4.input = inApf3.process();
    tankFeed = preDelay.output * (1.0f - diffuseInput) + inApf4.process() * diffuseInput;

    const float tankLeftIn = tankFeed + side * kStereoSideInjection;
    const float tankRightIn = tankFeed - side * kStereoSideInjection;
    tank.process(tankLeftIn, tankRightIn, &leftOut, &rightOut);
}

void Dattorro::clear() {
    leftInputDCBlock.clear();
    rightInputDCBlock.clear();

    inputLpf.clear();
    inputHpf.clear();
    preDelay.clear();
    inApf1.clear();
    inApf2.clear();
    inApf3.clear();
    inApf4.clear();

    tank.clear();
}

void Dattorro::setTimeScale(float timeScale) {
    constexpr float minTimeScale = 0.0001f;
    timeScale = timeScale < minTimeScale ? minTimeScale : timeScale;
    tank.setTimeScale(timeScale);
}

void Dattorro::setPreDelay(float t) {
    preDelayTime = t;
    preDelay.setDelayTime(preDelayTime * sampleRate);
}

void Dattorro::setSampleRate(float newSampleRate) {
    assert(newSampleRate > 0.f);

    sampleRate = newSampleRate;
    tank.setSampleRate(sampleRate);
    dattorroScaleFactor = sampleRate / dattorroSampleRate;
    setPreDelay(preDelayTime);
    inApf1.delay.setDelayTime(dattorroScale(kInApf1Time));
    inApf2.delay.setDelayTime(dattorroScale(kInApf2Time));
    inApf3.delay.setDelayTime(dattorroScale(kInApf3Time));
    inApf4.delay.setDelayTime(dattorroScale(kInApf4Time));

    leftInputDCBlock.setSampleRate(sampleRate);
    rightInputDCBlock.setSampleRate(sampleRate);
    inputLpf.setSampleRate(sampleRate);
    inputHpf.setSampleRate(sampleRate);

    clear();
}

void Dattorro::freeze(bool freezeFlag) {
    tank.freeze(freezeFlag);
}

void Dattorro::setInputFilterLowCutoffPitch(float pitch) {
    inputLowCut = pitch_to_hz(pitch);
}

void Dattorro::setInputFilterHighCutoffPitch(float pitch) {
    inputHighCut = pitch_to_hz(pitch);
}

void Dattorro::enableInputDiffusion(bool enable) {
    diffuseInput = enable ? 1.0f : 0.0f;
}

void Dattorro::setDecay(float newDecay) {
    assert(newDecay <= 1.0f);
    tank.setDecay(newDecay);
}

void Dattorro::setTankDiffusion(const float diffusion) {
    tank.setDiffusion(diffusion);
}

void Dattorro::setTankFilterHighCutFrequency(const float pitch) {
    float frequency = pitch_to_hz(pitch);
    tank.setHighCutFrequency(frequency);
}

void Dattorro::setTankFilterLowCutFrequency(const float pitch) {
    float frequency = pitch_to_hz(pitch);
    tank.setLowCutFrequency(frequency);
}

void Dattorro::setTankModSpeed(const float modSpeed) {
    tank.setModSpeed(modSpeed);
}

void Dattorro::setTankModDepth(const float modDepth) {
    tank.setModDepth(modDepth);
}

void Dattorro::setTankModShape(const float modShape) {
    tank.setModShape(modShape);
}

size_t Dattorro::getBufferSize() const {
    size_t total = 0;
    total += static_cast<size_t>(preDelay.getMaxLength());
    total += static_cast<size_t>(inApf1.delay.getMaxLength());
    total += static_cast<size_t>(inApf2.delay.getMaxLength());
    total += static_cast<size_t>(inApf3.delay.getMaxLength());
    total += static_cast<size_t>(inApf4.delay.getMaxLength());
    total += tank.getBufferSize();
    return total;
}

bool Dattorro::setBuffer(float *buffer, size_t bufferSize) {
    if (!buffer) {
        return false;
    }

    float *cursor = buffer;
    size_t remaining = bufferSize;

    const auto setDelayMemory = [&](InterpDelay<float> &delay) -> bool {
        const size_t len = static_cast<size_t>(delay.getMaxLength());
        if (remaining < len) {
            return false;
        }
        delay.setMemory(cursor, len);
        cursor += len;
        remaining -= len;
        return true;
    };

    if (!setDelayMemory(preDelay)) return false;
    if (!setDelayMemory(inApf1.delay)) return false;
    if (!setDelayMemory(inApf2.delay)) return false;
    if (!setDelayMemory(inApf3.delay)) return false;
    if (!setDelayMemory(inApf4.delay)) return false;

    if (!tank.setBuffer(cursor, remaining)) return false;

    return true;
}

float Dattorro::getLeftOutput() const {
    return leftOut;
}

float Dattorro::getRightOutput() const {
    return rightOut;
}

float Dattorro::dattorroScale(float delayTime) {
    return delayTime * dattorroScaleFactor;
}

