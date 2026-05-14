#pragma once
/*
    BSD 3-Clause License

    Copyright (c) 2023, KORG INC.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//*/

/*
 *  File: reverb.h
 *
 *  Reverb effect unit backed by the src/Dattorro DSP.
 *
 */

#include "processor.h"
#include "unit_revfx.h"
#include "Dattorro.hpp"

class Reverb : public Processor
{
public:
  uint32_t getBufferSize() const override final { return 0U; }

  enum
  {
    SIZE = 0U,
    DECAY,
    MIX,
    MODE,
    PREDELAY,
    DIFFUSION,
    HIGH_CUT,
    MOD_DEPTH,
    MOD_RATE,
    FREEZE,
    NUM_PARAMS
  };

  struct Params
  {
    // Main controls
    float size;
    float decay;
    float mix;
    // Edit controls
    uint32_t mode;
    float predelay_ms;
    float diffusion;
    float high_cut_pitch;
    float mod_depth;
    float mod_rate;
    bool freeze;

    void reset()
    {
      size = 0.5f;
      decay = 0.5f;
      mix = 0.f;
      mode = MODE_BYPASS;
      predelay_ms = 40.f;
      diffusion = 7.f;
      high_cut_pitch = 8.5f;
      mod_depth = 6.f;
      mod_rate = 0.2f;
      freeze = false;
    }

    Params() { reset(); }
  };

  enum
  {
    MODE_BYPASS = 0,
    MODE_NORM,
    MODE_WASH,
    MODE_DARK,
    MODE_FREEZE,
    NUM_MODE_VALUES,
  };

  inline void setParameter(uint8_t index, int32_t value) override final
  {
    // Keep this conversion layer thin: just decode/normalize host values.
    // All curve shaping and cross-parameter behavior lives in updateEngineParams().
    switch (index)
    {
    case SIZE:
      params_.size = param_10bit_to_f32(value);
      break;

    case DECAY:
      params_.decay = param_10bit_to_f32(value);
      break;

    case MIX:
      params_.mix = value / 1000.f;
      break;

    case MODE:
      params_.mode = static_cast<uint32_t>(value);
      break;

    case PREDELAY:
      params_.predelay_ms = static_cast<float>(value);
      break;

    case DIFFUSION:
      params_.diffusion = static_cast<float>(value) * 0.1f;
      break;

    case HIGH_CUT:
      params_.high_cut_pitch = static_cast<float>(value) * 0.1f;
      break;

    case MOD_DEPTH:
      params_.mod_depth = static_cast<float>(value) * 0.1f;
      break;

    case MOD_RATE:
      params_.mod_rate = static_cast<float>(value) * 0.01f;
      break;

    case FREEZE:
      params_.freeze = (value != 0);
      break;

    default:
      break;
    }
  }

  inline const char *getParameterStrValue(uint8_t index, int32_t value) const override final
  {
    static const char *mode_strings[NUM_MODE_VALUES] = {
      "BYPS",
      "NORM",
      "WASH",
      "DARK",
      "FREEZ",
    };

    switch (index)
    {
    case MODE:
      if (value >= MODE_BYPASS && value < NUM_MODE_VALUES)
        return mode_strings[value];
      break;
    default:
      break;
    }

    return nullptr;
  }

  Reverb() : reverb(48000.0, 16.0, 1.0)
  {
    params_.reset();
  }

  void init(float *allocated_buffer) override final
  {
    (void)allocated_buffer;
    params_.reset();

    reverb.setSampleRate(getSampleRate());
    reverb.clear();
    updateEngineParams(params_);
  }

  void teardown() override final {}

  void reset() override final
  {
    reverb.clear();
    updateEngineParams(params_);
  }

  void process(const float *__restrict in, float *__restrict out, uint32_t frames) override final
  {
    const Params p = params_;
    // Recompute mapped coefficients per render call so UI moves are immediate.
    updateEngineParams(p);

    const float wet = p.mix < -1.f ? 0.f : (p.mix > 1.f ? 1.f : (p.mix + 1.f) * 0.5f);
    const float dry = 1.f - wet;

    for (const float *out_end = out + frames * 2; out != out_end; in += 2, out += 2)
    {
      reverb.process(in[0] * 0.5f, in[1] * 0.5f);

      out[0] = in[0] * dry + static_cast<float>(reverb.getLeftOutput()) * wet;
      out[1] = in[1] * dry + static_cast<float>(reverb.getRightOutput()) * wet;
    }
  }

private:
  struct ModeTuning
  {
    // MODE does not replace normal controls; it offsets the "character" voicing.
    bool diffuse_input;
    float input_low_pitch;
    float low_cut_pitch;
    float mod_shape;
    bool force_freeze;
  };

  static float clampf(float v, float lo, float hi)
  {
    return v < lo ? lo : (v > hi ? hi : v);
  }

  static ModeTuning getModeTuning(uint32_t mode)
  {
    switch (mode)
    {
    case MODE_BYPASS:
      // Neutral voicing: avoid extra character offsets and forced states.
      return {true, 0.0f, 0.0f, 0.50f, false};
    case MODE_NORM:
      return {true, 0.0f, 2.0f, 0.50f, false};
    case MODE_WASH:
      return {true, 0.0f, 1.0f, 0.35f, false};
    case MODE_DARK:
      return {true, 0.0f, 0.5f, 0.70f, false};
    case MODE_FREEZE:
    default:
      return {true, 0.0f, 0.0f, 0.50f, true};
    }
  }

  void updateEngineParams(const Params &p)
  {
    const ModeTuning m = getModeTuning(p.mode);

    // Plateau-like pre-delay domain: ms from UI, seconds for Dattorro.
    const float pre_delay_sec = clampf(p.predelay_ms * 0.001f, 0.f, 0.5f);

    // Plateau-like size taper: square response then remap to ~0.01..4.0.
    const float size_norm = clampf(p.size, 0.f, 1.f);
    const float size_sq = size_norm * size_norm;
    const float size = 0.01f + size_sq * (4.0f - 0.01f);

    // Plateau-like decay shaping: map knob to 0.1..0.9999 then warp near long tails.
    const float decay_knob = 0.1f + clampf(p.decay, 0.f, 1.f) * (0.9999f - 0.1f);
    const float decay = 1.0f - (1.0f - decay_knob) * (1.0f - decay_knob);

    // Plateau-like mod rate response: square taper then map to 1..100 speed domain.
    float mod_rate = clampf(p.mod_rate, 0.f, 1.f);
    mod_rate = mod_rate * mod_rate;
    mod_rate = mod_rate * 99.f + 1.f;

    reverb.enableInputDiffusion(m.diffuse_input);
    reverb.setInputFilterLowCutoffPitch(m.input_low_pitch);
    reverb.setInputFilterHighCutoffPitch(10.0f);

    reverb.setTimeScale(size);
    reverb.setPreDelay(pre_delay_sec);
    reverb.setDecay(decay);
    reverb.setTankDiffusion(clampf(p.diffusion, 0.f, 10.f));
    reverb.setTankFilterLowCutFrequency(m.low_cut_pitch);
    reverb.setTankFilterHighCutFrequency(clampf(p.high_cut_pitch, 0.f, 10.f));
    reverb.setTankModDepth(clampf(p.mod_depth, 0.f, 16.f));
    reverb.setTankModSpeed(mod_rate);
    reverb.setTankModShape(m.mod_shape);

    // MODE_FREEZE always wins; otherwise respect FRZ toggle.
    reverb.freeze(m.force_freeze || p.freeze);
  }

  Dattorro reverb;
  Params params_;
};
