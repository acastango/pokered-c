/* music.c — Note sequencer driving the GB APU synthesis engine.
 *
 * Architecture:
 *   music_data_gen.h  — auto-generated note event arrays (tools/extract_audio.py)
 *   Music_Play()      — select a song, reset channel sequencers
 *   Music_Update()    — called once per frame (60 Hz); advances each channel
 *                       and calls Audio_WriteReg() when a new note fires
 *
 * Four music channels map to GB APU channels 0-3:
 *   CH0 = GB CH1 (square wave, melody)
 *   CH1 = GB CH2 (square wave, harmony)
 *   CH2 = GB CH3 (wave channel, bass/accompaniment)
 *   CH3 = GB CH4 (noise channel, drums/percussion)
 *
 * CH3 is initialised with a 50% square wave pattern so it sounds like
 * a square wave. The original pokered uses wMusicWaveInstrument tables
 * for more variety, but a square wave is correct for most songs.
 */
#include "music.h"
#include "../platform/audio.h"
#include "../data/music_data_gen.h"
#include <stddef.h>

/* ---- Per-channel sequencer state ---------------------------------- */
typedef struct {
    const ch_data_t *data;   /* NULL = not playing */
    int              pos;    /* current event index */
    int              delay;  /* frames remaining on current event */
    /* vibrato */
    uint16_t vib_base_freq;  /* freq of current note (for oscillation) */
    uint8_t  vib_delay_left; /* onset delay frames remaining */
    uint8_t  vib_rate;       /* half-period length in frames */
    int      vib_rate_left;  /* frames remaining in current half-period */
    int      vib_phase;      /* 0 = +depth, 1 = -depth */
    /* pitch slide */
    int      slide_active;
    uint8_t  slide_decreasing;
    uint8_t  slide_cur_lo;
    uint8_t  slide_cur_hi;
    uint8_t  slide_target_lo;
    uint8_t  slide_target_hi;
    uint8_t  slide_step_int;
    uint8_t  slide_step_frac;
    uint8_t  slide_cur_frac;
    /* CH4 drum instrument playback (noise sequences) */
    const drum_inst_t *drum_inst;
    int      drum_step;
    int      drum_timer;
} ch_seq_t;

static ch_seq_t gSeq[4];
static uint8_t  gCurrentMusic   = MUSIC_NONE;
static int      g_skip_update    = 0;  /* skip first Music_Update after Music_Play */
static uint8_t  g_suspend_count[4] = {0, 0, 0, 0};  /* nested channel ownership by SFX */

/* ---- Song table --------------------------------------------------- */
/* Each entry: up to 4 channels (NULL = unused). */
typedef struct {
    const ch_data_t *ch[4];
} song_t;

static const song_t kSongs[] = {
    /* MUSIC_NONE */           { { NULL,            NULL,            NULL            } },
    /* MUSIC_PALLET_TOWN */    { { &kPalletTown_Ch1,&kPalletTown_Ch2,&kPalletTown_Ch3} },
    /* MUSIC_POKECENTER */     { { &kPokecenter_Ch1,&kPokecenter_Ch2,&kPokecenter_Ch3} },
    /* MUSIC_GYM */            { { &kGym_Ch1,       &kGym_Ch2,       &kGym_Ch3       } },
    /* MUSIC_CITIES1 */        { { &kCities1_Ch1,   &kCities1_Ch2,   &kCities1_Ch3   } },
    /* MUSIC_CITIES2 */        { { &kCities2_Ch1,   &kCities2_Ch2,   &kCities2_Ch3   } },
    /* MUSIC_CELADON */        { { &kCeladon_Ch1,   &kCeladon_Ch2,   &kCeladon_Ch3   } },
    /* MUSIC_CINNABAR */       { { &kCinnabar_Ch1,  &kCinnabar_Ch2,  &kCinnabar_Ch3  } },
    /* MUSIC_VERMILION */      { { &kVermilion_Ch1, &kVermilion_Ch2, &kVermilion_Ch3 } },
    /* MUSIC_LAVENDER */       { { &kLavender_Ch1,  &kLavender_Ch2,  &kLavender_Ch3  } },
    /* MUSIC_SS_ANNE */        { { &kSSAnne_Ch1,    &kSSAnne_Ch2,    &kSSAnne_Ch3    } },
    /* MUSIC_ROUTES1 */        { { &kRoutes1_Ch1,   &kRoutes1_Ch2,   &kRoutes1_Ch3   } },
    /* MUSIC_ROUTES2 */        { { &kRoutes2_Ch1,   &kRoutes2_Ch2,   &kRoutes2_Ch3   } },
    /* MUSIC_ROUTES3 */        { { &kRoutes3_Ch1,   &kRoutes3_Ch2,   &kRoutes3_Ch3   } },
    /* MUSIC_ROUTES4 */        { { &kRoutes4_Ch1,   &kRoutes4_Ch2,   &kRoutes4_Ch3,   &kRoutes4_Ch4 } },
    /* MUSIC_INDIGO_PLATEAU */ { { &kIndigoPlateau_Ch1, &kIndigoPlateau_Ch2, &kIndigoPlateau_Ch3, &kIndigoPlateau_Ch4 } },
    /* MUSIC_OAKS_LAB */       { { &kOaksLab_Ch1,   &kOaksLab_Ch2,   &kOaksLab_Ch3   } },
    /* MUSIC_DUNGEON1 */       { { &kDungeon1_Ch1,  &kDungeon1_Ch2,  &kDungeon1_Ch3  } },
    /* MUSIC_DUNGEON2 */       { { &kDungeon2_Ch1,  &kDungeon2_Ch2,  &kDungeon2_Ch3  } },
    /* MUSIC_DUNGEON3 */       { { &kDungeon3_Ch1,  &kDungeon3_Ch2,  &kDungeon3_Ch3  } },
    /* MUSIC_POKEMON_TOWER */  { { &kPokemonTower_Ch1,&kPokemonTower_Ch2,&kPokemonTower_Ch3} },
    /* MUSIC_SILPH_CO */       { { &kSilphCo_Ch1,   &kSilphCo_Ch2,   &kSilphCo_Ch3   } },
    /* MUSIC_SAFARI_ZONE */    { { &kSafariZone_Ch1,&kSafariZone_Ch2,&kSafariZone_Ch3} },
    /* MUSIC_TITLE */          { { &kTitleScreen_Ch1,&kTitleScreen_Ch2,&kTitleScreen_Ch3,&kTitleScreen_Ch4 } },
    /* MUSIC_JIGGLYPUFF */     { { &kJigglypuffSong_Ch1, &kJigglypuffSong_Ch2, NULL               } },
    /* MUSIC_WILD_BATTLE */         { { &kWildBattle_Ch1,        &kWildBattle_Ch2,        &kWildBattle_Ch3        } },
    /* MUSIC_DEFEATED_WILD_MON */   { { &kDefeatedWildMon_Ch1,   &kDefeatedWildMon_Ch2,   &kDefeatedWildMon_Ch3   } },
    /* MUSIC_DEFEATED_TRAINER */    { { &kDefeatedTrainer_Ch1,   &kDefeatedTrainer_Ch2,   &kDefeatedTrainer_Ch3   } },
    /* MUSIC_DEFEATED_GYM_LEADER */ { { &kDefeatedGymLeader_Ch1, &kDefeatedGymLeader_Ch2, &kDefeatedGymLeader_Ch3 } },
    /* MUSIC_PKMN_HEALED */        { { &kPkmnHealed_Ch1,        &kPkmnHealed_Ch2,        &kPkmnHealed_Ch3        } },
    /* MUSIC_GYM_LEADER_BATTLE */  { { &kGymLeaderBattle_Ch1,   &kGymLeaderBattle_Ch2,   &kGymLeaderBattle_Ch3   } },
    /* MUSIC_TRAINER_BATTLE */     { { &kTrainerBattle_Ch1,     &kTrainerBattle_Ch2,     &kTrainerBattle_Ch3     } },
    /* MUSIC_MEET_RIVAL */         { { &kMeetRival_Ch1,         &kMeetRival_Ch2,         &kMeetRival_Ch3         } },
    /* MUSIC_MEET_MALE_TRAINER */  { { &kMeetMaleTrainer_Ch1,   &kMeetMaleTrainer_Ch2,   &kMeetMaleTrainer_Ch3   } },
    /* MUSIC_MEET_FEMALE_TRAINER */{ { &kMeetFemaleTrainer_Ch1, &kMeetFemaleTrainer_Ch2, &kMeetFemaleTrainer_Ch3 } },
    /* MUSIC_MUSEUM_GUY */         { { &kMuseumGuy_Ch1,         &kMuseumGuy_Ch2,         &kMuseumGuy_Ch3         } },
    /* MUSIC_MEET_EVIL_TRAINER */  { { &kMeetEvilTrainer_Ch1,   &kMeetEvilTrainer_Ch2,   &kMeetEvilTrainer_Ch3   } },
    /* MUSIC_SURFING */            { { &kSurfing_Ch1,           &kSurfing_Ch2,           &kSurfing_Ch3           } },
    /* MUSIC_MEET_PROF_OAK */     { { &kMeetProfOak_Ch1,       &kMeetProfOak_Ch2,       &kMeetProfOak_Ch3       } },
    /* MUSIC_INTRO_BATTLE */      { { &kIntroBattle_Ch1,       &kIntroBattle_Ch2,       &kIntroBattle_Ch3,      &kIntroBattle_Ch4 } },
    /* MUSIC_GAME_CORNER */       { { &kGameCorner_Ch1,        &kGameCorner_Ch2,        &kGameCorner_Ch3        } },
    /* MUSIC_BIKE_RIDING */       { { &kBikeRiding_Ch1,        &kBikeRiding_Ch2,        &kBikeRiding_Ch3        } },
};
#define NUM_SONGS  ((int)(sizeof(kSongs)/sizeof(kSongs[0])))

/* ---- Helpers ------------------------------------------------------ */

/* Reset vibrato state when a new note fires. */
static void reset_vib(ch_seq_t *seq, const note_evt_t *n) {
    seq->vib_base_freq  = n->freq;
    seq->vib_delay_left = n->vib_delay;
    seq->vib_rate       = n->vib_rate ? n->vib_rate : 1;
    seq->vib_rate_left  = seq->vib_rate;
    seq->vib_phase      = 0;
}

static void reset_slide(ch_seq_t *seq, const note_evt_t *n) {
    seq->slide_active = 0;
    seq->slide_decreasing = 0;
    seq->slide_cur_lo = (uint8_t)(n->freq & 0xFF);
    seq->slide_cur_hi = (uint8_t)((n->freq >> 8) & 0x07);
    seq->slide_target_lo = (uint8_t)(n->slide_target & 0xFF);
    seq->slide_target_hi = (uint8_t)((n->slide_target >> 8) & 0x07);
    seq->slide_step_int = 0;
    seq->slide_step_frac = 0;
    seq->slide_cur_frac = 0;
    if (n->freq == 0 || n->slide_target == 0 || n->slide_frames == 0) return;
    if (n->freq == n->slide_target) return;
    seq->slide_active = 1;

    /* Port of Audio3_InitPitchSlideVars division/path quirks from engine_3.asm. */
    {
        uint8_t divisor = n->slide_frames;
        if (divisor == 0) divisor = 1;

        uint8_t d = seq->slide_cur_hi;
        uint8_t e = seq->slide_cur_lo;
        uint8_t th = seq->slide_target_hi;
        uint8_t tl = seq->slide_target_lo;
        uint8_t diff_hi = 0;
        uint8_t diff_lo = 0;

        /* Decide direction by comparing current - target. */
        {
            uint16_t cur = ((uint16_t)d << 8) | e;
            uint16_t tgt = ((uint16_t)th << 8) | tl;
            if (cur >= tgt) {
                seq->slide_decreasing = 1;
                diff_hi = (uint8_t)((cur - tgt) >> 8);
                diff_lo = (uint8_t)(cur - tgt);
            } else {
                /* Increasing path bug-compatible diff calc from engine:
                 * when cur_lo > tgt_lo, effective diff is 0x200 larger.
                 */
                seq->slide_decreasing = 0;
                {
                    uint8_t lo = (uint8_t)(tl - e);
                    uint8_t borrow = (tl < e) ? 1 : 0;
                    uint8_t d_bug = (uint8_t)(d - borrow);
                    uint8_t hi = (uint8_t)(th - d_bug);
                    diff_hi = hi;
                    diff_lo = lo;
                }
            }
        }

        /* divideLoop: compute step_int (quotient+1) and remainder. */
        {
            uint8_t b = 0;
            uint8_t dh = diff_hi;
            uint8_t el = diff_lo;
            for (;;) {
                b++;
                if (el >= divisor) {
                    el = (uint8_t)(el - divisor);
                    continue;
                }
                if (dh == 0) break;
                dh--;
                continue;
            }
            seq->slide_step_int = b;
            seq->slide_step_frac = el;
            seq->slide_cur_frac = el;
        }
    }
}

/* Advance vibrato oscillator for channel c by one frame.
 * Writes freq (no trigger) to audio hardware when phase toggles. */
static void tick_vibrato(int c, ch_seq_t *seq) {
    const note_evt_t *n = &seq->data->notes[seq->pos];
    if (n->vib_depth == 0 || seq->vib_base_freq == 0) return;
    if (seq->vib_delay_left > 0) { seq->vib_delay_left--; return; }
    if (--seq->vib_rate_left > 0) return;
    seq->vib_phase    ^= 1;
    seq->vib_rate_left = seq->vib_rate;
    int16_t  delta  = seq->vib_phase ? -(int16_t)n->vib_depth : (int16_t)n->vib_depth;
    uint16_t af     = (uint16_t)((int16_t)seq->vib_base_freq + delta) & 0x7FFu;
    if (g_suspend_count[c] > 0) return;
    Audio_WriteReg(c, 3, (uint8_t)(af & 0xFF));
    Audio_WriteReg(c, 4, (uint8_t)((af >> 8) & 0x07));  /* no trigger bit */
}

static void tick_pitch_slide(int c, ch_seq_t *seq) {
    if (!seq->slide_active) return;
    {
        uint8_t d = seq->slide_cur_hi;
        uint8_t e = seq->slide_cur_lo;
        uint8_t th = seq->slide_target_hi;
        uint8_t tl = seq->slide_target_lo;
        int reached = 0;

        if (!seq->slide_decreasing) {
            /* Increasing path from Audio3_ApplyPitchSlide. */
            {
                uint16_t v = (((uint16_t)d << 8) | e) + seq->slide_step_int;
                d = (uint8_t)(v >> 8);
                e = (uint8_t)v;
            }
            {
                uint16_t s = (uint16_t)seq->slide_step_frac + (uint16_t)seq->slide_cur_frac;
                seq->slide_cur_frac = (uint8_t)s;
                if (s > 0xFF) {
                    uint16_t v = (((uint16_t)d << 8) | e) + 1;
                    d = (uint8_t)(v >> 8);
                    e = (uint8_t)v;
                }
            }
            if (d > th || (d == th && e > tl))
                reached = 1;
        } else {
            /* Decreasing path from Audio3_ApplyPitchSlide. */
            {
                uint16_t cur = ((uint16_t)d << 8) | e;
                cur = (uint16_t)(cur - seq->slide_step_int);
                d = (uint8_t)(cur >> 8);
                e = (uint8_t)cur;
            }
            {
                /* Engine mutates step_frac with add a each frame, then subtracts carry. */
                uint16_t dbl = (uint16_t)seq->slide_step_frac << 1;
                seq->slide_step_frac = (uint8_t)dbl;
                if (dbl > 0xFF) {
                    uint16_t cur = ((uint16_t)d << 8) | e;
                    cur = (uint16_t)(cur - 1);
                    d = (uint8_t)(cur >> 8);
                    e = (uint8_t)cur;
                }
            }
            if (d < th || (d == th && e < tl))
                reached = 1;
        }

        if (reached) {
            seq->slide_active = 0;
            d = th;
            e = tl;
        }

        seq->slide_cur_hi = d;
        seq->slide_cur_lo = e;
    }

    uint16_t f = (((uint16_t)seq->slide_cur_hi << 8) | seq->slide_cur_lo) & 0x07FFu;
    seq->vib_base_freq = f;
    if (g_suspend_count[c] > 0) return;
    Audio_WriteReg(c, 3, (uint8_t)(f & 0xFF));
    Audio_WriteReg(c, 4, (uint8_t)((f >> 8) & 0x07));  /* no trigger bit */
}

static void drum_fire_step(int c, ch_seq_t *seq) {
    if (!seq->drum_inst) return;
    if (seq->drum_step < 0 || seq->drum_step >= seq->drum_inst->count) return;
    const drum_step_t *st = &seq->drum_inst->steps[seq->drum_step];
    if (g_suspend_count[c] > 0) {
        seq->drum_timer = st->frames;
        return;
    }
    Audio_WriteReg(3, 2, st->env_byte);
    Audio_WriteReg(3, 3, st->nr43);
    Audio_WriteReg(3, 4, 0x80);
    seq->drum_timer = st->frames;
}

static void drum_start(ch_seq_t *seq, int inst_id) {
    seq->drum_inst = NULL;
    seq->drum_step = -1;
    seq->drum_timer = 0;
    if (inst_id <= 0 || inst_id >= 20) return;
    if (kDrumInst[inst_id].count == 0) return;
    seq->drum_inst = &kDrumInst[inst_id];
    seq->drum_step = 0;
}

static void tick_drum(int c, ch_seq_t *seq) {
    if (!seq->drum_inst) return;
    if (seq->drum_step < 0 || seq->drum_step >= seq->drum_inst->count) {
        seq->drum_inst = NULL;
        seq->drum_step = -1;
        seq->drum_timer = 0;
        return;
    }
    if (seq->drum_timer <= 0) {
        drum_fire_step(c, seq);
        return;
    }
    if (--seq->drum_timer <= 0) {
        seq->drum_step++;
        if (seq->drum_step >= seq->drum_inst->count) {
            seq->drum_inst = NULL;
            seq->drum_step = -1;
            seq->drum_timer = 0;
        }
    }
}

static void reset_drum(int c, ch_seq_t *seq, const note_evt_t *n) {
    drum_start(seq, n->duty);
    if (!seq->drum_inst) return;
    /* Trigger first drum step immediately on note start, then continue with
     * remaining steps during subsequent Music_Update ticks.
     */
    const drum_step_t *st0 = &seq->drum_inst->steps[0];
    if (g_suspend_count[c] == 0) {
        Audio_WriteReg(3, 2, st0->env_byte);
        Audio_WriteReg(3, 3, st0->nr43);
        Audio_WriteReg(3, 4, 0x80);
    }
    seq->drum_timer = st0->frames;
    seq->drum_step = 1;
}

static void fire_note(int ch, const note_evt_t *n) {
    if (ch == 3) {
        /* CH4 uses reset_drum/tick_drum path. */
        return;
    }

    if (n->freq > 0) {
        /* Ch3 (wave channel): load correct wave pattern before triggering */
        if (ch == 2) Audio_SetWaveInstrument(n->duty);
        /* NRx1: duty (bits 7-6) + max sound length */
        Audio_WriteReg(ch, 1, (uint8_t)((n->duty << 6) | 0x3F));
        /* NRx2: initial volume (bits 7-4) | dir (bit 3) | pace (bits 2-0) */
        Audio_WriteReg(ch, 2, n->env_byte);
        /* NRx3: freq low byte */
        Audio_WriteReg(ch, 3, (uint8_t)(n->freq & 0xFF));
        /* NRx4: freq high 3 bits + trigger */
        Audio_WriteReg(ch, 4, (uint8_t)(((n->freq >> 8) & 0x07) | 0x80));
    } else {
        /* Rest: silence channel without triggering */
        Audio_WriteReg(ch, 2, 0x00);  /* volume = 0 */
        Audio_WriteReg(ch, 4, 0x00);  /* no trigger */
    }
}

/* ---- Public API --------------------------------------------------- */
void Music_Play(uint8_t music_id) {
    if (music_id == gCurrentMusic) return;
    Music_Stop();
    if (music_id == MUSIC_NONE || music_id >= NUM_SONGS) return;

    gCurrentMusic = music_id;
    const song_t *s = &kSongs[music_id];

    for (int c = 0; c < 4; c++) {
        const ch_data_t *d = s->ch[c];
        if (!d || d->count == 0) {
            gSeq[c].data = NULL;
            continue;
        }
        gSeq[c].data  = d;
        gSeq[c].pos   = 0;  /* always start from the beginning (intro + loop) */
        gSeq[c].delay = 0;
        /* Fire first note immediately */
        fire_note(c, &d->notes[gSeq[c].pos]);
        reset_vib(&gSeq[c], &d->notes[gSeq[c].pos]);
        reset_slide(&gSeq[c], &d->notes[gSeq[c].pos]);
        if (c == 3) reset_drum(c, &gSeq[c], &d->notes[gSeq[c].pos]);
        gSeq[c].delay = d->notes[gSeq[c].pos].frames;
    }
    /* Prevent Music_Update from consuming a frame on the same tick as Music_Play.
     * In original pokered, PlayMusic always precedes the next VBlank audio tick. */
    g_skip_update = 1;
}

void Music_PlayFromLoop(uint8_t music_id) {
    /* Like Music_Play but starts each channel at its loop_start position,
     * skipping the intro.  Mirrors Music_RivalAlternateStart from
     * audio/alternate_tempo.asm. */
    Music_Stop();
    if (music_id == MUSIC_NONE || music_id >= NUM_SONGS) return;

    gCurrentMusic = music_id;
    const song_t *s = &kSongs[music_id];

    for (int c = 0; c < 4; c++) {
        const ch_data_t *d = s->ch[c];
        if (!d || d->count == 0) {
            gSeq[c].data = NULL;
            continue;
        }
        gSeq[c].data  = d;
        gSeq[c].pos   = d->loop_start;  /* skip intro */
        gSeq[c].delay = 0;
        fire_note(c, &d->notes[gSeq[c].pos]);
        reset_vib(&gSeq[c], &d->notes[gSeq[c].pos]);
        reset_slide(&gSeq[c], &d->notes[gSeq[c].pos]);
        if (c == 3) reset_drum(c, &gSeq[c], &d->notes[gSeq[c].pos]);
        gSeq[c].delay = d->notes[gSeq[c].pos].frames;
    }
    g_skip_update = 1;
}

void Music_Stop(void) {
    gCurrentMusic    = MUSIC_NONE;
    g_skip_update    = 0;
    g_suspend_count[0] = 0;
    g_suspend_count[1] = 0;
    g_suspend_count[2] = 0;
    g_suspend_count[3] = 0;
    for (int c = 0; c < 4; c++) {
        gSeq[c].data = NULL;
        gSeq[c].drum_inst = NULL;
        gSeq[c].drum_step = -1;
        gSeq[c].drum_timer = 0;
        Audio_WriteReg(c, 2, 0x00);  /* mute */
        Audio_WriteReg(c, 4, 0x00);
    }
}

void Music_Update(void) {
    /* Skip the first update after Music_Play to avoid a same-frame early advance */
    if (g_skip_update) { g_skip_update = 0; return; }

    for (int c = 0; c < 4; c++) {
        ch_seq_t *seq = &gSeq[c];
        if (!seq->data) continue;

        tick_pitch_slide(c, seq);
        if (c == 3) tick_drum(c, seq);
        /* GB engine gives pitch slide priority over vibrato while active. */
        if (c != 3 && !seq->slide_active)
            tick_vibrato(c, seq);

        if (--seq->delay > 0) continue;

        /* Advance to next event */
        seq->pos++;
        if (seq->pos >= seq->data->count) {
            int ls = seq->data->loop_start;
            if (ls >= 0)
                seq->pos = ls;
            else {
                seq->data = NULL;
                Audio_WriteReg(c, 2, 0x00);
                continue;
            }
        }

        const note_evt_t *n = &seq->data->notes[seq->pos];
        seq->delay = n->frames;
        reset_vib(seq, n);
        reset_slide(seq, n);
        if (c == 3) reset_drum(c, seq, n);
        /* Only write to hardware if channel is not owned by SFX.
         * Advancing pos/delay keeps this channel in sync with others. */
        if (g_suspend_count[c] == 0 && c != 3)
            fire_note(c, n);
    }
}

void Music_SuspendChannel(int c) {
    if (c < 0 || c >= 4) return;
    if (g_suspend_count[c] < 0xFF) g_suspend_count[c]++;
    /* Immediately silence the hardware channel so currently ringing music
     * notes do not bleed under higher-priority SFX like Pokémon cries. */
    Audio_WriteReg(c, 2, 0x00);
    Audio_WriteReg(c, 4, 0x00);
}

void Music_ResumeChannel(int c) {
    if (c < 0 || c >= 4) return;
    if (g_suspend_count[c] == 0) return;
    g_suspend_count[c]--;
    if (g_suspend_count[c] > 0) return;
    if (gSeq[c].data) {
        /* Re-fire the current note so music resumes seamlessly */
        if (c == 3)
            reset_drum(c, &gSeq[c], &gSeq[c].data->notes[gSeq[c].pos]);
        else
            fire_note(c, &gSeq[c].data->notes[gSeq[c].pos]);
    } else
        /* No music on this channel — mute so the SFX note doesn't ring out */
        Audio_WriteReg(c, 2, 0x00);
}

int Music_IsPlaying(void) {
    for (int c = 0; c < 4; c++)
        if (gSeq[c].data) return 1;
    return 0;
}

uint8_t Music_GetMapID(uint8_t map_id) {
    if (map_id >= (uint8_t)(sizeof(kMapMusicID)/sizeof(kMapMusicID[0])))
        return MUSIC_NONE;
    return kMapMusicID[map_id];
}
