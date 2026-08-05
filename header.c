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
 *  File: header.c
 *
 *  NTS-1 mkII reverb effect unit header definition
 *
 */

#include "unit_revfx.h"   // Note: Include base definitions for revfx units

// ---- Unit header definition  --------------------------------------------------------------------

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),                  // Size of this header. Leave as is.
    .target = UNIT_TARGET_PLATFORM | k_unit_module_revfx,  // Target platform and module pair for this unit
    .api = UNIT_API_VERSION,                               // API version for which unit was built. See runtime.h
    .dev_id = 0x4D616A69U,                                         // Developer ID. See https://github.com/korginc/logue-sdk/blob/master/developer_ids.md
    .unit_id = 0x00000002U,                                       // ID for this unit. Scoped within the context of a given dev_id.
    .version = 0x00010000U,                                // This unit's version: major.minor.patch (major<<16 minor<<8 patch).
    .name = "Gorge",                                       // Name for this unit, will be displayed on device
    .num_params = 10,                                      // Number of valid parameter descriptors. (max. 11)
    
    .params = {
        // Format: min, max, center (unused), default, type, frac. bits, frac. mode, <reserved>, name

        // See common/runtime.h for type enum and unit_param_t structure

        // Fixed/direct UI parameters (front panel)
        // A knob
        {0, 1023, 0, 460, k_unit_param_type_none, 1, 0, 0, {"SIZE"}},

        // B knob
        {0, 1023, 0, 470, k_unit_param_type_none, 1, 0, 0, {"DECY"}},

        // DELAY switch + B knob
        // Keep native drywet type so host/websim presents bipolar wet/dry UI.
        {-1000, 1000, 0, -300, k_unit_param_type_drywet, 1, 1, 0, {"MIX"}},

        // Edit menu parameters
        // Dedicated freeze toggle.
        {0, 1, 0, 0, k_unit_param_type_onoff, 0, 0, 0, {"FREEZE"}},
        // Mod depth uses 1 decimal fixed-point (0..16.0 internal domain).
        {0, 160, 0, 25, k_unit_param_type_none, 1, 1, 0, {"DEPTH MOD"}},
        // Mod rate as percentage, then shaped nonlinearly in DSP.
        {0, 100, 0, 12, k_unit_param_type_percent, 0, 0, 0, {"RATE MOD"}},
        // Mod shape is bipolar around center. Negative shortens rise, positive shortens fall.
        {-100, 100, 0, 0, k_unit_param_type_none, 0, 0, 0, {"SHAPE MOD"}},
        // Pre-delay in milliseconds.
        {0, 500, 0, 25, k_unit_param_type_msec, 0, 0, 0, {"PREDELAY"}},
        // Diffusion as percentage, internally mapped to 0..10.
        {0, 100, 0, 75, k_unit_param_type_percent, 0, 0, 0, {"DIFFUSION"}},
        // High-cut pitch control mapped to Dattorro's 0..10 pitch domain.
        {0, 100, 0, 72, k_unit_param_type_none, 0, 0, 0, {"HIGHCUT"}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}},
};

// Parameters:
// SIZE: Controls the size of the reverb space. Range: 0-1023.
// DECY: Controls the decay time of the reverb. Range: 0-1023.
// MIX: Controls the wet/dry mix. Range: -1000 to 1000.
// FREEZE: Toggles the freeze effect. Range: 0-1.
// DEPTH MOD: Modulation depth. Range: 0-160.
// RATE MOD: Modulation rate as a percentage. Range: 0-100.
// SHAPE MOD: Triangle LFO shape tilt around center. Range: -100 to 100.
// PREDELAY: Pre-delay time in milliseconds. Range: 0-500.
// DIFFUSION: Diffusion percentage. Range: 0-100.
// HIGHCUT: High-cut filter control. Range: 0-100.
