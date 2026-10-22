/*
 *  File: header.c
 *
 *  EraCrush - microKORG 2 modulation effect (modfx) unit header.
 *  modfx units get 8 params max: 3 on page 1, 5 on page 2.
 *  Names: max 8 characters.
 */

#include "unit.h"
#include "runtime.h"

// The load-meter build (tools/mk2-meter) reuses this header with its own
// name and unit ID, so both can be loaded side by side.
#ifndef ERACRUSH_UNIT_NAME
#define ERACRUSH_UNIT_NAME "EraCrush"
#endif
#ifndef ERACRUSH_UNIT_ID
#define ERACRUSH_UNIT_ID 0x8U  // 1 = VectorFM, 2 = Formant Mutator, 3 = Rollers, 4 = Dusty Pads, 5 = Horizon, 6 = (Dread Bass), 7 = Hoover, 8 = EraCrush
#endif

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_modfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x5477476EU,  // 'TwGn' (TwistedGenetics)
    .unit_id = ERACRUSH_UNIT_ID,
    .version = 0x00010000U,  // 1.0.0
    .name = ERACRUSH_UNIT_NAME,
    .num_presets = 0,
    .num_params = 8,
    .params = {
        // Format: min, max, center, default, type, fractional, frac. type, <reserved>, name

        // Page 1
        {0,   4, 0,   0, k_unit_param_type_strings, 0, 0, 0, {"Machine"}},
        {0, 100, 0, 100, k_unit_param_type_percent, 0, 0, 0, {"Rate"}},
        {0,  16, 0,   0, k_unit_param_type_strings, 0, 0, 0, {"Bits"}},

        // Page 2
        {0, 100, 0,  20, k_unit_param_type_percent, 0, 0, 0, {"Drive"}},
        {0, 100, 0,  40, k_unit_param_type_percent, 0, 0, 0, {"Alias"}},
        {0, 100, 0,  50, k_unit_param_type_percent, 0, 0, 0, {"Tone"}},
        {0, 100, 0,  15, k_unit_param_type_percent, 0, 0, 0, {"Age"}},
        {0, 100, 0, 100, k_unit_param_type_percent, 0, 0, 0, {"Mix"}}}};
