// Desktop tests + preview renderer for the EraCrush DSP core (core/eracrush_dsp.h).
// Run via test/run.sh. Writes preview WAVs to test/out/ and exits non-zero on failure.
#include <algorithm>
#include <cstdio>
#include <initializer_list>
#include <string>
#include <vector>

#include "eracrush_dsp.h"
#include "wav.h"

using namespace eracrush;

static const char *kNames[kNumMachines] = {"sp1200", "mpc60", "s950", "s1000", "amiga"};

static void run(Crusher &c, const std::vector<float> &L, const std::vector<float> &R, std::vector<float> &oL,
                std::vector<float> &oR) {
  const int n = (int)L.size();
  oL.assign(n, 0.f);
  oR.assign(n, 0.f);
  float in[128], out[128];
  for (int pos = 0; pos < n; pos += 64) {
    const int f = std::min(64, n - pos);
    for (int i = 0; i < f; i++) { in[2 * i] = L[pos + i]; in[2 * i + 1] = R[pos + i]; }
    c.process(in, out, f);
    for (int i = 0; i < f; i++) { oL[pos + i] = out[2 * i]; oR[pos + i] = out[2 * i + 1]; }
  }
}

// Magnitude of one frequency via windowed DFT over a 1 s steady-state segment
static double magAt(const std::vector<float> &x, double hz) {
  const int N = 24000, a = 12000;
  double re = 0, im = 0;
  for (int n = 0; n < N; n++) {
    const double w = 0.5 - 0.5 * std::cos(2 * M_PI * n / (N - 1));
    re += x[a + n] * w * std::cos(2 * M_PI * hz * n / 48000.0);
    im -= x[a + n] * w * std::sin(2 * M_PI * hz * n / 48000.0);
  }
  return std::sqrt(re * re + im * im) / (N * 0.25);
}
static double db(double v) { return 20 * std::log10(v + 1e-12); }

// A crude break: kick, snare, hats at 170 bpm, so previews have transients to chew on
static std::vector<float> drums(float sec) {
  const int n = (int)(sec * 48000);
  std::vector<float> x(n, 0.f);
  uint32_t rng = 1;
  auto rnd = [&]() { rng = rng * 1664525u + 1013904223u; return (int32_t)rng * (1.f / 2147483648.f); };
  const float beat = 60.f / 170.f;
  // pattern in 8ths over 2 bars: K . S . . K S .  | K . S K . . S .
  const char *pat = "K.S..KS.K.SK..S.";
  for (int step = 0; step * beat * 0.5f < sec; step++) {
    const char c = pat[step % 16];
    const int s0 = (int)(step * beat * 0.5f * 48000);
    for (int i = 0; i < 9000 && s0 + i < n; i++) {
      const float t = i / 48000.f;
      float v = 0.f;
      if (c == 'K') v += std::sin(2 * M_PI * (50 * t + 900 * (1 - std::exp(-t * 30)) / 30)) * std::exp(-t * 9);
      if (c == 'S') v += (rnd() * 0.7f + 0.4f * std::sin(2 * M_PI * 190 * t)) * std::exp(-t * 18);
      if (i < 2400) v += rnd() * 0.25f * std::exp(-t * 90);  // hat on every 8th
      x[s0 + i] += v * 0.6f;
    }
  }
  return x;
}

int main(int argc, char **argv) {
  const char *outDir = argc > 1 ? argv[1] : "test/out";
  int failures = 0;

  // 1) Mix = 0 must be a perfect bypass
  {
    Crusher c;
    c.params.mix = 0.f;
    c.reset();
    std::vector<float> L(48000), R(48000), oL, oR;
    for (int i = 0; i < 48000; i++) { L[i] = std::sin(i * 0.05f) * 0.8f; R[i] = std::cos(i * 0.031f) * 0.5f; }
    run(c, L, R, oL, oR);
    double err = 0;
    for (int i = 0; i < 48000; i++) err = std::max(err, (double)std::fabs(oL[i] - L[i]) + std::fabs(oR[i] - R[i]));
    std::printf("bypass (mix 0) max error: %.2e %s\n", err, err < 1e-6 ? "ok" : "FAIL");
    if (err >= 1e-6) failures++;
  }

  // 2) Aliasing physics: 15 kHz sine into SP1200 @ native 26.04 kHz should fold to 11.04 kHz
  {
    std::vector<float> L(60000), R, oL, oR;
    for (int i = 0; i < 60000; i++) L[i] = 0.5f * std::sin(2 * M_PI * 15000.0 * i / 48000.0);
    R = L;
    Crusher raw;   raw.params.machine = kSP1200; raw.params.alias = 1.f; raw.params.tone = 1.f; raw.params.drive = 0.f; raw.params.age = 0.f;
    Crusher clean; clean.params.machine = kSP1200; clean.params.alias = 0.f; clean.params.tone = 1.f; clean.params.drive = 0.f; clean.params.age = 0.f;
    raw.reset(); clean.reset();
    run(raw, L, R, oL, oR);
    const double aliasRaw = db(magAt(oL, 11040.0));
    run(clean, L, R, oL, oR);
    const double aliasClean = db(magAt(oL, 11040.0));
    std::printf("alias image @11.04 kHz: raw %.1f dBFS, AA filter on %.1f dBFS  %s\n", aliasRaw, aliasClean,
                (aliasRaw > -20 && aliasClean < aliasRaw - 25) ? "ok" : "FAIL");
    if (!(aliasRaw > -20 && aliasClean < aliasRaw - 25)) failures++;
  }

  // 3) Bit depth: quantisation noise floor should track bits (~6 dB/bit)
  {
    std::vector<float> L(60000), R, oL, oR;
    for (int i = 0; i < 60000; i++) L[i] = 0.7f * std::sin(2 * M_PI * 997.0 * i / 48000.0);
    R = L;
    double prevNoise = 0;
    std::printf("bits -> residual noise:");
    for (int bits : {4, 8, 12}) {
      Crusher c; Params &p = c.params;
      p.machine = kS1000; p.bits = bits; p.alias = 0.f; p.tone = 1.f; p.drive = 0.f; p.age = 0.f; p.rate = 1.f;
      c.reset();
      run(c, L, R, oL, oR);
      // residual = output minus best-fit sine at 997 Hz (estimate via the clean S1000 path at 16 bits)
      Crusher ref; Params &q = ref.params;
      q = p; q.bits = 16; ref.reset();
      std::vector<float> rL, rR;
      run(ref, L, R, rL, rR);
      double e = 0;
      for (int i = 12000; i < 60000; i++) e += (oL[i] - rL[i]) * (oL[i] - rL[i]);
      const double noise = db(std::sqrt(e / 48000));
      std::printf("  %d-bit %.1f dB", bits, noise);
      if (prevNoise != 0 && !(noise < prevNoise - 18)) failures++;
      prevNoise = noise;
    }
    std::printf("\n");
  }

  // 4) Every machine x corner settings: finite, bounded
  {
    const int before = failures;
    std::vector<float> L = drums(3.f), R = L, oL, oR;
    for (int m = 0; m < kNumMachines; m++)
      for (int corner = 0; corner < 3; corner++) {
        Crusher c; Params &p = c.params;
        const float k = corner == 0 ? 0.f : (corner == 1 ? 1.f : 0.5f);
        p.machine = m; p.rate = k; p.bits = corner == 0 ? 1 : (corner == 1 ? 16 : 0);
        p.drive = k; p.alias = k; p.tone = k; p.age = k; p.mix = 1.f;
        c.reset();
        run(c, L, R, oL, oR);
        Stats s = stats(oL);
        if (!s.finite || s.peak > 2.0) {
          std::printf("STRESS FAIL machine=%s corner=%d peak=%.3f finite=%d\n", kNames[m], corner, s.peak, s.finite);
          failures++;
        }
      }
    std::printf("stress corners: %s\n", failures > before ? "see above" : "all finite, bounded");
  }

  // 5) Previews: drums + saw riff through each machine at musical settings
  {
    std::vector<float> d = drums(5.6f);
    // Detuned polyBLEP saw riff (self-contained stand-in for a synth line)
    std::vector<float> syn(d.size(), 0.f);
    struct Ev { float t, len, note; } evs[] = {{0.f, 1.3f, 50}, {1.41f, 0.6f, 53}, {2.12f, 1.2f, 48}, {3.53f, 1.6f, 50}};
    auto blep = [](float t, float dt) {
      if (t < dt) { t /= dt; return t + t - t * t - 1.f; }
      if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
      return 0.f;
    };
    for (auto &ev : evs) {
      const int s0 = (int)(ev.t * 48000), len = (int)(ev.len * 48000);
      float ph[3] = {0.f, 0.33f, 0.71f};
      for (int i = 0; i < len + 2400 && s0 + i < (int)syn.size(); i++) {
        const float env = i < 240 ? i / 240.f : (i < len ? 1.f : std::exp(-(i - len) / 1200.f));
        float v = 0.f;
        for (int k = 0; k < 3; k++) {
          const float dt = 440.f * std::exp2((ev.note - 69.f + (k - 1) * 0.12f) / 12.f) / 48000.f;
          v += 2.f * ph[k] - 1.f - blep(ph[k], dt);
          ph[k] += dt; if (ph[k] >= 1.f) ph[k] -= 1.f;
        }
        syn[s0 + i] += v * 0.12f * env;
      }
    }
    std::vector<float> L(d.size()), R(d.size());
    for (size_t i = 0; i < d.size(); i++) { L[i] = d[i] * 0.8f + syn[i] * 1.1f; R[i] = d[i] * 0.8f + syn[i] * 0.9f; }
    write_wav((std::string(outDir) + "/crush_00_dry.wav").c_str(), L, R);

    struct Preset { int machine; float rate; int bits; float drive, alias, tone, age; const char *tag; } presets[] = {
        {kSP1200, 1.00f, 0, 0.35f, 0.75f, 0.50f, 0.15f, "sp1200"},
        {kMPC60,  1.00f, 0, 0.25f, 0.30f, 0.50f, 0.10f, "mpc60"},
        {kS950,   0.72f, 0, 0.20f, 0.10f, 0.50f, 0.10f, "s950_pitchtrick"},
        {kS1000,  1.00f, 0, 0.15f, 0.00f, 0.50f, 0.05f, "s1000"},
        {kAmiga,  1.00f, 0, 0.30f, 0.60f, 0.40f, 0.25f, "amiga"},
    };
    for (auto &pr : presets) {
      Crusher c; Params &p = c.params;
      p.machine = pr.machine; p.rate = pr.rate; p.bits = pr.bits; p.drive = pr.drive;
      p.alias = pr.alias; p.tone = pr.tone; p.age = pr.age; p.mix = 1.f;
      c.reset();
      std::vector<float> oL, oR;
      run(c, L, R, oL, oR);
      Stats s = stats(oL);
      std::printf("preview %-16s peak %.3f rms %.3f\n", pr.tag, s.peak, s.rms);
      char path[512];
      std::snprintf(path, sizeof path, "%s/crush_%02d_%s.wav", outDir, pr.machine + 1, pr.tag);
      write_wav(path, oL, oR);
    }
  }

  std::printf(failures ? "FAILURES: %d\n" : "ALL CHECKS PASSED\n", failures);
  return failures ? 1 : 0;
}
