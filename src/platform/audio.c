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
#include "../data/move_sfx_structs.h"
#include "hardware.h"
#include <SDL2/SDL.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

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
    uint16_t ch4_attack_boost; /* short post-trigger accent window (samples) */

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
static int               s_cry_mix_boost = 0;  /* 1 while a cry is active */
static int               s_pc_sfx_mix_boost = 0; /* 1 while PC SFX is active */
typedef struct {
    uint8_t active;
    uint16_t cmd_pos;
    int timer;
    uint8_t rotate_duty;
    uint8_t duty_state;
    uint8_t fixed_duty;
    uint8_t sweep_reg;
    int8_t loop_rem[64];
    const move_sfx_channel_def_t *def;
} move_sfx_chan_rt_t;
static struct {
    uint8_t active;
    uint8_t suspended[4];
    int8_t pitch_add;
    uint16_t tempo;
    move_sfx_chan_rt_t ch[4];
} sMoveSfx = {0};
static int sMoveSfxDebug = 0;

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
 *   r=0: 1048576 / 2^(shift+1)  Hz  (special case: divisor = 0.5)
 *   r>0: 524288  / r / 2^(shift+1)  Hz
 * Mirrors Pan Docs CH4 polynomial counter clock frequency formula. */
static uint32_t noise_freq_to_phase_inc(uint8_t nr43) {
    uint32_t shift = (nr43 >> 4) & 0xF;
    uint32_t r     = nr43 & 7;
    if (shift >= 15) return 0;
    uint64_t hz = (r == 0) ? ((uint64_t)1048576 >> (shift + 1))
                           : ((uint64_t)524288 / r >> (shift + 1));
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
            if (c == 0 && s_pc_sfx_mix_boost) raw = (raw * 4) / 3;
            if (s_cry_mix_boost) raw = (raw * 4) / 3;
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
            /* Allow multiple LFSR clocks in one host sample at high NR43 rates.
             * Single-step capping dulls CH4 transients and makes drums sound muffled. */
            uint32_t lfsr_clocks = ch[3].phase >> 24;
            ch[3].phase &= 0xFFFFFFu;
            while (lfsr_clocks--) {
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
            int32_t raw = (ch[3].lfsr & 1) ? (int32_t)ch[3].volume * 460 : 0;
            if (ch[3].ch4_attack_boost > 0) {
                /* Slight transient lift to better match DMG drum "snap". */
                raw = (raw * 6) / 5;
                ch[3].ch4_attack_boost--;
            }
            if (s_cry_mix_boost) raw = (raw * 4) / 3;
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
                if (channel == 3) {
                    ch[channel].lfsr = 0x7FFF;
                    ch[channel].ch4_attack_boost = (uint16_t)(AUDIO_SAMPLE_RATE / 125); /* ~8ms */
                }
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
/* In the original engine (Audio1_note_length), the command length nybble is
 * incremented before becoming a delay, so square/noise SFX lengths are len+1. */
static int sfx_cmd_len_frames(uint8_t len_field) { return (int)len_field + 1; }
static int sfx_cmd_len_frames_tempo(uint8_t len_field, uint16_t tempo) {
    int raw = (int)len_field + 1;
    int frames = (int)(((uint32_t)raw * (uint32_t)tempo) >> 8);
    return (frames > 0) ? frames : 1;
}

static uint8_t move_sfx_env_byte(int vol, int fade) {
    uint8_t nib = (fade < 0) ? (uint8_t)(0x8 | ((-fade) & 0x7)) : (uint8_t)(fade & 0x7);
    return (uint8_t)(((vol & 0xF) << 4) | nib);
}

static uint8_t move_sfx_sweep_byte(int pace, int slope) {
    uint8_t p = (uint8_t)(pace & 0xF);
    uint8_t s = (uint8_t)(slope < 0 ? (-slope) : slope) & 0x7;
    uint8_t d = (slope < 0) ? 0x8 : 0x0;
    return (uint8_t)((p << 4) | d | s);
}

static void move_sfx_channel_silence(uint8_t hw) {
    if (hw >= 4) return;
    if (hw == 0) Audio_WriteReg(0, 0, 0x08);
    Audio_WriteReg(hw, 2, 0x00);
    Audio_WriteReg(hw, 4, 0x00);
}

static void move_sfx_fire_square(uint8_t hw, move_sfx_chan_rt_t *st, const move_sfx_cmd_t *cmd) {
    int len = cmd->p0;
    int vol = cmd->p1;
    int fade = cmd->p2;
    uint16_t base_freq = (uint16_t)(cmd->p3 & 0x7FF);
    uint8_t lo = (uint8_t)(base_freq & 0xFFu);
    uint8_t hi = (uint8_t)((base_freq >> 8) & 0x07u);
    uint16_t lo_sum = (uint16_t)lo + (uint8_t)sMoveSfx.pitch_add;
    /* ASM parity (Audio2_ApplyFrequencyModifier): add wFrequencyModifier to
     * frequency low byte and carry into high byte. */
    lo = (uint8_t)(lo_sum & 0xFFu);
    if (lo_sum > 0xFFu) hi = (uint8_t)(hi + 1u);
    uint8_t duty = st->rotate_duty ? (uint8_t)((st->duty_state >> 6) & 0x3) : st->fixed_duty;
    if (hw == 0) Audio_WriteReg(0, 0, st->sweep_reg);
    Audio_WriteReg(hw, 1, (uint8_t)((duty << 6) | 0x3F));
    Audio_WriteReg(hw, 2, move_sfx_env_byte(vol, fade));
    Audio_WriteReg(hw, 3, lo);
    Audio_WriteReg(hw, 4, (uint8_t)((hi & 0x07u) | 0x80u));
    st->timer = sfx_cmd_len_frames_tempo((uint8_t)len, sMoveSfx.tempo);
}

static void move_sfx_fire_noise(move_sfx_chan_rt_t *st, const move_sfx_cmd_t *cmd) {
    int len = cmd->p0;
    int vol = cmd->p1;
    int fade = cmd->p2;
    int nr43 = cmd->p3;
    Audio_WriteReg(3, 2, move_sfx_env_byte(vol, fade));
    Audio_WriteReg(3, 3, (uint8_t)nr43);
    Audio_WriteReg(3, 4, 0x80);
    st->timer = sfx_cmd_len_frames_tempo((uint8_t)len, sMoveSfx.tempo);
}

static void move_sfx_advance_channel(uint8_t hw) {
    move_sfx_chan_rt_t *st;
    if (hw >= 4) return;
    st = &sMoveSfx.ch[hw];
    if (!st->active || !st->def) return;

    while (st->cmd_pos < st->def->cmd_count) {
        const move_sfx_cmd_t *cmd = &st->def->cmds[st->cmd_pos];
        switch (cmd->type) {
            case MOVE_SFX_CMD_DUTY_CYCLE:
                st->fixed_duty = (uint8_t)(cmd->p0 & 0x3);
                st->rotate_duty = 0;
                st->cmd_pos++;
                continue;
            case MOVE_SFX_CMD_DUTY_CYCLE_PATTERN:
                st->duty_state = (uint8_t)(((cmd->p0 & 0x3) << 6) |
                                           ((cmd->p1 & 0x3) << 4) |
                                           ((cmd->p2 & 0x3) << 2) |
                                           (cmd->p3 & 0x3));
                st->rotate_duty = 1;
                st->cmd_pos++;
                continue;
            case MOVE_SFX_CMD_PITCH_SWEEP:
                if (hw == 0) {
                    st->sweep_reg = move_sfx_sweep_byte(cmd->p0, cmd->p1);
                    Audio_WriteReg(0, 0, st->sweep_reg);
                }
                st->cmd_pos++;
                continue;
            case MOVE_SFX_CMD_SQUARE_NOTE:
                if (hw == 0 || hw == 1) {
                    move_sfx_fire_square(hw, st, cmd);
                }
                st->cmd_pos++;
                return;
            case MOVE_SFX_CMD_NOISE_NOTE:
                if (hw == 3) {
                    move_sfx_fire_noise(st, cmd);
                }
                st->cmd_pos++;
                return;
            case MOVE_SFX_CMD_SOUND_LOOP: {
                int count = cmd->p0;
                int target = cmd->p1;
                if (count == 0) {
                    st->cmd_pos = (uint16_t)((target >= 0) ? target : 0);
                    continue;
                }
                if (st->cmd_pos < 64u && st->loop_rem[st->cmd_pos] < 0) {
                    st->loop_rem[st->cmd_pos] = (int8_t)(count - 1);
                }
                if (st->cmd_pos < 64u && st->loop_rem[st->cmd_pos] > 0) {
                    st->loop_rem[st->cmd_pos]--;
                    st->cmd_pos = (uint16_t)((target >= 0) ? target : 0);
                    continue;
                }
                if (st->cmd_pos < 64u) st->loop_rem[st->cmd_pos] = -1;
                st->cmd_pos++;
                continue;
            }
            case MOVE_SFX_CMD_SOUND_RET:
            default:
                st->active = 0;
                st->timer = 0;
                move_sfx_channel_silence(hw);
                return;
        }
    }

    st->active = 0;
    st->timer = 0;
    move_sfx_channel_silence(hw);
}

int Audio_PlayMoveSFXBySymbol(const char *symbol) {
    return Audio_PlayMoveSFXBySymbolModified(symbol, 0, 0x80);
}

void Audio_SetMoveSfxDebug(int on) {
    sMoveSfxDebug = on ? 1 : 0;
}

int Audio_IsMoveSfxDebug(void) {
    return sMoveSfxDebug;
}

int Audio_PlayMoveSFXBySymbolModified(const char *symbol, int8_t pitch_add, uint8_t tempo_mod) {
    uint16_t i;
    uint16_t c;
    if (!symbol || !*symbol) return 0;

    if (sMoveSfx.active) {
        for (c = 0; c < 4; c++) {
            sMoveSfx.ch[c].active = 0;
            sMoveSfx.ch[c].def = NULL;
            move_sfx_channel_silence((uint8_t)c);
            if (sMoveSfx.suspended[c]) {
                Music_ResumeChannel((uint8_t)c);
                sMoveSfx.suspended[c] = 0;
            }
        }
        sMoveSfx.active = 0;
    }

    for (i = 0; i < gMoveSfxStructCount; i++) {
        const move_sfx_def_t *def = &gMoveSfxStructs[i];
        {
            const unsigned char *a = (const unsigned char *)def->symbol;
            const unsigned char *b = (const unsigned char *)symbol;
            int eq = 1;
            while (*a || *b) {
                if (toupper(*a) != toupper(*b)) {
                    eq = 0;
                    break;
                }
                if (*a) a++;
                if (*b) b++;
            }
            if (!eq) continue;
        }

        memset(&sMoveSfx, 0, sizeof(sMoveSfx));
        sMoveSfx.active = 1;
        sMoveSfx.pitch_add = pitch_add;
        sMoveSfx.tempo = (uint16_t)tempo_mod + 0x80u;
        if (sMoveSfxDebug) {
            printf("[SFXDBG] resolve sym=%s -> def[%u]=%s channels=%u pitch_add=%d tempo_mod=0x%02X\n",
                   symbol, (unsigned)i, def->symbol ? def->symbol : "(null)",
                   (unsigned)def->channel_count, (int)pitch_add, (unsigned)tempo_mod);
        }

        for (c = 0; c < def->channel_count; c++) {
            const move_sfx_channel_def_t *chd = &def->channels[c];
            uint8_t hw = (uint8_t)(chd->hw_channel - 5u);
            if (chd->hw_channel == 8u) hw = 3u;
            if (hw > 3u) continue;
            sMoveSfx.ch[hw].active = 1;
            sMoveSfx.ch[hw].def = chd;
            sMoveSfx.ch[hw].cmd_pos = 0;
            sMoveSfx.ch[hw].timer = 0;
            sMoveSfx.ch[hw].fixed_duty = 2u;
            sMoveSfx.ch[hw].sweep_reg = 0x08u;
            memset(sMoveSfx.ch[hw].loop_rem, -1, sizeof(sMoveSfx.ch[hw].loop_rem));
            if (!sMoveSfx.suspended[hw]) {
                Music_SuspendChannel(hw);
                sMoveSfx.suspended[hw] = 1;
            }
            move_sfx_advance_channel(hw);
        }
        return 1;
    }
    return 0;
}

static const sfx_note_t kPressAB[] = {
    { 1984, 0x91,  1 },   /* vol=9,  dec, pace=1 — 1 frame, barely fades */
    { 2000, 0x81,  1 },   /* vol=8,  dec, pace=1 */
    { 1984, 0x91,  1 },   /* vol=9,  dec, pace=1 */
    { 2000, 0xA1, 13 },   /* vol=10, dec, pace=1 — fades to silence over ~10 frames */
};

static int sfx_step  = -1;   /* -1 = idle */
static int sfx_timer = 0;

/* PressAB uses CH5 (our channel 0) as a low-priority UI chirp.
 * When a higher-priority CH5 SFX starts, cancel PressAB and release its
 * music-channel ownership immediately to avoid suspend-count drift. */
static void cancel_pressab_sfx(void) {
    if (sfx_step >= 0) {
        sfx_step = -1;
        Music_ResumeChannel(0);
    }
}

static void sfx_fire(int step) {
    const sfx_note_t *n = &kPressAB[step];
    /* Route through Audio_WriteReg so envelope state is set correctly.
     * NR11 duty=0 (12.5%) — softer than 50%, matches original SFX Ch5 default. */
    Audio_WriteReg(0, 1, (2 << 6) | 0x3F);          /* duty=2 (50%), max length */
    Audio_WriteReg(0, 2, n->env_byte);               /* volume + envelope */
    Audio_WriteReg(0, 3, (uint8_t)(n->freq & 0xFF)); /* freq low */
    Audio_WriteReg(0, 4, (uint8_t)(((n->freq >> 8) & 0x07) | 0x80)); /* freq high + trigger */
    sfx_timer = sfx_cmd_len_frames(n->frames);
}

void Audio_PlaySFX_PressAB(void) {
    /* Keep button chirps low-priority: if any SFX is active, don't let
     * PressAB preempt it (ASM-style channel arbitration behavior). */
    if (Audio_IsSFXPlaying())
        return;
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

/* ---- Collision bump SFX (SFX_COLLISION → SFX_Collision_1_Ch5) -----
 * audio/sfx/collision_1.asm:
 *   duty_cycle 2
 *   pitch_sweep 5, -2          → NR10=0x5A: pace=5, decrease, shift=2
 *   square_note 15, 15, 1, 768
 *   pitch_sweep 0, 8           → NR10=0x08 (disable sweep)
 *
 * Original only plays if the SFX is not already active (CHAN5 check).
 */
#define COLLISION_SFX_FRAMES 18
static int collision_sfx_active = 0;
static int collision_sfx_timer  = 0;

void Audio_PlaySFX_Collision(void) {
    if (collision_sfx_active) return;   /* already playing — skip */
    Music_SuspendChannel(0);
    Audio_WriteReg(0, 0, 0x5A);                              /* NR10: pace=5, dec, shift=2 */
    Audio_WriteReg(0, 1, (2 << 6) | 0x3F);                  /* NR11: duty=2 (50%) */
    Audio_WriteReg(0, 2, 0xF1);                              /* NR12: vol=15, dec, pace=1 */
    Audio_WriteReg(0, 3, (uint8_t)(768 & 0xFF));             /* NR13: freq low */
    Audio_WriteReg(0, 4, (uint8_t)(((768 >> 8) & 0x07) | 0x80)); /* NR14: freq high + trigger */
    collision_sfx_active = 1;
    collision_sfx_timer  = COLLISION_SFX_FRAMES;
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
static int                 noise_music_suspended = 0;

static void noise_sfx_fire(int step) {
    const noise_note_t *n = &noise_sfx_seq[step];
    Audio_WriteReg(3, 2, n->env_byte);
    Audio_WriteReg(3, 3, n->nr43);
    Audio_WriteReg(3, 4, 0x80);  /* trigger */
    noise_sfx_timer = sfx_cmd_len_frames(n->frames);
}

static void play_noise_sfx(const noise_note_t *seq, int count) {
    if (!noise_music_suspended) {
        Music_SuspendChannel(3);
        noise_music_suspended = 1;
    }
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

/* ---- PC menu SFX (square channel — Ch5) ---------------------------
 * Source:
 *   audio/sfx/turn_on_pc_1.asm   (SFX_TURN_ON_PC)
 *   audio/sfx/enter_pc_1.asm     (SFX_ENTER_PC)
 *   audio/sfx/turn_off_pc_1.asm  (SFX_TURN_OFF_PC)
 */
static const sfx_note_t kTurnOnPC[] = {
    { 1984, 0xF2, 15 },
    {    0, 0x00, 15 },
    { 1920, 0xA1,  3 },
    { 1792, 0xA1,  3 },
    { 1856, 0xA1,  3 },
    { 1792, 0xA1,  3 },
    { 1920, 0xA1,  3 },
    { 1792, 0xA1,  3 },
    { 1984, 0xA1,  3 },
    { 1792, 0xA1,  8 },
};
static const sfx_note_t kEnterPC[] = {
    { 1792, 0xF0, 6 },
    {    0, 0x00, 4 },
    { 1792, 0xF0, 6 },
    {    0, 0x00, 1 },
};
static const sfx_note_t kTurnOffPC[] = {
    { 1536, 0xF0, 4 },
    { 1024, 0xF0, 4 },
    {  512, 0xF0, 4 },
    {    0, 0x00, 1 },
};

static const sfx_note_t *pc_sfx_seq   = NULL;
static int               pc_sfx_count = 0;
static int               pc_sfx_step  = -1;
static int               pc_sfx_timer = 0;

static void pc_sfx_fire(int step) {
    const sfx_note_t *n = &pc_sfx_seq[step];
    Audio_WriteReg(0, 0, 0x08);                             /* disable sweep */
    Audio_WriteReg(0, 1, (2 << 6) | 0x3F);                 /* duty=2 (50%) */
    if (n->freq > 0) {
        Audio_WriteReg(0, 2, n->env_byte);
        Audio_WriteReg(0, 3, (uint8_t)(n->freq & 0xFF));
        Audio_WriteReg(0, 4, (uint8_t)(((n->freq >> 8) & 0x07) | 0x80));
    } else {
        Audio_WriteReg(0, 2, 0x00);
        Audio_WriteReg(0, 4, 0x00);
    }
    pc_sfx_timer = sfx_cmd_len_frames(n->frames);
}

static void play_pc_sfx(const sfx_note_t *seq, int count) {
    cancel_pressab_sfx(); /* cancel text chirp if active (shared Ch5) */
    Music_SuspendChannel(0);
    s_pc_sfx_mix_boost = 1;
    pc_sfx_seq   = seq;
    pc_sfx_count = count;
    pc_sfx_step  = 0;
    pc_sfx_fire(0);
}

void Audio_PlaySFX_StartMenu(void) {
    play_noise_sfx(kStartMenu, 2);
}

void Audio_PlaySFX_TurnOnPC(void) {
    play_pc_sfx(kTurnOnPC, (int)(sizeof(kTurnOnPC) / sizeof(kTurnOnPC[0])));
}

void Audio_PlaySFX_EnterPC(void) {
    play_pc_sfx(kEnterPC, (int)(sizeof(kEnterPC) / sizeof(kEnterPC[0])));
}

void Audio_PlaySFX_TurnOffPC(void) {
    play_pc_sfx(kTurnOffPC, (int)(sizeof(kTurnOffPC) / sizeof(kTurnOffPC[0])));
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
    cancel_pressab_sfx();
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

/* ---- Imported move SFX (direct ASM ports) ---------------------------
 * SFX_Battle_24 (Absorb / Leech Life family):
 *   Ch5: duty 1, pitch_sweep 9,7, square_note 15,15,2,1792
 *   Ch8: noise_note 15,3,-7,34 ; noise_note 15,15,2,33
 */
static const noise_note_t kBattle24Noise[] = {
    { 0x22, 0x3F, 15 },
    { 0x21, 0xF2, 15 },
};
static int battle24_timer = 0;
void Audio_PlaySFX_Battle24(void) {
    Music_SuspendChannel(0);
    Audio_WriteReg(0, 0, 0x97);
    Audio_WriteReg(0, 1, (1 << 6) | 0x3F);
    Audio_WriteReg(0, 2, 0xF2);
    Audio_WriteReg(0, 3, (uint8_t)(1792 & 0xFF));
    Audio_WriteReg(0, 4, (uint8_t)(((1792 >> 8) & 0x07) | 0x80));
    battle24_timer = sfx_cmd_len_frames(15);
    play_noise_sfx(kBattle24Noise, 2);
}

/* SFX_Battle_28 (Aurora Beam / Ice Beam family)
 * audio/sfx/battle_28.asm:
 *   Ch5: duty 0, (1984,1792) loop x12
 *   Ch6: duty pattern 2,3,0,3, (1985,1793) loop x12
 *   Ch8: noise (73,41) loop x6
 */
static const noise_note_t kBattle28Noise[] = {
    { 73, 0xD1, 1 }, { 41, 0xD1, 1 },
    { 73, 0xD1, 1 }, { 41, 0xD1, 1 },
    { 73, 0xD1, 1 }, { 41, 0xD1, 1 },
    { 73, 0xD1, 1 }, { 41, 0xD1, 1 },
    { 73, 0xD1, 1 }, { 41, 0xD1, 1 },
    { 73, 0xD1, 1 }, { 41, 0xD1, 1 },
};
static int battle28_step = -1;
static int battle28_timer = 0;
static int battle28_active = 0;
static int battle28_music_suspended = 0;

static void battle28_ch1_fire(int step) {
    uint16_t freq = (step & 1) ? 1792u : 1984u;
    Audio_WriteReg(0, 0, 0x08);
    Audio_WriteReg(0, 1, (0u << 6) | 0x3Fu);
    Audio_WriteReg(0, 2, 0xF1);
    Audio_WriteReg(0, 3, (uint8_t)(freq & 0xFFu));
    Audio_WriteReg(0, 4, (uint8_t)(((freq >> 8) & 0x07u) | 0x80u));
}

static void battle28_ch2_fire(int step) {
    static const uint8_t duty_pattern[4] = { 2u, 3u, 0u, 3u };
    uint16_t freq = (step & 1) ? 1793u : 1985u;
    uint8_t duty = duty_pattern[(uint8_t)step & 3u];
    Audio_WriteReg(1, 1, (uint8_t)((duty << 6) | 0x3Fu));
    Audio_WriteReg(1, 2, 0xE1);
    Audio_WriteReg(1, 3, (uint8_t)(freq & 0xFFu));
    Audio_WriteReg(1, 4, (uint8_t)(((freq >> 8) & 0x07u) | 0x80u));
}

void Audio_PlaySFX_Battle28(void) {
    if (!battle28_music_suspended) {
        Music_SuspendChannel(0);
        Music_SuspendChannel(1);
        battle28_music_suspended = 1;
    }
    battle28_step = 0;
    battle28_timer = sfx_cmd_len_frames(1);
    battle28_active = 1;
    battle28_ch1_fire(0);
    battle28_ch2_fire(0);
    play_noise_sfx(kBattle28Noise, (int)(sizeof(kBattle28Noise) / sizeof(kBattle28Noise[0])));
}

/* SFX_Battle_29 (Blizzard / Dragon Rage family)
 * audio/sfx/battle_29.asm:
 *   Ch5: duty pattern 3,0,2,1
 *        (11,15,3,288) (9,13,3,336) loop x5
 *        then (8,14,3,304) (15,12,2,272)
 *   Ch8: (10,15,3,53) (14,15,6,69) loop x4
 *        then (12,15,4,188) (12,15,5,156) (15,15,4,172)
 */
typedef struct { uint8_t duty; uint16_t freq; uint8_t env; uint8_t frames; } battle29_sq_note_t;
static const battle29_sq_note_t kBattle29Ch1[] = {
    { 3, 288, 0xF3, 11 }, { 0, 336, 0xD3, 9 },
    { 2, 288, 0xF3, 11 }, { 1, 336, 0xD3, 9 },
    { 3, 288, 0xF3, 11 }, { 0, 336, 0xD3, 9 },
    { 2, 288, 0xF3, 11 }, { 1, 336, 0xD3, 9 },
    { 3, 288, 0xF3, 11 }, { 0, 336, 0xD3, 9 },
    { 2, 304, 0xE3, 8 },  { 1, 272, 0xC2, 15 },
};
static const noise_note_t kBattle29Noise[] = {
    { 53, 0xF3, 10 }, { 69, 0xF6, 14 },
    { 53, 0xF3, 10 }, { 69, 0xF6, 14 },
    { 53, 0xF3, 10 }, { 69, 0xF6, 14 },
    { 53, 0xF3, 10 }, { 69, 0xF6, 14 },
    { 188, 0xF4, 12 }, { 156, 0xF5, 12 }, { 172, 0xF4, 15 },
};
static int battle29_step = -1;
static int battle29_timer = 0;
static int battle29_active = 0;
static int battle29_music_suspended = 0;

static void battle29_ch1_fire(int step) {
    const battle29_sq_note_t *n = &kBattle29Ch1[step];
    Audio_WriteReg(0, 0, 0x08);
    Audio_WriteReg(0, 1, (uint8_t)((n->duty << 6) | 0x3Fu));
    Audio_WriteReg(0, 2, n->env);
    Audio_WriteReg(0, 3, (uint8_t)(n->freq & 0xFFu));
    Audio_WriteReg(0, 4, (uint8_t)(((n->freq >> 8) & 0x07u) | 0x80u));
    battle29_timer = sfx_cmd_len_frames(n->frames);
}

void Audio_PlaySFX_Battle29(void) {
    if (!battle29_music_suspended) {
        Music_SuspendChannel(0);
        battle29_music_suspended = 1;
    }
    battle29_step = 0;
    battle29_active = 1;
    battle29_ch1_fire(0);
    play_noise_sfx(kBattle29Noise, (int)(sizeof(kBattle29Noise) / sizeof(kBattle29Noise[0])));
}

/* SFX_Battle_2A (Acid / Hydro Pump family) */
typedef struct { uint8_t duty; uint16_t freq; uint8_t env; uint8_t frames; } battle_sq_note_t;
static const battle_sq_note_t kBattle2ACh1[] = {
    { 0, 1536, 0xF4, 4 }, { 3, 1280, 0xC4, 3 }, { 2, 1536, 0xB5, 5 }, { 1, 1728, 0xE2, 13 },
    { 0, 1536, 0xF4, 4 }, { 3, 1280, 0xC4, 3 }, { 2, 1536, 0xB5, 5 }, { 1, 1728, 0xE2, 13 },
    { 0, 1536, 0xF4, 4 }, { 3, 1280, 0xC4, 3 }, { 2, 1536, 0xB5, 5 }, { 1, 1728, 0xE2, 13 },
    { 0, 1536, 0xD1, 8 },
};
static const battle_sq_note_t kBattle2ACh2[] = {
    { 2, 1504, 0xE4, 5 }, { 0, 1248, 0xB4, 4 }, { 3, 1512, 0xA5, 6 }, { 1, 1696, 0xD1, 14 },
    { 2, 1504, 0xE4, 5 }, { 0, 1248, 0xB4, 4 }, { 3, 1512, 0xA5, 6 }, { 1, 1696, 0xD1, 14 },
    { 2, 1504, 0xE4, 5 }, { 0, 1248, 0xB4, 4 }, { 3, 1512, 0xA5, 6 }, { 1, 1696, 0xD1, 14 },
};
static const noise_note_t kBattle2ANoise[] = {
    { 0x33, 0xC3, 5 }, { 0x43, 0x92, 3 }, { 0x33, 0xB5, 10 }, { 0x32, 0xC3, 15 },
    { 0x33, 0xC3, 5 }, { 0x43, 0x92, 3 }, { 0x33, 0xB5, 10 }, { 0x32, 0xC3, 15 },
};
static int battle2a_ch1_step = -1, battle2a_ch1_timer = 0;
static int battle2a_ch2_step = -1, battle2a_ch2_timer = 0;
static int battle2a_active = 0;
static int battle2a_music_suspended = 0;
static void battle2a_ch1_fire(int step) {
    const battle_sq_note_t *n = &kBattle2ACh1[step];
    Audio_WriteReg(0, 0, 0x08);
    Audio_WriteReg(0, 1, (uint8_t)((n->duty << 6) | 0x3F));
    Audio_WriteReg(0, 2, n->env);
    Audio_WriteReg(0, 3, (uint8_t)(n->freq & 0xFF));
    Audio_WriteReg(0, 4, (uint8_t)(((n->freq >> 8) & 0x07) | 0x80));
    battle2a_ch1_timer = sfx_cmd_len_frames(n->frames);
}
static void battle2a_ch2_fire(int step) {
    const battle_sq_note_t *n = &kBattle2ACh2[step];
    Audio_WriteReg(1, 1, (uint8_t)((n->duty << 6) | 0x3F));
    Audio_WriteReg(1, 2, n->env);
    Audio_WriteReg(1, 3, (uint8_t)(n->freq & 0xFF));
    Audio_WriteReg(1, 4, (uint8_t)(((n->freq >> 8) & 0x07) | 0x80));
    battle2a_ch2_timer = sfx_cmd_len_frames(n->frames);
}
void Audio_PlaySFX_Battle2A(void) {
    if (!battle2a_music_suspended) {
        Music_SuspendChannel(0);
        Music_SuspendChannel(1);
        battle2a_music_suspended = 1;
    }
    battle2a_ch1_step = 0;
    battle2a_ch2_step = 0;
    battle2a_active = 1;
    battle2a_ch1_fire(0);
    battle2a_ch2_fire(0);
    play_noise_sfx(kBattle2ANoise, (int)(sizeof(kBattle2ANoise) / sizeof(kBattle2ANoise[0])));
}

/* SFX_Battle_0D (Acid Armor) */
static const noise_note_t kBattle0DNoise[] = {
    { 0x34, 0x8F, 15 }, { 0x35, 0xF2, 8 }, { 0x55, 0xF1, 10 },
};
void Audio_PlaySFX_Battle0D(void) {
    play_noise_sfx(kBattle0DNoise, (int)(sizeof(kBattle0DNoise) / sizeof(kBattle0DNoise[0])));
}

/* SFX_Faint_Fall only (used by Agility), without thud tail. */
static int faint_fall_only_timer = 0;
void Audio_PlaySFX_FaintFallOnly(void) {
    Music_SuspendChannel(0);
    Audio_WriteReg(0, 0, 0xAF);
    Audio_WriteReg(0, 1, (1 << 6) | 0x3F);
    Audio_WriteReg(0, 2, 0xF2);
    Audio_WriteReg(0, 3, (uint8_t)(1920 & 0xFF));
    Audio_WriteReg(0, 4, (uint8_t)(((1920 >> 8) & 0x07) | 0x80));
    faint_fall_only_timer = sfx_cmd_len_frames(15);
}

/* ---- Ball toss SFX (SFX_BALL_TOSS) --------------------------------
 * audio/sfx/ball_toss.asm
 *
 * Ch5:
 *   duty_cycle 2
 *   pitch_sweep 2,-7 -> NR10=0x2F
 *   square_note 15,15,2,1920
 *
 * Ch6:
 *   duty_cycle 2
 *   square_note 15,12,2,1922
 */
static int ball_toss_ch1_timer = 0;
static int ball_toss_ch2_timer = 0;
static int ball_toss_active    = 0;

void Audio_PlaySFX_BallToss(void) {
    cancel_pressab_sfx();
    ledge_sfx_active    = 0;
    collision_sfx_active = 0;
    ball_poof_timer     = 0;
    Music_SuspendChannel(0);
    Music_SuspendChannel(1);
    Music_SuspendChannel(2);
    Music_SuspendChannel(3);
    Audio_WriteReg(0, 0, 0x2F);
    Audio_WriteReg(0, 1, (2 << 6) | 0x3F);
    Audio_WriteReg(0, 2, 0xF2);
    Audio_WriteReg(0, 3, (uint8_t)(1920 & 0xFF));
    Audio_WriteReg(0, 4, (uint8_t)(((1920 >> 8) & 0x07) | 0x80));
    Audio_WriteReg(1, 1, (2 << 6) | 0x3F);
    Audio_WriteReg(1, 2, 0xC2);
    Audio_WriteReg(1, 3, (uint8_t)(1922 & 0xFF));
    Audio_WriteReg(1, 4, (uint8_t)(((1922 >> 8) & 0x07) | 0x80));
    ball_toss_ch1_timer = sfx_cmd_len_frames(15);
    ball_toss_ch2_timer = sfx_cmd_len_frames(15);
    ball_toss_active    = 1;
}

/* ---- Tink SFX (SFX_TINK) ------------------------------------------
 * audio/sfx/tink_1.asm
 *
 * Ch5:
 *   duty_cycle 2
 *   pitch_sweep 3,-2 -> NR10=0x3A
 *   square_note 4,15,2,512
 *   pitch_sweep 2,2  -> NR10=0x22
 *   square_note 8,14,2,512
 */
typedef struct { uint8_t sweep; uint16_t freq; uint8_t env; uint8_t frames; } tink_sfx_note_t;
static const tink_sfx_note_t kTinkSFX[] = {
    { 0x3A, 512, 0xF2, 4 },
    { 0x22, 512, 0xE2, 8 },
};
#define TINK_SFX_NOTES 2
static int tink_sfx_step  = -1;
static int tink_sfx_timer = 0;

static void tink_sfx_fire(int step) {
    const tink_sfx_note_t *n = &kTinkSFX[step];
    Audio_WriteReg(0, 0, n->sweep);
    Audio_WriteReg(0, 1, (2 << 6) | 0x3F);
    Audio_WriteReg(0, 2, n->env);
    Audio_WriteReg(0, 3, (uint8_t)(n->freq & 0xFF));
    Audio_WriteReg(0, 4, (uint8_t)(((n->freq >> 8) & 0x07) | 0x80));
    tink_sfx_timer = sfx_cmd_len_frames(n->frames);
}

void Audio_PlaySFX_Tink(void) {
    cancel_pressab_sfx();
    ledge_sfx_active     = 0;
    collision_sfx_active = 0;
    ball_poof_timer      = 0;
    Music_SuspendChannel(0);
    tink_sfx_step = 0;
    tink_sfx_fire(0);
}

/* ---- Caught-mon fanfare (SFX_CAUGHT_MON) --------------------------
 * audio/sfx/caught_mon.asm
 *
 * Ch5+Ch6 are ported directly. Ch7 wave harmony is omitted for now,
 * matching the simplification used by several other jingles here.
 */
typedef struct { uint16_t freq; uint8_t env; uint8_t frames; } caught_sq_note_t;
static const caught_sq_note_t kCaughtMonCh1[] = {
    { 1650, 0xB2, 12 }, /* E_3 */
    { 1694, 0xB2, 12 }, /* F#3 */
    { 1732, 0xB2, 12 }, /* G#3 */
    { 1732, 0xB2,  6 }, /* G#3 */
    { 1732, 0xB2,  6 }, /* G#3 */
    { 1783, 0xB2, 12 }, /* B_3 */
    { 1812, 0xB2, 12 }, /* C#4 */
    { 1838, 0xB2, 12 }, /* D#4 */
    { 1838, 0xB2,  6 }, /* D#4 */
    { 1838, 0xB2,  6 }, /* D#4 */
    { 1849, 0xB5, 48 }, /* E_4 */
};
static const caught_sq_note_t kCaughtMonCh2[] = {
    { 1891, 0xC2, 12 }, /* G#4 */
    { 1891, 0xC2,  6 }, /* G#4 */
    { 1891, 0xC2,  6 }, /* G#4 */
    { 1849, 0xC2, 12 }, /* E_4 */
    { 1849, 0xC2,  6 }, /* E_4 */
    { 1849, 0xC2,  6 }, /* E_4 */
    { 1915, 0xC2, 12 }, /* B_4 */
    { 1915, 0xC2,  6 }, /* B_4 */
    { 1915, 0xC2,  6 }, /* B_4 */
    { 1899, 0xC2, 12 }, /* A_4 */
    { 1899, 0xC2,  6 }, /* A_4 */
    { 1899, 0xC2,  6 }, /* A_4 */
    { 1891, 0xC5, 48 }, /* G#4 */
};
#define CAUGHT_MON_CH1_NOTES 11
#define CAUGHT_MON_CH2_NOTES 13
static int caught_mon_ch1_step  = -1;
static int caught_mon_ch1_timer = 0;
static int caught_mon_ch2_step  = -1;
static int caught_mon_ch2_timer = 0;
static int caught_mon_active    = 0;

static void caught_mon_ch1_fire(int step) {
    const caught_sq_note_t *n = &kCaughtMonCh1[step];
    Audio_WriteReg(0, 0, 0x08);
    Audio_WriteReg(0, 1, (3 << 6) | 0x3F);
    Audio_WriteReg(0, 2, n->env);
    Audio_WriteReg(0, 3, (uint8_t)(n->freq & 0xFF));
    Audio_WriteReg(0, 4, (uint8_t)(((n->freq >> 8) & 0x07) | 0x80));
    caught_mon_ch1_timer = n->frames;
}

static void caught_mon_ch2_fire(int step) {
    const caught_sq_note_t *n = &kCaughtMonCh2[step];
    Audio_WriteReg(1, 1, (2 << 6) | 0x3F);
    Audio_WriteReg(1, 2, n->env);
    Audio_WriteReg(1, 3, (uint8_t)(n->freq & 0xFF));
    Audio_WriteReg(1, 4, (uint8_t)(((n->freq >> 8) & 0x07) | 0x80));
    caught_mon_ch2_timer = n->frames;
}

void Audio_PlaySFX_CaughtMon(void) {
    cancel_pressab_sfx();
    ledge_sfx_active     = 0;
    collision_sfx_active = 0;
    ball_poof_timer      = 0;
    ball_toss_active     = 0;
    tink_sfx_step        = -1;
    Music_SuspendChannel(0);
    Music_SuspendChannel(1);
    caught_mon_ch1_step = 0;
    caught_mon_ch2_step = 0;
    caught_mon_active   = 1;
    caught_mon_ch1_fire(0);
    caught_mon_ch2_fire(0);
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

/* ---- Cut field move SFX (SFX_CUT → SFX_Cut_1_Ch8, noise channel) ---------
 * Source: audio/sfx/cut_1.asm — 5 noise notes, total 24 frames.
 * noise_note format: frames, volume, fade, nr43
 * env_byte = (volume << 4) | fade */
static const noise_note_t kCutSFX[] = {
    { 0x24, 0xF7,  2 },   /* noise_note  2, 15, 7,  36 */
    { 0x34, 0xF7,  2 },   /* noise_note  2, 15, 7,  52 */
    { 0x44, 0xF7,  4 },   /* noise_note  4, 15, 7,  68 */
    { 0x55, 0xF4,  8 },   /* noise_note  8, 15, 4,  85 */
    { 0x44, 0xF1,  8 },   /* noise_note  8, 15, 1,  68 */
};

void Audio_PlaySFX_Cut(void) {
    play_noise_sfx(kCutSFX, (int)(sizeof(kCutSFX) / sizeof(kCutSFX[0])));
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
    heal_sfx_timer = sfx_cmd_len_frames(n->frames);
}

void Audio_PlaySFX_HealingMachine(void) {
    Music_SuspendChannel(0);
    heal_sfx_step = 0;
    heal_sfx_fire(0);
}

void Audio_PlaySFX_LevelUp(void) {
    /* Cries take full audio priority in Red/Blue: pause all active music
     * channels so the cry briefly cuts the music completely. */
    Music_SuspendChannel(0);
    Music_SuspendChannel(1);
    Music_SuspendChannel(2);
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
    purchase_ch1_timer = sfx_cmd_len_frames(n->frames);
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
    purchase_ch2_timer = sfx_cmd_len_frames(n->frames);
}

void Audio_PlaySFX_Purchase(void) {
    cancel_pressab_sfx();
    lvl_up_step    = -1;   /* cancel level-up if playing */
    Music_SuspendChannel(0);
    Music_SuspendChannel(1);
    purchase_ch1_step = 0;
    purchase_ch2_step = 0;
    purchase_active   = 1;
    purchase_ch1_fire(0);
    purchase_ch2_fire(0);
}

/* ---- Get Key Item jingle (SFX_GET_KEY_ITEM → SFX_Get_Key_Item_1) ----
 * audio/sfx/get_key_item_1.asm — plays when receiving a starter / key item.
 * Ch5 (square ch[0]):  A#3→C4×3→D#4→F4×3→A#4  (ascending fanfare)
 * Ch6 (square ch[1]):  G4×3→D#4→G#4×3→A#4×3→D#5  (harmony)
 * Ch7 (wave ch[2]):    omitted — square channels carry the melody.
 *
 * note_type 5 = 5 frames per length unit.  tempo 256 = 1:1. */
static const sq_sfx_note_t kGetKeyCh1[] = {
    { 1767, 0xA4, 20 },  /* A#3,  note_type 5,10,4, len 4 */
    { 1798, 0xB1, 10 },  /* C4,   note_type 5,11,1, len 2 */
    { 1798, 0xB1,  5 },  /* C4,   len 1 */
    { 1798, 0xB1,  5 },  /* C4,   len 1 */
    { 1838, 0xA4, 20 },  /* D#4,  note_type 5,10,4, len 4 */
    { 1861, 0xB1, 10 },  /* F4,   note_type 5,11,1, len 2 */
    { 1861, 0xB1,  5 },  /* F4,   len 1 */
    { 1861, 0xB1,  5 },  /* F4,   len 1 */
    { 1908, 0xB4, 40 },  /* A#4,  note_type 5,11,4, len 8 */
};
static const sq_sfx_note_t kGetKeyCh2[] = {
    { 1881, 0xD1, 10 },  /* G4,   note_type 5,13,1, len 2 */
    { 1881, 0xD1,  5 },  /* G4,   len 1 */
    { 1881, 0xD1,  5 },  /* G4,   len 1 */
    { 1838, 0xC4, 20 },  /* D#4,  note_type 5,12,4, len 4 */
    { 1891, 0xD1, 10 },  /* G#4,  note_type 5,13,1, len 2 */
    { 1891, 0xD1,  5 },  /* G#4,  len 1 */
    { 1891, 0xD1,  5 },  /* G#4,  len 1 */
    { 1908, 0xD1, 10 },  /* A#4,  len 2 */
    { 1908, 0xD1,  5 },  /* A#4,  len 1 */
    { 1908, 0xD1,  5 },  /* A#4,  len 1 */
    { 1943, 0xC4, 40 },  /* D#5,  note_type 5,12,4, len 8 */
};
#define GET_KEY_CH1_NOTES  9
#define GET_KEY_CH2_NOTES  11

static int get_key_ch1_step  = -1;
static int get_key_ch1_timer =  0;
static int get_key_ch2_step  = -1;
static int get_key_ch2_timer =  0;
static int get_key_active    =  0;

static void get_key_ch1_fire(int step) {
    const sq_sfx_note_t *n = &kGetKeyCh1[step];
    Audio_WriteReg(0, 0, 0x08);                              /* NR10: disable sweep */
    Audio_WriteReg(0, 1, (2 << 6) | 0x3F);                  /* duty=2 (50%) */
    Audio_WriteReg(0, 2, n->env);
    Audio_WriteReg(0, 3, (uint8_t)(n->freq & 0xFF));
    Audio_WriteReg(0, 4, (uint8_t)(((n->freq >> 8) & 0x07) | 0x80));
    get_key_ch1_timer = n->frames;
}

static void get_key_ch2_fire(int step) {
    const sq_sfx_note_t *n = &kGetKeyCh2[step];
    Audio_WriteReg(1, 1, (2 << 6) | 0x3F);                  /* duty=2 (50%) */
    Audio_WriteReg(1, 2, n->env);
    Audio_WriteReg(1, 3, (uint8_t)(n->freq & 0xFF));
    Audio_WriteReg(1, 4, (uint8_t)(((n->freq >> 8) & 0x07) | 0x80));
    get_key_ch2_timer = n->frames;
}

int Audio_IsSFXPlaying_GetKeyItem(void) { return get_key_active; }

/* ---- Gym trash-can switch click (SFX_SWITCH → SFX_Switch_1_Ch5) ----
 * audio/sfx/switch_1.asm:
 *   duty_cycle 2
 *   square_note 4,  0, 0,    0  → 4 frames, silent
 *   square_note 2, 15, 1, 1664  → 2 frames, vol=15, dec, pace=1
 *   square_note 1,  0, 0,    0  → 1 frame,  silent
 *   square_note 4, 15, 1, 1920  → 4 frames, vol=15, dec, pace=1
 *   square_note 4,  0, 0,    0  → 4 frames, silent
 */
static const sfx_note_t kSwitch[] = {
    {    0, 0x00, 4 },   /* silent lead-in */
    { 1664, 0xF1, 2 },   /* first click  — vol=15, dec, pace=1 */
    {    0, 0x00, 1 },   /* silent gap */
    { 1920, 0xF1, 4 },   /* second click — vol=15, dec, pace=1 */
    {    0, 0x00, 4 },   /* silent tail */
};
#define SWITCH_SFX_NOTES 5
static int switch_sfx_step  = -1;
static int switch_sfx_timer =  0;

static void switch_sfx_fire(int step) {
    const sfx_note_t *n = &kSwitch[step];
    Audio_WriteReg(0, 0, 0x08);                              /* NR10: disable sweep */
    Audio_WriteReg(0, 1, (2 << 6) | 0x3F);                  /* duty=2 (50%) */
    if (n->freq > 0) {
        Audio_WriteReg(0, 2, n->env_byte);
        Audio_WriteReg(0, 3, (uint8_t)(n->freq & 0xFF));
        Audio_WriteReg(0, 4, (uint8_t)(((n->freq >> 8) & 0x07) | 0x80));
    } else {
        Audio_WriteReg(0, 2, 0x00);
        Audio_WriteReg(0, 4, 0x00);
    }
    switch_sfx_timer = sfx_cmd_len_frames(n->frames);
}

void Audio_PlaySFX_Switch(void) {
    Music_SuspendChannel(0);
    switch_sfx_step = 0;
    switch_sfx_fire(0);
}

/* ---- Denied / wrong-answer SFX (SFX_DENIED → SFX_Denied_1_Ch5/Ch6) -
 * audio/sfx/denied_1.asm:
 *
 * SFX_Denied_1_Ch5 (square ch[0], duty_cycle 3):
 *   pitch_sweep 5, -2            → NR10=0x5A: pace=5, decrease, shift=2
 *   square_note  4, 15, 0, 1280  → 4 frames,  vol=15, hold
 *   pitch_sweep  0,  8           → NR10=0x08 (disable sweep)
 *   square_note  4,  0, 0,    0  → 4 frames,  silent
 *   square_note 15, 15, 0, 1280  → 15 frames, vol=15, hold
 *   square_note  1,  0, 0,    0  → 1 frame,   silent
 *
 * SFX_Denied_1_Ch6 (square ch[1], duty_cycle 3):
 *   square_note  4, 15, 0, 1025  → 4 frames,  vol=15, hold
 *   square_note  4,  0, 0,    0  → 4 frames,  silent
 *   square_note 15, 15, 0, 1025  → 15 frames, vol=15, hold
 *   square_note  1,  0, 0,    0  → 1 frame,   silent
 *
 * env = (vol << 4) | fade; fade=0 = hold.
 * NR10=0x5A: (pace=5)<<4 | (dec=1)<<3 | (shift=2) = 0x5A.
 */
static const heal_sfx_note_t kDeniedCh1[] = {
    { 0x5A, 1280, 0xF0,  4 },   /* sweep down, vol=15, hold */
    { 0x08,    0, 0x00,  4 },   /* sweep off, silent */
    { 0x08, 1280, 0xF0, 15 },   /* no sweep,  vol=15, hold */
    { 0x08,    0, 0x00,  1 },   /* silent tail */
};
static const sq_sfx_note_t kDeniedCh2[] = {
    { 1025, 0xF0,  4 },   /* vol=15, hold */
    {    0, 0x00,  4 },   /* silent */
    { 1025, 0xF0, 15 },   /* vol=15, hold */
    {    0, 0x00,  1 },   /* silent tail */
};
#define DENIED_NOTES 4

static int denied_ch1_step  = -1;
static int denied_ch1_timer =  0;
static int denied_ch2_step  = -1;
static int denied_ch2_timer =  0;
static int denied_active    =  0;

static void denied_ch1_fire(int step) {
    const heal_sfx_note_t *n = &kDeniedCh1[step];
    Audio_WriteReg(0, 0, n->sweep);
    Audio_WriteReg(0, 1, (3 << 6) | 0x3F);                  /* duty=3 (75%) */
    if (n->freq > 0) {
        Audio_WriteReg(0, 2, n->env);
        Audio_WriteReg(0, 3, (uint8_t)(n->freq & 0xFF));
        Audio_WriteReg(0, 4, (uint8_t)(((n->freq >> 8) & 0x07) | 0x80));
    } else {
        Audio_WriteReg(0, 2, 0x00);
        Audio_WriteReg(0, 4, 0x00);
    }
    denied_ch1_timer = sfx_cmd_len_frames(n->frames);
}

static void denied_ch2_fire(int step) {
    const sq_sfx_note_t *n = &kDeniedCh2[step];
    Audio_WriteReg(1, 1, (3 << 6) | 0x3F);                  /* duty=3 (75%) */
    if (n->freq > 0) {
        Audio_WriteReg(1, 2, n->env);
        Audio_WriteReg(1, 3, (uint8_t)(n->freq & 0xFF));
        Audio_WriteReg(1, 4, (uint8_t)(((n->freq >> 8) & 0x07) | 0x80));
    } else {
        Audio_WriteReg(1, 2, 0x00);
        Audio_WriteReg(1, 4, 0x00);
    }
    denied_ch2_timer = sfx_cmd_len_frames(n->frames);
}

void Audio_PlaySFX_Denied(void) {
    Music_SuspendChannel(0);
    Music_SuspendChannel(1);
    denied_ch1_step = 0;
    denied_ch2_step = 0;
    denied_active   = 1;
    denied_ch1_fire(0);
    denied_ch2_fire(0);
}

/* ---- Shooting star SFX (SFX_SHOOTING_STAR → audio/sfx/shooting_star.asm) --
 * Ch5 sequence:
 *   duty_cycle_pattern 3,2,1,0
 *   pitch_sweep 2,-7
 *   square_note 4, 4,0,2016
 *   square_note 4, 6,0,2016
 *   square_note 4, 8,0,2016
 *   square_note 8,10,0,2016
 *   square_note 8,10,0,2016
 *   square_note 8, 8,0,2016
 *   square_note 8, 6,0,2016
 *   square_note 8, 3,0,2016
 *   square_note 15,1,2,2016
 *   pitch_sweep 0,8
 */
typedef struct {
    uint8_t  duty;
    uint16_t freq;
    uint8_t  env_byte;
    uint8_t  frames;
} shooting_star_note_t;

static const shooting_star_note_t kShootingStarSFX[] = {
    { 3, 2016, 0x40,  4 },
    { 2, 2016, 0x60,  4 },
    { 1, 2016, 0x80,  4 },
    { 0, 2016, 0xA0,  8 },
    { 3, 2016, 0xA0,  8 },
    { 2, 2016, 0x80,  8 },
    { 1, 2016, 0x60,  8 },
    { 0, 2016, 0x30,  8 },
    { 3, 2016, 0x12, 15 },
};
#define SHOOTING_STAR_NOTES ((int)(sizeof(kShootingStarSFX) / sizeof(kShootingStarSFX[0])))

static int shooting_star_step  = -1;
static int shooting_star_timer = 0;

static void shooting_star_fire(int step) {
    const shooting_star_note_t *n = &kShootingStarSFX[step];
    Audio_WriteReg(0, 1, (uint8_t)((n->duty << 6) | 0x3F));
    Audio_WriteReg(0, 2, n->env_byte);
    Audio_WriteReg(0, 3, (uint8_t)(n->freq & 0xFF));
    Audio_WriteReg(0, 4, (uint8_t)(((n->freq >> 8) & 0x07) | 0x80));
    shooting_star_timer = sfx_cmd_len_frames(n->frames);
}

void Audio_PlaySFX_ShootingStar(void) {
    cancel_pressab_sfx();
    Music_SuspendChannel(0);
    Audio_WriteReg(0, 0, 0x2F); /* pitch_sweep 2, -7 */
    shooting_star_step = 0;
    shooting_star_fire(0);
}

/* ---- Intro cinematic SFX (engine/movie/intro.asm) -----------------
 * SFX_Intro_Hip / SFX_Intro_Hop: one square note on Ch5 with sweep.
 * SFX_Intro_Raise / Crash / Lunge: noise-channel bursts on Ch8.
 */
static int intro_sq_timer = 0;

static void play_intro_sq(uint16_t freq) {
    Music_SuspendChannel(0);
    Audio_WriteReg(0, 0, 0x26);                 /* pitch_sweep 2, +6 */
    Audio_WriteReg(0, 1, (2 << 6) | 0x3F);      /* duty_cycle 2 */
    Audio_WriteReg(0, 2, 0xC2);                 /* square_note *,12,2 */
    Audio_WriteReg(0, 3, (uint8_t)(freq & 0xFF));
    Audio_WriteReg(0, 4, (uint8_t)(((freq >> 8) & 0x07) | 0x80));
    intro_sq_timer = sfx_cmd_len_frames(12);
}

void Audio_PlaySFX_IntroHip(void) { play_intro_sq(1856); }
void Audio_PlaySFX_IntroHop(void) { play_intro_sq(1664); }

static const noise_note_t kIntroRaise[] = {
    { 0x21, 0x67,  2 },   /* noise_note 2,  6, -7, 33 */
    { 0x31, 0xA7,  2 },   /* noise_note 2, 10, -7, 49 */
    { 0x41, 0xF2, 15 },   /* noise_note 15,15,  2, 65 */
};
static const noise_note_t kIntroCrash[] = {
    { 0x32, 0xD2,  2 },   /* noise_note 2,13,2,50 */
    { 0x43, 0xF2, 15 },   /* noise_note 15,15,2,67 */
};
static const noise_note_t kIntroLunge[] = {
    { 0x10, 0x20,  6 },   /* noise_note 6, 2, 0,16 */
    { 0x40, 0x27,  6 },   /* noise_note 6, 2,-7,64 */
    { 0x41, 0x47,  6 },   /* noise_note 6, 4,-7,65 */
    { 0x41, 0x87,  6 },   /* noise_note 6, 8,-7,65 */
    { 0x42, 0xC7,  6 },   /* noise_note 6,12,-7,66 */
    { 0x42, 0xD7,  8 },   /* noise_note 8,13, 7,66 */
    { 0x43, 0xE7, 15 },   /* noise_note 15,14,7,67 */
    { 0x43, 0xF2, 15 },   /* noise_note 15,15,2,67 */
};

void Audio_PlaySFX_IntroRaise(void) { play_noise_sfx(kIntroRaise, 3); }
void Audio_PlaySFX_IntroCrash(void) { play_noise_sfx(kIntroCrash, 2); }
void Audio_PlaySFX_IntroLunge(void) { play_noise_sfx(kIntroLunge, 8); }

/* Forward declaration — defined below with note tables */
static int get_item1_active = 0;

/* Returns 1 if any SFX channel is still active.
 * Mirrors WaitForSoundToFinish (home/delay.asm): polls wChannelSoundIDs
 * for Ch5 (sfx_step/ledge/collision), Ch6 (lvl_up/heal/purchase/get_key),
 * and Ch8 (noise_sfx_step) until all are silent. */
int Audio_IsSFXPlaying(void) {
    return sfx_step >= 0
        || pc_sfx_step >= 0
        || ledge_sfx_active
        || collision_sfx_active
        || noise_sfx_step >= 0
        || ball_toss_active
        || tink_sfx_step >= 0
        || caught_mon_active
        || lvl_up_step >= 0
        || heal_sfx_step >= 0
        || purchase_active
        || get_key_active
        || get_item1_active
        || switch_sfx_step >= 0
        || shooting_star_step >= 0
        || denied_active;
}

void Audio_PlaySFX_GetKeyItem(void) {
    cancel_pressab_sfx();
    lvl_up_step     = -1;   /* cancel level-up if playing */
    purchase_active =  0;   /* cancel purchase if playing */
    Music_SuspendChannel(0);
    Music_SuspendChannel(1);
    get_key_ch1_step = 0;
    get_key_ch2_step = 0;
    get_key_active   = 1;
    get_key_ch1_fire(0);
    get_key_ch2_fire(0);
}

/* ---- Get Item 1 jingle (SFX_GET_ITEM_1 → SFX_Get_Item1_1) ----------
 * audio/sfx/get_item1_1.asm — plays when receiving a regular item or TM.
 * Ch5 (square ch[0]):  G#3×3 → E4          note_type 4/12, vol 11
 * Ch6 (square ch[1]):  E4×3  → B4          note_type 4/12, vol 12
 * Ch7 (wave): omitted — square channels carry the jingle.
 *
 * Frequencies: pokered octave N = standard octave N+1.
 *   G#3 pokered = G#4 std = 415.3 Hz → reg 1732
 *   E4  pokered = E5  std = 659.3 Hz → reg 1849
 *   B4  pokered = B5  std = 987.8 Hz → reg 1915 */
static const sq_sfx_note_t kGetItem1Ch1[] = {
    { 1732, 0xB1,  8 },  /* G#3, note_type 4, len 2 × 3 */
    { 1732, 0xB1,  8 },
    { 1732, 0xB1,  8 },
    { 1849, 0xB3, 48 },  /* E4,  note_type 12, len 4    */
};
static const sq_sfx_note_t kGetItem1Ch2[] = {
    { 1849, 0xC1,  8 },  /* E4,  note_type 4, len 2 × 3 */
    { 1849, 0xC1,  8 },
    { 1849, 0xC1,  8 },
    { 1915, 0xC3, 48 },  /* B4,  note_type 12, len 4    */
};
#define GET_ITEM1_CH1_NOTES 4
#define GET_ITEM1_CH2_NOTES 4

static int get_item1_ch1_step  = -1;
static int get_item1_ch1_timer =  0;
static int get_item1_ch2_step  = -1;
static int get_item1_ch2_timer =  0;

static void get_item1_ch1_fire(int step) {
    const sq_sfx_note_t *n = &kGetItem1Ch1[step];
    Audio_WriteReg(0, 0, 0x08);                              /* NR10: disable sweep */
    Audio_WriteReg(0, 1, (2 << 6) | 0x3F);                  /* duty=2 (50%) */
    Audio_WriteReg(0, 2, n->env);
    Audio_WriteReg(0, 3, (uint8_t)(n->freq & 0xFF));
    Audio_WriteReg(0, 4, (uint8_t)(((n->freq >> 8) & 0x07) | 0x80));
    get_item1_ch1_timer = n->frames;
}

static void get_item1_ch2_fire(int step) {
    const sq_sfx_note_t *n = &kGetItem1Ch2[step];
    Audio_WriteReg(1, 1, (2 << 6) | 0x3F);                  /* duty=2 (50%) */
    Audio_WriteReg(1, 2, n->env);
    Audio_WriteReg(1, 3, (uint8_t)(n->freq & 0xFF));
    Audio_WriteReg(1, 4, (uint8_t)(((n->freq >> 8) & 0x07) | 0x80));
    get_item1_ch2_timer = n->frames;
}

void Audio_PlaySFX_GetItem1(void) {
    cancel_pressab_sfx();
    lvl_up_step     = -1;   /* cancel level-up if playing */
    purchase_active =  0;   /* cancel purchase if playing */
    get_key_active  =  0;   /* cancel get-key if playing  */
    Music_SuspendChannel(0);
    Music_SuspendChannel(1);
    get_item1_ch1_step = 0;
    get_item1_ch2_step = 0;
    get_item1_active   = 1;
    get_item1_ch1_fire(0);
    get_item1_ch2_fire(0);
}

/* ---- SS Anne Horn (SFX_SS_ANNE_HORN → SFX_SS_Anne_Horn_1) --------------
 * audio/sfx/ss_anne_horn_1.asm
 * Ch5 (square ch[0]): duty 2, freq 1280, vol 15, 7 notes w/ 4-frame rest
 * Ch6 (square ch[1]): duty 3, freq 1154, vol 15, same structure
 * Total duration ~109 frames (~1.8 s). */

static const sq_sfx_note_t kSSAnneHornCh1[] = {
    { 1280, 0xF0, 15 },  /* note 1 */
    {    0, 0x00,  4 },  /* rest   */
    { 1280, 0xF0, 15 },  /* notes 2-7 */
    { 1280, 0xF0, 15 },
    { 1280, 0xF0, 15 },
    { 1280, 0xF0, 15 },
    { 1280, 0xF0, 15 },
    { 1280, 0xF2, 15 },  /* fade on last */
};
static const sq_sfx_note_t kSSAnneHornCh2[] = {
    { 1154, 0xF0, 15 },
    {    0, 0x00,  4 },
    { 1154, 0xF0, 15 },
    { 1154, 0xF0, 15 },
    { 1154, 0xF0, 15 },
    { 1154, 0xF0, 15 },
    { 1154, 0xF0, 15 },
    { 1154, 0xF2, 15 },
};
#define HORN_CH_NOTES 8

static int horn_ch1_step  = -1;
static int horn_ch1_timer =  0;
static int horn_ch2_step  = -1;
static int horn_ch2_timer =  0;
static int horn_active    =  0;

static void horn_ch1_fire(int step) {
    const sq_sfx_note_t *n = &kSSAnneHornCh1[step];
    if (n->freq == 0) {
        Audio_WriteReg(0, 4, 0x00);  /* silence ch1 */
    } else {
        Audio_WriteReg(0, 0, 0x08);
        Audio_WriteReg(0, 1, (2 << 6) | 0x3F);
        Audio_WriteReg(0, 2, n->env);
        Audio_WriteReg(0, 3, (uint8_t)(n->freq & 0xFF));
        Audio_WriteReg(0, 4, (uint8_t)(((n->freq >> 8) & 0x07) | 0x80));
    }
    horn_ch1_timer = sfx_cmd_len_frames(n->frames);
}

static void horn_ch2_fire(int step) {
    const sq_sfx_note_t *n = &kSSAnneHornCh2[step];
    if (n->freq == 0) {
        Audio_WriteReg(1, 4, 0x00);
    } else {
        Audio_WriteReg(1, 1, (3 << 6) | 0x3F);
        Audio_WriteReg(1, 2, n->env);
        Audio_WriteReg(1, 3, (uint8_t)(n->freq & 0xFF));
        Audio_WriteReg(1, 4, (uint8_t)(((n->freq >> 8) & 0x07) | 0x80));
    }
    horn_ch2_timer = sfx_cmd_len_frames(n->frames);
}

int Audio_IsSFXPlaying_SSAnneHorn(void) { return horn_active; }

void Audio_PlaySFX_SSAnneHorn(void) {
    Music_SuspendChannel(0);
    Music_SuspendChannel(1);
    horn_ch1_step = 0;
    horn_ch2_step = 0;
    horn_active   = 1;
    horn_ch1_fire(0);
    horn_ch2_fire(0);
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

void Audio_PlayCryModified(uint8_t species, int8_t pitch_add, uint8_t tempo_add) {
    /* species: 1-based internal ID; CryData table is 0-based */
    if (species == 0 || species > NUM_POKEMON_CRIES) return;
    const pokemon_cry_t *pc = &g_pokemon_cries[species - 1];
    if (pc->base_cry >= NUM_BASE_CRIES) return;

    {
        uint8_t base_pitch = (uint8_t)pc->pitch_mod;
        uint8_t base_tempo = pc->tempo_mod;
        uint8_t add_pitch = (uint8_t)pitch_add;
        uint8_t pitch_u = (uint8_t)(base_pitch + add_pitch);
        uint8_t tempo_u = (uint8_t)(base_tempo + tempo_add);
        cry_pitch = (int8_t)pitch_u;
        cry_tempo = (uint16_t)((uint16_t)tempo_u + 0x80u);
    }

    cry_cur   = &g_cry_defs[pc->base_cry];
    s_cry_mix_boost = 1;

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

void Audio_PlayCry(uint8_t species) {
    Audio_PlayCryModified(species, 0, 0);
}

int Audio_IsCryPlaying(void) {
    return cry_cur != NULL;
}

/* ---- Frame tick -------------------------------------------------- */
void Audio_Update(void) {
    /* Advance music sequencer */
    Music_Update();

    /* Advance generic move-SFX runtime from imported ASM command streams. */
    if (sMoveSfx.active) {
        uint8_t hw;
        uint8_t any_active = 0;
        for (hw = 0; hw < 4; hw++) {
            move_sfx_chan_rt_t *st = &sMoveSfx.ch[hw];
            if (!st->active) continue;
            any_active = 1;
            if (st->rotate_duty && st->timer > 0 && (hw == 0 || hw == 1)) {
                st->duty_state = (uint8_t)((st->duty_state << 2) | (st->duty_state >> 6));
                Audio_WriteReg(hw, 1, (uint8_t)((((st->duty_state >> 6) & 0x3) << 6) | 0x3F));
            }
            if (--st->timer <= 0) {
                move_sfx_advance_channel(hw);
            }
        }
        if (!any_active) {
            for (hw = 0; hw < 4; hw++) {
                if (sMoveSfx.suspended[hw]) {
                    Music_ResumeChannel(hw);
                    sMoveSfx.suspended[hw] = 0;
                }
            }
            sMoveSfx.active = 0;
        }
    }

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

    /* Advance collision-bump SFX */
    if (collision_sfx_active) {
        if (--collision_sfx_timer <= 0) {
            Audio_WriteReg(0, 0, 0x08);  /* NR10: disable sweep */
            Music_ResumeChannel(0);
            collision_sfx_active = 0;
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

    /* Advance intro hip/hop square note */
    if (intro_sq_timer > 0) {
        if (--intro_sq_timer <= 0) {
            Audio_WriteReg(0, 0, 0x08);  /* pitch_sweep 0,8 */
            Music_ResumeChannel(0);
        }
    }

    /* Advance shooting-star Ch5 SFX */
    if (shooting_star_step >= 0) {
        if (--shooting_star_timer <= 0) {
            shooting_star_step++;
            if (shooting_star_step >= SHOOTING_STAR_NOTES) {
                shooting_star_step = -1;
                Audio_WriteReg(0, 0, 0x08); /* pitch_sweep 0,8 */
                Music_ResumeChannel(0);
            } else {
                shooting_star_fire(shooting_star_step);
            }
        }
    }

    /* Advance ball-poof square SFX */
    if (ball_poof_timer > 0) {
        if (--ball_poof_timer <= 0) {
            Audio_WriteReg(0, 0, 0x08);  /* NR10: disable sweep */
            Music_ResumeChannel(0);
        }
    }

    /* Advance SFX_Battle_24 (Absorb family), then restore CH1 sweep/music. */
    if (battle24_timer > 0) {
        if (--battle24_timer <= 0) {
            Audio_WriteReg(0, 0, 0x08);
            Music_ResumeChannel(0);
        }
    }

    /* Advance SFX_Battle_28 dual-square loop and restore channels. */
    if (battle28_active) {
        if (--battle28_timer <= 0) {
            battle28_step++;
            if (battle28_step >= 24) {
                battle28_active = 0;
                battle28_step = -1;
                Audio_WriteReg(0, 2, 0x00);
                Audio_WriteReg(0, 4, 0x00);
                Audio_WriteReg(1, 2, 0x00);
                Audio_WriteReg(1, 4, 0x00);
                if (battle28_music_suspended) {
                    Music_ResumeChannel(0);
                    Music_ResumeChannel(1);
                    battle28_music_suspended = 0;
                }
            } else {
                battle28_ch1_fire(battle28_step);
                battle28_ch2_fire(battle28_step);
                battle28_timer = sfx_cmd_len_frames(1);
            }
        }
    }

    /* Advance SFX_Battle_29 Ch5 sequence and restore channel. */
    if (battle29_active) {
        if (--battle29_timer <= 0) {
            battle29_step++;
            if (battle29_step >= (int)(sizeof(kBattle29Ch1) / sizeof(kBattle29Ch1[0]))) {
                battle29_active = 0;
                battle29_step = -1;
                Audio_WriteReg(0, 2, 0x00);
                Audio_WriteReg(0, 4, 0x00);
                if (battle29_music_suspended) {
                    Music_ResumeChannel(0);
                    battle29_music_suspended = 0;
                }
            } else {
                battle29_ch1_fire(battle29_step);
            }
        }
    }

    /* Advance SFX_Battle_2A dual-square sequencer and restore channels. */
    if (battle2a_active) {
        if (battle2a_ch1_step >= 0 && --battle2a_ch1_timer <= 0) {
            battle2a_ch1_step++;
            if (battle2a_ch1_step >= (int)(sizeof(kBattle2ACh1) / sizeof(kBattle2ACh1[0]))) {
                battle2a_ch1_step = -1;
                Audio_WriteReg(0, 0, 0x08);
                Audio_WriteReg(0, 2, 0x00);
                Audio_WriteReg(0, 4, 0x00);
            } else {
                battle2a_ch1_fire(battle2a_ch1_step);
            }
        }
        if (battle2a_ch2_step >= 0 && --battle2a_ch2_timer <= 0) {
            battle2a_ch2_step++;
            if (battle2a_ch2_step >= (int)(sizeof(kBattle2ACh2) / sizeof(kBattle2ACh2[0]))) {
                battle2a_ch2_step = -1;
                Audio_WriteReg(1, 2, 0x00);
                Audio_WriteReg(1, 4, 0x00);
            } else {
                battle2a_ch2_fire(battle2a_ch2_step);
            }
        }
        if (battle2a_ch1_step < 0 && battle2a_ch2_step < 0) {
            battle2a_active = 0;
            if (battle2a_music_suspended) {
                Music_ResumeChannel(0);
                Music_ResumeChannel(1);
                battle2a_music_suspended = 0;
            }
        }
    }

    /* Advance SFX_Faint_Fall (fall-only variant), then restore CH1. */
    if (faint_fall_only_timer > 0) {
        if (--faint_fall_only_timer <= 0) {
            Audio_WriteReg(0, 0, 0x08);
            Music_ResumeChannel(0);
        }
    }

    /* Advance PC SFX (single-channel Ch5 sequence) */
    if (pc_sfx_step >= 0) {
        if (--pc_sfx_timer <= 0) {
            pc_sfx_step++;
            if (pc_sfx_step >= pc_sfx_count) {
                pc_sfx_step = -1;
                s_pc_sfx_mix_boost = 0;
                Music_ResumeChannel(0);
            } else {
                pc_sfx_fire(pc_sfx_step);
            }
        }
    }

    /* Advance ball-toss SFX */
    if (ball_toss_active) {
        if (ball_toss_ch1_timer > 0 && --ball_toss_ch1_timer <= 0) {
            Audio_WriteReg(0, 0, 0x08);
            Audio_WriteReg(0, 2, 0x00);
            Audio_WriteReg(0, 4, 0x00);
        }
        if (ball_toss_ch2_timer > 0 && --ball_toss_ch2_timer <= 0) {
            Audio_WriteReg(1, 2, 0x00);
            Audio_WriteReg(1, 4, 0x00);
        }
        if (ball_toss_ch1_timer <= 0 && ball_toss_ch2_timer <= 0) {
            ball_toss_active = 0;
            Music_ResumeChannel(0);
            Music_ResumeChannel(1);
        }
    }

    /* Advance tink SFX */
    if (tink_sfx_step >= 0) {
        if (--tink_sfx_timer <= 0) {
            tink_sfx_step++;
            if (tink_sfx_step >= TINK_SFX_NOTES) {
                tink_sfx_step = -1;
                Audio_WriteReg(0, 0, 0x08);
                Music_ResumeChannel(0);
            } else {
                tink_sfx_fire(tink_sfx_step);
            }
        }
    }

    /* Advance switch click SFX */
    if (switch_sfx_step >= 0) {
        if (--switch_sfx_timer <= 0) {
            switch_sfx_step++;
            if (switch_sfx_step >= SWITCH_SFX_NOTES) {
                switch_sfx_step = -1;
                Music_ResumeChannel(0);
            } else {
                switch_sfx_fire(switch_sfx_step);
            }
        }
    }

    /* Advance denied SFX (two independent channels) */
    if (denied_active) {
        if (denied_ch1_step >= 0) {
            if (--denied_ch1_timer <= 0) {
                denied_ch1_step++;
                if (denied_ch1_step >= DENIED_NOTES)
                    denied_ch1_step = -1;
                else
                    denied_ch1_fire(denied_ch1_step);
            }
        }
        if (denied_ch2_step >= 0) {
            if (--denied_ch2_timer <= 0) {
                denied_ch2_step++;
                if (denied_ch2_step >= DENIED_NOTES)
                    denied_ch2_step = -1;
                else
                    denied_ch2_fire(denied_ch2_step);
            }
        }
        if (denied_ch1_step < 0 && denied_ch2_step < 0) {
            denied_active = 0;
            Music_ResumeChannel(0);
            Music_ResumeChannel(1);
        }
    }

    /* Advance building enter/exit noise SFX */
    if (noise_sfx_step >= 0) {
        if (--noise_sfx_timer <= 0) {
            noise_sfx_step++;
            if (noise_sfx_step >= noise_sfx_count) {
                noise_sfx_step = -1;
                if (noise_music_suspended) {
                    Music_ResumeChannel(3);
                    noise_music_suspended = 0;
                }
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

    /* Advance get-key-item jingle (two independent channels) */
    if (get_key_active) {
        if (get_key_ch1_step >= 0) {
            if (--get_key_ch1_timer <= 0) {
                get_key_ch1_step++;
                if (get_key_ch1_step >= GET_KEY_CH1_NOTES)
                    get_key_ch1_step = -1;
                else
                    get_key_ch1_fire(get_key_ch1_step);
            }
        }
        if (get_key_ch2_step >= 0) {
            if (--get_key_ch2_timer <= 0) {
                get_key_ch2_step++;
                if (get_key_ch2_step >= GET_KEY_CH2_NOTES)
                    get_key_ch2_step = -1;
                else
                    get_key_ch2_fire(get_key_ch2_step);
            }
        }
        if (get_key_ch1_step < 0 && get_key_ch2_step < 0) {
            get_key_active = 0;
            Music_ResumeChannel(0);
            Music_ResumeChannel(1);
        }
    }

    /* Advance get-item-1 jingle */
    if (get_item1_active) {
        if (get_item1_ch1_step >= 0) {
            if (--get_item1_ch1_timer <= 0) {
                get_item1_ch1_step++;
                if (get_item1_ch1_step >= GET_ITEM1_CH1_NOTES)
                    get_item1_ch1_step = -1;
                else
                    get_item1_ch1_fire(get_item1_ch1_step);
            }
        }
        if (get_item1_ch2_step >= 0) {
            if (--get_item1_ch2_timer <= 0) {
                get_item1_ch2_step++;
                if (get_item1_ch2_step >= GET_ITEM1_CH2_NOTES)
                    get_item1_ch2_step = -1;
                else
                    get_item1_ch2_fire(get_item1_ch2_step);
            }
        }
        if (get_item1_ch1_step < 0 && get_item1_ch2_step < 0) {
            get_item1_active = 0;
            Music_ResumeChannel(0);
            Music_ResumeChannel(1);
        }
    }

    /* Advance caught-mon fanfare */
    if (caught_mon_active) {
        if (caught_mon_ch1_step >= 0) {
            if (--caught_mon_ch1_timer <= 0) {
                caught_mon_ch1_step++;
                if (caught_mon_ch1_step >= CAUGHT_MON_CH1_NOTES)
                    caught_mon_ch1_step = -1;
                else
                    caught_mon_ch1_fire(caught_mon_ch1_step);
            }
        }
        if (caught_mon_ch2_step >= 0) {
            if (--caught_mon_ch2_timer <= 0) {
                caught_mon_ch2_step++;
                if (caught_mon_ch2_step >= CAUGHT_MON_CH2_NOTES)
                    caught_mon_ch2_step = -1;
                else
                    caught_mon_ch2_fire(caught_mon_ch2_step);
            }
        }
        if (caught_mon_ch1_step < 0 && caught_mon_ch2_step < 0) {
            caught_mon_active = 0;
            Music_ResumeChannel(0);
            Music_ResumeChannel(1);
        }
    }

    /* Advance SS Anne Horn */
    if (horn_active) {
        if (horn_ch1_step >= 0) {
            if (--horn_ch1_timer <= 0) {
                horn_ch1_step++;
                if (horn_ch1_step >= HORN_CH_NOTES) horn_ch1_step = -1;
                else horn_ch1_fire(horn_ch1_step);
            }
        }
        if (horn_ch2_step >= 0) {
            if (--horn_ch2_timer <= 0) {
                horn_ch2_step++;
                if (horn_ch2_step >= HORN_CH_NOTES) horn_ch2_step = -1;
                else horn_ch2_fire(horn_ch2_step);
            }
        }
        if (horn_ch1_step < 0 && horn_ch2_step < 0) {
            horn_active = 0;
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
            s_cry_mix_boost = 0;
            Music_ResumeChannel(0);
            Music_ResumeChannel(1);
            Music_ResumeChannel(2);
            Music_ResumeChannel(3);
        }
    }
}

void Audio_Quit(void) {
    if (audio_dev)   SDL_CloseAudioDevice(audio_dev);
    if (audio_mutex) SDL_DestroyMutex(audio_mutex);
}
