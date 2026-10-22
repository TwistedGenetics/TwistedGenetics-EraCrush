/*
 *  File: unit.cc
 *
 *  EraCrush - microKORG 2 mod FX unit: SDK glue around eracrush::Crusher (core/eracrush_dsp.h).
 *
 *  Built with -DERACRUSH_LOADMETER (tools/mk2-meter), the MACHINE display also
 *  shows the CPU load of this unit, e.g. "SP12 9%".
 */

#include "unit.h"
#include "runtime.h"
#include "unit_modfx.h"
#include "utils/int_math.h"
#include "eracrush_dsp.h"
#ifdef ERACRUSH_LOADMETER
#include <stdio.h>
#include <time.h>
#endif

namespace {

enum {
  kParamMachine = 0,
  kParamRate,
  kParamBits,
  kParamDrive,
  kParamAlias,
  kParamTone,
  kParamAge,
  kParamMix,
  kNumParams
};

unit_runtime_desc_t s_desc;
eracrush::Crusher s_crusher;
int32_t s_params[kNumParams];

#ifdef ERACRUSH_LOADMETER
// Time spent in unit_render as a share of the buffer's real-time length.
// Peak over ~0.5 s. 100 % = no time left (crackles).
float s_load_peak = 0.f, s_load_show = 0.f;
uint32_t s_load_frames = 0;
char s_machine_str[16];
inline double nowNs() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}
#endif

void applyParam(uint8_t id, int32_t value) {
  s_params[id] = value;
  eracrush::Params &p = s_crusher.params;
  switch (id) {
    case kParamMachine: p.machine = clipminmaxi32(0, value, eracrush::kNumMachines - 1); break;
    case kParamRate:    p.rate = value * 0.01f; break;
    case kParamBits:    p.bits = clipminmaxi32(0, value, 16); break;
    case kParamDrive:   p.drive = value * 0.01f; break;
    case kParamAlias:   p.alias = value * 0.01f; break;
    case kParamTone:    p.tone = value * 0.01f; break;
    case kParamAge:     p.age = value * 0.01f; break;
    case kParamMix:     p.mix = value * 0.01f; break;
    default: break;
  }
}

}  // namespace

__attribute__((used)) int8_t unit_init(const unit_runtime_desc_t *desc) {
  if (!desc) return k_unit_err_undef;
  if (desc->target != unit_header.target) return k_unit_err_target;
  if (!UNIT_API_IS_COMPAT(desc->api)) return k_unit_err_api_version;
  if (desc->samplerate != 48000) return k_unit_err_samplerate;
  if (desc->input_channels != 2 || desc->output_channels != 2) return k_unit_err_geometry;

  s_desc = *desc;
  for (int i = 0; i < kNumParams; i++) applyParam(i, unit_header.params[i].init);
  s_crusher.reset();
  return k_unit_err_none;
}

__attribute__((used)) void unit_teardown() {}

__attribute__((used)) void unit_reset() { s_crusher.reset(); }

__attribute__((used)) void unit_resume() { s_crusher.reset(); }

__attribute__((used)) void unit_suspend() {}

__attribute__((used)) void unit_render(const float *in, float *out, uint32_t frames) {
#ifdef ERACRUSH_LOADMETER
  const double t_start = nowNs();
#endif
  s_crusher.process(in, out, (int)frames);
#ifdef ERACRUSH_LOADMETER
  const double used = nowNs() - t_start;
  const float load = (float)(used / ((double)frames * (1e9 / 48000.0)));
  if (load > s_load_peak) s_load_peak = load;
  s_load_frames += frames;
  if (s_load_frames >= 24000) {
    s_load_show = s_load_peak;
    s_load_peak = 0.f;
    s_load_frames = 0;
  }
#endif
}

__attribute__((used)) void unit_set_param_value(uint8_t id, int32_t value) {
  if (id >= kNumParams) return;
  applyParam(id, value);
}

__attribute__((used)) int32_t unit_get_param_value(uint8_t id) {
  return (id < kNumParams) ? s_params[id] : 0;
}

__attribute__((used)) const char *unit_get_param_str_value(uint8_t id, int32_t value) {
  static const char *kBitNames[17] = {"AUTO", "1",  "2",  "3",  "4",  "5",  "6",  "7", "8",
                                      "9",    "10", "11", "12", "13", "14", "15", "16"};
  switch (id) {
    case kParamMachine: {
      if (value < 0 || value >= eracrush::kNumMachines) return nullptr;
#ifdef ERACRUSH_LOADMETER
      static const char *kShort[eracrush::kNumMachines] = {"SP12", "MPC", "S950", "S1K", "AMGA"};
      snprintf(s_machine_str, sizeof s_machine_str, "%s %d%%", kShort[value], (int)(s_load_show * 100.f + 0.5f));
      return s_machine_str;
#else
      static const char *kNames[eracrush::kNumMachines] = {"SP1200", "MPC60", "S950", "S1000", "AMIGA"};
      return kNames[value];
#endif
    }
    case kParamBits:
      if (value >= 0 && value <= 16) return kBitNames[value];
      return nullptr;
    default:
      return nullptr;
  }
}

__attribute__((used)) void unit_set_tempo(uint32_t tempo) { (void)tempo; }
