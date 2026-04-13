/* music.c — Note sequencer driving the GB APU synthesis engine.
 *
 * Architecture:
 *   music_data_gen.h  — auto-generated note event arrays (tools/extract_audio.py)
 *   Music_Play()      — select a song, reset channel sequencers
 *   Music_Update()    — called once per frame (60 Hz); advances each channel
 *                       and calls Audio_WriteReg() when a new note fires
 *
 * Three music channels map to GB APU channels 0-2:
 *   CH0 = GB CH1 (square wave, melody)
 *   CH1 = GB CH2 (square wave, harmony)
 *   CH2 = GB CH3 (wave channel, bass/accompaniment)
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
} ch_seq_t;

static ch_seq_t gSeq[3];
static uint8_t  gCurrentMusic   = MUSIC_NONE;
static int      g_skip_update    = 0;  /* skip first Music_Update after Music_Play */
static uint8_t  g_suspend_count[3] = {0, 0, 0};  /* nested channel ownership by SFX */

/* ---- Song table --------------------------------------------------- */
/* Each entry: up to 3 channels (NULL = unused). */
typedef struct {
    const ch_data_t *ch[3];
} song_t;

static const song_t kSongs[] = {
    /* MUSIC_NONE */           { { NULL,            NULL,            NULL            } },
    /* MUSIC_PALLET_TOWN */    { { &kPalletTown_Ch1,&kPalletTown_Ch2,&kPalletTown_Ch3} },
    /* MUSIC_POKECENTER */     { { &kPokecenter_Ch1,&kPokecenter_Ch2,&kPokecenter_Ch3} },
    /* MUSIC_GYM */            { { &kGym_Ch1,       &kGym_Ch2,       &kGym_Ch3       } },
    /* MUSIC_CITIES1 */        { { &kCities1_Ch1,   &kCities1_Ch2,   &kCities1_Ch3   } },
    /* MUSIC_CITIES2 */        { { &kCities2_Ch1,   &kCities2_Ch2,   &kCities2_Ch3   } },
    /* MUSIC_CELADON */        { { NULL,            NULL,            NULL            } },
    /* MUSIC_CINNABAR */       { { NULL,            NULL,            NULL            } },
    /* MUSIC_VERMILION */      { { &kVermilion_Ch1, &kVermilion_Ch2, &kVermilion_Ch3 } },
    /* MUSIC_LAVENDER */       { { NULL,            NULL,            NULL            } },
    /* MUSIC_SS_ANNE */        { { &kSSAnne_Ch1,    &kSSAnne_Ch2,    &kSSAnne_Ch3    } },
    /* MUSIC_ROUTES1 */        { { &kRoutes1_Ch1,   &kRoutes1_Ch2,   &kRoutes1_Ch3   } },
    /* MUSIC_ROUTES2 */        { { &kRoutes2_Ch1,   &kRoutes2_Ch2,   &kRoutes2_Ch3   } },
    /* MUSIC_ROUTES3 */        { { &kRoutes3_Ch1,   &kRoutes3_Ch2,   &kRoutes3_Ch3   } },
    /* MUSIC_ROUTES4 */        { { NULL,            NULL,            NULL            } },
    /* MUSIC_INDIGO_PLATEAU */ { { NULL,            NULL,            NULL            } },
    /* MUSIC_OAKS_LAB */       { { &kOaksLab_Ch1,   &kOaksLab_Ch2,   &kOaksLab_Ch3   } },
    /* MUSIC_DUNGEON1 */       { { NULL,            NULL,            NULL            } },
    /* MUSIC_DUNGEON2 */       { { &kDungeon2_Ch1,  &kDungeon2_Ch2,  &kDungeon2_Ch3  } },
    /* MUSIC_DUNGEON3 */       { { &kDungeon3_Ch1,  &kDungeon3_Ch2,  &kDungeon3_Ch3  } },
    /* MUSIC_POKEMON_TOWER */  { { NULL,            NULL,            NULL            } },
    /* MUSIC_SILPH_CO */       { { NULL,            NULL,            NULL            } },
    /* MUSIC_SAFARI_ZONE */    { { &kSafariZone_Ch1,&kSafariZone_Ch2,&kSafariZone_Ch3} },
    /* MUSIC_TITLE */          { { NULL,            NULL,            NULL            } },
    /* MUSIC_JIGGLYPUFF */     { { NULL,                NULL,                NULL                } },
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

static void fire_note(int ch, const note_evt_t *n) {
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

    for (int c = 0; c < 3; c++) {
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

    for (int c = 0; c < 3; c++) {
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
    for (int c = 0; c < 3; c++) {
        gSeq[c].data = NULL;
        Audio_WriteReg(c, 2, 0x00);  /* mute */
        Audio_WriteReg(c, 4, 0x00);
    }
}

void Music_Update(void) {
    /* Skip the first update after Music_Play to avoid a same-frame early advance */
    if (g_skip_update) { g_skip_update = 0; return; }

    for (int c = 0; c < 3; c++) {
        ch_seq_t *seq = &gSeq[c];
        if (!seq->data) continue;

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
        /* Only write to hardware if channel is not owned by SFX.
         * Advancing pos/delay keeps this channel in sync with others. */
        if (g_suspend_count[c] == 0)
            fire_note(c, n);
    }
}

void Music_SuspendChannel(int c) {
    if (c < 0 || c >= 3) return;
    if (g_suspend_count[c] < 0xFF) g_suspend_count[c]++;
    /* Immediately silence the hardware channel so currently ringing music
     * notes do not bleed under higher-priority SFX like Pokémon cries. */
    Audio_WriteReg(c, 2, 0x00);
    Audio_WriteReg(c, 4, 0x00);
}

void Music_ResumeChannel(int c) {
    if (c < 0 || c >= 3) return;
    if (g_suspend_count[c] == 0) return;
    g_suspend_count[c]--;
    if (g_suspend_count[c] > 0) return;
    if (gSeq[c].data)
        /* Re-fire the current note so music resumes seamlessly */
        fire_note(c, &gSeq[c].data->notes[gSeq[c].pos]);
    else
        /* No music on this channel — mute so the SFX note doesn't ring out */
        Audio_WriteReg(c, 2, 0x00);
}

int Music_IsPlaying(void) {
    for (int c = 0; c < 3; c++)
        if (gSeq[c].data) return 1;
    return 0;
}

uint8_t Music_GetMapID(uint8_t map_id) {
    if (map_id >= (uint8_t)(sizeof(kMapMusicID)/sizeof(kMapMusicID[0])))
        return MUSIC_NONE;
    return kMapMusicID[map_id];
}
