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

    /* CH4 noise LFSR */
    uint16_t lfsr;
    int      lfsr_narrow;

    /* Stored freq register (11-bit) built from NRx3 + NRx4 writes */
    uint16_t freq_reg;    /* 11-bit GB frequency register value */
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

/* ---- SDL audio callback ------------------------------------------ */
static void audio_callback(void *userdata, uint8_t *stream, int len) {
    (void)userdata;
    int16_t *out     = (int16_t *)stream;
    int      samples = len / sizeof(int16_t);

    SDL_LockMutex(audio_mutex);
    for (int i = 0; i < samples; i++) {
        int32_t mix = 0;

        /* CH0/CH1: square wave */
        for (int c = 0; c < 2; c++) {
            if (!ch[c].enabled) continue;
            /* duty table: 12.5%, 25%, 50%, 75% (out of 8 steps) */
            static const uint8_t duty_steps[4] = { 1, 2, 4, 6 };
            int high = ((ch[c].phase >> 21) & 7) < duty_steps[ch[c].duty & 3];
            mix += high ? (int32_t)ch[c].volume * 400
                        : -(int32_t)ch[c].volume * 400;
            ch[c].phase = (ch[c].phase + ch[c].phase_inc) & 0xFFFFFFu;
        }

        /* CH2: wave channel */
        if (ch[2].enabled) {
            ch[2].phase = (ch[2].phase + ch[2].phase_inc) & 0xFFFFFFu;
            int sidx    = (ch[2].phase >> (24 - 5)) & 31; /* 32 samples */
            uint8_t nib = (ch[2].wave_ram[sidx / 2] >> (sidx & 1 ? 0 : 4)) & 0xF;
            mix += ((int32_t)nib - 8) * (int32_t)(ch[2].volume ? 200 : 0);
        }

        /* CH3: LFSR noise */
        if (ch[3].enabled) {
            ch[3].phase += ch[3].phase_inc;
            if (ch[3].phase >= (1u << 24)) {
                ch[3].phase -= (1u << 24);
                int xb = (ch[3].lfsr ^ (ch[3].lfsr >> 1)) & 1;
                ch[3].lfsr = (uint16_t)((ch[3].lfsr >> 1) |
                             (xb << (ch[3].lfsr_narrow ? 6 : 14)));
            }
            int high = !(ch[3].lfsr & 1);
            mix += high ? (int32_t)ch[3].volume * 400
                        : -(int32_t)ch[3].volume * 400;
        }

        if (mix >  28000) mix =  28000;
        if (mix < -28000) mix = -28000;
        *out++ = (int16_t)mix;
    }
    SDL_UnlockMutex(audio_mutex);
}

/* ---- Public API ------------------------------------------ */
int Audio_Init(void) {
    memset(ch, 0, sizeof(ch));

    /* Seed LFSR for noise channel */
    ch[3].lfsr = 0x7FFF;

    /* CH2 (wave): load 50% square wave pattern so music sounds right.
     * 16 bytes = 32 4-bit samples; first 16 = 0xF (high), last 16 = 0x0 (low). */
    for (int i = 0; i < 8; i++)  ch[2].wave_ram[i]     = 0xFF;
    for (int i = 8; i < 16; i++) ch[2].wave_ram[i]     = 0x00;

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
        case 1:
            if (channel < 2)
                ch[channel].duty = (value >> 6) & 3;
            break;
        case 2:
            ch[channel].volume = (value >> 4) & 0xF;
            if (ch[channel].volume == 0 && channel != 2)
                ch[channel].enabled = 0;
            break;
        case 3:
            ch[channel].freq_reg = (ch[channel].freq_reg & 0x700u) |
                                    (uint16_t)value;
            break;
        case 4:
            ch[channel].freq_reg = (ch[channel].freq_reg & 0x00FFu) |
                                   ((uint16_t)(value & 0x07) << 8);
            if (value & 0x80) {  /* trigger */
                ch[channel].enabled   = 1;
                ch[channel].phase     = 0;
                ch[channel].phase_inc = freq_to_phase_inc(ch[channel].freq_reg,
                                                          channel == 2);
                if (channel == 3) ch[channel].lfsr = 0x7FFF;
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
 *   square_note 12, 10, 1, 2000  → 13 frames at 2730 Hz
 */
typedef struct { uint16_t freq; uint8_t vol; uint8_t frames; } sfx_note_t;

static const sfx_note_t kPressAB[] = {
    { 1984,  9,  1 },
    { 2000,  8,  1 },
    { 1984,  9,  1 },
    { 2000, 10, 13 },
};

static int sfx_step  = -1;   /* -1 = idle */
static int sfx_timer = 0;

static void sfx_fire(int step) {
    const sfx_note_t *n = &kPressAB[step];
    SDL_LockMutex(audio_mutex);
    /* SFX_Press_AB_1 uses Ch5 = GB CH1 = our ch[0] (melody channel) */
    ch[0].duty      = 2;          /* 50% duty matches duty_cycle 2 in press_ab_1.asm */
    ch[0].volume    = n->vol;
    ch[0].freq_reg  = n->freq;
    ch[0].phase     = 0;
    ch[0].phase_inc = freq_to_phase_inc(n->freq, 0);
    ch[0].enabled   = 1;
    SDL_UnlockMutex(audio_mutex);
    sfx_timer = n->frames;
}

void Audio_PlaySFX_PressAB(void) {
    sfx_step = 0;
    Music_SuspendChannel(0);  /* pause melody so sequencer doesn't fight SFX */
    sfx_fire(0);
}

/* ---- Frame tick -------------------------------------------------- */
void Audio_Update(void) {
    /* Advance music sequencer */
    Music_Update();

    /* Advance text-beep SFX */
    if (sfx_step < 0) return;
    if (--sfx_timer > 0) return;

    sfx_step++;
    int total = (int)(sizeof(kPressAB) / sizeof(kPressAB[0]));
    if (sfx_step >= total) {
        sfx_step = -1;
        Music_ResumeChannel(0);  /* hand ch[0] back to music sequencer */
        return;
    }
    sfx_fire(sfx_step);
}

void Audio_Quit(void) {
    if (audio_dev)   SDL_CloseAudioDevice(audio_dev);
    if (audio_mutex) SDL_DestroyMutex(audio_mutex);
}
