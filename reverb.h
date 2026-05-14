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
    TIME = 0U,
    DEPTH,
    MIX,
    PARAM4,
    NUM_PARAMS
  };

  // Note: Make sure that default param values correspond to declarations in header.c
  struct Params
  {
    float time;
    float depth;
    float mix;
    uint32_t param4;

    void reset()
    {
      time = 0.25f;
      depth = 0.25f;
      mix = 0.f;
      param4 = 1;
    }

    Params() { reset(); }
  };

  enum
  {
    PARAM4_VALUE0 = 0,
    PARAM4_VALUE1,
    PARAM4_VALUE2,
    PARAM4_VALUE3,
    NUM_PARAM4_VALUES,
  };

  inline void setParameter(uint8_t index, int32_t value) override final
  {
    switch (index)
    {
    case TIME:
      params_.time = param_10bit_to_f32(value);
      reverb.setTimeScale(0.15 + params_.time * 3.0);
      break;

    case DEPTH:
      params_.depth = param_10bit_to_f32(value);
      reverb.setTankModDepth(params_.depth * 16.0);
      break;

    case MIX:
      params_.mix = value / 1000.f;
      break;

    case PARAM4:
      params_.param4 = static_cast<uint32_t>(value);
      applyMode(params_.param4);
      break;

    default:
      break;
    }
  }

  inline const char *getParameterStrValue(uint8_t index, int32_t value) const override final
  {
    static const char *param4_strings[NUM_PARAM4_VALUES] = {
      "NORM",
      "WASH",
      "DARK",
      "FREEZ",
    };

    switch (index)
    {
    case PARAM4:
      if (value >= PARAM4_VALUE0 && value < NUM_PARAM4_VALUES)
        return param4_strings[value];
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
    reverb.setInputFilterLowCutoffPitch(0.0);
    reverb.setInputFilterHighCutoffPitch(10.0);
    applyMode(params_.param4);
    reverb.setTimeScale(0.15 + params_.time * 3.0);
    reverb.setTankModDepth(params_.depth * 16.0);
  }

  void teardown() override final {}

  void reset() override final
  {
    reverb.clear();
    applyMode(params_.param4);
    reverb.setTimeScale(0.15 + params_.time * 3.0);
    reverb.setTankModDepth(params_.depth * 16.0);
  }

  void process(const float *__restrict in, float *__restrict out, uint32_t frames) override final
  {
    const Params p = params_;
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
  void applyMode(uint32_t mode)
  {
    switch (mode)
    {
    case PARAM4_VALUE0:
      reverb.freeze(false);
      reverb.enableInputDiffusion(true);
      reverb.setPreDelay(0.03);
      reverb.setDecay(0.84);
      reverb.setTankDiffusion(4.0);
      reverb.setTankFilterLowCutFrequency(2.0);
      reverb.setTankFilterHighCutFrequency(8.5);
      reverb.setTankModSpeed(0.5);
      reverb.setTankModShape(0.50);
      break;

    case PARAM4_VALUE1:
      reverb.freeze(false);
      reverb.enableInputDiffusion(true);
      reverb.setPreDelay(0.06);
      reverb.setDecay(0.94);
      reverb.setTankDiffusion(7.5);
      reverb.setTankFilterLowCutFrequency(1.5);
      reverb.setTankFilterHighCutFrequency(9.0);
      reverb.setTankModSpeed(0.3);
      reverb.setTankModShape(0.35);
      break;

    case PARAM4_VALUE2:
      reverb.freeze(false);
      reverb.enableInputDiffusion(true);
      reverb.setPreDelay(0.04);
      reverb.setDecay(0.97);
      reverb.setTankDiffusion(6.0);
      reverb.setTankFilterLowCutFrequency(0.5);
      reverb.setTankFilterHighCutFrequency(6.0);
      reverb.setTankModSpeed(0.2);
      reverb.setTankModShape(0.70);
      break;

    case PARAM4_VALUE3:
    default:
      reverb.enableInputDiffusion(true);
      reverb.setPreDelay(0.05);
      reverb.setDecay(1.0);
      reverb.setTankDiffusion(10.0);
      reverb.setTankFilterLowCutFrequency(0.0);
      reverb.setTankFilterHighCutFrequency(10.0);
      reverb.setTankModSpeed(0.1);
      reverb.setTankModShape(0.50);
      reverb.freeze(true);
      break;
    }
  }

  Dattorro reverb;
  Params params_;
};
