# EraCrush

A sampler-era crusher for the **Korg microKORG 2** modulation FX slot.

Each machine sets its own native sample rate, bit depth, converter type and output filter voicing. The signal path is drive into a steep anti-alias filter, then a converter clip, a sample & hold at the machine's clock, the quantiser, and the machine's output filter. You get the grit of 90s jungle and hip-hop samplers on anything the mk2 plays. These are characterful approximations of the classic boxes, not circuit models.

## Download & install

Go to [**Releases**](https://github.com/TwistedGenetics/TwistedGenetics-EraCrush/releases/latest) and download `EraCrush.mk2unit`.

Needs microKORG 2 firmware **2.0.0 or later** (the version that added logue SDK support).

- **KORG Sound Librarian** (Korg's official app for v2.0): connect the microKORG 2 by USB, add `EraCrush.mk2unit` as a user **modulation** effect and send it to the synth.
- **Or USB mass storage:** hold the first option button while powering on, then copy the file into the mod FX units folder in `Units/` (next to `Units/Oscs/`, inside a `SlotN` folder) on the drive that appears.

EraCrush then appears at the end of the modulation effect list.

**`ECmeter.mk2unit`** (optional, also in each Release) is the same effect with its CPU load shown next to the machine name, e.g. `SP12 9%` (peak over the last half second; 100 % = out of time, crackles).

## Controls

| Page | Knob | Range | What it does |
|---|---|---|---|
| 1 | **Machine** | 5 machines | Picks the box (below) |
| 1 | **Rate** | 0–100 % | Sample clock, from native (100 %) down ~4.3 octaves. The pitch-trick zone |
| 1 | **Bits** | AUTO, 1–16 | AUTO uses the machine's native bits. Anything else overrides it |
| 2 | **Drive** | 0–100 % | Up to +18 dB into the converter. Louder and dirtier |
| 2 | **Alias** | 0–100 % | 0 = steep 8-pole tracking AA filter (clean, Akai-style). 100 = no AA filter (raw SP-1200 fold-back) |
| 2 | **Tone** | 0–100 % | Output filter. 50 % = machine voicing, ±2 octaves either side, 100 % = fully open |
| 2 | **Age** | 0–100 % | Converter hiss plus sample-clock jitter |
| 2 | **Mix** | 0–100 % | Dry/wet (0 % is a bit-exact bypass) |

### Machines

| Machine | Rate | Bits | Converter | Output filter |
|---|---|---|---|---|
| **SP1200** | 26.04 kHz | 12 | Linear, hard clip | Fixed 4-pole ~11 kHz with a bit of bite |
| **MPC60** | 40 kHz | 12 | Companded ("non-linear"), soft clip | Smooth 2-pole ~15 kHz |
| **S950** | 31.25 kHz | 12 | Linear | Tracks the sample rate |
| **S1000** | 44.1 kHz | 16 | Linear, clean | Tracks the sample rate |
| **AMIGA** | 28.9 kHz | 8 | Linear, hard clip | Fixed ~4.4 kHz "LED" voicing |

## Starter settings

- **SP-1200 drums:** SP1200, Rate 100, Drive 35, Alias 75, Tone 50, Age 15.
- **S950 pitch trick:** S950, Rate 70–75, Alias 10. Dark, grainy, still musical.
- **MPC60 glue:** MPC60, Drive 25, Alias 30, Mix 60–80. Subtle, fat.
- **Amiga crunch:** AMIGA, Drive 30, Alias 60, Tone 40, Age 25.
- **Destroy:** any machine, Bits 3–5, Rate 20–40, Alias 100, Tone 100.

## Test checklist (by ear on the hardware)

- [ ] Each machine sounds distinct at default settings
- [ ] Rate sweep: no clicks, aliasing grows as Rate drops with Alias high
- [ ] Alias 0 vs 100 on a bright patch: clean vs metallic fold-back
- [ ] Bits AUTO / 8 / 4 / 1 behave as expected
- [ ] Mix 0 is identical to the effect off

## CPU and memory

It's a light effect: an 8-pole anti-alias filter, a sample & hold and two output biquads per channel. It needs no delay memory and sits well under the 16 KB modfx load limit; CI fails the build if it ever isn't.

## Building

GitHub Actions builds every push with Korg's own toolchain and publishes a Release when the `.version` in `microkorg2/header.c` changes.

Locally (Linux):

```
./build.sh        # fetches Korg's toolchain + logue SDK into .deps/, builds dist/EraCrush.mk2unit and dist/ECmeter.mk2unit
./test/run.sh     # desktop DSP tests, no toolchain needed; preview WAVs land in test/out/
```

The tests check bit-exact bypass at Mix 0, that a 15 kHz tone folds back to 11.04 kHz on the SP1200 (and is suppressed with Alias 0), about 6 dB of noise floor per bit, and stability at every machine and knob extreme.

## Layout

```
core/eracrush_dsp.h     DSP engine (plain C++, no SDK dependencies)
microkorg2/             unit glue, header, Makefile, config.mk
tools/mk2-meter/        ECmeter: same effect with a CPU meter in the MACHINE display
test/                   desktop tests + preview renders
build.sh                toolchain + SDK fetch and build
```

## Unit ID

dev_id `0x5477476E` ('TwGn'), unit_id `8` (ECmeter: `0x108`).
