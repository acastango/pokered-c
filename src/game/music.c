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
} ch_seq_t;

static ch_seq_t gSeq[3];
static uint8_t  gCurrentMusic   = MUSIC_NONE;
static int      g_skip_update    = 0;  /* skip first Music_Update after Music_Play */
static int      g_suspended_mask = 0;  /* bitmask: channel suspended by SFX */

/* ---- Song table --------------------------------------------------- */
/* Each entry: up to 3 channels (NULL = unused). */
typedef struct {
    const ch_data_t *ch[3];
} song_t;

static const song_t kSongs[] = {
    /* MUSIC_NONE */           { { NULL,            NULL,            NULL            } },
    /* MUSIC_PALLET_TOWN */    { { &kPalletTown_Ch1,&kPalletTown_Ch2,&kPalletTown_Ch3} },
    /* MUSIC_POKECENTER */     { { &kPokecenter_Ch1,&kPokecenter_Ch2,&kPokecenter_Ch3} },
    /* MUSIC_GYM */            { { NULL,            NULL,            NULL            } },
    /* MUSIC_CITIES1 */        { { &kCities1_Ch1,   &kCities1_Ch2,   &kCities1_Ch3   } },
    /* MUSIC_CITIES2 */        { { &kCities2_Ch1,   &kCities2_Ch2,   &kCities2_Ch3   } },
    /* MUSIC_CELADON */        { { NULL,            NULL,            NULL            } },
    /* MUSIC_CINNABAR */       { { NULL,            NULL,            NULL            } },
    /* MUSIC_VERMILION */      { { NULL,            NULL,            NULL            } },
    /* MUSIC_LAVENDER */       { { NULL,            NULL,            NULL            } },
    /* MUSIC_SS_ANNE */        { { NULL,            NULL,            NULL            } },
    /* MUSIC_ROUTES1 */        { { &kRoutes1_Ch1,   &kRoutes1_Ch2,   &kRoutes1_Ch3   } },
    /* MUSIC_ROUTES2 */        { { NULL,            NULL,            NULL            } },
    /* MUSIC_ROUTES3 */        { { &kRoutes3_Ch1,   &kRoutes3_Ch2,   &kRoutes3_Ch3   } },
    /* MUSIC_ROUTES4 */        { { NULL,            NULL,            NULL            } },
    /* MUSIC_INDIGO_PLATEAU */ { { NULL,            NULL,            NULL            } },
    /* MUSIC_OAKS_LAB */       { { &kOaksLab_Ch1,   &kOaksLab_Ch2,   &kOaksLab_Ch3   } },
    /* MUSIC_DUNGEON1 */       { { NULL,            NULL,            NULL            } },
    /* MUSIC_DUNGEON2 */       { { &kDungeon2_Ch1,  &kDungeon2_Ch2,  &kDungeon2_Ch3  } },
    /* MUSIC_DUNGEON3 */       { { &kDungeon3_Ch1,  &kDungeon3_Ch2,  &kDungeon3_Ch3  } },
    /* MUSIC_POKEMON_TOWER */  { { NULL,            NULL,            NULL            } },
    /* MUSIC_SILPH_CO */       { { NULL,            NULL,            NULL            } },
    /* MUSIC_SAFARI_ZONE */    { { NULL,            NULL,            NULL            } },
    /* MUSIC_TITLE */          { { NULL,            NULL,            NULL            } },
    /* MUSIC_JIGGLYPUFF */     { { NULL,                NULL,                NULL                } },
    /* MUSIC_WILD_BATTLE */         { { &kWildBattle_Ch1,        &kWildBattle_Ch2,        &kWildBattle_Ch3        } },
    /* MUSIC_DEFEATED_WILD_MON */   { { &kDefeatedWildMon_Ch1,   &kDefeatedWildMon_Ch2,   &kDefeatedWildMon_Ch3   } },
    /* MUSIC_DEFEATED_TRAINER */    { { &kDefeatedTrainer_Ch1,   &kDefeatedTrainer_Ch2,   &kDefeatedTrainer_Ch3   } },
    /* MUSIC_DEFEATED_GYM_LEADER */ { { &kDefeatedGymLeader_Ch1, &kDefeatedGymLeader_Ch2, &kDefeatedGymLeader_Ch3 } },
    /* MUSIC_PKMN_HEALED */        { { &kPkmnHealed_Ch1,        &kPkmnHealed_Ch2,        &kPkmnHealed_Ch3        } },
    /* MUSIC_TRAINER_BATTLE */     { { &kTrainerBattle_Ch1,     &kTrainerBattle_Ch2,     &kTrainerBattle_Ch3     } },
};
#define NUM_SONGS  ((int)(sizeof(kSongs)/sizeof(kSongs[0])))

/* ---- Helpers ------------------------------------------------------ */
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
        gSeq[c].delay = d->notes[gSeq[c].pos].frames;
    }
    /* Prevent Music_Update from consuming a frame on the same tick as Music_Play.
     * In original pokered, PlayMusic always precedes the next VBlank audio tick. */
    g_skip_update = 1;
}

void Music_Stop(void) {
    gCurrentMusic    = MUSIC_NONE;
    g_skip_update    = 0;
    g_suspended_mask = 0;
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
        /* Only write to hardware if channel is not owned by SFX.
         * Advancing pos/delay keeps this channel in sync with others. */
        if (!(g_suspended_mask & (1 << c)))
            fire_note(c, n);
    }
}

void Music_SuspendChannel(int c) {
    g_suspended_mask |= (1 << c);
}

void Music_ResumeChannel(int c) {
    g_suspended_mask &= ~(1 << c);
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
