/* audio.c — GB APU channel synthesis via SDL2
 *
 * Layered design:
 *   audio.c   — synthesis engine: SDL2 callback, Audio_WriteReg, SFX
 *   music.c   — note sequencer: Music_Play / Music_Update (calls Audio_WriteReg)
 *
 * Audio_Update() is called once per frame (60 Hz) by game.c.
 * It ticks the music sequencer then handles the text-beep SFX state machine.
 */
#include "audio.h"
#include "../game/music.h"
#include "../data/cry_data.h"
#include "hardware.h"
#include <SDL2/SDL.h>
#include <string.h>
#include <stdint.h>

/* ---- Per-channel synthesis state ---------------------------------- */
typedef struct {
    int      enabled;
    uint32_t phase;       /* sample phase accumulator (24-bit fixed point) */
    uint32_t phase_inc;   /* phase advance per sample */
    uint8_t  volume;      /* 0-15 */
    uint8_t  duty;        /* 0-3 → 12.5/25/50/75% */

    /* CH3 wave */
    uint8_t  wave_ram[16];
    int      wave_pos;
    uint32_t wave_start_phase; /* phase to use on trigger: first ascending zero crossing */
    uint8_t  wave_playing;     /* 1 = wave was running when last note fired; skip phase reset */
    uint8_t  wave_vol_shift;   /* NR32 bits 6-5: 0=mute, 1=100%, 2=50%, 3=25% */

    /* CH4 noise LFSR */
    uint16_t lfsr;
    int      lfsr_narrow;

    /* Stored freq register (11-bit) built from NRx3 + NRx4 writes */
    uint16_t freq_reg;    /* 11-bit GB frequency register value */

    /* NR10 frequency sweep (CH0 / ch[0] only) */
    uint8_t  sweep_pace;   /* 0 = disabled; 1-7 = ticks per sweep at 128 Hz */
    uint8_t  sweep_dir;    /* 0 = increase, 1 = decrease */
    uint8_t  sweep_shift;  /* 0-7: delta = freq_reg >> shift */
    uint32_t sweep_timer;  /* sample counter within current sweep step */

    /* Volume envelope (NRx2) — mirrors GB APU envelope unit */
    uint8_t  env_pace;    /* 0 = disabled; 1-7 = ticks per volume step at 64 Hz */
    uint8_t  env_dir;     /* 0 = decrease, 1 = increase */
    uint8_t  env_initial; /* initial volume on trigger */
    uint32_t env_counter; /* sample counter within current envelope step */
} gb_channel_t;

static gb_channel_t ch[4];
static SDL_AudioDeviceID audio_dev  = 0;
static SDL_mutex        *audio_mutex = NULL;

/* ---- Frequency → phase_inc --------------------------------------- */
/* hz = base / (2048 - gb_freq11)   base=131072 square, base=65536 wave
 * phase_inc = hz * (1<<24) / SAMPLE_RATE  (24-bit fixed point)        */
static uint32_t freq_to_phase_inc(uint16_t gb_freq, int is_wave) {
    uint32_t base  = is_wave ? 65536u : 131072u;
    uint32_t denom = (uint32_t)(2048u - (gb_freq & 0x7FFu));
    if (denom == 0) return 0;
    return (uint32_t)((uint64_t)base * (1u << 24) /
                      ((uint64_t)AUDIO_SAMPLE_RATE * denom));
}

/* GB noise channel (NR43) LFSR clock rate:
 *   r=0: 1048576 / 2^shift  Hz  (special case: divisor = 0.5)
 *   r>0: 524288  / r / 2^shift  Hz
 * Mirrors Pan Docs CH4 polynomial counter clock frequency formula. */
static uint32_t noise_freq_to_phase_inc(uint8_t nr43) {
    uint32_t shift = (nr43 >> 4) & 0xF;
    uint32_t r     = nr43 & 7;
    if (shift >= 14) return 0;
    uint64_t hz = (r == 0) ? ((uint64_t)1048576 >> shift)
                           : ((uint64_t)524288 / r >> shift);
    if (hz == 0) return 0;
    return (uint32_t)(hz * (1u << 24) / AUDIO_SAMPLE_RATE);
}

/* ---- SDL audio callback ------------------------------------------ */
static void audio_callback(void *userdata, uint8_t *stream, int len) {
    (void)userdata;
    int16_t *out     = (int16_t *)stream;
    int      samples = len / sizeof(int16_t);

    /* GB envelope runs at 64 Hz; samples per envelope clock tick */
    static const uint32_t kEnvSamplesPerTick = AUDIO_SAMPLE_RATE / 64;

    SDL_LockMutex(audio_mutex);
    for (int i = 0; i < samples; i++) {
        int32_t mix = 0;

        /* CH0/CH1: square wave
         * Real GB DAC outputs 0 when duty step is low, volume when high
         * (unipolar).  We synthesize the same then DC-block with a gentle
         * HPF to remove the offset — matching what the physical capacitor
         * does on real hardware. */
        for (int c = 0; c < 2; c++) {
            if (!ch[c].enabled) continue;

            /* NR10 frequency sweep — CH0 only.
             * GB sweep clock runs at 128 Hz; fires every pace × (SAMPLE_RATE/128) samples.
             * Mirrors the GB APU sweep unit: new_freq = freq ± (freq >> shift).
             * Overflow (freq >= 2048) silences the channel. */
            if (c == 0 && ch[0].sweep_pace > 0 && ch[0].sweep_shift > 0) {
                if (++ch[0].sweep_timer >= (uint32_t)ch[0].sweep_pace * (AUDIO_SAMPLE_RATE / 128)) {
                    ch[0].sweep_timer = 0;
                    uint16_t delta = ch[0].freq_reg >> ch[0].sweep_shift;
                    if (ch[0].sweep_dir == 0) {   /* increase */
                        ch[0].freq_reg += delta;
                        if (ch[0].freq_reg >= 2048) { ch[0].enabled = 0; continue; }
                    } else {                       /* decrease */
                        if (delta > ch[0].freq_reg) { ch[0].enabled = 0; continue; }
                        ch[0].freq_reg -= delta;
                    }
                    ch[0].phase_inc = freq_to_phase_inc(ch[0].freq_reg, 0);
                }
            }

            /* Volume envelope: advance at env_pace × 64 Hz.
             * Note: envelope reaching 0 does NOT disable the channel —
             * it just stays at volume 0 until retriggered. */
            if (ch[c].env_pace > 0) {
                ch[c].env_counter++;
                if (ch[c].env_counter >= ch[c].env_pace * kEnvSamplesPerTick) {
                    ch[c].env_counter = 0;
                    if (ch[c].env_dir == 0) {
                        if (ch[c].volume > 0) ch[c].volume--;
                    } else {
                        if (ch[c].volume < 15) ch[c].volume++;
                    }
                }
            }

            /* duty table: 12.5%, 25%, 50%, 75% (out of 8 steps)
             * Unipolar output: high = volume, low = 0 (matches real DAC) */
            static const uint8_t duty_steps[4] = { 1, 2, 4, 6 };
            int high = ((ch[c].phase >> 21) & 7) < duty_steps[ch[c].duty & 3];
            int32_t raw = high ? (int32_t)ch[c].volume * 400 : 0;
            /* DC-blocking HPF (fc ≈ 5 Hz, same as wave channel) */
            static int32_t sq_hp_xprev[2] = {0,0}, sq_hp_yprev[2] = {0,0};
            int32_t hp = (int32_t)(((int64_t)32745 * (sq_hp_yprev[c] + raw - sq_hp_xprev[c])) >> 15);
            sq_hp_xprev[c] = raw;
            sq_hp_yprev[c] = hp;
            mix += hp;
            ch[c].phase = (ch[c].phase + ch[c].phase_inc) & 0xFFFFFFu;
        }

        /* CH2: wave channel
         * Volume is controlled by NR32 shift code (wave_vol_shift):
         *   0 = mute, 1 = 100% (>>0), 2 = 50% (>>1), 3 = 25% (>>2)
         *
         * DC-blocking high-pass filter (fc ≈ 5 Hz) emulates the DMG DAC's
         * AC-coupling capacitor.  This removes DC offset from note triggers
         * without eating bass content.  The DMG's physical capacitor has a
         * very low cutoff — we use 5 Hz which preserves fundamentals down
         * to the lowest wave channel notes (~65 Hz).
         *
         * y[n] = α·(y[n-1] + x[n] − x[n-1])
         * α = e^(−2π·5/44100) ≈ 0.99929 → Q15 ≈ 32745
         */
        if (ch[2].enabled) {
            ch[2].phase = (ch[2].phase + ch[2].phase_inc) & 0xFFFFFFu;
            int sidx    = (ch[2].phase >> (24 - 5)) & 31; /* 32 samples */
            uint8_t nib = (ch[2].wave_ram[sidx / 2] >> (sidx & 1 ? 0 : 4)) & 0xF;
            /* Apply NR32 volume shift: 0=mute, 1=no shift, 2=>>1, 3=>>2 */
            int32_t sample;
            if (ch[2].wave_vol_shift == 0) {
                sample = 0;
            } else {
                sample = ((int32_t)nib - 8) >> (ch[2].wave_vol_shift - 1);
            }
            /* Scale to match pulse channel amplitude range.
             * Pulse channels: ±volume*400, max ±6000.
             * Wave 4-bit range after centering: -8..+7, so at full vol
             * we scale by 750 to get ±6000 peak — matching pulse at vol=15. */
            int32_t raw = sample * 750;
            /* HP filter state: static because the callback runs continuously */
            static int32_t hp_xprev = 0, hp_yprev = 0;
            int32_t hp = (int32_t)(((int64_t)32745 * (hp_yprev + raw - hp_xprev)) >> 15);
            hp_xprev = raw;
            hp_yprev = hp;
            mix += hp;
        }

        /* CH3: LFSR noise
         * Output: when LFSR bit 0 is 0, emit 0; when 1, emit volume.
         * (Unipolar, same as pulse channels on real hardware.) */
        if (ch[3].enabled) {
            ch[3].phase += ch[3].phase_inc;
            if (ch[3].phase >= (1u << 24)) {
                ch[3].phase -= (1u << 24);
                int xb = (ch[3].lfsr ^ (ch[3].lfsr >> 1)) & 1;
                ch[3].lfsr = (uint16_t)((ch[3].lfsr >> 1) |
                             (xb << (ch[3].lfsr_narrow ? 6 : 14)));
            }
            /* Volume envelope — envelope reaching 0 does NOT disable channel */
            if (ch[3].env_pace > 0) {
                if (++ch[3].env_counter >= ch[3].env_pace * kEnvSamplesPerTick) {
                    ch[3].env_counter = 0;
                    if (ch[3].env_dir == 0) {
                        if (ch[3].volume > 0) ch[3].volume--;
                    } else {
                        if (ch[3].volume < 15) ch[3].volume++;
                    }
                }
            }
            int32_t raw = (ch[3].lfsr & 1) ? (int32_t)ch[3].volume * 400 : 0;
            /* DC-blocking HPF (fc ≈ 5 Hz) */
            static int32_t ns_hp_xprev = 0, ns_hp_yprev = 0;
            int32_t hp = (int32_t)(((int64_t)32745 * (ns_hp_yprev + raw - ns_hp_xprev)) >> 15);
            ns_hp_xprev = raw;
            ns_hp_yprev = hp;
            mix += hp;
        }

        if (mix >  28000) mix =  28000;
        if (mix < -28000) mix = -28000;
        *out++ = (int16_t)mix;
    }
    SDL_UnlockMutex(audio_mutex);
}

/* ---- Wave instrument table (from audio/wave_samples.asm) ----------
 * 5 instruments × 16 bytes = 32 4-bit samples packed high-nibble-first.
 * wave0: smooth sine-like curve (pokecenter, most songs)
 * wave1: slightly rounder sine
 * wave2: double-hump resonant
 * wave3: peaked sine with notch
 * wave4: sawtooth-like
 */
static const uint8_t kWavePatterns[5][16] = {
    /* wave0 */ { 0x02,0x46,0x8A,0xCE,0xFF,0xFE,0xED,0xDC,
                  0xCB,0xA9,0x87,0x65,0x44,0x33,0x22,0x11 },
    /* wave1 */ { 0x02,0x46,0x8A,0xCE,0xEF,0xFF,0xFE,0xEE,
                  0xDD,0xCB,0xA9,0x87,0x65,0x43,0x22,0x11 },
    /* wave2 */ { 0x13,0x69,0xBD,0xEE,0xEE,0xFF,0xFF,0xED,
                  0xDE,0xFF,0xFF,0xEE,0xEE,0xDB,0x96,0x31 },
    /* wave3 */ { 0x02,0x46,0x8A,0xCD,0xEF,0xFE,0xDE,0xFF,
                  0xEE,0xDC,0xBA,0x98,0x76,0x54,0x32,0x10 },
    /* wave4 */ { 0x01,0x23,0x45,0x67,0x8A,0xCD,0xEE,0xF7,
                  0x7F,0xEE,0xDC,0xA8,0x76,0x54,0x32,0x10 },
};

void Audio_SetWaveInstrument(int idx) {
    if (idx < 0 || idx >= 5) idx = 0;
    /* Find first ascending zero crossing (nib transitions from <8 to ≥8).
     * Triggering the wave channel at this phase gives raw output = 0 at onset,
     * which (combined with the HP filter) produces a click-free note start. */
    uint32_t start_phase = 0;
    for (int i = 1; i < 32; i++) {
        int prev = (kWavePatterns[idx][(i-1)/2] >> ((i-1)&1 ? 0 : 4)) & 0xF;
        int curr = (kWavePatterns[idx][i  /2] >> (i    &1 ? 0 : 4)) & 0xF;
        if (prev < 8 && curr >= 8) { start_phase = (uint32_t)i * (1u << 19); break; }
    }
    if (audio_mutex) SDL_LockMutex(audio_mutex);
    memcpy(ch[2].wave_ram, kWavePatterns[idx], 16);
    ch[2].wave_start_phase = start_phase;
    if (audio_mutex) SDL_UnlockMutex(audio_mutex);
}

/* ---- Public API ------------------------------------------ */
int Audio_Init(void) {
    memset(ch, 0, sizeof(ch));

    /* Seed LFSR for noise channel */
    ch[3].lfsr = 0x7FFF;

    /* CH2 (wave): default to instrument 0; sets wave_ram and wave_start_phase. */
    Audio_SetWaveInstrument(0);

    audio_mutex = SDL_CreateMutex();
    if (!audio_mutex) return -1;

    SDL_AudioSpec want = {0}, have;
    want.freq     = AUDIO_SAMPLE_RATE;
    want.format   = AUDIO_S16SYS;
    want.channels = AUDIO_CHANNELS;
    want.samples  = AUDIO_BUFFER_SIZE;
    want.callback = audio_callback;

    audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!audio_dev) return -1;

    SDL_PauseAudioDevice(audio_dev, 0);
    return 0;
}

/* Write a GB APU register for channel 0-3.
 * reg: 1=NRx1 (duty/length), 2=NRx2 (volume/envelope),
 *      3=NRx3 (freq low),    4=NRx4 (freq high + trigger) */
void Audio_WriteReg(int channel, int reg, uint8_t value) {
    if (channel < 0 || channel > 3) return;
    SDL_LockMutex(audio_mutex);
    switch (reg) {
        case 0:  /* NR10 — frequency sweep, CH0 only */
            if (channel == 0) {
                ch[0].sweep_pace  = (value >> 4) & 0x7;
                ch[0].sweep_dir   = (value >> 3) & 0x1;
                ch[0].sweep_shift = value & 0x7;
                ch[0].sweep_timer = 0;
            }
            break;
        case 1:
            if (channel < 2)
                ch[channel].duty = (value >> 6) & 3;
            break;
        case 2:
            if (channel == 2) {
                /* NR32: wave channel volume is bits 6-5 (shift code).
                 * The env_byte from music data encodes this as the volume
                 * field in bits 7-4.  Map: 0=mute, 1=100%, 2=50%, 3=25%.
                 * No envelope on CH3 — the real APU has none. */
                uint8_t vol_code = (value >> 4) & 0xF;
                /* Clamp to valid NR32 range: only 0-3 are meaningful */
                if (vol_code > 3) vol_code = 1;  /* treat as full volume */
                ch[2].wave_vol_shift = vol_code;
                ch[2].env_pace = 0;  /* no envelope on wave channel */
                if (vol_code == 0) {
                    ch[2].wave_playing = 0;  /* muted: next trigger = cold start */
                }
            } else {
                ch[channel].env_initial = (value >> 4) & 0xF;
                ch[channel].env_dir     = (value >> 3) & 0x1;
                ch[channel].env_pace    = value & 0x7;
                ch[channel].volume      = ch[channel].env_initial;
                if (ch[channel].volume == 0)
                    ch[channel].enabled = 0;
            }
            break;
        case 3:
            if (channel == 3) {
                /* NR43: noise polynomial counter — store raw byte, set LFSR width */
                ch[3].freq_reg    = value;
                ch[3].lfsr_narrow = (value >> 3) & 1;
            } else {
                ch[channel].freq_reg = (ch[channel].freq_reg & 0x700u) |
                                        (uint16_t)value;
            }
            break;
        case 4:
            if (channel != 3) {
                ch[channel].freq_reg = (ch[channel].freq_reg & 0x00FFu) |
                                       ((uint16_t)(value & 0x07) << 8);
            }
            if (value & 0x80) {  /* trigger */
                ch[channel].enabled = 1;
                if (channel == 2) {
                    /* Skip phase reset for continuous note-to-note transitions.
                     * wave_playing is cleared by any mute (reg2 vol=0), so it is
                     * 0 for the first note of a song and 1 for subsequent notes.
                     * Resetting phase on every trigger causes a waveform step of
                     * ~x_prev at the 4.5 Hz arpeggio rate → audible bass/feedback.
                     * Real GB uses the DAC AC-coupling cap to absorb that step;
                     * we avoid it by keeping the wave running uninterrupted. */
                    if (!ch[2].wave_playing)
                        ch[2].phase = ch[2].wave_start_phase;
                    ch[2].wave_playing = 1;
                } else {
                    ch[channel].phase = 0u;
                }
                ch[channel].phase_inc = (channel == 3)
                    ? noise_freq_to_phase_inc((uint8_t)ch[3].freq_reg)
                    : freq_to_phase_inc(ch[channel].freq_reg, channel == 2);
                /* Reload volume and reset envelope counter on trigger.
                 * Wave channel (ch2) has no envelope — its volume is set
                 * by NR32 (wave_vol_shift), so skip envelope reload. */
                if (channel != 2) {
                    ch[channel].volume      = ch[channel].env_initial;
                    ch[channel].env_counter = 0;
                }
                if (channel == 3) ch[channel].lfsr = 0x7FFF;
                /* CH0 sweep: on trigger, copy freq to shadow register,
                 * reset timer, set enabled flag.  If shift is non-zero,
                 * perform an immediate frequency calculation + overflow
                 * check — this can disable the channel right on trigger. */
                if (channel == 0) {
                    ch[0].sweep_timer = 0;
                    /* Enabled if either pace or shift is non-zero */
                    if (ch[0].sweep_pace > 0 || ch[0].sweep_shift > 0) {
                        /* Immediate overflow check when shift != 0 */
                        if (ch[0].sweep_shift > 0) {
                            uint16_t delta = ch[0].freq_reg >> ch[0].sweep_shift;
                            uint16_t new_freq;
                            if (ch[0].sweep_dir == 0) {
                                new_freq = ch[0].freq_reg + delta;
                            } else {
                                new_freq = (delta > ch[0].freq_reg) ? 0 : ch[0].freq_reg - delta;
                            }
                            if (new_freq >= 2048) {
                                ch[0].enabled = 0;
                            }
                        }
                    }
                }
            }
            break;
    }
    SDL_UnlockMutex(audio_mutex);
}

/* ---- Text-beep SFX (SFX_PRESS_AB) --------------------------------
 * From audio/sfx/press_ab_1.asm:
 *   square_note  0, 9, 1, 1984   → 1 frame  at 2048 Hz
 *   square_note  0, 8, 1, 2000   → 1 frame  at 2730 Hz
 *   square_note  0, 9, 1, 1984   → 1 frame  at 2048 Hz
 *   square_note 12, 10, 1, 2000  → 13 frames at 2730 Hz, fading out
 *
 * env_byte = NRx2: bits 7-4 = initial vol, bit 3 = dir (0=dec), bits 2-0 = pace.
 * duty = 0 (12.5%) per original Ch5 default.  Envelope pace=1 matches fade=1 arg.
 */
typedef struct { uint16_t freq; uint8_t env_byte; uint8_t frames; } sfx_note_t;

static const sfx_note_t kPressAB[] = {
    { 1984, 0x91,  1 },   /* vol=9,  dec, pace=1 — 1 frame, barely fades */
    { 2000, 0x81,  1 },   /* vol=8,  dec, pace=1 */
    { 1984, 0x91,  1 },   /* vol=9,  dec, pace=1 */
    { 2000, 0xA1, 13 },   /* vol=10, dec, pace=1 — fades to silence over ~10 frames */
};

static int sfx_step  = -1;   /* -1 = idle */
static int sfx_timer = 0;

static void sfx_fire(int step) {
    const sfx_note_t *n = &kPressAB[step];
    /* Route through Audio_WriteReg so envelope state is set correctly.
     * NR11 duty=0 (12.5%) — softer than 50%, matches original SFX Ch5 default. */
    Audio_WriteReg(0, 1, (2 << 6) | 0x3F);          /* duty=2 (50%), max length */
    Audio_WriteReg(0, 2, n->env_byte);               /* volume + envelope */
    Audio_WriteReg(0, 3, (uint8_t)(n->freq & 0xFF)); /* freq low */
    Audio_WriteReg(0, 4, (uint8_t)(((n->freq >> 8) & 0x07) | 0x80)); /* freq high + trigger */
    sfx_timer = n->frames;
}

void Audio_PlaySFX_PressAB(void) {
    sfx_step = 0;
    Music_SuspendChannel(0);  /* pause melody so sequencer doesn't fight SFX */
    sfx_fire(0);
}

/* ---- Ledge-hop SFX (SFX_LEDGE → SFX_Ledge_1_Ch5) -----------------
 * From audio/sfx/ledge_1.asm:
 *   duty_cycle 2
 *   pitch_sweep 9, 5        → NR10 = 0x95: pace=1, increase, shift=5
 *   square_note 15, 15, 2, 1024
 *   pitch_sweep 0, 8        → NR10 = 0x08: disable sweep
 *   sound_ret
 *
 * NR10 = 0x95 (1001 0101): bits 6-4=001 pace=1, bit3=0 increase, bits 2-0=101 shift=5.
 * Each sweep step fires every 1 × (SAMPLE_RATE/128) ≈ 344 samples.
 * freq_reg starts at 1024; rises ~32 steps over 15 frames → rapid pitch ascent.
 * env_byte = 0xF2: initial vol=15, dir=decrease, pace=2.
 */
#define LEDGE_SFX_FRAMES 20   /* ~15 frames for the note + a few for sweep tail */
static int ledge_sfx_active = 0;
static int ledge_sfx_timer  = 0;

void Audio_PlaySFX_Ledge(void) {
    Music_SuspendChannel(0);
    Audio_WriteReg(0, 0, 0x95);                              /* NR10: sweep on */
    Audio_WriteReg(0, 1, (2 << 6) | 0x3F);                  /* duty=2 (50%) */
    Audio_WriteReg(0, 2, 0xF2);                              /* vol=15, dec, pace=2 */
    Audio_WriteReg(0, 3, (uint8_t)(1024 & 0xFF));            /* freq low */
    Audio_WriteReg(0, 4, (uint8_t)(((1024 >> 8) & 0x07) | 0x80)); /* freq high + trigger */
    ledge_sfx_active = 1;
    ledge_sfx_timer  = LEDGE_SFX_FRAMES;
}

/* ---- Building enter/exit SFX (noise channel, ch[3]) ---------------
 * Original: home/overworld.asm PlayMapChangeSound → SFX_GO_INSIDE /
 *           SFX_GO_OUTSIDE, both on Ch8 (GB CH4, noise).
 *
 * Noise channel never carries music so no Music_Suspend needed.
 *
 * noise_note length, volume, fade, nr43:
 *   env_byte = (volume << 4) | fade   (fade > 0 = decrease)
 *
 * SFX_Go_Inside_1_Ch8:                  SFX_Go_Outside_1_Ch8:
 *   noise_note 9,  15, 1, 68 (0x44)      noise_note  2, 15, 1, 84 (0x54)
 *   noise_note 8,  13, 1, 67 (0x43)      noise_note 12,  7, 1, 35 (0x23)
 *                                         noise_note  2, 11, 1, 84 (0x54)
 *                                         noise_note 12,  6, 1, 35 (0x23)
 *                                         noise_note  6,  4, 1, 84 (0x54)
 */
typedef struct { uint8_t nr43; uint8_t env_byte; uint8_t frames; } noise_note_t;

static const noise_note_t kGoInside[] = {
    { 0x44, 0xF1,  9 },
    { 0x43, 0xD1,  8 },
};
static const noise_note_t kGoOutside[] = {
    { 0x54, 0xF1,  2 },
    { 0x23, 0x71, 12 },
    { 0x54, 0xB1,  2 },
    { 0x23, 0x61, 12 },
    { 0x54, 0x41,  6 },
};

static const noise_note_t *noise_sfx_seq   = NULL;
static int                 noise_sfx_count = 0;
static int                 noise_sfx_step  = -1;
static int                 noise_sfx_timer = 0;

static void noise_sfx_fire(int step) {
    const noise_note_t *n = &noise_sfx_seq[step];
    Audio_WriteReg(3, 2, n->env_byte);
    Audio_WriteReg(3, 3, n->nr43);
    Audio_WriteReg(3, 4, 0x80);  /* trigger */
    noise_sfx_timer = n->frames;
}

static void play_noise_sfx(const noise_note_t *seq, int count) {
    noise_sfx_seq   = seq;
    noise_sfx_count = count;
    noise_sfx_step  = 0;
    noise_sfx_fire(0);
}

/* ---- Start menu open SFX (noise channel — Ch8) --------------------
 * Source: audio/sfx/start_menu_1.asm (SFX_Start_Menu_1_Ch8):
 *   noise_note 1, 14, 2, 51   → frames=1, vol=14, fade=2, nr43=0x33
 *   noise_note 8, 14, 1, 34   → frames=8, vol=14, fade=1, nr43=0x22
 * env_byte = (vol << 4) | fade  (all fades are positive = decreasing) */
static const noise_note_t kStartMenu[] = {
    { 0x33, 0xE2,  1 },
    { 0x22, 0xE1,  8 },
};

void Audio_PlaySFX_StartMenu(void) {
    play_noise_sfx(kStartMenu, 2);
}

void Audio_PlaySFX_GoInside(void) {
    play_noise_sfx(kGoInside, 2);
}

void Audio_PlaySFX_GoOutside(void) {
    play_noise_sfx(kGoOutside, 5);
}

/* ---- Battle hit SFX (noise channel — Ch8) ------------------------
 * Source: audio/sfx/damage.asm, super_effective.asm, not_very_effective.asm
 * noise_note length, volume, fade, nr43
 *   env_byte = (volume << 4) | (fade < 0 ? 0x08 | (-fade) : fade) */
static const noise_note_t kDamage[] = {
    { 0x44, 0xF4,  2 },   /* noise_note  2, 15, 4, 68  */
    { 0x14, 0xF4,  2 },   /* noise_note  2, 15, 4, 20  */
    { 0x32, 0xF1, 15 },   /* noise_note 15, 15, 1, 50  */
};
static const noise_note_t kSuperEffective[] = {
    { 0x34, 0xF1,  4 },   /* noise_note  4, 15, 1, 52  */
    { 0x64, 0xF2, 15 },   /* noise_note 15, 15, 2, 100 */
};
static const noise_note_t kNotVeryEffective[] = {
    { 0x55, 0x8F,  4 },   /* noise_note  4,  8,-7, 85  vol=8, inc, pace=7 */
    { 0x44, 0xF4,  2 },   /* noise_note  2, 15, 4, 68  */
    { 0x22, 0xF4,  8 },   /* noise_note  8, 15, 4, 34  */
    { 0x21, 0xF2, 15 },   /* noise_note 15, 15, 2, 33  */
};

void Audio_PlaySFX_BattleHit(uint8_t dmg_mult) {
    if (dmg_mult == 0) return;  /* immune — no sound */
    if (dmg_mult >= 20)
        play_noise_sfx(kSuperEffective,   2);
    else if (dmg_mult <= 9)
        play_noise_sfx(kNotVeryEffective, 4);
    else
        play_noise_sfx(kDamage,           3);
}

/* ---- Ball poof SFX (SFX_BALL_POOF) --------------------------------
 * SFX_Ball_Poof_Ch5:
 *   duty_cycle 2       → NR11 duty=2 (50%)
 *   pitch_sweep 1, 6   → NR10=0x16: period=1, increase, shift=6
 *   square_note 15, 15, 2, 1024
 *   pitch_sweep 0, 8   → NR10=0x08 (disable sweep) after note
 *
 * SFX_Ball_Poof_Ch8:
 *   noise_note 15, 10, 2, 34   → nr43=0x22, env=(10<<4)|2=0xA2, 15f
 */
static const noise_note_t kBallPoof[] = {
    { 0x22, 0xA2, 15 },
};
static int ball_poof_timer = 0;

void Audio_PlaySFX_BallPoof(void) {
    /* Cancel any in-progress channel-0 SFX (PressAB, ledge) so they don't
     * overwrite the poof note on their next sfx_fire / resume call. */
    sfx_step        = -1;
    ledge_sfx_active = 0;
    Music_SuspendChannel(0);
    Audio_WriteReg(0, 0, 0x16);                                    /* NR10: sweep period=1, inc, shift=6 */
    Audio_WriteReg(0, 1, (2 << 6) | 0x3F);                        /* NR11: duty=2 (50%) */
    Audio_WriteReg(0, 2, 0xF2);                                    /* NR12: vol=15, dec, pace=2 */
    Audio_WriteReg(0, 3, (uint8_t)(1024 & 0xFF));                  /* NR13: freq low */
    Audio_WriteReg(0, 4, (uint8_t)(((1024 >> 8) & 0x07) | 0x80)); /* NR14: freq high + trigger */
    ball_poof_timer = 15;
    play_noise_sfx(kBallPoof, 1);
}

/* ---- Faint SFX: SFX_FAINT_FALL then SFX_FAINT_THUD -------------- *
 * SFX_Faint_Fall_Ch5  (square, Ch5 = ch[0]):                        *
 *   duty_cycle 1                                                     *
 *   pitch_sweep 10,-7 → NR10=0xAF (pace=2, dec, shift=7)           *
 *   square_note 15, 15, 2, 1920                                      *
 *   pitch_sweep  0, 8 → NR10=0x08 (disable)                         *
 *                                                                    *
 * SFX_Faint_Thud_Ch5  (square, ch[0]):                              *
 *   square_note 15, 13, 1, 512                                       *
 *   pitch_sweep 0, 8 (disable)                                       *
 *                                                                    *
 * SFX_Faint_Thud_Ch8  (noise, ch[3]):                               *
 *   noise_note  4, 15, 5, 51                                         *
 *   noise_note  8, 15, 4, 34                                         *
 *   noise_note 15, 15, 2, 33                                         */
static const noise_note_t kFaintThudNoise[] = {
    { 0x33, 0xF5,  4 },   /* noise_note  4, 15, 5, 51 */
    { 0x22, 0xF4,  8 },   /* noise_note  8, 15, 4, 34 */
    { 0x21, 0xF2, 15 },   /* noise_note 15, 15, 2, 33 */
};

typedef enum { FAINT_IDLE = 0, FAINT_FALL, FAINT_THUD } faint_sfx_state_t;
static faint_sfx_state_t faint_state = FAINT_IDLE;
static int               faint_timer = 0;

/* ---- Run SFX (SFX_RUN → SFX_Run_Ch8, noise channel) --------------
 * audio/sfx/run.asm — 11 noise notes, total ~26 frames.
 * noise_note format: frames, volume, fade, nr43
 * env_byte = (volume << 4) | fade  (fade = decrease pace) */
static const noise_note_t kRunSFX[] = {
    { 0x23, 0x61,  2 },   /* noise_note  2,  6, 1, 35 */
    { 0x33, 0xA1,  2 },   /* noise_note  2, 10, 1, 51 */
    { 0x33, 0xC1,  2 },   /* noise_note  2, 12, 1, 51 */
    { 0x11, 0x51,  2 },   /* noise_note  2,  5, 1, 17 */
    { 0x33, 0xF1,  2 },   /* noise_note  2, 15, 1, 51 */
    { 0x11, 0x41,  2 },   /* noise_note  2,  4, 1, 17 */
    { 0x33, 0xC1,  2 },   /* noise_note  2, 12, 1, 51 */
    { 0x11, 0x31,  2 },   /* noise_note  2,  3, 1, 17 */
    { 0x33, 0x81,  2 },   /* noise_note  2,  8, 1, 51 */
    { 0x11, 0x31,  2 },   /* noise_note  2,  3, 1, 17 */
    { 0x33, 0x41,  8 },   /* noise_note  8,  4, 1, 51 */
};

void Audio_PlaySFX_Run(void) {
    play_noise_sfx(kRunSFX, (int)(sizeof(kRunSFX) / sizeof(kRunSFX[0])));
}

/* ---- Level-Up jingle (SFX_LEVEL_UP) — Ch5 (ch[0]) + Ch6 (ch[1]) ---
 * audio/sfx/level_up.asm:
 *   SFX_Level_Up_Ch5: tempo 256, speed ladder, duty=2, toggle_perfect_pitch
 *   SFX_Level_Up_Ch6: tempo 256, speed ladder, duty=2, vibrato (skipped)
 *
 * Frequencies computed via calc_freq(note_idx, pokered_octave, perfect_pitch):
 *   Ch5 uses perfect_pitch=1 (freq+1), Ch6 does not.
 *
 * note_evt layout: { freq, frames, duty, volume, env_byte }
 * env_byte = NRx2 = (vol << 4) | fade   (fade = decrease-pace nibble)
 *
 * Both channels share identical frame durations so one timer drives both.
 * Total duration: 24+8+8+8+12+12+12+48 = 132 frames (~2.2 s). */
typedef struct { uint16_t freq; uint8_t env; uint8_t frames; } sq_sfx_note_t;

static const sq_sfx_note_t kLvlUpCh1[] = {
    /* note_type 6,11,4: F_4 */  { 1861, 0xB4, 24 },
    /* note_type 4,11,2: C_4 */  { 1798, 0xB2,  8 },
    /*              F_4 */       { 1861, 0xB2,  8 },
    /*              C_4 */       { 1798, 0xB2,  8 },
    /* note_type 6,11,3: D#4 */  { 1838, 0xB3, 12 },
    /*              D#4 */       { 1838, 0xB3, 12 },
    /*              E_4 */       { 1850, 0xB3, 12 },
    /* note_type 6,11,4: F_4 */  { 1861, 0xB4, 48 },
};

static const sq_sfx_note_t kLvlUpCh2[] = {
    /* note_type 6,12,4: A_4 */  { 1899, 0xC4, 24 },
    /* note_type 4,12,2: A_4 */  { 1899, 0xC2,  8 },
    /*              A_4 */       { 1899, 0xC2,  8 },
    /*              A_4 */       { 1899, 0xC2,  8 },
    /* note_type 6,12,4: A#4 */  { 1907, 0xC4, 12 },
    /*              A#4 */       { 1907, 0xC4, 12 },
    /*              A#4 */       { 1907, 0xC4, 12 },
    /* note_type 6,12,4: A_4 */  { 1899, 0xC4, 48 },
};

#define LVL_UP_NOTES 8
static int lvl_up_step  = -1;
static int lvl_up_timer =  0;

static void lvl_up_fire(int step) {
    const sq_sfx_note_t *n1 = &kLvlUpCh1[step];
    const sq_sfx_note_t *n2 = &kLvlUpCh2[step];
    /* Ch1 (ch[0]) — disable sweep, set duty/env/freq, trigger */
    Audio_WriteReg(0, 0, 0x08);
    Audio_WriteReg(0, 1, (2 << 6) | 0x3F);
    Audio_WriteReg(0, 2, n1->env);
    Audio_WriteReg(0, 3, (uint8_t)(n1->freq & 0xFF));
    Audio_WriteReg(0, 4, (uint8_t)(((n1->freq >> 8) & 0x07) | 0x80));
    /* Ch2 (ch[1]) */
    Audio_WriteReg(1, 1, (2 << 6) | 0x3F);
    Audio_WriteReg(1, 2, n2->env);
    Audio_WriteReg(1, 3, (uint8_t)(n2->freq & 0xFF));
    Audio_WriteReg(1, 4, (uint8_t)(((n2->freq >> 8) & 0x07) | 0x80));
    lvl_up_timer = n1->frames;
}

/* ---- Healing machine SFX (SFX_HEALING_MACHINE) — Ch5 (ch[0]) -------
 * audio/sfx/healing_machine_1.asm:
 *   duty_cycle 2
 *   pitch_sweep 2, -4   → NR10 = (pace=2<<4)|(dec<<3)|(shift=4) = 0x2C
 *   square_note 4, 15, 2, 1280
 *   pitch_sweep 2,  2   → NR10 = (pace=2<<4)|(inc=0)|(shift=2)  = 0x22
 *   square_note 2, 15, 1, 1280
 *   pitch_sweep 0,  0   → NR10 = 0x00 (sweep off)
 *   square_note 1,  0,  0, 0  (silence)
 */
typedef struct { uint8_t sweep; uint16_t freq; uint8_t env; uint8_t frames; } heal_sfx_note_t;

static const heal_sfx_note_t kHealSFX[] = {
    { 0x2C, 1280, 0xF2, 4 },   /* down-sweep, vol=15, fade=2 */
    { 0x22, 1280, 0xF1, 2 },   /* up-sweep,   vol=15, fade=1 */
    { 0x00,    0, 0x00, 1 },   /* sweep off,  silence */
};
#define HEAL_SFX_NOTES  3

static int heal_sfx_step  = -1;
static int heal_sfx_timer =  0;

static void heal_sfx_fire(int step) {
    const heal_sfx_note_t *n = &kHealSFX[step];
    Audio_WriteReg(0, 0, n->sweep);
    Audio_WriteReg(0, 1, (2 << 6) | 0x3F);
    if (n->freq > 0) {
        Audio_WriteReg(0, 2, n->env);
        Audio_WriteReg(0, 3, (uint8_t)(n->freq & 0xFF));
        Audio_WriteReg(0, 4, (uint8_t)(((n->freq >> 8) & 0x07) | 0x80));
    } else {
        Audio_WriteReg(0, 2, 0x00);
        Audio_WriteReg(0, 4, 0x00);
    }
    heal_sfx_timer = n->frames;
}

void Audio_PlaySFX_HealingMachine(void) {
    Music_SuspendChannel(0);
    heal_sfx_step = 0;
    heal_sfx_fire(0);
}

void Audio_PlaySFX_LevelUp(void) {
    Music_SuspendChannel(0);
    Music_SuspendChannel(1);
    lvl_up_step = 0;
    lvl_up_fire(0);
}

/* ---- Purchase kaching SFX (SFX_PURCHASE → SFX_Purchase_1) ----------
 * audio/sfx/purchase_1.asm:
 *
 * SFX_Purchase_1_Ch5 (square ch[0]):
 *   duty_cycle 2
 *   square_note  4, 14, 1, 1792   → env=0xE1, 4 frames
 *   square_note  8, 15, 2, 2016   → env=0xF2, 8 frames
 *
 * SFX_Purchase_1_Ch6 (square ch[1]):
 *   duty_cycle 1
 *   square_note  1,  0, 8,    0   → silent lead-in, 1 frame
 *   square_note  4,  9, 1, 1729   → env=0x91, 4 frames
 *   square_note  8, 10, 2, 1953   → env=0xA2, 8 frames
 *
 * The two channels advance independently (different note counts / lengths).
 * Music resumes on both channels when both are done. */
static const sq_sfx_note_t kPurchaseCh1[] = {
    { 1792, 0xE1, 4 },
    { 2016, 0xF2, 8 },
};
static const sq_sfx_note_t kPurchaseCh2[] = {
    {    0, 0x08, 1 },   /* vol=0, fade=8 — silent lead-in */
    { 1729, 0x91, 4 },
    { 1953, 0xA2, 8 },
};
#define PURCHASE_CH1_NOTES 2
#define PURCHASE_CH2_NOTES 3

static int purchase_ch1_step  = -1;
static int purchase_ch1_timer =  0;
static int purchase_ch2_step  = -1;
static int purchase_ch2_timer =  0;
static int purchase_active    =  0;

static void purchase_ch1_fire(int step) {
    const sq_sfx_note_t *n = &kPurchaseCh1[step];
    Audio_WriteReg(0, 0, 0x08);                              /* NR10: disable sweep */
    Audio_WriteReg(0, 1, (2 << 6) | 0x3F);                  /* duty=2 (50%) */
    Audio_WriteReg(0, 2, n->env);
    Audio_WriteReg(0, 3, (uint8_t)(n->freq & 0xFF));
    Audio_WriteReg(0, 4, (uint8_t)(((n->freq >> 8) & 0x07) | 0x80));
    purchase_ch1_timer = n->frames;
}

static void purchase_ch2_fire(int step) {
    const sq_sfx_note_t *n = &kPurchaseCh2[step];
    Audio_WriteReg(1, 1, (2 << 6) | 0x3F);                  /* duty=2 (50%) — source: duty_cycle 2 */
    if (n->freq == 0) {
        /* Silent lead-in: mute channel */
        Audio_WriteReg(1, 2, 0x00);
        Audio_WriteReg(1, 4, 0x00);
    } else {
        Audio_WriteReg(1, 2, n->env);
        Audio_WriteReg(1, 3, (uint8_t)(n->freq & 0xFF));
        Audio_WriteReg(1, 4, (uint8_t)(((n->freq >> 8) & 0x07) | 0x80));
    }
    purchase_ch2_timer = n->frames;
}

void Audio_PlaySFX_Purchase(void) {
    sfx_step       = -1;   /* cancel PressAB if playing */
    lvl_up_step    = -1;   /* cancel level-up if playing */
    Music_SuspendChannel(0);
    Music_SuspendChannel(1);
    purchase_ch1_step = 0;
    purchase_ch2_step = 0;
    purchase_active   = 1;
    purchase_ch1_fire(0);
    purchase_ch2_fire(0);
}

void Audio_PlaySFX_Faint(void) {
    Music_SuspendChannel(0);
    /* NR10 = 0xAF: pace=2, decrease, shift=7 — rapid falling sweep */
    Audio_WriteReg(0, 0, 0xAF);
    Audio_WriteReg(0, 1, (1 << 6) | 0x3F);           /* duty=1 (25%) */
    Audio_WriteReg(0, 2, 0xF2);                       /* vol=15, dec, pace=2 */
    Audio_WriteReg(0, 3, (uint8_t)(1920 & 0xFF));
    Audio_WriteReg(0, 4, (uint8_t)(((1920 >> 8) & 0x07) | 0x80));
    faint_state = FAINT_FALL;
    faint_timer = 15;
}

/* ---- Pokémon cry playback (PlayCry / GetCryData) -------------------
 * Ports home/pokemon.asm:PlayCry → GetCryData → PlaySound.
 * Three channels play simultaneously:
 *   ch5 → audio ch[0] (square CH1)
 *   ch6 → audio ch[1] (square CH2)
 *   ch8 → audio ch[3] (noise  CH4)
 *
 * Pitch modifier: added (signed) to each note's 11-bit GB freq register.
 * Tempo modifier: sfx_tempo = tempo_mod + 0x80 (16-bit).
 *   Duration (frames) = (note.len * sfx_tempo) >> 8.
 *   tempo_mod=0x80 → sfx_tempo=0x100 → duration=len (normal speed).
 *
 * Duty pattern (duty_cycle_pattern): 4 duty values packed as
 *   (d0<<6)|(d1<<4)|(d2<<2)|d3.  Rotates left 2 bits per frame.
 *   duty_cycle (fixed): rotation disabled; duty is constant. */

typedef struct {
    int     step;        /* current note index; -1 = idle */
    int     timer;       /* frames remaining for current note */
    uint8_t duty_state;  /* rotating duty pattern byte */
    uint8_t rotate_duty; /* 1 = pattern advances each frame */
} cry_ch_t;

static cry_ch_t           cry_ch5   = { -1, 0, 0, 0 };
static cry_ch_t           cry_ch6   = { -1, 0, 0, 0 };
static cry_ch_t           cry_ch8_s = { -1, 0, 0, 0 };
static const cry_def_t   *cry_cur   = NULL;
static int8_t             cry_pitch = 0;
static uint16_t           cry_tempo = 256;  /* sfx_tempo = tempo_mod+0x80 */

static int cry_frames(uint8_t len) {
    int f = (int)(((uint32_t)len * (uint32_t)cry_tempo) >> 8);
    return f > 0 ? f : 1;
}

static void cry_sq_fire(int hw_ch, cry_ch_t *st, const cry_sq_ch_t *def) {
    const cry_sq_note_t *n = &def->notes[st->step];
    int freq = (int)n->freq + (int)cry_pitch;
    if (freq < 0)    freq = 0;
    if (freq > 2047) freq = 2047;
    uint8_t duty    = (st->duty_state >> 6) & 3;
    uint8_t env     = (uint8_t)((n->vol << 4) | n->fade);
    uint8_t nrx1    = (uint8_t)((duty << 6) | 0x3F);
    if (hw_ch == 0) Audio_WriteReg(0, 0, 0x08);  /* NR10: disable sweep */
    Audio_WriteReg(hw_ch, 1, nrx1);
    Audio_WriteReg(hw_ch, 2, env);
    Audio_WriteReg(hw_ch, 3, (uint8_t)(freq & 0xFF));
    Audio_WriteReg(hw_ch, 4, (uint8_t)(((freq >> 8) & 0x07) | 0x80));
    st->timer = cry_frames(n->len);
}

static void cry_noise_fire(cry_ch_t *st, const cry_noise_ch_t *def) {
    const cry_noise_note_t *n = &def->notes[st->step];
    Audio_WriteReg(3, 2, (uint8_t)((n->vol << 4) | n->fade));
    Audio_WriteReg(3, 3, n->nr43);
    Audio_WriteReg(3, 4, 0x80);
    st->timer = cry_frames(n->len);
}

void Audio_PlayCry(uint8_t species) {
    /* species: 1-based internal ID; CryData table is 0-based */
    if (species == 0 || species > NUM_POKEMON_CRIES) return;
    const pokemon_cry_t *pc = &g_pokemon_cries[species - 1];
    if (pc->base_cry >= NUM_BASE_CRIES) return;

    cry_cur   = &g_cry_defs[pc->base_cry];
    cry_pitch = pc->pitch_mod;
    cry_tempo = (uint16_t)((uint16_t)pc->tempo_mod + 0x80u);

    Music_SuspendChannel(0);
    Music_SuspendChannel(1);

    if (cry_cur->ch5.n_notes > 0) {
        cry_ch5.step       = 0;
        cry_ch5.duty_state = cry_cur->ch5.duty_pattern;
        cry_ch5.rotate_duty = cry_cur->ch5.rotate_duty;
        cry_sq_fire(0, &cry_ch5, &cry_cur->ch5);
    } else { cry_ch5.step = -1; }

    if (cry_cur->ch6.n_notes > 0) {
        cry_ch6.step       = 0;
        cry_ch6.duty_state = cry_cur->ch6.duty_pattern;
        cry_ch6.rotate_duty = cry_cur->ch6.rotate_duty;
        cry_sq_fire(1, &cry_ch6, &cry_cur->ch6);
    } else { cry_ch6.step = -1; }

    if (cry_cur->ch8.n_notes > 0) {
        cry_ch8_s.step = 0;
        cry_noise_fire(&cry_ch8_s, &cry_cur->ch8);
    } else { cry_ch8_s.step = -1; }
}

/* ---- Frame tick -------------------------------------------------- */
void Audio_Update(void) {
    /* Advance music sequencer */
    Music_Update();

    /* Advance text-beep SFX */
    if (sfx_step >= 0) {
        if (--sfx_timer <= 0) {
            sfx_step++;
            int total = (int)(sizeof(kPressAB) / sizeof(kPressAB[0]));
            if (sfx_step >= total) {
                sfx_step = -1;
                Music_ResumeChannel(0);
            } else {
                sfx_fire(sfx_step);
            }
        }
    }

    /* Advance ledge-hop SFX */
    if (ledge_sfx_active) {
        if (--ledge_sfx_timer <= 0) {
            Audio_WriteReg(0, 0, 0x08);  /* NR10: disable sweep */
            Music_ResumeChannel(0);
            ledge_sfx_active = 0;
        }
    }

    /* Advance ball-poof square SFX */
    if (ball_poof_timer > 0) {
        if (--ball_poof_timer <= 0) {
            Audio_WriteReg(0, 0, 0x08);  /* NR10: disable sweep */
            Music_ResumeChannel(0);
        }
    }

    /* Advance building enter/exit noise SFX */
    if (noise_sfx_step >= 0) {
        if (--noise_sfx_timer <= 0) {
            noise_sfx_step++;
            if (noise_sfx_step >= noise_sfx_count) {
                noise_sfx_step = -1;
            } else {
                noise_sfx_fire(noise_sfx_step);
            }
        }
    }

    /* Advance healing machine SFX */
    if (heal_sfx_step >= 0) {
        if (--heal_sfx_timer <= 0) {
            heal_sfx_step++;
            if (heal_sfx_step >= HEAL_SFX_NOTES) {
                heal_sfx_step = -1;
                Music_ResumeChannel(0);
            } else {
                heal_sfx_fire(heal_sfx_step);
            }
        }
    }

    /* Advance purchase kaching SFX (two independent channels) */
    if (purchase_active) {
        if (purchase_ch1_step >= 0) {
            if (--purchase_ch1_timer <= 0) {
                purchase_ch1_step++;
                if (purchase_ch1_step >= PURCHASE_CH1_NOTES)
                    purchase_ch1_step = -1;
                else
                    purchase_ch1_fire(purchase_ch1_step);
            }
        }
        if (purchase_ch2_step >= 0) {
            if (--purchase_ch2_timer <= 0) {
                purchase_ch2_step++;
                if (purchase_ch2_step >= PURCHASE_CH2_NOTES)
                    purchase_ch2_step = -1;
                else
                    purchase_ch2_fire(purchase_ch2_step);
            }
        }
        if (purchase_ch1_step < 0 && purchase_ch2_step < 0) {
            purchase_active = 0;
            Music_ResumeChannel(0);
            Music_ResumeChannel(1);
        }
    }

    /* Advance level-up jingle */
    if (lvl_up_step >= 0) {
        if (--lvl_up_timer <= 0) {
            lvl_up_step++;
            if (lvl_up_step >= LVL_UP_NOTES) {
                lvl_up_step = -1;
                Music_ResumeChannel(0);
                Music_ResumeChannel(1);
            } else {
                lvl_up_fire(lvl_up_step);
            }
        }
    }

    /* Advance faint fall → thud sequence */
    if (faint_state == FAINT_FALL) {
        if (--faint_timer <= 0) {
            /* Disable sweep, play thud on square */
            Audio_WriteReg(0, 0, 0x08);                           /* NR10: sweep off */
            Audio_WriteReg(0, 1, (1 << 6) | 0x3F);               /* duty=1 */
            Audio_WriteReg(0, 2, 0xD1);                           /* vol=13, dec, pace=1 */
            Audio_WriteReg(0, 3, (uint8_t)(512 & 0xFF));
            Audio_WriteReg(0, 4, (uint8_t)(((512 >> 8) & 0x07) | 0x80));
            /* Simultaneously fire noise thud */
            play_noise_sfx(kFaintThudNoise, 3);
            faint_state = FAINT_THUD;
            faint_timer = 30;   /* ~0.5 s for thud to fade out */
        }
    } else if (faint_state == FAINT_THUD) {
        if (--faint_timer <= 0) {
            Music_ResumeChannel(0);
            faint_state = FAINT_IDLE;
        }
    }

    /* Advance cry sequencer (ch5, ch6, ch8 advance independently) */
    if (cry_cur != NULL) {
        /* ch5 (square CH1 → hw ch[0]) */
        if (cry_ch5.step >= 0) {
            if (cry_ch5.rotate_duty) {
                uint8_t s = cry_ch5.duty_state;
                s = (uint8_t)((s << 2) | (s >> 6));
                cry_ch5.duty_state = s;
                Audio_WriteReg(0, 1, (uint8_t)(((s >> 6) << 6) | 0x3F));
            }
            if (--cry_ch5.timer <= 0) {
                cry_ch5.step++;
                if (cry_ch5.step >= cry_cur->ch5.n_notes) {
                    cry_ch5.step = -1;
                } else {
                    cry_sq_fire(0, &cry_ch5, &cry_cur->ch5);
                }
            }
        }
        /* ch6 (square CH2 → hw ch[1]) */
        if (cry_ch6.step >= 0) {
            if (cry_ch6.rotate_duty) {
                uint8_t s = cry_ch6.duty_state;
                s = (uint8_t)((s << 2) | (s >> 6));
                cry_ch6.duty_state = s;
                Audio_WriteReg(1, 1, (uint8_t)(((s >> 6) << 6) | 0x3F));
            }
            if (--cry_ch6.timer <= 0) {
                cry_ch6.step++;
                if (cry_ch6.step >= cry_cur->ch6.n_notes) {
                    cry_ch6.step = -1;
                } else {
                    cry_sq_fire(1, &cry_ch6, &cry_cur->ch6);
                }
            }
        }
        /* ch8 (noise CH4 → hw ch[3]) */
        if (cry_ch8_s.step >= 0) {
            if (--cry_ch8_s.timer <= 0) {
                cry_ch8_s.step++;
                if (cry_ch8_s.step >= cry_cur->ch8.n_notes) {
                    cry_ch8_s.step = -1;
                } else {
                    cry_noise_fire(&cry_ch8_s, &cry_cur->ch8);
                }
            }
        }
        /* All three channels done → resume music */
        if (cry_ch5.step < 0 && cry_ch6.step < 0 && cry_ch8_s.step < 0) {
            cry_cur = NULL;
            Music_ResumeChannel(0);
            Music_ResumeChannel(1);
        }
    }
}

void Audio_Quit(void) {
    if (audio_dev)   SDL_CloseAudioDevice(audio_dev);
    if (audio_mutex) SDL_DestroyMutex(audio_mutex);
}
