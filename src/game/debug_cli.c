/* debug_cli.c — File-based game controller for Claude Code / scripted testing.
 *
 * Input injection: flat per-frame button sequence fed into Input_Update().
 * State dump:      ASCII grid (overworld) or text battle summary written to
 *                  bugs/cli_state.txt after each command finishes.
 */
#include "debug_cli.h"
#include "overworld.h"          /* Map_GetTile, gCamX, gCamY, Tile_IsPassable */
#include "npc.h"                /* NPC_GetCount, NPC_GetTilePos */
#include "player.h"             /* Player_GetLedgeDir */
#include "warp.h"               /* Warp_IsDoorTile */
#include "text.h"               /* Text_IsOpen, Text_GetCurrentStr */
#include "yesno.h"
#include "trainer_sight.h"      /* Trainer_IsEngaging */
#include "pokecenter.h"         /* Pokecenter_IsWaitingYesNo */
#include "pokemon.h"            /* Pokemon_GetName, Pokemon_InitMon */
#include "battle/battle_ui.h"   /* BattleUI_GetState */
#include "battle/battle_init.h" /* Battle_Start */
#include "battle/battle_exp.h"  /* Battle_AddExpDirect, gDebugExpRate */
#include "battle/battle.h"      /* Battle hittrace diagnostics */
#include "music.h"              /* Music_Play */
#include "rockethideout_b4f_scripts.h"
#include "pallet_scripts.h"
#include "oakslab_scripts.h"
#include "viridian_mart_scripts.h"
#include "mtmoon_scripts.h"
#include "cerulean_scripts.h"
#include "route22_scripts.h"
#include "route24_scripts.h"
#include "bills_house_scripts.h"
#include "route2gate_scripts.h"
#include "ss_anne_scripts.h"
#include "vermilion_gym_scripts.h"
#include "rockethideout_scripts.h"
#include "game_corner_scripts.h"
#include "pokemontower2f_scripts.h"
#include "pokemontower5f_scripts.h"
#include "pokemontower6f_scripts.h"
#include "pokemontower7f_scripts.h"
#include "mrfujis_house_scripts.h"
#include "saffron_city_scripts.h"
#include "gate_scripts.h"
#include "gym_scripts.h"
#include "../data/base_stats.h" /* gSpeciesToDex */
#include "../data/map_data.h"       /* gMapTable */
#include "../data/event_data.h"    /* map_events_t, gMapEvents */
#include "../data/event_flag_names.h"
#include "../data/moves_data.h"     /* BMOVE */
#include "../data/trainer_sprites.h"
#include "../data/event_constants.h"
#include "inventory.h"
#include "badge.h"
#include "../platform/hardware.h"
#include "../platform/save.h"
#include "../game/constants.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif
#include "../data/font_data.h"

#define CMD_FILE   "bugs/cli_cmd.txt"
#define STATE_FILE "bugs/cli_state.txt"
#define REPLAY_DIR "bugs/replays"

/* ---- Button masks (matching input.c bit layout) ------------------- */
#define BTN_A      0x01
#define BTN_B      0x02
#define BTN_SELECT 0x04
#define BTN_START  0x08
#define BTN_RIGHT  0x10
#define BTN_LEFT   0x20
#define BTN_UP     0x40
#define BTN_DOWN   0x80

/* ---- Input injection (read by Input_Update each frame) ------------ */
uint8_t gCliButtons = 0;
int     gCliFrames  = 0;

/* ---- Sequence queue: flat array of per-frame button bytes --------- */
#define SEQ_MAX 512
static uint8_t s_seq[SEQ_MAX];
static int     s_seq_len = 0;
static int     s_seq_pos = 0;

/* Append press_frames of btn, then gap_frames of 0 */
static void seq_push(uint8_t btn, int press_frames, int gap_frames) {
    for (int i = 0; i < press_frames && s_seq_len < SEQ_MAX; i++)
        s_seq[s_seq_len++] = btn;
    for (int i = 0; i < gap_frames && s_seq_len < SEQ_MAX; i++)
        s_seq[s_seq_len++] = 0;
}

static void seq_clear(void) { s_seq_len = 0; s_seq_pos = 0; }

/* ---- Internal state ----------------------------------------------- */
static int s_poll_timer     = 0;
static int s_wait_remaining = 0;
static int s_pending_write  = 0;
static int s_script_trace_enabled = 0;
static int s_script_trace_to_file = 0;
static uint8_t s_trace_prev_map = 0xFF;
static uint8_t s_trace_prev_trainer_engaging = 0xFF;
static uint8_t s_trace_prev_text_open = 0xFF;
static uint8_t s_trace_prev_gate_active = 0xFF;
static uint8_t s_trace_prev_route22_active = 0xFF;
static uint8_t s_trace_prev_route24_active = 0xFF;
static uint8_t s_trace_prev_ssanne_active = 0xFF;
static uint8_t s_trace_prev_viridian_mart_active = 0xFF;
static uint8_t s_trace_prev_gym_active = 0xFF;
static uint8_t s_trace_prev_rockethideout_b4f_active = 0xFF;
static int s_temp_npc_walkoff_active = 0;
static int s_temp_npc_walkoff_idx = -1;
static int s_temp_npc_walkoff_phase = 0; /* 0=none,1=spawned_wait_text_open,2=wait_text_close,3=walkoff */
static int s_temp_npc_walkoff_pretext_frames = 0;

/* ---- Simple debug scene runner ------------------------------------ */
#define SCENE_CMD_MAX 128
#define SCENE_ACTOR_MAX 16
#define SCENE_DEF_MAX 16
#define SCENE_DEF_LINE_MAX 32
typedef enum scene_op_t {
    SCOP_NOP = 0,
    SCOP_SPAWN,
    SCOP_DESPAWN,
    SCOP_FACE,
    SCOP_MOVE,
    SCOP_SAY,
    SCOP_ASK,
    SCOP_BATTLESTART,
    SCOP_BATTLEEND,
    SCOP_MUSIC,
    SCOP_WAIT,
    SCOP_WAIT_TEXT,
    SCOP_LOCK_INPUT,
    SCOP_END
} scene_op_t;
typedef struct scene_cmd_t {
    scene_op_t op;
    char actor[24];
    int a, b, c;
    char text[160];
    uint8_t team_count;
    uint8_t team_species[6];
    uint8_t team_level[6];
    uint8_t team_moves[6][4];
} scene_cmd_t;
typedef struct scene_actor_t {
    int used;
    char name[24];
    int npc_idx;
    int spawned_by_scene;
    uint8_t sprite_id;
    int last_x;
    int last_y;
} scene_actor_t;
static int s_scene_active = 0;
static scene_cmd_t s_scene_cmds[SCENE_CMD_MAX];
static int s_scene_cmd_count = 0;
static int s_scene_pc = 0;
static int s_scene_wait = 0;
static int s_scene_wait_yesno = 0;
static int s_scene_wait_say = 0;
static int s_scene_say_opened = 0;
static int s_scene_wait_battle = 0;
static int s_scene_wait_battleend_text = 0;
static int s_scene_battle_started = 0;
static int s_scene_battlestart_pending = 0;
static int s_scene_battlestart_saw_text = 0;
static int s_scene_battlestart_delay = 0;
static int s_scene_battlestart_tc = 0;
static int s_scene_battlestart_tn = 0;
static int s_scene_last_yesno = -1; /* -1=none, 0=no, 1=yes */
static uint8_t s_scene_yesno_prev_joyignore = 0;
static int s_scene_yesno_restore_joyignore = 0;
static int s_scene_yesno_prev_scripted_movement = 0;
static int s_scene_yesno_restore_scripted_movement = 0;
static int s_scene_move_steps_left = 0;
static int s_scene_move_dir = 0;
static int s_scene_move_actor = -1;
static scene_actor_t s_scene_actors[SCENE_ACTOR_MAX];
typedef struct scene_def_t {
    int used;
    char name[32];
    char lines[SCENE_DEF_LINE_MAX][192];
    int line_count;
} scene_def_t;
static scene_def_t s_scene_defs[SCENE_DEF_MAX];
/* ---- Scene trigger points (tile based, debug-only wrapper layer) -- */
#define SCENE_TRIGGER_MAX 16
typedef struct scene_trigger_t {
    int used;
    char scene[64];
    uint8_t map_id;
    int x;
    int y;
    int armed;
    uint8_t cond_kind;   /* 0=none, 1=event_set, 2=event_clear */
    uint16_t cond_event;
} scene_trigger_t;
static scene_trigger_t s_scene_triggers[SCENE_TRIGGER_MAX];
typedef struct scene_npc_binding_t {
    int used;
    int npc_idx;
    uint8_t map_id;
    uint8_t sprite_id;
    int tile_x;
    int tile_y;
    char name[24];
    char scene[64];
} scene_npc_binding_t;
static scene_npc_binding_t s_scene_npc_bindings[SCENE_ACTOR_MAX];
static int s_dsl_bank_enabled = 0;
static int s_dsl_bank_init_done = 0;
static uint8_t s_dsl_bank_last_map = 0xFF;

#define SCRIPT_TRACE_LOG_PATH "bugs/script_trace.log"
#define SCRIPT_TRACE_LOG_PATH_OLD "bugs/script_trace.log.1"
#define SCRIPT_TRACE_MAX_BYTES (256 * 1024)
extern int Game_IsWarpFadeActive(void);

/* ---- Deterministic replay --------------------------------------- */
#define REPLAY_MAGIC   0x31504C52u /* "RLP1" */
#define REPLAY_VERSION 1u

typedef struct replay_header_t {
    uint32_t magic;
    uint32_t version;
    uint32_t state_size;
    uint32_t input_frames;
} replay_header_t;

static int      s_replay_recording = 0;
static int      s_replay_playing = 0;
static uint32_t s_replay_play_pos = 0;
static uint32_t s_replay_play_len = 0;
static uint8_t *s_replay_play_buf = NULL;
static FILE    *s_replay_rec_fp = NULL;
static char     s_replay_name[96] = {0};
static char     s_replay_tmp_state[192] = {0};
static char     s_replay_tmp_input[192] = {0};

/* ---- Sidebar history --------------------------------------------- */
#define CLI_HIST_MAX   64
#define CLI_HIST_WIDTH 72
static char s_hist[CLI_HIST_MAX][CLI_HIST_WIDTH + 1];
static uint8_t s_hist_color[CLI_HIST_MAX];
static int  s_hist_head = 0;
static int  s_hist_count = 0;
static int  s_last_cmd_hist_slot = -1;
static int  s_last_cmd_valid = 1;

static int cli_hist_push_color(const char *line, uint8_t color) {
    int slot;
    size_t i = 0;
    if (!line || !*line) return -1;
    slot = s_hist_head;
    while (line[i] && i < CLI_HIST_WIDTH) {
        char c = line[i];
        s_hist[s_hist_head][i++] = (c >= 32 && c <= 126) ? c : ' ';
    }
    s_hist[s_hist_head][i] = '\0';
    s_hist_color[s_hist_head] = color;
    s_hist_head = (s_hist_head + 1) % CLI_HIST_MAX;
    if (s_hist_count < CLI_HIST_MAX) s_hist_count++;
    return slot;
}

static int cli_hist_push(const char *line) {
    return cli_hist_push_color(line, CLI_HIST_COLOR_DEFAULT);
}

int DebugCLI_GetHistoryCount(void) { return s_hist_count; }

const char *DebugCLI_GetHistoryLine(int newest_index) {
    int idx;
    if (newest_index < 0 || newest_index >= s_hist_count) return NULL;
    idx = s_hist_head - 1 - newest_index;
    while (idx < 0) idx += CLI_HIST_MAX;
    return s_hist[idx];
}

int DebugCLI_GetHistoryColor(int newest_index) {
    int idx;
    if (newest_index < 0 || newest_index >= s_hist_count) return CLI_HIST_COLOR_DEFAULT;
    idx = s_hist_head - 1 - newest_index;
    while (idx < 0) idx += CLI_HIST_MAX;
    return s_hist_color[idx];
}

int DebugCLI_IsReplayPlaying(void) { return s_replay_playing; }

void DebugCLI_HistoryPushExternal(const char *line) {
    if (!line || !*line) return;
    cli_hist_push_color(line, CLI_HIST_COLOR_LOG);
}

/* ---- Move animation lab (auto-play move sequence in a controlled battle) */
static int s_animlab_enabled  = 0;
static int s_animlab_move_id  = 1;  /* next move ID to test (1..NUM_MOVE_DEFS-1) */
static int s_animlab_loops    = 0;  /* full passes through move IDs */
static int s_animlab_level    = 50;

/* ---- In-game console state --------------------------------------- */
#define CON_TOP_ROW      16
#define CON_IN_ROW       17
#define CON_BUFMAX       64
#define CON_BLINK_PERIOD 60

static char    s_con_buf[CON_BUFMAX + 1] = {0};
static int     s_con_len   = 0;
static int     s_con_open  = 0;
static int     s_con_blink = 0;
static uint8_t s_con_saved[2 * SCREEN_WIDTH];
static int     s_con_overlay_enabled = 1;
static int     s_con_always_open = 0;

/* ---- Helpers ------------------------------------------------------ */
static int get_scene(void) {
    extern int Game_GetScene(void);
    return Game_GetScene();
}

static const char *scene_name(int sc) {
    switch (sc) {
        case 0: return "OVERWORLD";
        case 1: return "BATTLE_TRANS";
        case 2: return "BATTLE";
        default: return "MENU";
    }
}

static const char *facing_name(uint8_t dir) {
    switch (dir) {
        case 0:  return "DOWN";
        case 4:  return "UP";
        case 8:  return "LEFT";
        case 12: return "RIGHT";
        default: return "?";
    }
}

static const char *status_str(uint8_t st) {
    if (st & 0x07) return "SLP";
    if (st & 0x40) return "PSN";
    if (st & 0x10) return "BRN";
    if (st & 0x20) return "FRZ";
    if (st & 0x08) return "PAR";
    return "OK";
}

static const char *bui_state_name(int s) {
    /* Matches bui_state_t enum order in battle_ui.c */
    switch (s) {
        case  0: return "INACTIVE";
        case  1: return "SLIDE_IN";
        case  2: return "APPEARED";
        case  3: return "SEND_OUT";
        case  4: return "ENEMY_SLIDE_OUT";
        case  5: return "TRAINER_SLIDE_OUT";
        case  6: return "ENEMY_SEND_OUT";
        case  7: return "POKEMON_APPEAR";
        case  8: return "INTRO";
        case  9: return "DRAW_HUD";
        case 10: return "MENU";
        case 11: return "MOVE_SELECT";
        case 12: return "MOVE_ANIM";
        case 13: return "HP_ANIM";
        case 14: return "EXEC_MOVE_B";
        case 15: return "EXEC_SECOND";
        case 16: return "TURN_END";
        case 17: return "TURN_FINISH";
        case 18: return "EXP_DRAIN";
        case 19: return "LEVELUP_STATS";
        case 20: return "ENEMY_FAINT_ANIM";
        case 21: return "PLAYER_FAINTED";
        case 22: return "USE_NEXT_MON";
        case 23: return "PARTY_SELECT";
        case 24: return "SWITCH_SELECT";
        case 25: return "RETREAT_ANIM";
        case 26: return "SWITCH_ENEMY_TURN";
        case 27: return "BAG_BATTLE";
        case 28: return "BALL_THROW";
        case 29: return "BALL_POOF";
        case 30: return "BALL_SHAKE";
        case 31: return "CAUGHT";
        case 32: return "END";
        default: return "?";
    }
}

static int debugcli_start_npc_walkoff(int print_log) {
    int spawn_x = (int)wXCoord + 1;
    int spawn_y = (int)wYCoord;
    int npc_face = 0;
    int idx;
    if (s_temp_npc_walkoff_active && s_temp_npc_walkoff_idx >= 0) {
        NPC_DebugDespawn(s_temp_npc_walkoff_idx);
        s_temp_npc_walkoff_active = 0;
        s_temp_npc_walkoff_idx = -1;
        s_temp_npc_walkoff_phase = 0;
        s_temp_npc_walkoff_pretext_frames = 0;
    }
    if (wPlayerDirection == 0) npc_face = 0;          /* down */
    else if (wPlayerDirection == 4) npc_face = 1;     /* up */
    else if (wPlayerDirection == 8) npc_face = 2;     /* left */
    else if (wPlayerDirection == 12) npc_face = 3;    /* right */
    idx = NPC_DebugSpawn(0x01, spawn_x, spawn_y, npc_face, 0 /* stay */);
    if (idx < 0) {
        if (print_log) printf("[cli] npc_walkoff: failed to spawn NPC (no free slot?)\n");
        return 0;
    }
    s_temp_npc_walkoff_active = 1;
    s_temp_npc_walkoff_idx = idx;
    s_temp_npc_walkoff_phase = 1;
    s_temp_npc_walkoff_pretext_frames = 24; /* explicit visible beat before dialogue */
    wJoyIgnore = PAD_CTRL_PAD; /* lock movement, keep A/B for text flow */
    if (print_log) {
        printf("[cli] npc_walkoff: spawned npc idx=%d at (%d,%d), dialogue->walkoff sequence started\n",
               idx, spawn_x, spawn_y);
    }
    return 1;
}

int DebugCLI_TriggerNpcWalkoff(void) {
    return debugcli_start_npc_walkoff(0);
}

int DebugCLI_IsNpcWalkoffActive(void) {
    return s_temp_npc_walkoff_active;
}

static const char *hittrace_reason_name(uint8_t r) {
    switch (r) {
        case BHTR_HIT: return "hit";
        case BHTR_MISS_DREAM_EATER: return "miss:dream_eater_target_awake";
        case BHTR_HIT_SWIFT: return "hit:swift_always";
        case BHTR_MISS_INVULNERABLE: return "miss:target_invulnerable";
        case BHTR_MISS_MIST: return "miss:mist_block";
        case BHTR_HIT_XACCURACY: return "hit:x_accuracy_bypass";
        case BHTR_MISS_ACCURACY_ROLL: return "miss:accuracy_roll";
        default: return "unknown";
    }
}

static int first_alive_party_slot(void) {
    for (int i = 0; i < wPartyCount && i < PARTY_LENGTH; i++) {
        if (wPartyMons[i].base.hp > 0) return i;
    }
    return -1;
}

static void animlab_set_player_move(uint8_t move_id) {
    if (move_id == 0 || move_id >= NUM_MOVE_DEFS) move_id = 1;

    int slot = (int)wPlayerMonNumber;
    if (slot < 0 || slot >= PARTY_LENGTH || slot >= wPartyCount) slot = 0;

    wBattleMon.moves[0] = move_id;
    wBattleMon.moves[1] = 0;
    wBattleMon.moves[2] = 0;
    wBattleMon.moves[3] = 0;
    wBattleMon.pp[0]    = gMoves[move_id].pp;
    wBattleMon.pp[1]    = 0;
    wBattleMon.pp[2]    = 0;
    wBattleMon.pp[3]    = 0;

    wPartyMons[slot].base.moves[0] = move_id;
    wPartyMons[slot].base.moves[1] = 0;
    wPartyMons[slot].base.moves[2] = 0;
    wPartyMons[slot].base.moves[3] = 0;
    wPartyMons[slot].base.pp[0]    = gMoves[move_id].pp;
    wPartyMons[slot].base.pp[1]    = 0;
    wPartyMons[slot].base.pp[2]    = 0;
    wPartyMons[slot].base.pp[3]    = 0;
}

static void animlab_set_enemy_harmless(void) {
    /* Keep enemy harmless and deterministic so we can focus on move visuals. */
    wEnemyMon.moves[0] = MOVE_GROWL;
    wEnemyMon.moves[1] = 0;
    wEnemyMon.moves[2] = 0;
    wEnemyMon.moves[3] = 0;
    wEnemyMon.pp[0]    = gMoves[MOVE_GROWL].pp;
    wEnemyMon.pp[1]    = 0;
    wEnemyMon.pp[2]    = 0;
    wEnemyMon.pp[3]    = 0;
}

static void animlab_start_battle(int level) {
    int alive = first_alive_party_slot();
    if (alive < 0) {
        Pokemon_InitMon(&wPartyMons[0], STARTER1, (uint8_t)level);
        wPartyCount = 1;
        alive = 0;
    }

    wPartyMons[alive].base.hp     = wPartyMons[alive].max_hp;
    wPartyMons[alive].base.status = 0;

    wCurPartySpecies = SPECIES_RHYDON;
    wCurEnemyLevel   = (uint8_t)level;

    Music_Play(MUSIC_WILD_BATTLE);
    Battle_Start();

    animlab_set_enemy_harmless();
    animlab_set_player_move((uint8_t)s_animlab_move_id);

    BattleUI_Enter();
    extern void Game_SetScene(int);
    Game_SetScene(2); /* SCENE_BATTLE */

    s_animlab_enabled = 1;
    s_animlab_level   = level;
    printf("[cli] animlab: started (level %d), auto-playing move animations\n", level);
}

static int count_bits8(uint8_t value) {
#ifdef _MSC_VER
    unsigned int bits = value;
    bits = bits - ((bits >> 1) & 0x55u);
    bits = (bits & 0x33u) + ((bits >> 2) & 0x33u);
    return (int)((bits + (bits >> 4)) & 0x0Fu);
#else
    return __builtin_popcount((unsigned int)value);
#endif
}

/* Resolve move by numeric token (hex/dec) or move name.
 * Name matching is case-insensitive and ignores spaces/underscores. */
static int cli_resolve_move_id(const char *move_str) {
    if (!move_str || !*move_str) return 0;

    char *end;
    long parsed = strtol(move_str, &end, 0);
    if (end != move_str && *end == '\0') return (int)parsed;

    char needle[32] = {0};
    int ni = 0;
    for (int i = 0; move_str[i] && ni < 31; i++) {
        char c = (char)tolower((unsigned char)move_str[i]);
        if (c != ' ' && c != '_') needle[ni++] = c;
    }
    for (int id = 1; id <= 0xA5; id++) {
        const char *nm = gMoveNames[id];
        char norm[32] = {0};
        int nnorm = 0;
        for (int i = 0; nm[i] && nnorm < 31; i++) {
            char c = (char)tolower((unsigned char)nm[i]);
            if (c != ' ' && c != '_') norm[nnorm++] = c;
        }
        if (strcmp(needle, norm) == 0) return id;
    }
    return 0;
}

/* Resolve species by dex number or name (case-insensitive, ignores spaces/_). */
static int cli_resolve_species_id(const char *species_str) {
    if (!species_str || !*species_str) return 0;
    {
        char *end;
        long parsed = strtol(species_str, &end, 0);
        if (end != species_str && *end == '\0') {
            if (parsed >= 1 && parsed <= 151) return gDexToSpecies[parsed];
            return (int)parsed;
        }
    }

    {
        char needle[40] = {0};
        int ni = 0;
        for (int i = 0; species_str[i] && ni < 39; i++) {
            char c = (char)tolower((unsigned char)species_str[i]);
            if (c != ' ' && c != '_') needle[ni++] = c;
        }
        for (int dex = 1; dex <= 151; dex++) {
            const char *nm = Pokemon_GetName((uint8_t)dex);
            char norm[40] = {0};
            int nn = 0;
            for (int i = 0; nm[i] && nn < 39; i++) {
                char c = (char)tolower((unsigned char)nm[i]);
                if (c != ' ' && c != '_') norm[nn++] = c;
            }
            if (strcmp(needle, norm) == 0) return gDexToSpecies[dex];
        }
    }
    return 0;
}

/* Parse one custom team slot token:
 * - empty markers: "", "-", "empty", "none", "0"
 * - filled format: species,level,move1,move2,move3,move4
 */
static int scene_parse_team_slot(const char *slot_str,
                                 uint8_t *out_species,
                                 uint8_t *out_level,
                                 uint8_t out_moves[4]) {
    char buf[160];
    char parts[6][32];
    int part_count = 0;
    int start = 0;
    int len;
    if (!slot_str || !out_species || !out_level || !out_moves) return 0;

    {
        char lower[24] = {0};
        int li = 0;
        while (*slot_str == ' ' || *slot_str == '\t') slot_str++;
        for (int i = 0; slot_str[i] && li < (int)sizeof(lower) - 1; i++)
            lower[li++] = (char)tolower((unsigned char)slot_str[i]);
        lower[li] = '\0';
        if (*slot_str == '\0' ||
            strcmp(slot_str, "-") == 0 ||
            strcmp(slot_str, "0") == 0 ||
            strcmp(lower, "empty") == 0 ||
            strcmp(lower, "none") == 0) {
            *out_species = 0;
            *out_level = 0;
            memset(out_moves, 0, 4);
            return 1;
        }
    }

    snprintf(buf, sizeof(buf), "%s", slot_str);
    len = (int)strlen(buf);
    for (int i = 0; i <= len && part_count < 6; i++) {
        if (buf[i] == ',' || buf[i] == '\0') {
            int n = i - start;
            while (n > 0 && (buf[start] == ' ' || buf[start] == '\t')) { start++; n--; }
            while (n > 0 && (buf[start + n - 1] == ' ' || buf[start + n - 1] == '\t')) n--;
            if (n <= 0) return 0;
            if (n >= (int)sizeof(parts[0])) n = (int)sizeof(parts[0]) - 1;
            memcpy(parts[part_count], buf + start, (size_t)n);
            parts[part_count][n] = '\0';
            part_count++;
            start = i + 1;
        }
    }
    if (part_count != 6) return 0;

    {
        int species = cli_resolve_species_id(parts[0]);
        int level = (int)strtol(parts[1], NULL, 0);
        if (species <= 0 || species > 255) return 0;
        if (level < 1 || level > 100) return 0;
        *out_species = (uint8_t)species;
        *out_level = (uint8_t)level;
        for (int i = 0; i < 4; i++) {
            int move = cli_resolve_move_id(parts[2 + i]);
            if (move < 0 || move > 255) return 0;
            out_moves[i] = (uint8_t)move;
        }
    }
    return 1;
}

static int cli_parse_arg(const char *src, int arg_index, char *out, size_t out_sz) {
    int idx = 0;
    const char *p = src;
    if (!src || !out || out_sz == 0) return 0;
    out[0] = '\0';
    while (*p) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        if (idx == arg_index) {
            size_t n = 0;
            char quote = 0;
            if (*p == '"' || *p == '\'') {
                quote = *p++;
                while (*p && *p != quote && n + 1 < out_sz) out[n++] = *p++;
                if (*p == quote) p++;
            } else {
                while (*p && *p != ' ' && *p != '\t' && n + 1 < out_sz) out[n++] = *p++;
            }
            out[n] = '\0';
            return n > 0;
        }
        if (*p == '"' || *p == '\'') {
            char q = *p++;
            while (*p && *p != q) p++;
            if (*p == q) p++;
        } else {
            while (*p && *p != ' ' && *p != '\t') p++;
        }
        idx++;
    }
    return 0;
}

static void cli_sanitize_key(const char *in, char *out, size_t out_sz) {
    size_t n = 0;
    if (!in || !out || out_sz == 0) return;
    for (size_t i = 0; in[i] && n + 1 < out_sz; i++) {
        char c = in[i];
        if ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '_' || c == '-') {
            out[n++] = c;
        } else if (c == ' ') {
            out[n++] = '_';
        }
    }
    out[n] = '\0';
}

static void cli_build_state_path(const char *key, char *path_out, size_t path_sz) {
    char clean[80] = {0};
    int all_digits = 1;
    if (!key || !*key) {
        snprintf(path_out, path_sz, "bugs/qs_slot_1.state");
        return;
    }
    for (int i = 0; key[i]; i++) {
        if (key[i] < '0' || key[i] > '9') { all_digits = 0; break; }
    }
    cli_sanitize_key(key, clean, sizeof(clean));
    if (clean[0] == '\0') strcpy(clean, "slot_1");
    if (all_digits) snprintf(path_out, path_sz, "bugs/qs_slot_%s.state", clean);
    else snprintf(path_out, path_sz, "bugs/qs_%s.state", clean);
}

static int cli_file_exists(const char *path) {
    struct stat st;
    return (path && stat(path, &st) == 0 && S_ISREG(st.st_mode));
}

static int cli_make_dir_if_needed(const char *path) {
    struct stat st;
    if (!path || !*path) return -1;
    if (stat(path, &st) == 0) return S_ISDIR(st.st_mode) ? 0 : -1;
#ifdef _WIN32
    return _mkdir(path);
#else
    return mkdir(path, 0777);
#endif
}

static int cli_copy_file(const char *src, const char *dst) {
    FILE *in = NULL, *out = NULL;
    char buf[4096];
    size_t n;
    in = fopen(src, "rb");
    if (!in) return -1;
    out = fopen(dst, "wb");
    if (!out) { fclose(in); return -1; }
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) { fclose(in); fclose(out); return -1; }
    }
    fclose(in);
    fclose(out);
    return 0;
}

static int cli_backup_state_if_exists(const char *state_path) {
    char backup_dir[192] = {0};
    char backup_file[256] = {0};
    char base[96] = {0};
    char ext[24] = {0};
    int next_idx = 1;
    const char *name = NULL;
    const char *dot = NULL;
    DIR *d = NULL;
    struct dirent *ent;

    if (!cli_file_exists(state_path)) return 0;

    name = strrchr(state_path, '/');
    name = name ? name + 1 : state_path;
    dot = strrchr(name, '.');
    if (dot) {
        size_t blen = (size_t)(dot - name);
        if (blen >= sizeof(base)) blen = sizeof(base) - 1;
        memcpy(base, name, blen);
        base[blen] = '\0';
        snprintf(ext, sizeof(ext), "%s", dot);
    } else {
        snprintf(base, sizeof(base), "%s", name);
        snprintf(ext, sizeof(ext), ".state");
    }

    snprintf(backup_dir, sizeof(backup_dir), "bugs/%s_backup", base);
    if (cli_make_dir_if_needed(backup_dir) != 0) return -1;

    d = opendir(backup_dir);
    if (d) {
        while ((ent = readdir(d)) != NULL) {
            int idx = -1;
            char pat[128] = {0};
            snprintf(pat, sizeof(pat), "%s_%%d%s", base, ext);
            if (sscanf(ent->d_name, pat, &idx) == 1 && idx >= next_idx) {
                next_idx = idx + 1;
            }
        }
        closedir(d);
    }

    snprintf(backup_file, sizeof(backup_file), "%s/%s_%03d%s", backup_dir, base, next_idx, ext);
    return cli_copy_file(state_path, backup_file);
}

static void cli_reload_after_state_load(void) {
    Map_Load(wCurMap);
    NPC_Load();
    PalletScripts_OnMapLoad();
    OaksLabScripts_OnMapLoad();
    ViridianMartScripts_OnMapLoad();
    MtMoon_OnMapLoad();
    Route22Scripts_OnMapLoad();
    CeruleanScripts_OnMapLoad();
    Route24Scripts_OnMapLoad();
    BillsHouseScripts_OnMapLoad();
    Route2GateScripts_OnMapLoad();
    SSAnneScripts_OnMapLoad();
    VermilionGymScripts_OnMapLoad();
    RocketHideoutB4FScripts_OnMapLoad();
    RocketHideoutScripts_OnMapLoad();
    GameCornerScripts_OnMapLoad();
    PokemonTower2FScripts_OnMapLoad();
    PokemonTower5FScripts_OnMapLoad();
    PokemonTower6FScripts_OnMapLoad();
    PokemonTower7FScripts_OnMapLoad();
    MrFujisHouseScripts_OnMapLoad();
    SaffronCityScripts_OnMapLoad();
    Trainer_LoadMap();
    if (Save_StateWasBattle()) {
        BattleUI_Restore();
    }
}

static int cli_make_dir_tree(const char *path) {
    char tmp[256];
    size_t len;
    if (!path || !*path) return -1;
    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    for (size_t i = 1; i < len; i++) {
        if (tmp[i] == '/' || tmp[i] == '\\') {
            char save = tmp[i];
            tmp[i] = '\0';
            if (*tmp) cli_make_dir_if_needed(tmp);
            tmp[i] = save;
        }
    }
    return cli_make_dir_if_needed(tmp);
}

static void cli_build_replay_path(const char *name, char *out_path, size_t out_sz) {
    char clean[96] = {0};
    cli_sanitize_key(name, clean, sizeof(clean));
    if (clean[0] == '\0') snprintf(clean, sizeof(clean), "replay");
    snprintf(out_path, out_sz, "%s/%s.rpl", REPLAY_DIR, clean);
}

static void replay_reset_playback(void) {
    if (s_replay_play_buf) {
        free(s_replay_play_buf);
        s_replay_play_buf = NULL;
    }
    s_replay_playing = 0;
    s_replay_play_pos = 0;
    s_replay_play_len = 0;
}

static void replay_reset_recording(void) {
    if (s_replay_rec_fp) {
        fclose(s_replay_rec_fp);
        s_replay_rec_fp = NULL;
    }
    s_replay_recording = 0;
    s_replay_name[0] = '\0';
    s_replay_tmp_state[0] = '\0';
    s_replay_tmp_input[0] = '\0';
}

static void cli_script_trace_reset_latches(void) {
    s_trace_prev_map = 0xFF;
    s_trace_prev_trainer_engaging = 0xFF;
    s_trace_prev_text_open = 0xFF;
    s_trace_prev_gate_active = 0xFF;
    s_trace_prev_route22_active = 0xFF;
    s_trace_prev_route24_active = 0xFF;
    s_trace_prev_ssanne_active = 0xFF;
    s_trace_prev_viridian_mart_active = 0xFF;
    s_trace_prev_gym_active = 0xFF;
    s_trace_prev_rockethideout_b4f_active = 0xFF;
}

static void cli_script_trace_log_line(const char *line) {
    FILE *fp;
    struct stat st;
    if (!line || !*line) return;
    if (!s_script_trace_to_file) return;

    if (stat(SCRIPT_TRACE_LOG_PATH, &st) == 0 && st.st_size >= SCRIPT_TRACE_MAX_BYTES) {
        remove(SCRIPT_TRACE_LOG_PATH_OLD);
        rename(SCRIPT_TRACE_LOG_PATH, SCRIPT_TRACE_LOG_PATH_OLD);
    }

    fp = fopen(SCRIPT_TRACE_LOG_PATH, "a");
    if (!fp) return;
    fprintf(fp, "%s\n", line);
    fclose(fp);
}

static void cli_script_trace_emitf(const char *fmt, ...) {
    va_list ap;
    char buf[192];
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    printf("%s\n", buf);
    cli_script_trace_log_line(buf);
}

static int replay_start_record(const char *name) {
    char clean[96] = {0};
    if (!name || !*name) return -1;
    cli_sanitize_key(name, clean, sizeof(clean));
    if (clean[0] == '\0') return -1;

    cli_make_dir_tree(REPLAY_DIR);
    replay_reset_playback();
    replay_reset_recording();

    snprintf(s_replay_name, sizeof(s_replay_name), "%s", clean);
    snprintf(s_replay_tmp_state, sizeof(s_replay_tmp_state), "%s/%s.tmp.state", REPLAY_DIR, clean);
    snprintf(s_replay_tmp_input, sizeof(s_replay_tmp_input), "%s/%s.tmp.inputs", REPLAY_DIR, clean);

    if (Save_StateWrite(s_replay_tmp_state) != 0) return -1;
    s_replay_rec_fp = fopen(s_replay_tmp_input, "wb");
    if (!s_replay_rec_fp) {
        remove(s_replay_tmp_state);
        return -1;
    }
    s_replay_recording = 1;
    return 0;
}

static int replay_stop_record(char *out_final_path, size_t out_sz) {
    FILE *state = NULL, *in = NULL, *out = NULL;
    long state_sz_l = 0, input_sz_l = 0;
    size_t state_sz, input_sz;
    replay_header_t hdr;
    char final_path[192] = {0};
    char copy_buf[4096];
    size_t n;

    if (!s_replay_recording || !s_replay_rec_fp) return -1;
    fclose(s_replay_rec_fp);
    s_replay_rec_fp = NULL;
    s_replay_recording = 0;

    cli_build_replay_path(s_replay_name, final_path, sizeof(final_path));
    if (cli_backup_state_if_exists(final_path) != 0) {
        /* best effort backup */
    }

    state = fopen(s_replay_tmp_state, "rb");
    in = fopen(s_replay_tmp_input, "rb");
    out = fopen(final_path, "wb");
    if (!state || !in || !out) goto fail;

    if (fseek(state, 0, SEEK_END) != 0) goto fail;
    state_sz_l = ftell(state);
    if (state_sz_l < 0) goto fail;
    rewind(state);

    if (fseek(in, 0, SEEK_END) != 0) goto fail;
    input_sz_l = ftell(in);
    if (input_sz_l < 0) goto fail;
    rewind(in);

    state_sz = (size_t)state_sz_l;
    input_sz = (size_t)input_sz_l;

    hdr.magic = REPLAY_MAGIC;
    hdr.version = REPLAY_VERSION;
    hdr.state_size = (uint32_t)state_sz;
    hdr.input_frames = (uint32_t)input_sz;
    if (fwrite(&hdr, 1, sizeof(hdr), out) != sizeof(hdr)) goto fail;

    while ((n = fread(copy_buf, 1, sizeof(copy_buf), state)) > 0) {
        if (fwrite(copy_buf, 1, n, out) != n) goto fail;
    }
    while ((n = fread(copy_buf, 1, sizeof(copy_buf), in)) > 0) {
        if (fwrite(copy_buf, 1, n, out) != n) goto fail;
    }

    fclose(state); fclose(in); fclose(out);
    remove(s_replay_tmp_state);
    remove(s_replay_tmp_input);
    if (out_final_path && out_sz > 0) snprintf(out_final_path, out_sz, "%s", final_path);
    s_replay_name[0] = '\0';
    return 0;

fail:
    if (state) fclose(state);
    if (in) fclose(in);
    if (out) fclose(out);
    return -1;
}

static int replay_start_play(const char *name) {
    FILE *fp = NULL;
    replay_header_t hdr;
    char path[192] = {0};
    char tmp_state[192] = {0};
    uint8_t *state_buf = NULL;

    if (!name || !*name) return -1;
    cli_build_replay_path(name, path, sizeof(path));

    fp = fopen(path, "rb");
    if (!fp) return -1;
    if (fread(&hdr, 1, sizeof(hdr), fp) != sizeof(hdr)) { fclose(fp); return -1; }
    if (hdr.magic != REPLAY_MAGIC || hdr.version != REPLAY_VERSION) { fclose(fp); return -1; }
    if (hdr.state_size == 0) { fclose(fp); return -1; }

    state_buf = (uint8_t *)malloc(hdr.state_size);
    if (!state_buf) { fclose(fp); return -1; }
    if (fread(state_buf, 1, hdr.state_size, fp) != hdr.state_size) {
        free(state_buf); fclose(fp); return -1;
    }

    replay_reset_playback();
    s_replay_play_buf = NULL;
    if (hdr.input_frames > 0) {
        s_replay_play_buf = (uint8_t *)malloc(hdr.input_frames);
        if (!s_replay_play_buf) { free(state_buf); fclose(fp); return -1; }
        if (fread(s_replay_play_buf, 1, hdr.input_frames, fp) != hdr.input_frames) {
            free(state_buf); replay_reset_playback(); fclose(fp); return -1;
        }
    }
    fclose(fp);

    snprintf(tmp_state, sizeof(tmp_state), "%s/.replay_load_%s.state", REPLAY_DIR, name);
    {
        FILE *sf = fopen(tmp_state, "wb");
        if (!sf) { free(state_buf); replay_reset_playback(); return -1; }
        if (fwrite(state_buf, 1, hdr.state_size, sf) != hdr.state_size) {
            fclose(sf); free(state_buf); replay_reset_playback(); return -1;
        }
        fclose(sf);
    }
    free(state_buf);

    if (Save_StateLoad(tmp_state) != 0) {
        remove(tmp_state);
        replay_reset_playback();
        return -1;
    }
    remove(tmp_state);
    cli_reload_after_state_load();

    s_replay_playing = 1;
    s_replay_play_pos = 0;
    s_replay_play_len = hdr.input_frames;
    return 0;
}

typedef struct cli_eventdiff_snapshot_t {
    int valid;
    uint8_t badges;
    uint8_t map;
    uint8_t x;
    uint8_t y;
    uint8_t party_count;
    uint8_t key_flags[15];
} cli_eventdiff_snapshot_t;

static cli_eventdiff_snapshot_t s_eventdiff = {0};

static const uint16_t s_eventdiff_keys[15] = {
    EVENT_GOT_POKEDEX,
    EVENT_BEAT_BROCK,
    EVENT_BEAT_MISTY,
    EVENT_BEAT_LT_SURGE,
    EVENT_BEAT_ERIKA,
    EVENT_BEAT_KOGA,
    EVENT_BEAT_BLAINE,
    EVENT_BEAT_SABRINA,
    EVENT_GOT_HM01,
    EVENT_GOT_TM34,
    EVENT_GOT_TM11,
    EVENT_GOT_TM24,
    EVENT_GOT_TM21,
    EVENT_GOT_TM06,
    EVENT_GOT_TM38
};

static void cli_gym_clear(const char *name) {
    if (strcmp(name, "brock") == 0) {
        ClearEvent(EVENT_BEAT_BROCK);
        ClearEvent(EVENT_GOT_TM34);
        ClearEvent(EVENT_BEAT_PEWTER_GYM_TRAINER_0);
        wObtainedBadges &= (uint8_t)~(1u << BADGE_BOULDER);
    } else if (strcmp(name, "misty") == 0) {
        ClearEvent(EVENT_BEAT_MISTY);
        ClearEvent(EVENT_GOT_TM11);
        ClearEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_0);
        ClearEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_1);
        wObtainedBadges &= (uint8_t)~(1u << BADGE_CASCADE);
    } else if (strcmp(name, "surge") == 0) {
        ClearEvent(EVENT_BEAT_LT_SURGE);
        ClearEvent(EVENT_GOT_TM24);
        ClearEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_0);
        ClearEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_1);
        ClearEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_2);
        wObtainedBadges &= (uint8_t)~(1u << BADGE_THUNDER);
    } else if (strcmp(name, "erika") == 0) {
        ClearEvent(EVENT_BEAT_ERIKA);
        ClearEvent(EVENT_GOT_TM21);
        ClearEvent(EVENT_BEAT_CELADON_GYM_TRAINER_0);
        ClearEvent(EVENT_BEAT_CELADON_GYM_TRAINER_1);
        ClearEvent(EVENT_BEAT_CELADON_GYM_TRAINER_2);
        ClearEvent(EVENT_BEAT_CELADON_GYM_TRAINER_3);
        ClearEvent(EVENT_BEAT_CELADON_GYM_TRAINER_4);
        ClearEvent(EVENT_BEAT_CELADON_GYM_TRAINER_5);
        ClearEvent(EVENT_BEAT_CELADON_GYM_TRAINER_6);
        wObtainedBadges &= (uint8_t)~(1u << BADGE_RAINBOW);
    } else if (strcmp(name, "koga") == 0) {
        ClearEvent(EVENT_BEAT_KOGA);
        ClearEvent(EVENT_GOT_TM06);
        ClearEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_0);
        ClearEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_1);
        ClearEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_2);
        ClearEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_3);
        ClearEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_4);
        ClearEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_5);
        wObtainedBadges &= (uint8_t)~(1u << BADGE_SOUL);
    } else if (strcmp(name, "blaine") == 0) {
        ClearEvent(EVENT_BEAT_BLAINE);
        ClearEvent(EVENT_GOT_TM38);
        ClearEvent(EVENT_BEAT_CINNABAR_GYM_TRAINER_0);
        ClearEvent(EVENT_BEAT_CINNABAR_GYM_TRAINER_1);
        ClearEvent(EVENT_BEAT_CINNABAR_GYM_TRAINER_2);
        ClearEvent(EVENT_BEAT_CINNABAR_GYM_TRAINER_3);
        ClearEvent(EVENT_BEAT_CINNABAR_GYM_TRAINER_4);
        ClearEvent(EVENT_BEAT_CINNABAR_GYM_TRAINER_5);
        ClearEvent(EVENT_BEAT_CINNABAR_GYM_TRAINER_6);
        wObtainedBadges &= (uint8_t)~(1u << BADGE_VOLCANO);
    }
}

static int cli_is_numeric_token(const char *s) {
    if (!s || !*s) return 0;
    if (s[0] == '+' || s[0] == '-') s++;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
        if (!*s) return 0;
        while (*s) {
            if (!((*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'f') || (*s >= 'A' && *s <= 'F')))
                return 0;
            s++;
        }
        return 1;
    }
    while (*s) {
        if (*s < '0' || *s > '9') return 0;
        s++;
    }
    return 1;
}

static void cli_norm(char *dst, size_t dst_sz, const char *src) {
    size_t n = 0;
    if (!dst || dst_sz == 0) return;
    dst[0] = '\0';
    if (!src) return;
    for (size_t i = 0; src[i] && n + 1 < dst_sz; i++) {
        char c = (char)tolower((unsigned char)src[i]);
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) dst[n++] = c;
    }
    dst[n] = '\0';
}

static char *cli_trim(char *s) {
    char *e;
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') s++;
    e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n')) e--;
    *e = '\0';
    return s;
}

/* Normalize common UTF-8 punctuation into ASCII so scene scripts authored
 * from rich editors don't break parser/text behavior. */
static void scene_normalize_ascii(char *s) {
    unsigned char *buf = (unsigned char*)s;
    int ri = 0;
    int wi = 0;
    if (!s) return;
    while (buf[ri]) {
        if (buf[ri] < 0x80) {
            buf[wi++] = buf[ri++];
            continue;
        }
        if (buf[ri] == 0xE2 && buf[ri + 1] == 0x80 && buf[ri + 2]) {
            unsigned char t = buf[ri + 2];
            if (t == 0x98 || t == 0x99) buf[wi++] = '\'';      /* ‘ ’ */
            else if (t == 0x9C || t == 0x9D) buf[wi++] = '"';  /* “ ” */
            else if (t == 0x93 || t == 0x94) buf[wi++] = '-';  /* – — */
            ri += 3;
            continue;
        }
        ri++;
    }
    buf[wi] = '\0';
}

static int scene_parse_dir(const char *tok) {
    if (!tok) return -1;
    if (strcmp(tok, "down") == 0) return 0;
    if (strcmp(tok, "up") == 0) return 1;
    if (strcmp(tok, "left") == 0) return 2;
    if (strcmp(tok, "right") == 0) return 3;
    return -1;
}

static int cli_resolve_trainer_class_id(const char *tok) {
    char want[40];
    if (!tok || !*tok) return 0;
    if (cli_is_numeric_token(tok)) {
        int v = (int)strtol(tok, NULL, 0);
        if (v >= 1 && v <= NUM_TRAINERS) return v;
        return 0;
    }
    cli_norm(want, sizeof(want), tok);
    for (int i = 0; i < NUM_TRAINERS; i++) {
        char norm[40];
        cli_norm(norm, sizeof(norm), gTrainerClassNames[i]);
        if (strcmp(want, norm) == 0) return i + 1;
    }
    return 0;
}

static uint8_t scene_trainer_class_to_overworld_sprite(int trainer_class) {
    switch (trainer_class) {
        case 34: return 0x0C; /* BROCK */
        case 35: return 0x06; /* MISTY */
        case 36: return 0x07; /* LT_SURGE */
        case 37: return 0x0D; /* ERIKA */
        case 38: return 0x18; /* KOGA proxy */
        case 39: return 0x0B; /* BLAINE proxy */
        case 40: return 0x06; /* SABRINA proxy */
        case 44: return 0x3B; /* LORELEI */
        case 45: return 0x3A; /* BRUNO */
        case 46: return 0x39; /* AGATHA */
        case 47: return 0x1E; /* LANCE */
        default: return 0x04; /* generic */
    }
}

static int scene_parse_sprite(const char *tok) {
    static const struct { const char *name; int id; } k[] = {
        { "YOUNGSTER", 0x04 }, { "GAMBLER", 0x0B }, { "SUPER_NERD", 0x0C },
        { "GIRL", 0x0D }, { "COOLTRAINER_M", 0x07 }, { "COOLTRAINER_F", 0x06 },
        { "ROCKET", 0x18 }, { "GUARD", 0x31 }, { NULL, 0 }
    };
    if (!tok || !*tok) return -1;
    for (int i = 0; k[i].name; i++) if (strcmp(tok, k[i].name) == 0) return k[i].id;
    {
        int tc = cli_resolve_trainer_class_id(tok);
        if (tc > 0) return scene_trainer_class_to_overworld_sprite(tc);
    }
    if (cli_is_numeric_token(tok)) return (int)strtol(tok, NULL, 0);
    return -1;
}

static int scene_parse_music_track(const char *tok) {
    char norm[48];
    if (!tok || !*tok) return -1;
    cli_norm(norm, sizeof(norm), tok);
    if (strcmp(norm, "wildbattle") == 0 || strcmp(norm, "wild") == 0)
        return MUSIC_WILD_BATTLE;
    if (strcmp(norm, "trainerbattle") == 0 || strcmp(norm, "trainer") == 0)
        return MUSIC_TRAINER_BATTLE;
    if (strcmp(norm, "gymleader") == 0 || strcmp(norm, "gymleaderbattle") == 0 ||
        strcmp(norm, "elitefour") == 0 || strcmp(norm, "elite4") == 0 ||
        strcmp(norm, "elite_4") == 0 || strcmp(norm, "gymelitefour") == 0)
        return MUSIC_GYM_LEADER_BATTLE;
    if (strcmp(norm, "champion") == 0 || strcmp(norm, "championbattle") == 0)
        return MUSIC_GYM_LEADER_BATTLE; /* Gen1 parity bucket: boss battle theme */
    return -1;
}

static int scene_defs_find(const char *name) {
    for (int i = 0; i < SCENE_DEF_MAX; i++) {
        if (s_scene_defs[i].used && strcmp(s_scene_defs[i].name, name) == 0) return i;
    }
    return -1;
}

static int scene_defs_add(const char *name) {
    int i = scene_defs_find(name);
    if (i >= 0) return i;
    for (i = 0; i < SCENE_DEF_MAX; i++) {
        if (!s_scene_defs[i].used) {
            s_scene_defs[i].used = 1;
            s_scene_defs[i].line_count = 0;
            snprintf(s_scene_defs[i].name, sizeof(s_scene_defs[i].name), "%s", name);
            return i;
        }
    }
    return -1;
}

static int scene_is_line_command_start(const char *s) {
    if (!s || !*s) return 0;
    if (strncmp(s, "spawn ", 6) == 0) return 1;
    if (strncmp(s, "despawn ", 8) == 0) return 1;
    if (strncmp(s, "face ", 5) == 0) return 1;
    if (strncmp(s, "move ", 5) == 0) return 1;
    if (strncmp(s, "say ", 4) == 0) return 1;
    if (strncmp(s, "ask ", 4) == 0) return 1;
    if (strncmp(s, "battlestart", 11) == 0) return 1;
    if (strncmp(s, "battlend", 8) == 0) return 1;
    if (strncmp(s, "music ", 6) == 0) return 1;
    if (strncmp(s, "wait ", 5) == 0) return 1;
    if (strcmp(s, "wait_text") == 0) return 1;
    if (strcmp(s, "lock_input on") == 0) return 1;
    if (strcmp(s, "lock_input off") == 0) return 1;
    if (strcmp(s, "end") == 0) return 1;
    if (strncmp(s, "use ", 4) == 0) return 1;
    if (strncmp(s, "include ", 8) == 0) return 1;
    if (strncmp(s, "def ", 4) == 0) return 1;
    if (strcmp(s, "enddef") == 0) return 1;
    return 0;
}

static void scene_apply_args(const char *src, char *dst, int dst_sz, char args[16][96]) {
    int di = 0;
    for (int si = 0; src[si] && di + 1 < dst_sz; si++) {
        /* Parse 2-digit placeholders first so $10..$16 are not consumed as $1. */
        if (src[si] == '$' && src[si + 1] == '1' && src[si + 2] >= '0' && src[si + 2] <= '6') {
            int ai = 9 + (src[si + 2] - '0'); /* $10..$16 */
            const char *a = args[ai];
            for (int k = 0; a[k] && di + 1 < dst_sz; k++) dst[di++] = a[k];
            si += 2;
        } else if (src[si] == '$' && src[si + 1] >= '1' && src[si + 1] <= '9') {
            int ai = src[si + 1] - '1';
            const char *a = args[ai];
            for (int k = 0; a[k] && di + 1 < dst_sz; k++) dst[di++] = a[k];
            si++;
        } else {
            dst[di++] = src[si];
        }
    }
    dst[di] = '\0';
}

/* Split arg string into up to max_out tokens, honoring quoted segments. */
static int scene_split_args_quoted(const char *s, char out[][96], int max_out) {
    int count = 0;
    int i = 0;
    while (s && s[i] && count < max_out) {
        int oi = 0;
        while (s[i] == ' ' || s[i] == '\t') i++;
        if (!s[i]) break;
        if (s[i] == '"') {
            i++;
            while (s[i] && s[i] != '"' && oi + 1 < 96) out[count][oi++] = s[i++];
            if (s[i] == '"') i++;
        } else {
            while (s[i] && s[i] != ' ' && s[i] != '\t' && oi + 1 < 96) out[count][oi++] = s[i++];
        }
        out[count][oi] = '\0';
        count++;
    }
    return count;
}

static void scene_unescape_text(char *s) {
    char out[160];
    int oi = 0;
    for (int i = 0; s[i] && oi + 1 < (int)sizeof(out); i++) {
        if (s[i] == '\\' && s[i + 1]) {
            char n = s[i + 1];
            if (n == 'n') { out[oi++] = '\n'; i++; continue; }
            if (n == 'f') { out[oi++] = '\f'; i++; continue; }
            if (n == 't') { out[oi++] = '\t'; i++; continue; }
            if (n == '\\') { out[oi++] = '\\'; i++; continue; }
            if (n == '"') { out[oi++] = '"'; i++; continue; }
        }
        out[oi++] = s[i];
    }
    out[oi] = '\0';
    snprintf(s, 160, "%s", out);
}

/* Auto-format plain scene dialogue into textbox-friendly flow:
 * - wraps at 18 chars per line
 * - max 2 lines per page (then inserts '\f')
 * - avoids mid-word wrapping unless a single word exceeds line width
 * - preserves explicit control chars: '\n', '\f', '@'
 * - appends '@' terminator if missing
 */
static void scene_format_dialog_text(char *s) {
    enum { MAX_COLS = 18, MAX_LINES = 2 };
    char out[160];
    int oi = 0;
    int i = 0;
    int col = 0;
    int line = 0; /* 0..MAX_LINES-1 */
    int ended = 0;

    while (s[i] && oi + 1 < (int)sizeof(out)) {
        char c = s[i];

        if (c == '@') {
            out[oi++] = '@';
            ended = 1;
            break;
        }
        if (c == '\f') {
            out[oi++] = '\f';
            col = 0;
            line = 0;
            i++;
            continue;
        }
        if (c == '\n') {
            out[oi++] = '\n';
            col = 0;
            line++;
            if (line >= MAX_LINES) line = MAX_LINES - 1;
            i++;
            continue;
        }

        if (c == ' ' || c == '\t' || c == '\r') {
            i++;
            continue; /* normalize whitespace between words */
        }

        /* Parse one word token (until whitespace/control/terminator). */
        {
            int ws = i;
            int we = i;
            int wlen;
            while (s[we] && s[we] != '@' && s[we] != '\n' && s[we] != '\f' &&
                   s[we] != ' ' && s[we] != '\t' && s[we] != '\r') {
                we++;
            }
            wlen = we - ws;
            if (wlen <= 0) {
                i = we;
                continue;
            }

            if (wlen > MAX_COLS) {
                int p = ws;
                if (col > 0) {
                    if (line == 0) { out[oi++] = '\n'; line = 1; }
                    else { out[oi++] = '\f'; line = 0; }
                    col = 0;
                }
                while (p < we && oi + 1 < (int)sizeof(out)) {
                    int chunk = we - p;
                    if (chunk > MAX_COLS) chunk = MAX_COLS;
                    for (int k = 0; k < chunk && oi + 1 < (int)sizeof(out); k++)
                        out[oi++] = s[p + k];
                    p += chunk;
                    col = chunk;
                    if (p < we && oi + 1 < (int)sizeof(out)) {
                        if (line == 0) { out[oi++] = '\n'; line = 1; }
                        else { out[oi++] = '\f'; line = 0; }
                        col = 0;
                    }
                }
                i = we;
                continue;
            }

            if (col == 0) {
                for (int k = 0; k < wlen && oi + 1 < (int)sizeof(out); k++)
                    out[oi++] = s[ws + k];
                col = wlen;
            } else if (col + 1 + wlen <= MAX_COLS) {
                out[oi++] = ' ';
                for (int k = 0; k < wlen && oi + 1 < (int)sizeof(out); k++)
                    out[oi++] = s[ws + k];
                col += 1 + wlen;
            } else {
                if (line == 0) { out[oi++] = '\n'; line = 1; }
                else { out[oi++] = '\f'; line = 0; }
                col = 0;
                for (int k = 0; k < wlen && oi + 1 < (int)sizeof(out); k++)
                    out[oi++] = s[ws + k];
                col = wlen;
            }

            i = we;
        }
    }

    if (!ended && oi + 1 < (int)sizeof(out))
        out[oi++] = '@';
    out[oi] = '\0';
    snprintf(s, 160, "%s", out);
}

static void scene_reset_runtime(void) {
    s_scene_active = 0;
    s_scene_cmd_count = 0;
    s_scene_pc = 0;
    s_scene_wait = 0;
    s_scene_wait_yesno = 0;
    s_scene_wait_say = 0;
    s_scene_say_opened = 0;
    s_scene_wait_battle = 0;
    s_scene_wait_battleend_text = 0;
    s_scene_battle_started = 0;
    s_scene_battlestart_pending = 0;
    s_scene_battlestart_saw_text = 0;
    s_scene_battlestart_delay = 0;
    s_scene_battlestart_tc = 0;
    s_scene_battlestart_tn = 0;
    s_scene_last_yesno = -1;
    s_scene_yesno_prev_joyignore = 0;
    s_scene_yesno_restore_joyignore = 0;
    s_scene_yesno_prev_scripted_movement = 0;
    s_scene_yesno_restore_scripted_movement = 0;
    s_scene_move_steps_left = 0;
    s_scene_move_dir = 0;
    s_scene_move_actor = -1;
    memset(s_scene_actors, 0, sizeof(s_scene_actors));
}

static int scene_find_actor(const char *name) {
    for (int i = 0; i < SCENE_ACTOR_MAX; i++) {
        if (s_scene_actors[i].used && strcmp(s_scene_actors[i].name, name) == 0)
            return i;
    }
    return -1;
}

static int scene_add_actor(const char *name, int npc_idx) {
    int i = scene_find_actor(name);
    if (i >= 0) { s_scene_actors[i].npc_idx = npc_idx; return i; }
    for (i = 0; i < SCENE_ACTOR_MAX; i++) {
        if (!s_scene_actors[i].used) {
            s_scene_actors[i].used = 1;
            snprintf(s_scene_actors[i].name, sizeof(s_scene_actors[i].name), "%s", name);
            s_scene_actors[i].npc_idx = npc_idx;
            return i;
        }
    }
    return -1;
}

static int scene_npc_binding_find_by_idx(int npc_idx) {
    for (int i = 0; i < SCENE_ACTOR_MAX; i++) {
        if (!s_scene_npc_bindings[i].used) continue;
        if (s_scene_npc_bindings[i].npc_idx != npc_idx) continue;
        if (s_scene_npc_bindings[i].map_id != wCurMap) continue;
        return i;
    }
    return -1;
}

static int scene_npc_binding_alloc(void) {
    for (int i = 0; i < SCENE_ACTOR_MAX; i++)
        if (!s_scene_npc_bindings[i].used) return i;
    return -1;
}

static const char *kDslBankDataPaths[] = {
    "debug/dsl_bank_scene_npc.dat",
    "../debug/dsl_bank_scene_npc.dat",
    "build/debug/dsl_bank_scene_npc.dat"
};
static const char *kDslBankCfgPaths[] = {
    "debug/dsl_bank_scene_npc.cfg",
    "../debug/dsl_bank_scene_npc.cfg",
    "build/debug/dsl_bank_scene_npc.cfg"
};

static void dsl_bank_save(void) {
    for (int pi = 0; pi < (int)(sizeof(kDslBankDataPaths) / sizeof(kDslBankDataPaths[0])); pi++) {
        FILE *fp = fopen(kDslBankDataPaths[pi], "w");
        if (!fp) continue;
        fprintf(fp, "DSLBANK1\n");
        for (int i = 0; i < SCENE_ACTOR_MAX; i++) {
            scene_npc_binding_t *b = &s_scene_npc_bindings[i];
            if (!b->used) continue;
            fprintf(fp, "%u|%u|%d|%d|%s|%s\n",
                    (unsigned)b->map_id, (unsigned)b->sprite_id, b->tile_x, b->tile_y,
                    b->name, b->scene);
        }
        fclose(fp);
    }
}

static void dsl_bank_write_cfg(void) {
    for (int pi = 0; pi < (int)(sizeof(kDslBankCfgPaths) / sizeof(kDslBankCfgPaths[0])); pi++) {
        FILE *fp = fopen(kDslBankCfgPaths[pi], "w");
        if (!fp) continue;
        fprintf(fp, "%d\n", s_dsl_bank_enabled ? 1 : 0);
        fclose(fp);
    }
}

static void dsl_bank_load(void) {
    FILE *fp = NULL;
    char line[256];
    for (int pi = 0; pi < (int)(sizeof(kDslBankDataPaths) / sizeof(kDslBankDataPaths[0])); pi++) {
        fp = fopen(kDslBankDataPaths[pi], "r");
        if (fp) break;
    }
    if (!fp) return;
    memset(s_scene_npc_bindings, 0, sizeof(s_scene_npc_bindings));
    if (!fgets(line, sizeof(line), fp)) { fclose(fp); return; }
    while (fgets(line, sizeof(line), fp)) {
        unsigned map_id = 0, sprite_id = 0;
        int tx = 0, ty = 0;
        char name[24] = {0}, scene[64] = {0};
        int slot = scene_npc_binding_alloc();
        if (slot < 0) break;
        if (sscanf(line, "%u|%u|%d|%d|%23[^|]|%63[^\n]", &map_id, &sprite_id, &tx, &ty, name, scene) != 6)
            continue;
        s_scene_npc_bindings[slot].used = 1;
        s_scene_npc_bindings[slot].npc_idx = -1;
        s_scene_npc_bindings[slot].map_id = (uint8_t)map_id;
        s_scene_npc_bindings[slot].sprite_id = (uint8_t)sprite_id;
        s_scene_npc_bindings[slot].tile_x = tx;
        s_scene_npc_bindings[slot].tile_y = ty;
        snprintf(s_scene_npc_bindings[slot].name, sizeof(s_scene_npc_bindings[slot].name), "%s", name);
        snprintf(s_scene_npc_bindings[slot].scene, sizeof(s_scene_npc_bindings[slot].scene), "%s", scene);
    }
    fclose(fp);
}

static void dsl_bank_ensure_current_map_spawns(void) {
    for (int i = 0; i < SCENE_ACTOR_MAX; i++) {
        scene_npc_binding_t *b = &s_scene_npc_bindings[i];
        int idx;
        if (!b->used) continue;
        if (b->map_id != wCurMap) continue;
        idx = NPC_FindAtTile(b->tile_x, b->tile_y);
        if (idx < 0) idx = NPC_DebugSpawn(b->sprite_id, b->tile_x, b->tile_y, 0, 0);
        b->npc_idx = idx;
    }
}

static void dsl_bank_init_if_needed(void) {
    FILE *fp;
    if (s_dsl_bank_init_done) return;
    s_dsl_bank_init_done = 1;
    fp = NULL;
    for (int pi = 0; pi < (int)(sizeof(kDslBankCfgPaths) / sizeof(kDslBankCfgPaths[0])); pi++) {
        fp = fopen(kDslBankCfgPaths[pi], "r");
        if (fp) break;
    }
    if (!fp) return;
    {
        int enabled = 0;
        if (fscanf(fp, "%d", &enabled) == 1) s_dsl_bank_enabled = enabled ? 1 : 0;
    }
    fclose(fp);
    if (s_dsl_bank_enabled) {
        dsl_bank_load();
        dsl_bank_ensure_current_map_spawns();
        s_dsl_bank_last_map = wCurMap;
    }
}

static void scene_track_actor_positions(void) {
    int tx, ty;
    for (int i = 0; i < SCENE_ACTOR_MAX; i++) {
        if (!s_scene_actors[i].used) continue;
        if (s_scene_actors[i].npc_idx < 0 || s_scene_actors[i].npc_idx >= NPC_GetCount()) continue;
        NPC_GetTilePos(s_scene_actors[i].npc_idx, &tx, &ty);
        s_scene_actors[i].last_x = tx;
        s_scene_actors[i].last_y = ty;
    }
}

static void scene_restore_spawned_actors_after_battle(void) {
    int restored_any = 0;
    for (int i = 0; i < SCENE_ACTOR_MAX; i++) {
        int idx;
        if (!s_scene_actors[i].used) continue;
        if (!s_scene_actors[i].spawned_by_scene) continue;
        idx = NPC_DebugSpawn(s_scene_actors[i].sprite_id,
                             s_scene_actors[i].last_x,
                             s_scene_actors[i].last_y,
                             0, 0);
        s_scene_actors[i].npc_idx = idx;
        if (idx >= 0) restored_any = 1;
    }
    if (restored_any) {
        /* Ensure restored actors are visible immediately, even if text is open. */
        NPC_BuildView(gScrollPxX, gScrollPxY);
    }
}

static void scene_start_trainer_battle(int trainer_class, int trainer_no, const char *defeat_text) {
    extern void Game_StartTrainerBattleScripted(uint8_t trainer_class, uint8_t trainer_no);
    /* Never carry debug scene input locks into battle UI. */
    wJoyIgnore = 0;
    gScriptedMovement = 0;
    gEngagedTrainerClass = (uint8_t)trainer_class;
    gEngagedTrainerNo = (uint8_t)trainer_no;
    gTrainerAfterText = (defeat_text && *defeat_text) ? defeat_text : NULL;
    Game_StartTrainerBattleScripted((uint8_t)trainer_class, (uint8_t)trainer_no);
}

static void scene_start_custom_trainer_battle(const scene_cmd_t *cmd) {
    extern void Game_StartCustomTrainerBattleScripted(uint8_t trainer_class,
                                                      uint8_t music_id,
                                                      const uint8_t species[6],
                                                      const uint8_t level[6],
                                                      const uint8_t moves[6][4],
                                                      uint8_t count);
    if (!cmd) return;
    wJoyIgnore = 0;
    gScriptedMovement = 0;
    gTrainerAfterText = (cmd->text[0] != '\0') ? cmd->text : NULL;
    Game_StartCustomTrainerBattleScripted((uint8_t)cmd->a, (uint8_t)cmd->b, cmd->team_species, cmd->team_level, cmd->team_moves, cmd->team_count);
}

static int scene_pick_random_map_trainer(int *out_class, int *out_no) {
    const map_events_t *ev;
    int idx;
    if (!out_class || !out_no) return 0;
    if (wCurMap >= NUM_MAPS) return 0;
    ev = &gMapEvents[wCurMap];
    if (!ev->trainers || ev->num_trainers == 0) return 0;
    idx = rand() % ev->num_trainers;
    *out_class = ev->trainers[idx].trainer_class;
    *out_no = ev->trainers[idx].trainer_no;
    return 1;
}

static int scene_load_file(const char *name) {
    char path[192];
    FILE *fp = NULL;
    char line[1024];
    int lineno = 0;
    static const char *kScenePaths[] = {
        "debug/scenes/%s.scene",       /* canonical when cwd is repo root */
        "../debug/scenes/%s.scene",    /* canonical when cwd is build/ */
        "build/debug/scenes/%s.scene", /* canonical alt from repo root */
        "bugs/scenes/%s.scene",        /* legacy fallback */
        "../bugs/scenes/%s.scene",     /* legacy fallback */
        "build/bugs/scenes/%s.scene"   /* legacy fallback */
    };
    s_scene_cmd_count = 0;
    for (int i = 0; i < (int)(sizeof(kScenePaths)/sizeof(kScenePaths[0])); i++) {
        snprintf(path, sizeof(path), kScenePaths[i], name);
        fp = fopen(path, "r");
        if (fp) break;
    }
    if (!fp) return -1;
    memset(s_scene_defs, 0, sizeof(s_scene_defs));
    while (fgets(line, sizeof(line), fp)) {
        char *s = cli_trim(line);
        scene_normalize_ascii(s);
        scene_cmd_t cmd;
        memset(&cmd, 0, sizeof(cmd));
        lineno++;
        if (*s == '\0' || *s == '#') continue;
        if (s_scene_cmd_count >= SCENE_CMD_MAX) break;

        if (strncmp(s, "include ", 8) == 0) {
            char inc[64] = {0};
            FILE *ifp = NULL;
            char ipath[192];
            if (sscanf(s + 8, "%63s", inc) == 1) {
                static const char *kScenePaths[] = {
                    "debug/scenes/%s.scene",
                    "../debug/scenes/%s.scene",
                    "build/debug/scenes/%s.scene",
                    "bugs/scenes/%s.scene",
                    "../bugs/scenes/%s.scene",
                    "build/bugs/scenes/%s.scene"
                };
                for (int pi = 0; pi < (int)(sizeof(kScenePaths)/sizeof(kScenePaths[0])); pi++) {
                    snprintf(ipath, sizeof(ipath), kScenePaths[pi], inc);
                    ifp = fopen(ipath, "r");
                    if (ifp) break;
                }
                if (ifp) {
                    while (fgets(line, sizeof(line), ifp)) {
                        char *ds = cli_trim(line);
                        scene_normalize_ascii(ds);
                        if (strncmp(ds, "def ", 4) != 0) continue;
                        {
                            char defname[32] = {0};
                            int didx;
                            if (sscanf(ds + 4, "%31s", defname) != 1) continue;
                            didx = scene_defs_add(defname);
                            if (didx < 0) continue;
                            while (fgets(line, sizeof(line), ifp)) {
                                char *dline = cli_trim(line);
                                scene_normalize_ascii(dline);
                                if (strcmp(dline, "enddef") == 0) break;
                                if (*dline == '\0' || *dline == '#') continue;
                                if (s_scene_defs[didx].line_count < SCENE_DEF_LINE_MAX) {
                                    snprintf(s_scene_defs[didx].lines[s_scene_defs[didx].line_count],
                                             sizeof(s_scene_defs[didx].lines[0]), "%s", dline);
                                    s_scene_defs[didx].line_count++;
                                }
                            }
                        }
                    }
                    fclose(ifp);
                } else {
                    printf("[scene] include not found: %s\n", inc);
                }
            }
            continue;
        }

        if (strncmp(s, "def ", 4) == 0) {
            char defname[32] = {0};
            int didx;
            if (sscanf(s + 4, "%31s", defname) != 1) continue;
            didx = scene_defs_add(defname);
            if (didx < 0) continue;
            while (fgets(line, sizeof(line), fp)) {
                char *ds = cli_trim(line);
                scene_normalize_ascii(ds);
                lineno++;
                if (strcmp(ds, "enddef") == 0) break;
                if (*ds == '\0' || *ds == '#') continue;
                if (s_scene_defs[didx].line_count < SCENE_DEF_LINE_MAX) {
                    snprintf(s_scene_defs[didx].lines[s_scene_defs[didx].line_count],
                             sizeof(s_scene_defs[didx].lines[0]), "%s", ds);
                    s_scene_defs[didx].line_count++;
                }
            }
            continue;
        }

        if (strncmp(s, "use ", 4) == 0) {
            char defname[32] = {0};
            char args[16][96];
            char expanded[1024];
            char toks[17][96];
            char usebuf[1024];
            char use_music[96] = {0};
            int didx;
            memset(args, 0, sizeof(args));
            memset(toks, 0, sizeof(toks));
            {
                int nt;
                snprintf(usebuf, sizeof(usebuf), "%s", s + 4);
                /* Multiline `use` support: allow subsequent lines of quoted args.
                 * Continue consuming lines until we hit another command line. */
                while (1) {
                    long pos = ftell(fp);
                    char cont_line[1024];
                    char *ct;
                    if (!fgets(cont_line, sizeof(cont_line), fp)) break;
                    ct = cli_trim(cont_line);
                    scene_normalize_ascii(ct);
                    if (*ct == '\0' || *ct == '#') continue;
                    if (strncmp(ct, "music ", 6) == 0) {
                        const char *mv = ct + 6;
                        while (*mv == ' ' || *mv == '\t') mv++;
                        if (*mv) {
                            snprintf(use_music, sizeof(use_music), "%s", mv);
                        }
                        continue;
                    }
                    if (scene_is_line_command_start(ct)) {
                        fseek(fp, pos, SEEK_SET);
                        break;
                    }
                    if ((int)strlen(usebuf) + 1 + (int)strlen(ct) < (int)sizeof(usebuf)) {
                        strcat(usebuf, " ");
                        strcat(usebuf, ct);
                    }
                }
                nt = scene_split_args_quoted(usebuf, toks, 17);
                if (nt < 1) continue;
                snprintf(defname, sizeof(defname), "%s", toks[0]);
                for (int ti = 1; ti < nt && ti <= 16; ti++)
                    snprintf(args[ti - 1], sizeof(args[ti - 1]), "%s", toks[ti]);
                if (use_music[0] != '\0') {
                    /* Inline `music ...` in a multiline `use` block is a named
                     * override for arg $2, while preserving $1 as pre-text. */
                    for (int ai = 15; ai >= 2; ai--)
                        snprintf(args[ai], sizeof(args[ai]), "%s", args[ai - 1]);
                    snprintf(args[1], sizeof(args[1]), "%s", use_music);
                }
                if (strcmp(defname, "battle_intro_custom") == 0) {
                    printf("[scene] use battle_intro_custom args: $1='%s' $2='%s' $3='%s' $10='%s' $11='%s'\n",
                           args[0], args[1], args[2], args[9], args[10]);
                }
            }
            didx = scene_defs_find(defname);
            if (didx < 0) {
                printf("[scene] unknown def '%s' at line %d\n", defname, lineno);
                continue;
            }
            for (int li = 0; li < s_scene_defs[didx].line_count; li++) {
                s = s_scene_defs[didx].lines[li];
                scene_apply_args(s, expanded, sizeof(expanded), args);
                s = expanded;

                memset(&cmd, 0, sizeof(cmd));
                if (strncmp(s, "spawn ", 6) == 0) {
                    char id[24], sprite[32], x[32], y[32];
                    if (sscanf(s + 6, "%23s %31s %31s %31s", id, sprite, x, y) != 4) continue;
                    cmd.op = SCOP_SPAWN;
                    snprintf(cmd.actor, sizeof(cmd.actor), "%s", id);
                    cmd.a = scene_parse_sprite(sprite);
                    cmd.b = cli_is_numeric_token(x) ? (int)strtol(x, NULL, 0) : 0;
                    cmd.c = cli_is_numeric_token(y) ? (int)strtol(y, NULL, 0) : 0;
                    if (!cli_is_numeric_token(x) && strncmp(x, "player+", 7) == 0) cmd.b = (int)wXCoord + (int)strtol(x + 7, NULL, 0);
                    if (!cli_is_numeric_token(y) && strncmp(y, "player+", 7) == 0) cmd.c = (int)wYCoord + (int)strtol(y + 7, NULL, 0);
                } else if (strncmp(s, "despawn ", 8) == 0) {
                    cmd.op = SCOP_DESPAWN;
                    sscanf(s + 8, "%23s", cmd.actor);
                } else if (strncmp(s, "face ", 5) == 0) {
                    char id[24], dir[24];
                    if (sscanf(s + 5, "%23s %23s", id, dir) != 2) continue;
                    cmd.op = SCOP_FACE;
                    snprintf(cmd.actor, sizeof(cmd.actor), "%s", id);
                    if (strcmp(dir, "player") == 0) cmd.a = -2; else cmd.a = scene_parse_dir(dir);
                } else if (strncmp(s, "move ", 5) == 0) {
                    char id[24], dir[24], steps[24];
                    if (sscanf(s + 5, "%23s %23s %23s", id, dir, steps) != 3) continue;
                    cmd.op = SCOP_MOVE;
                    snprintf(cmd.actor, sizeof(cmd.actor), "%s", id);
                    cmd.a = scene_parse_dir(dir);
                    cmd.b = (int)strtol(steps, NULL, 0);
                } else if (strncmp(s, "say ", 4) == 0) {
                    cmd.op = SCOP_SAY;
                    snprintf(cmd.text, sizeof(cmd.text), "%s", s + 4);
                    {
                        size_t n = strlen(cmd.text);
                        if (n >= 2 && cmd.text[0] == '"' && cmd.text[n - 1] == '"') {
                            memmove(cmd.text, cmd.text + 1, n - 2);
                            cmd.text[n - 2] = '\0';
                        }
                    }
                    scene_unescape_text(cmd.text);
                    scene_format_dialog_text(cmd.text);
                } else if (strncmp(s, "ask ", 4) == 0) {
                    cmd.op = SCOP_ASK;
                    snprintf(cmd.text, sizeof(cmd.text), "%s", s + 4);
                    {
                        size_t n = strlen(cmd.text);
                        if (n >= 2 && cmd.text[0] == '"' && cmd.text[n - 1] == '"') {
                            memmove(cmd.text, cmd.text + 1, n - 2);
                            cmd.text[n - 2] = '\0';
                        }
                    }
                    scene_unescape_text(cmd.text);
                    scene_format_dialog_text(cmd.text);
                } else if (strncmp(s, "battlestart", 11) == 0) {
                    char btoks[10][96];
                    int nt = scene_split_args_quoted(s + 11, btoks, 10);
                    memset(&cmd, 0, sizeof(cmd));
                    cmd.op = SCOP_BATTLESTART;
                    cmd.a = 34; cmd.b = 1; /* default Brock */
                    if (nt >= 1 && strcmp(btoks[0], "custom") == 0) {
                        int slot_base = 1;
                        int ok = 1;
                        cmd.c = 1; /* custom team mode */
                        cmd.b = MUSIC_TRAINER_BATTLE; /* default custom battle music */
                        if (nt > 1) {
                            int mid = scene_parse_music_track(btoks[1]);
                            if (mid > 0) {
                                cmd.b = mid;
                                slot_base = 2;
                            }
                        }
                        if (nt > slot_base) {
                            int tc = cli_resolve_trainer_class_id(btoks[slot_base]);
                            if (tc > 0) {
                                cmd.a = tc;
                                slot_base++;
                            }
                        }
                        for (int si = 0; si < 6; si++) {
                            int tix = slot_base + si;
                            const char *slot = (tix < nt) ? btoks[tix] : "empty";
                            if (!scene_parse_team_slot(slot,
                                                       &cmd.team_species[si],
                                                       &cmd.team_level[si],
                                                       cmd.team_moves[si])) {
                                ok = 0;
                                break;
                            }
                            if (cmd.team_species[si] != 0) cmd.team_count = (uint8_t)(si + 1);
                        }
                        if (nt > slot_base + 6) {
                            snprintf(cmd.text, sizeof(cmd.text), "%s", btoks[slot_base + 6]);
                            scene_unescape_text(cmd.text);
                            scene_format_dialog_text(cmd.text);
                        }
                        if (!ok) {
                            printf("[scene] bad custom battlestart spec near '%s' (nt=%d slot_base=%d)\n",
                                   s, nt, slot_base);
                            continue;
                        }
                        printf("[scene] battlestart custom parsed: music=%d trainer=%d team_count=%u\n",
                               cmd.b, cmd.a, (unsigned)cmd.team_count);
                    } else if (nt >= 1) {
                        if (strcmp(btoks[0], "random") == 0) { cmd.a = -1; cmd.b = 0; }
                        else if (cli_is_numeric_token(btoks[0])) {
                            cmd.a = (int)strtol(btoks[0], NULL, 0);
                            if (nt >= 2 && cli_is_numeric_token(btoks[1]))
                                cmd.b = (int)strtol(btoks[1], NULL, 0);
                        }
                        if (nt >= 3) {
                            snprintf(cmd.text, sizeof(cmd.text), "%s", btoks[2]);
                            scene_unescape_text(cmd.text);
                            scene_format_dialog_text(cmd.text);
                        }
                    }
                } else if (strncmp(s, "battlend", 8) == 0) {
                    cmd.op = SCOP_BATTLEEND;
                    if (sscanf(s + 8, " %159[^\n]", cmd.text) != 1) snprintf(cmd.text, sizeof(cmd.text), "Battle complete.@");
                    {
                        size_t n = strlen(cmd.text);
                        if (n >= 2 && cmd.text[0] == '"' && cmd.text[n - 1] == '"') {
                            memmove(cmd.text, cmd.text + 1, n - 2);
                            cmd.text[n - 2] = '\0';
                        }
                    }
                    scene_unescape_text(cmd.text);
                    scene_format_dialog_text(cmd.text);
                } else if (strncmp(s, "music ", 6) == 0) {
                    int mid = scene_parse_music_track(s + 6);
                    if (mid < 0) continue;
                    cmd.op = SCOP_MUSIC;
                    cmd.a = mid;
                } else if (strncmp(s, "wait ", 5) == 0) { cmd.op = SCOP_WAIT; cmd.a = (int)strtol(s + 5, NULL, 0); }
                else if (strcmp(s, "wait_text") == 0) { cmd.op = SCOP_WAIT_TEXT; }
                else if (strcmp(s, "lock_input on") == 0) { cmd.op = SCOP_LOCK_INPUT; cmd.a = 1; }
                else if (strcmp(s, "lock_input off") == 0) { cmd.op = SCOP_LOCK_INPUT; cmd.a = 0; }
                else if (strcmp(s, "end") == 0) { cmd.op = SCOP_END; }
                else continue;

                if (s_scene_cmd_count < SCENE_CMD_MAX) s_scene_cmds[s_scene_cmd_count++] = cmd;
            }
            continue;
        }

        if (strncmp(s, "spawn ", 6) == 0) {
            char id[24], sprite[32], x[32], y[32];
            if (sscanf(s + 6, "%23s %31s %31s %31s", id, sprite, x, y) != 4) continue;
            cmd.op = SCOP_SPAWN;
            snprintf(cmd.actor, sizeof(cmd.actor), "%s", id);
            cmd.a = scene_parse_sprite(sprite);
            cmd.b = cli_is_numeric_token(x) ? (int)strtol(x, NULL, 0) : 0;
            cmd.c = cli_is_numeric_token(y) ? (int)strtol(y, NULL, 0) : 0;
            if (!cli_is_numeric_token(x) && strncmp(x, "player+", 7) == 0) cmd.b = (int)wXCoord + (int)strtol(x + 7, NULL, 0);
            if (!cli_is_numeric_token(y) && strncmp(y, "player+", 7) == 0) cmd.c = (int)wYCoord + (int)strtol(y + 7, NULL, 0);
        } else if (strncmp(s, "despawn ", 8) == 0) {
            cmd.op = SCOP_DESPAWN;
            sscanf(s + 8, "%23s", cmd.actor);
        } else if (strncmp(s, "face ", 5) == 0) {
            char id[24], dir[24];
            if (sscanf(s + 5, "%23s %23s", id, dir) != 2) continue;
            cmd.op = SCOP_FACE;
            snprintf(cmd.actor, sizeof(cmd.actor), "%s", id);
            if (strcmp(dir, "player") == 0) cmd.a = -2; else cmd.a = scene_parse_dir(dir);
        } else if (strncmp(s, "move ", 5) == 0) {
            char id[24], dir[24], steps[24];
            if (sscanf(s + 5, "%23s %23s %23s", id, dir, steps) != 3) continue;
            cmd.op = SCOP_MOVE;
            snprintf(cmd.actor, sizeof(cmd.actor), "%s", id);
            cmd.a = scene_parse_dir(dir);
            cmd.b = (int)strtol(steps, NULL, 0);
        } else if (strncmp(s, "say ", 4) == 0) {
            cmd.op = SCOP_SAY;
            snprintf(cmd.text, sizeof(cmd.text), "%s", s + 4);
            {
                size_t n = strlen(cmd.text);
                if (n >= 2 && cmd.text[0] == '"' && cmd.text[n - 1] == '"') {
                    memmove(cmd.text, cmd.text + 1, n - 2);
                    cmd.text[n - 2] = '\0';
                }
            }
            scene_unescape_text(cmd.text);
            scene_format_dialog_text(cmd.text);
        } else if (strncmp(s, "ask ", 4) == 0) {
            cmd.op = SCOP_ASK;
            snprintf(cmd.text, sizeof(cmd.text), "%s", s + 4);
            {
                size_t n = strlen(cmd.text);
                if (n >= 2 && cmd.text[0] == '"' && cmd.text[n - 1] == '"') {
                    memmove(cmd.text, cmd.text + 1, n - 2);
                    cmd.text[n - 2] = '\0';
                }
            }
            scene_unescape_text(cmd.text);
            scene_format_dialog_text(cmd.text);
        } else if (strncmp(s, "battlestart", 11) == 0 || strncmp(s, "battlend", 8) == 0) {
            printf("[scene] %s:%d raw battle ops are disallowed at top-level; use a reusable def (e.g. include defs_battle + use battle_intro ...)\n",
                   path, lineno);
            fclose(fp);
            return -2;
        } else if (strncmp(s, "music ", 6) == 0) {
            int mid = scene_parse_music_track(s + 6);
            if (mid < 0) continue;
            cmd.op = SCOP_MUSIC;
            cmd.a = mid;
        } else if (strncmp(s, "wait ", 5) == 0) {
            cmd.op = SCOP_WAIT;
            cmd.a = (int)strtol(s + 5, NULL, 0);
        } else if (strcmp(s, "wait_text") == 0) {
            cmd.op = SCOP_WAIT_TEXT;
        } else if (strcmp(s, "lock_input on") == 0) {
            cmd.op = SCOP_LOCK_INPUT; cmd.a = 1;
        } else if (strcmp(s, "lock_input off") == 0) {
            cmd.op = SCOP_LOCK_INPUT; cmd.a = 0;
        } else if (strcmp(s, "end") == 0) {
            cmd.op = SCOP_END;
        } else {
            continue;
        }
        s_scene_cmds[s_scene_cmd_count++] = cmd;
    }
    fclose(fp);
    return s_scene_cmd_count;
}

static int scene_trigger_find_slot(const char *scene, int map_id, int x, int y) {
    for (int i = 0; i < SCENE_TRIGGER_MAX; i++) {
        if (!s_scene_triggers[i].used) continue;
        if (strcmp(s_scene_triggers[i].scene, scene) == 0 &&
            (int)s_scene_triggers[i].map_id == map_id &&
            s_scene_triggers[i].x == x &&
            s_scene_triggers[i].y == y) return i;
    }
    return -1;
}

static int scene_trigger_alloc_slot(void) {
    for (int i = 0; i < SCENE_TRIGGER_MAX; i++) if (!s_scene_triggers[i].used) return i;
    return -1;
}

static int scene_parse_coord_expr(const char *tok, int is_x, int *out) {
    const char *base = is_x ? "player.x" : "player.y";
    const char *base_short = "player";
    if (!tok || !out) return 0;
    if (cli_is_numeric_token(tok)) {
        *out = (int)strtol(tok, NULL, 0);
        return 1;
    }
    if (strncmp(tok, base, strlen(base)) == 0) {
        int v = is_x ? (int)wXCoord : (int)wYCoord;
        const char *p = tok + strlen(base);
        if (*p == '\0') {
            *out = v;
            return 1;
        }
        if ((*p == '+' || *p == '-') && cli_is_numeric_token(p + 1)) {
            int off = (int)strtol(p + 1, NULL, 0);
            if (*p == '-') off = -off;
            *out = v + off;
            return 1;
        }
    }
    if (strncmp(tok, base_short, strlen(base_short)) == 0) {
        int v = is_x ? (int)wXCoord : (int)wYCoord;
        const char *p = tok + strlen(base_short);
        if (*p == '\0') {
            *out = v;
            return 1;
        }
        if ((*p == '+' || *p == '-') && cli_is_numeric_token(p + 1)) {
            int off = (int)strtol(p + 1, NULL, 0);
            if (*p == '-') off = -off;
            *out = v + off;
            return 1;
        }
    }
    return 0;
}

static int cli_resolve_event_token(const char *tok, uint16_t *out_event) {
    char want[96];
    if (!tok || !*tok || !out_event) return 0;
    if (cli_is_numeric_token(tok)) {
        long v = strtol(tok, NULL, 0);
        if (v < 0 || v > 65535) return 0;
        *out_event = (uint16_t)v;
        return 1;
    }

    cli_norm(want, sizeof(want), tok);
    for (int i = 0; i <= 4095; i++) {
        const char *name = EventFlagName((uint16_t)i);
        char norm[96], short_norm[96];
        if (!name || strcmp(name, "UNKNOWN_EVENT") == 0) continue;
        cli_norm(norm, sizeof(norm), name);
        if (strcmp(norm, want) == 0) {
            *out_event = (uint16_t)i;
            return 1;
        }
        if (strncmp(norm, "event", 5) == 0) {
            snprintf(short_norm, sizeof(short_norm), "%s", norm + 5);
            if (strcmp(short_norm, want) == 0) {
                *out_event = (uint16_t)i;
                return 1;
            }
        }
    }
    return 0;
}

static int cli_resolve_map_token(const char *tok, int *out_map_id) {
    char want[64];
    if (!tok || !*tok || !out_map_id) return 0;
    if (strcmp(tok, "here") == 0 || strcmp(tok, "current") == 0) {
        *out_map_id = (int)wCurMap;
        return 1;
    }
    if (cli_is_numeric_token(tok)) {
        *out_map_id = (int)strtol(tok, NULL, 0);
        return (*out_map_id >= 0 && *out_map_id < NUM_MAPS);
    }

    cli_norm(want, sizeof(want), tok);
    for (int i = 0; i < NUM_MAPS; i++) {
        char have[64];
        cli_norm(have, sizeof(have), gMapTable[i].name);
        if (strcmp(want, have) == 0) {
            *out_map_id = i;
            return 1;
        }
    }
    return 0;
}

/* ---- State writers ------------------------------------------------ */

static void write_battle_state(FILE *fp) {
    int bui = BattleUI_GetState();
    const char *btype = (wIsInBattle == 2) ? "TRAINER" : "WILD";

    fprintf(fp, "=== BATTLE (%s) ===\n", btype);
    fprintf(fp, "UI State: %s\n\n", bui_state_name(bui));

    {
        battle_hittrace_t ht = Battle_GetLastHitTrace();
        fprintf(fp, "HITTRACE: %s\n", Battle_HitTraceIsEnabled() ? "ON" : "OFF");
        if (ht.seq > 0) {
            const char *mname = (ht.move_num < NUM_MOVE_DEFS && gMoveNames[ht.move_num])
                ? gMoveNames[ht.move_num] : "???";
            fprintf(fp,
                "  seq=%lu turn=%s move=%u(%s) effect=0x%02X base_acc=%u scaled_acc=%u roll=%u missed=%u reason=%s\n\n",
                (unsigned long)ht.seq,
                ht.player_turn ? "enemy" : "player",
                ht.move_num, mname, ht.move_effect,
                ht.base_acc, ht.scaled_acc, ht.roll, ht.missed,
                hittrace_reason_name(ht.reason));
        } else {
            fprintf(fp, "  (no MoveHitTest samples yet)\n\n");
        }
    }

    if (s_animlab_enabled) {
        uint8_t next = (uint8_t)((s_animlab_move_id > 0 && s_animlab_move_id < NUM_MOVE_DEFS)
            ? s_animlab_move_id : 1);
        const char *next_name = gMoveNames[next] ? gMoveNames[next] : "???";
        fprintf(fp, "ANIMLAB: ON  next=%d (%s)  loops=%d\n\n",
                (int)next, next_name, s_animlab_loops);
    }

    /* Enemy mon */
    fprintf(fp, "ENEMY:  %s Lv%d  HP: %d/%d  [%s]\n",
            Pokemon_GetName(gSpeciesToDex[wEnemyMon.species]),
            wEnemyMon.level,
            wEnemyMon.hp, wEnemyMon.max_hp,
            status_str(wEnemyMon.status));

    /* Player mon */
    fprintf(fp, "PLAYER: %s Lv%d  HP: %d/%d  [%s]\n\n",
            Pokemon_GetName(gSpeciesToDex[wBattleMon.species]),
            wBattleMon.level,
            wBattleMon.hp, wBattleMon.max_hp,
            status_str(wBattleMon.status));

    /* Moves */
    fprintf(fp, "Moves:\n");
    for (int i = 0; i < 4; i++) {
        uint8_t mid = wBattleMon.moves[i];
        if (!mid) { fprintf(fp, "  [%d] ---\n", i + 1); continue; }
        uint8_t pp = wBattleMon.pp[i] & 0x3F;
        const char *mname = (mid < NUM_MOVE_DEFS && gMoveNames[mid]) ? gMoveNames[mid] : "???";
        fprintf(fp, "  [%d] %-12s  %d pp\n", i + 1, mname, pp);
    }

    /* What can I do right now */
    fprintf(fp, "\n");
    if (bui == 10 /* BUI_MENU */) {
        fprintf(fp, ">> Waiting for action: fight <1-4> | run | pkmn | bag\n");
    } else if (bui == 11 /* BUI_MOVE_SELECT */) {
        fprintf(fp, ">> Waiting for move: fight <1-4> | b (back)\n");
    } else if (bui == 22 /* BUI_USE_NEXT_MON */) {
        fprintf(fp, ">> \"Use next Pokemon?\"  a (yes) | b (no)\n");
    } else if (bui == 23 || bui == 24 /* BUI_PARTY_SELECT / SWITCH_SELECT */) {
        fprintf(fp, ">> Choose next Pokemon from party menu\n");
    } else {
        fprintf(fp, ">> Animation in progress — wait or press a/b to advance text\n");
    }
}

static char tile_char(int mx, int my, int px, int py, int nc,
                      const map_events_t *ev) {
    /* px/py and NPC positions are in game coords; convert to tile coords.
     * mx/my are already tile coords (from gCamX + tx loop).
     * Signs and items use game coords (*2 / *2+1); hidden events are already
     * in tile coords (x=orig*2, y=orig*2+1 per event_data.h). */
    if (mx == px * 2 && my == py * 2 + 1) return '@';
    for (int i = 0; i < nc; i++) {
        int ntx, nty;
        NPC_GetTilePos(i, &ntx, &nty);
        if (ntx * 2 == mx && nty * 2 + 1 == my) return 'N';
    }
    if (ev) {
        for (int i = 0; i < ev->num_signs; i++)
            if ((int)ev->signs[i].x * 2 == mx && (int)ev->signs[i].y * 2 + 1 == my) return 'S';
        for (int i = 0; i < ev->num_items; i++)
            if ((int)ev->items[i].x * 2 == mx && (int)ev->items[i].y * 2 + 1 == my) return 'I';
        for (int i = 0; i < ev->num_hidden_events; i++)
            if ((int)ev->hidden_events[i].x == mx && (int)ev->hidden_events[i].y == my) return 'H';
    }
    uint8_t tid = Map_GetTile(mx, my);
    if (Warp_IsDoorTile(tid))                    return '+';
    if (tid == wGrassTile && wGrassTile != 0xFF) return '"';
    int ld = Player_GetLedgeDir(tid);
    if (ld ==  0) return 'v';
    if (ld ==  4) return '^';
    if (ld ==  8) return '<';
    if (ld == 12) return '>';
    if (!Tile_IsPassable(tid))                   return '#';
    return '.';
}

static void write_overworld_state(FILE *fp) {
    const char *mname = (wCurMap < NUM_MAPS) ? gMapTable[wCurMap].name : "???";
    fprintf(fp, "=== OVERWORLD ===\n");
    fprintf(fp, "Map: %d (%s)  Player: (%d, %d)  Facing: %s\n\n",
            wCurMap, mname, wXCoord, wYCoord, facing_name(wPlayerDirection));

    static const char *legend[] = {
        "@  = Player",
        "#  = Wall/Solid",
        ".  = Open",
        "\"  = Grass",
        "+  = Warp/Door",
        "N  = NPC",
        "^v<> = Ledge",
        "S  = Sign",
        "I  = Item",
        "H  = Hidden event",
    };
    static const int LEGEND_COUNT = (int)(sizeof(legend) / sizeof(legend[0]));

    int nc = NPC_GetCount();
    int px = (int)wXCoord, py = (int)wYCoord;
    const map_events_t *ev = (wCurMap < NUM_MAPS) ? &gMapEvents[wCurMap] : NULL;

    fprintf(fp, "+");
    for (int x = 0; x < SCREEN_WIDTH; x++) fprintf(fp, "-");
    fprintf(fp, "+  Legend:\n");
    for (int ty = 0; ty < SCREEN_HEIGHT; ty++) {
        fprintf(fp, "|");
        for (int tx = 0; tx < SCREEN_WIDTH; tx++)
            fprintf(fp, "%c", tile_char(gCamX + tx, gCamY + ty, px, py, nc, ev));
        fprintf(fp, "|");
        if (ty < LEGEND_COUNT) fprintf(fp, "  %s", legend[ty]);
        fprintf(fp, "\n");
    }
    fprintf(fp, "+");
    for (int x = 0; x < SCREEN_WIDTH; x++) fprintf(fp, "-");
    fprintf(fp, "+\n");
}

static void write_state(void) {
    FILE *fp = fopen(STATE_FILE, "w");
    if (!fp) return;

    /* Text lock: when a dialogue page is open, show only the text.
     * Movement and battle commands are blocked until text is dismissed. */
    if (Text_IsOpen()) {
        char tbuf[256];
        fprintf(fp, "=== TEXT ===\n");
        if (Text_GetCurrentStr(tbuf, sizeof(tbuf)))
            fprintf(fp, "%s\n", tbuf);
        else
            fprintf(fp, "<dialog open>\n");
        fprintf(fp, "\n>> press a to continue | b to dismiss\n");
        fclose(fp);
        return;
    }

    /* Pokecenter YES/NO drawn directly to tilemap — not via text system */
    if (Pokecenter_IsWaitingYesNo()) {
        fprintf(fp, "=== POKECENTER ===\n");
        fprintf(fp, "Nurse Joy: Shall we heal your Pokemon?\n");
        fprintf(fp, "\n>> a (yes, heal) | b (no, cancel)\n");
        fclose(fp);
        return;
    }

    int sc = get_scene();

    if (sc == 2 /* BATTLE */ || sc == 1 /* BATTLE_TRANS */) {
        write_battle_state(fp);
    } else {
        write_overworld_state(fp);
    }

    /* Party summary (always) */
    fprintf(fp, "\nParty (%d):\n", wPartyCount);
    for (int i = 0; i < wPartyCount && i < 6; i++) {
        const party_mon_t *m = &wPartyMons[i];
        fprintf(fp, "  [%d] %s Lv%d  HP:%d/%d  [%s]\n",
                i + 1,
                Pokemon_GetName(gSpeciesToDex[m->base.species]),
                m->level, (int)m->base.hp, (int)m->max_hp,
                status_str(m->base.status));
    }

    /* Money / badges */
    unsigned money = ((unsigned)wPlayerMoney[0] >> 4) * 100000
                   + ((unsigned)wPlayerMoney[0] & 0xF) * 10000
                   + ((unsigned)wPlayerMoney[1] >> 4) * 1000
                   + ((unsigned)wPlayerMoney[1] & 0xF) * 100
                   + ((unsigned)wPlayerMoney[2] >> 4) * 10
                   + ((unsigned)wPlayerMoney[2] & 0xF);
    fprintf(fp, "\nMoney: $%u  Badges: %d  Frame: %d\n",
            money, count_bits8(wObtainedBadges),
            (int)(255 - hFrameCounter));

    if (Trainer_IsEngaging())
        fprintf(fp, "\n!! TRAINER SPOTTED YOU — engaging\n");

    fclose(fp);
}

/* ---- Command parser ----------------------------------------------- */
#define FRAMES_PER_TILE 20
#define PRESS  1   /* frames held for a menu button press */
#define GAP    8   /* release gap between presses (lets state machine update) */

/* Navigate main battle menu to position (0=FIGHT 1=PKMN 2=ITEM 3=RUN)
 * then press A.  Cursor always resets to 0 at turn start. */
static void seq_battle_menu(int pos) {
    if (pos & 2) seq_push(BTN_DOWN,  PRESS, GAP);
    if (pos & 1) seq_push(BTN_RIGHT, PRESS, GAP);
    seq_push(BTN_A, PRESS, GAP);
}

/* Select move N (1-4) from the move list.  Assumes we're in MOVE_SELECT
 * state with cursor at 0 (top). */
static void seq_move_select(int n) {
    for (int i = 1; i < n; i++)
        seq_push(BTN_DOWN, PRESS, GAP);
    seq_push(BTN_A, PRESS, GAP);
}

/* ---- Console helpers --------------------------------------------- */

#define CON_TMIDX(row, col) ((unsigned)((row) + 2) * SCROLL_MAP_W + ((col) + 2))

static int con_char_to_tile(unsigned char c) {
    if (c >= 'A' && c <= 'Z') return Font_CharToTile(0x80 + (c - 'A'));
    if (c >= 'a' && c <= 'z') return Font_CharToTile(0xA0 + (c - 'a'));
    if (c == ' ')              return BLANK_TILE_SLOT;
    if (c >= '0' && c <= '9') return Font_CharToTile(0xF6 + (c - '0'));
    if (c == '.')              return Font_CharToTile(0xE8);
    if (c == ',')              return Font_CharToTile(0xF4);
    if (c == '-')              return Font_CharToTile(0xE3);
    if (c == ':')              return Font_CharToTile(0x9C);
    if (c == '/')              return Font_CharToTile(0xF3);
    if (c == '?')              return Font_CharToTile(0xE6);
    if (c == '!')              return Font_CharToTile(0xE7);
    return BLANK_TILE_SLOT;
}

static void con_draw(void) {
    if (!s_con_overlay_enabled) return;
    /* Top border: ┌──────────────────┐ */
    gScrollTileMap[CON_TMIDX(CON_TOP_ROW, 0)] = (uint8_t)Font_CharToTile(0x79);
    for (int c = 1; c < SCREEN_WIDTH - 1; c++)
        gScrollTileMap[CON_TMIDX(CON_TOP_ROW, c)] = (uint8_t)Font_CharToTile(0x7A);
    gScrollTileMap[CON_TMIDX(CON_TOP_ROW, SCREEN_WIDTH - 1)] = (uint8_t)Font_CharToTile(0x7B);

    /* Input row: │: text_           │ */
    gScrollTileMap[CON_TMIDX(CON_IN_ROW, 0)]              = (uint8_t)Font_CharToTile(0x7C);
    gScrollTileMap[CON_TMIDX(CON_IN_ROW, SCREEN_WIDTH-1)] = (uint8_t)Font_CharToTile(0x7C);
    gScrollTileMap[CON_TMIDX(CON_IN_ROW, 1)]              = (uint8_t)con_char_to_tile(':');

    /* Text area: cols 2..(SW-3), cursor at SW-2  (SW-4 = 16 visible chars) */
    int text_cols = SCREEN_WIDTH - 4;
    int start = (s_con_len > text_cols) ? s_con_len - text_cols : 0;
    int col = 2;
    for (int i = start; i < s_con_len && col <= SCREEN_WIDTH - 3; i++, col++)
        gScrollTileMap[CON_TMIDX(CON_IN_ROW, col)] =
            (uint8_t)con_char_to_tile((unsigned char)s_con_buf[i]);
    /* Blinking cursor */
    if (col <= SCREEN_WIDTH - 2) {
        gScrollTileMap[CON_TMIDX(CON_IN_ROW, col)] =
            (s_con_blink < CON_BLINK_PERIOD / 2)
            ? (uint8_t)Font_CharToTile(0x7A)  /* ─ on  */
            : BLANK_TILE_SLOT;                  /* blank off */
        col++;
    }
    for (; col <= SCREEN_WIDTH - 2; col++)
        gScrollTileMap[CON_TMIDX(CON_IN_ROW, col)] = BLANK_TILE_SLOT;
}

static void process_cmd(const char *cmd) {
    s_last_cmd_valid = 1;
    char verb[32] = {0};
    int  n = 1;
    sscanf(cmd, "%31s %d", verb, &n);
    if (n < 1) n = 1;

    seq_clear();

    /* ---- Text/dialogue lock: only a/b accepted while text is open ----
     * Allow a small whitelist of debug-management commands to still run. */
    {
        int allow_when_text_open =
            (strcmp(verb, "state") == 0) ||
            (strcmp(verb, "scene_npc") == 0) ||
            (strcmp(verb, "scene-npc") == 0) ||
            (strcmp(verb, "scenenpc") == 0) ||
            (strcmp(verb, "npcscene") == 0) ||
            (strcmp(verb, "scene_trigger") == 0) ||
            (strcmp(verb, "scene_run") == 0) ||
            (strcmp(verb, "scene_stop") == 0) ||
            (strcmp(verb, "dsl_bank") == 0);
    if ((Text_IsOpen() || Pokecenter_IsWaitingYesNo()) && !allow_when_text_open) {
        if (strcmp(verb, "a") == 0 || strcmp(verb, "interact") == 0) {
            for (int i = 0; i < n; i++) seq_push(BTN_A, PRESS, GAP);
        } else if (strcmp(verb, "b") == 0 || strcmp(verb, "back") == 0) {
            seq_push(BTN_B, PRESS, GAP);
        } else {
            printf("[cli] Text is open — only a/b accepted\n");
            write_state();
            return;
        }
        if (s_seq_len > 0) {
            s_wait_remaining = 20;
            s_pending_write  = 1;
        }
        return;
    }
    }

    /* ---- Overworld / generic commands ---- */
    if      (strcmp(verb, "up")    == 0) seq_push(BTN_UP,    n * FRAMES_PER_TILE, 5);
    else if (strcmp(verb, "down")  == 0) seq_push(BTN_DOWN,  n * FRAMES_PER_TILE, 5);
    else if (strcmp(verb, "left")  == 0) seq_push(BTN_LEFT,  n * FRAMES_PER_TILE, 5);
    else if (strcmp(verb, "right") == 0) seq_push(BTN_RIGHT, n * FRAMES_PER_TILE, 5);
    else if (strcmp(verb, "a") == 0 || strcmp(verb, "interact") == 0) {
        for (int i = 0; i < n; i++) seq_push(BTN_A, PRESS, GAP);
    } else if (strcmp(verb, "b") == 0 || strcmp(verb, "back") == 0)
        seq_push(BTN_B,      PRESS, GAP);
    else if (strcmp(verb, "start") == 0 || strcmp(verb, "menu") == 0)
        seq_push(BTN_START,  PRESS, GAP);
    else if (strcmp(verb, "select") == 0)
        seq_push(BTN_SELECT, PRESS, GAP);
    else if (strcmp(verb, "wait")  == 0)
        seq_push(0, n, 0);
    else if (strcmp(verb, "state") == 0) {
        write_state();
        return;
    }
    else if (strcmp(verb, "tile_info") == 0 || strcmp(verb, "tileinfo") == 0) {
        /* Dump tile IDs and passability for player pos + 4 neighbours.
         * Useful for identifying incorrectly-passable wall tiles. */
        int px = (int)wXCoord, py = (int)wYCoord;
        static const struct { const char *label; int dx, dy; } dirs[] = {
            { "HERE ", 0,  0 },
            { "UP   ", 0, -1 },
            { "DOWN ", 0,  1 },
            { "LEFT ",-1,  0 },
            { "RIGHT", 1,  0 },
        };
        printf("[tile_info] player @ game(%d,%d)  tile(%d,%d)\n",
               px, py, px*2, py*2+1);
        for (int i = 0; i < 5; i++) {
            int gx = px + dirs[i].dx;
            int gy = py + dirs[i].dy;
            uint8_t tid = Map_GetGameTile(gx, gy);
            printf("  %s  game(%2d,%2d)  tile(%2d,%2d)  id=0x%02X  %s\n",
                   dirs[i].label, gx, gy, gx*2, gy*2+1, tid,
                   Tile_IsPassable(tid) ? "PASS" : "SOLID");
        }
        return;
    }
    /* ---- Battle commands ---- */
    /* ---- Cheat commands ---- */
    else if (strcmp(verb, "setlevel") == 0) {
        /* setlevel <slot 1-6> <level 1-100>
         * Re-initialises the party mon at slot with the same species at the
         * new level (full HP, updated moves, stats recalculated). */
        int slot = 1, level = 20;
        sscanf(cmd, "%*s %d %d", &slot, &level);
        slot--;  /* 1-based → 0-based */
        if (slot < 0 || slot >= wPartyCount) {
            printf("[cli] setlevel: slot out of range (party has %d)\n", wPartyCount);
        } else if (level < 1 || level > 100) {
            printf("[cli] setlevel: level must be 1-100\n");
        } else {
            uint8_t species = wPartyMons[slot].base.species;
            Pokemon_InitMon(&wPartyMons[slot], species, (uint8_t)level);
            printf("[cli] setlevel: slot %d (%s) → Lv%d\n",
                   slot + 1,
                   Pokemon_GetName(gSpeciesToDex[species]),
                   level);
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "animlab") == 0) {
        char mode[16] = "start";
        int level = 50;
        sscanf(cmd, "%*s %15s %d", mode, &level);

        if (strcmp(mode, "start") == 0 || strcmp(mode, "on") == 0) {
            if (level < 5 || level > 100) level = 50;
            s_animlab_move_id = 1;
            s_animlab_loops   = 0;
            animlab_start_battle(level);
        } else if (strcmp(mode, "stop") == 0 || strcmp(mode, "off") == 0) {
            s_animlab_enabled = 0;
            printf("[cli] animlab: stopped (battle remains under manual control)\n");
        } else if (strcmp(mode, "status") == 0) {
            printf("[cli] animlab: %s (next move %d, loops %d, level %d)\n",
                   s_animlab_enabled ? "ON" : "OFF",
                   s_animlab_move_id, s_animlab_loops, s_animlab_level);
        } else {
            printf("[cli] animlab: use 'animlab start [level]', 'animlab stop', or 'animlab status'\n");
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "hittrace") == 0) {
        char arg[16] = {0};
        sscanf(cmd, "%*s %15s", arg);
        if (strcmp(arg, "on") == 0) {
            Battle_HitTraceEnable(1);
            printf("[cli] hittrace: ON\n");
        } else if (strcmp(arg, "off") == 0) {
            Battle_HitTraceEnable(0);
            printf("[cli] hittrace: OFF\n");
        } else if (strcmp(arg, "reset") == 0) {
            Battle_HitTraceReset();
            printf("[cli] hittrace: reset\n");
        } else if (strcmp(arg, "status") == 0 || arg[0] == '\0') {
            battle_hittrace_t ht = Battle_GetLastHitTrace();
            printf("[cli] hittrace: %s seq=%lu move=0x%02X effect=0x%02X missed=%u reason=%s\n",
                   Battle_HitTraceIsEnabled() ? "ON" : "OFF",
                   (unsigned long)ht.seq, ht.move_num, ht.move_effect, ht.missed,
                   hittrace_reason_name(ht.reason));
        } else {
            printf("[cli] hittrace: use 'hittrace on|off|reset|status'\n");
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "fight") == 0) {
        /* fight [1-4]: navigate main menu to FIGHT, then select the move */
        seq_battle_menu(0);          /* cursor 0 = FIGHT, press A */
        if (n >= 1 && n <= 4)
            seq_move_select(n);      /* navigate move list + A */
    }
    else if (strcmp(verb, "run")  == 0) seq_battle_menu(3); /* cursor 3 = RUN */
    else if (strcmp(verb, "pkmn") == 0 || strcmp(verb, "pokemon") == 0)
        seq_battle_menu(1);          /* cursor 1 = PKMN */
    else if (strcmp(verb, "bag")  == 0 || strcmp(verb, "item") == 0)
        seq_battle_menu(2);          /* cursor 2 = ITEM */
    else if (strcmp(verb, "teleport") == 0 || strcmp(verb, "tele") == 0) {
        static const struct { const char *name; int map, x, y; } kPlaces[] = {
            /* Towns / Cities (ASM FlyWarpDataPtr destinations; in front of Pokecenters) */
            { "pallet",           0,   5,  6 },
            { "pallet_town",      0,   5,  6 },
            { "viridian",         1,  23, 26 },
            { "viridian_city",    1,  23, 26 },
            { "pewter",           2,  13, 26 },
            { "pewter_city",      2,  13, 26 },
            { "cerulean",         3,  19, 18 },
            { "cerulean_city",    3,  19, 18 },
            { "vermilion",        5,  11,  4 },
            { "vermilion_city",   5,  11,  4 },
            { "lavender",         4,   3,  6 },
            { "lavender_town",    4,   3,  6 },
            { "celadon",          6,  41, 10 },
            { "celadon_city",     6,  41, 10 },
            { "fuchsia",          7,  19, 28 },
            { "fuchsia_city",     7,  19, 28 },
            { "cinnabar",         8,  11, 12 },
            { "cinnabar_island",  8,  11, 12 },
            { "indigo",           9,   9,  6 },
            { "indigo_plateau",   9,   9,  6 },
            { "saffron",         10,   9, 30 },
            { "saffron_city",    10,   9, 30 },
            { "route_4_fly",     15,  11,  6 },
            { "route_10_fly",    21,  11, 20 },
            /* Gyms */
            { "viridian_gym",    52,   8,  9 },
            { "pewter_gym",      54,   8,  9 },
            { "cerulean_gym",    65,   8,  9 },
            { "vermilion_gym",   92,   8,  9 },
            { "celadon_gym",    135,   8,  9 },
            { "fuchsia_gym",    166,   8,  9 },
            { "saffron_gym",    178,   8,  9 },
            { "cinnabar_gym",   234,   8,  9 },
            /* Key locations */
            { "oaks_lab",        37,  12, 11 },
            { "oaks_lab",        37,  12, 11 },
            { "viridian_forest", 51,  14, 40 },
            { "mt_moon",         59,  14, 10 },
            { "rock_tunnel",    155,  14, 10 },
            { "pokemon_tower",  142,   8,  9 },
            { "silph_co",       200,   8,  9 },
            { "safari_zone",    217,  28, 20 },
            /* Routes (spawn near south entrance) */
            { "route_1",         12,  14, 70 },
            { "route_2",         13,  14, 10 },
            { "route_3",         14,  14, 10 },
            { "route_4",         15,  14, 10 },
            { "route_5",         16,  14, 10 },
            { "route_6",         17,  14, 70 },
            { "route_7",         18,  14, 10 },
            { "route_8",         19,  14, 70 },
            { "route_9",         20,  14, 10 },
            { "route_10",        21,  14, 10 },
            { "route_11",        22,  14, 10 },
            { "route_12",        23,  14, 10 },
            { "route_24",        33,  14, 10 },
            { "route_25",        34,  14, 10 },
            { NULL, 0, 0, 0 }
        };

        int map_id, x, y;
        char name_arg[64] = {0};
        sscanf(cmd, "%*s %63s", name_arg);

        /* Named location lookup */
        int found = 0;
        if (name_arg[0] && !(name_arg[0] >= '0' && name_arg[0] <= '9')) {
            /* lowercase the argument */
            for (int k = 0; name_arg[k]; k++)
                if (name_arg[k] >= 'A' && name_arg[k] <= 'Z') name_arg[k] += 32;
            for (int k = 0; kPlaces[k].name; k++) {
                if (strcmp(name_arg, kPlaces[k].name) == 0) {
                    map_id = kPlaces[k].map;
                    x      = kPlaces[k].x;
                    y      = kPlaces[k].y;
                    found  = 1;
                    break;
                }
            }
            if (!found) {
                printf("[cli] Unknown location: %s\n", name_arg);
                write_state();
                return;
            }
        } else {
            /* Numeric: teleport <map_id> <x> <y> (decimal or 0x hex) */
            map_id = 0; x = 3; y = 3;
            { char m[16]="0", xs[16]="3", ys[16]="3";
              sscanf(cmd, "%*s %15s %15s %15s", m, xs, ys);
              map_id = (int)strtol(m,  NULL, 0);
              x      = (int)strtol(xs, NULL, 0);
              y      = (int)strtol(ys, NULL, 0); }
        }

        extern void Map_Load(uint8_t map_id);
        extern void NPC_Load(void);
        extern void PalletScripts_OnMapLoad(void);
        extern void OaksLabScripts_OnMapLoad(void);
        extern void Display_LoadMapPalette(void);
        wCurMap  = (uint8_t)map_id;
        wXCoord  = (uint8_t)x;
        wYCoord  = (uint8_t)y;
        /* Apply darkness for cave maps, clear it for outdoor maps */
        gMapPalOffset = (map_id == 0x52 /* ROCK_TUNNEL_1F */ ||
                         map_id == 0xE8 /* ROCK_TUNNEL_B1F */) ? 6 : 0;
        Map_Load(wCurMap);
        NPC_Load();
        PalletScripts_OnMapLoad();
        OaksLabScripts_OnMapLoad();
        Display_LoadMapPalette();
        printf("[cli] teleport → map %d (%d,%d)\n", map_id, x, y);
        write_state();
        return;
    }
    else if (strcmp(verb, "givebadge") == 0) {
        /* givebadge <n>  — set badge bit n (0=Boulder … 7=Earth) */
        char tok[16] = {0};
        sscanf(cmd, "%*s %15s", tok);
        int n = (int)strtol(tok, NULL, 0);
        if (n < 0 || n > 7) {
            printf("[cli] givebadge: n must be 0-7\n");
        } else {
            wObtainedBadges |= (uint8_t)(1u << n);
            printf("[cli] givebadge %d → wObtainedBadges=0x%02X\n", n, wObtainedBadges);
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "quicksave") == 0) {
        char key[96] = {0};
        char path[160] = {0};
        if (!cli_parse_arg(cmd, 1, key, sizeof(key))) strcpy(key, "1");
        cli_build_state_path(key, path, sizeof(path));
        if (cli_backup_state_if_exists(path) != 0)
            printf("[cli] quicksave backup failed (continuing): %s\n", path);
        if (Save_StateWrite(path) == 0)
            printf("[cli] quicksave %s -> %s\n", key, path);
        else
            printf("[cli] quicksave failed: %s\n", path);
        write_state();
        return;
    }
    else if (strcmp(verb, "quickload") == 0) {
        char key[96] = {0};
        char path[160] = {0};
        if (!cli_parse_arg(cmd, 1, key, sizeof(key))) strcpy(key, "1");
        cli_build_state_path(key, path, sizeof(path));
        if (Save_StateLoad(path) == 0) {
            cli_reload_after_state_load();
            printf("[cli] quickload %s <- %s\n", key, path);
        } else {
            printf("[cli] quickload failed: %s\n", path);
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "csave") == 0) {
        char sub[32] = {0};
        char name[96] = {0};
        char path[160] = {0};
        if (!cli_parse_arg(cmd, 1, sub, sizeof(sub))) {
            printf("[cli] csave usage: csave create <name> | csave load <name>\n");
            write_state();
            return;
        }
        if (!cli_parse_arg(cmd, 2, name, sizeof(name))) {
            printf("[cli] csave: missing name\n");
            write_state();
            return;
        }
        cli_build_state_path(name, path, sizeof(path));
        if (strcmp(sub, "create") == 0 || strcmp(sub, "save") == 0) {
            if (cli_backup_state_if_exists(path) != 0)
                printf("[cli] csave backup failed (continuing): %s\n", path);
            if (Save_StateWrite(path) == 0)
                printf("[cli] csave create %s -> %s\n", name, path);
            else
                printf("[cli] csave create failed: %s\n", path);
        } else if (strcmp(sub, "load") == 0) {
            if (Save_StateLoad(path) == 0) {
                cli_reload_after_state_load();
                printf("[cli] csave load %s <- %s\n", name, path);
            } else {
                printf("[cli] csave load failed: %s\n", path);
            }
        } else {
            printf("[cli] csave usage: csave create <name> | csave load <name>\n");
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "replay") == 0) {
        char sub[32] = {0};
        char name[96] = {0};
        char final_path[192] = {0};
        if (!cli_parse_arg(cmd, 1, sub, sizeof(sub))) {
            printf("[cli] replay usage: replay record <name> | replay stop | replay play <name> | replay status\n");
            write_state();
            return;
        }
        if (strcmp(sub, "record") == 0) {
            if (!cli_parse_arg(cmd, 2, name, sizeof(name))) {
                printf("[cli] replay record: missing name\n");
            } else if (replay_start_record(name) == 0) {
                printf("[cli] replay record: started '%s'\n", name);
            } else {
                printf("[cli] replay record: failed\n");
            }
        } else if (strcmp(sub, "stop") == 0) {
            int did = 0;
            if (s_replay_recording) {
                if (replay_stop_record(final_path, sizeof(final_path)) == 0)
                    printf("[cli] replay stop: saved %s\n", final_path);
                else
                    printf("[cli] replay stop: failed to finalize recording\n");
                did = 1;
            }
            if (s_replay_playing) {
                replay_reset_playback();
                printf("[cli] replay stop: playback stopped\n");
                did = 1;
            }
            if (!did) printf("[cli] replay stop: nothing active\n");
        } else if (strcmp(sub, "play") == 0) {
            if (!cli_parse_arg(cmd, 2, name, sizeof(name))) {
                printf("[cli] replay play: missing name\n");
            } else if (replay_start_play(name) == 0) {
                printf("[cli] replay play: started '%s' (%lu frames)\n",
                       name, (unsigned long)s_replay_play_len);
            } else {
                printf("[cli] replay play: failed for '%s'\n", name);
            }
        } else if (strcmp(sub, "status") == 0) {
            printf("[cli] replay status: record=%s play=%s frame=%lu/%lu\n",
                   s_replay_recording ? "ON" : "OFF",
                   s_replay_playing ? "ON" : "OFF",
                   (unsigned long)s_replay_play_pos,
                   (unsigned long)s_replay_play_len);
        } else {
            printf("[cli] replay usage: replay record <name> | replay stop | replay play <name> | replay status\n");
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "setflag") == 0) {
        /* setflag <n> — set event flag n (decimal or 0x hex) */
        char tok[16] = {0};
        sscanf(cmd, "%*s %15s", tok);
        int flag = (int)strtol(tok, NULL, 0);
        SetEvent((uint16_t)flag);
        printf("[cli] setflag %d → set\n", flag);
        write_state();
        return;
    }
    else if (strcmp(verb, "clearflag") == 0) {
        /* clearflag <n> — clear event flag n (decimal or 0x hex) */
        char tok[16] = {0};
        sscanf(cmd, "%*s %15s", tok);
        int flag = (int)strtol(tok, NULL, 0);
        ClearEvent((uint16_t)flag);
        printf("[cli] clearflag %d → cleared\n", flag);
        write_state();
        return;
    }
    else if (strcmp(verb, "battle_seed") == 0) {
        char tok[32] = {0};
        uint8_t seed;
        if (!cli_parse_arg(cmd, 1, tok, sizeof(tok))) {
            printf("[cli] battle_seed usage: battle_seed <0-255>\n");
            write_state();
            return;
        }
        seed = (uint8_t)strtol(tok, NULL, 0);
        hRandomAdd = seed;
        hRandomSub = (uint8_t)~seed;
        printf("[cli] battle_seed: hRandomAdd=0x%02X hRandomSub=0x%02X\n", hRandomAdd, hRandomSub);
        write_state();
        return;
    }
    else if (strcmp(verb, "rng_state") == 0) {
        printf("[cli] rng_state: add=0x%02X sub=0x%02X frame=%u\n",
               hRandomAdd, hRandomSub, (unsigned)hFrameCounter);
        write_state();
        return;
    }
    else if (strcmp(verb, "sprint_holdb") == 0) {
        char sub[16] = {0};
        if (!cli_parse_arg(cmd, 1, sub, sizeof(sub))) strcpy(sub, "status");
        if (strcmp(sub, "on") == 0 || strcmp(sub, "1") == 0) {
            Player_SetHoldBSprintEnabled(1);
            printf("[cli] sprint_holdb: ON (hold X/B to move 2x)\n");
        } else if (strcmp(sub, "off") == 0 || strcmp(sub, "0") == 0) {
            Player_SetHoldBSprintEnabled(0);
            printf("[cli] sprint_holdb: OFF\n");
        } else if (strcmp(sub, "status") == 0) {
            printf("[cli] sprint_holdb: %s\n",
                   Player_GetHoldBSprintEnabled() ? "ON" : "OFF");
        } else {
            printf("[cli] sprint_holdb usage: sprint_holdb on|off|status\n");
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "npc_walkoff") == 0) {
        debugcli_start_npc_walkoff(1);
        write_state();
        return;
    }
    else if (strcmp(verb, "dsl_bank") == 0) {
        char sub[16] = {0};
        if (!cli_parse_arg(cmd, 1, sub, sizeof(sub))) strcpy(sub, "status");
        if (strcmp(sub, "on") == 0) {
            s_dsl_bank_enabled = 1;
            dsl_bank_load();
            dsl_bank_ensure_current_map_spawns();
            s_dsl_bank_last_map = wCurMap;
            dsl_bank_write_cfg();
            dsl_bank_save();
            printf("[cli] dsl_bank: ON (persisting scene_npc bindings)\n");
        } else if (strcmp(sub, "off") == 0) {
            s_dsl_bank_enabled = 0;
            dsl_bank_write_cfg();
            printf("[cli] dsl_bank: OFF\n");
        } else if (strcmp(sub, "save") == 0) {
            dsl_bank_save();
            printf("[cli] dsl_bank: saved\n");
        } else if (strcmp(sub, "load") == 0) {
            dsl_bank_load();
            dsl_bank_ensure_current_map_spawns();
            s_dsl_bank_last_map = wCurMap;
            printf("[cli] dsl_bank: loaded\n");
        } else if (strcmp(sub, "clear") == 0) {
            for (int pi = 0; pi < (int)(sizeof(kDslBankDataPaths) / sizeof(kDslBankDataPaths[0])); pi++)
                remove(kDslBankDataPaths[pi]);
            memset(s_scene_npc_bindings, 0, sizeof(s_scene_npc_bindings));
            printf("[cli] dsl_bank: cleared\n");
        } else if (strcmp(sub, "status") == 0) {
            int cnt = 0;
            for (int i = 0; i < SCENE_ACTOR_MAX; i++) if (s_scene_npc_bindings[i].used) cnt++;
            printf("[cli] dsl_bank: %s, bindings=%d\n", s_dsl_bank_enabled ? "ON" : "OFF", cnt);
        } else {
            printf("[cli] dsl_bank usage: dsl_bank on|off|status|save|load|clear\n");
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "scene_npc") == 0 ||
             strcmp(verb, "scene-npc") == 0 ||
             strcmp(verb, "scenenpc") == 0 ||
             strcmp(verb, "npcscene") == 0) {
        char sub[16] = {0};
        if (!cli_parse_arg(cmd, 1, sub, sizeof(sub))) {
            printf("[cli] scene_npc usage:\n");
            printf("      scene_npc add <name> <scene> <sprite> <x_expr> <y_expr>\n");
            printf("      scene_npc list\n");
            printf("      scene_npc clear [name]\n");
            write_state();
            return;
        }
        if (strcmp(sub, "add") == 0) {
            char name[24] = {0}, scene[64] = {0}, sprite[32] = {0}, xexpr[32] = {0}, yexpr[32] = {0};
            int x = 0, y = 0, sprite_id = -1, idx = -1, slot = -1, ncmd;
            if (!cli_parse_arg(cmd, 2, name, sizeof(name)) ||
                !cli_parse_arg(cmd, 3, scene, sizeof(scene)) ||
                !cli_parse_arg(cmd, 4, sprite, sizeof(sprite)) ||
                !cli_parse_arg(cmd, 5, xexpr, sizeof(xexpr)) ||
                !cli_parse_arg(cmd, 6, yexpr, sizeof(yexpr))) {
                printf("[cli] scene_npc add usage: scene_npc add <name> <scene> <sprite> <x_expr> <y_expr>\n");
                write_state();
                return;
            }
            if (!scene_parse_coord_expr(xexpr, 1, &x) || !scene_parse_coord_expr(yexpr, 0, &y)) {
                printf("[cli] scene_npc add: bad coordinate expression(s)\n");
                write_state();
                return;
            }
            sprite_id = scene_parse_sprite(sprite);
            if (sprite_id < 0) {
                printf("[cli] scene_npc add: bad sprite '%s'\n", sprite);
                write_state();
                return;
            }
            ncmd = scene_load_file(scene);
            if (ncmd <= 0) {
                printf("[cli] scene_npc add: scene '%s' not found or empty\n", scene);
                write_state();
                return;
            }
            idx = NPC_DebugSpawn((uint8_t)sprite_id, x, y, 0, 0);
            if (idx < 0) {
                printf("[cli] scene_npc add: failed to spawn NPC\n");
                write_state();
                return;
            }
            slot = scene_npc_binding_alloc();
            if (slot < 0) {
                printf("[cli] scene_npc add: no free bindings\n");
                NPC_DebugDespawn(idx);
                write_state();
                return;
            }
            memset(&s_scene_npc_bindings[slot], 0, sizeof(s_scene_npc_bindings[slot]));
            s_scene_npc_bindings[slot].used = 1;
            s_scene_npc_bindings[slot].npc_idx = idx;
            s_scene_npc_bindings[slot].map_id = wCurMap;
            s_scene_npc_bindings[slot].sprite_id = (uint8_t)sprite_id;
            s_scene_npc_bindings[slot].tile_x = x;
            s_scene_npc_bindings[slot].tile_y = y;
            snprintf(s_scene_npc_bindings[slot].name, sizeof(s_scene_npc_bindings[slot].name), "%s", name);
            snprintf(s_scene_npc_bindings[slot].scene, sizeof(s_scene_npc_bindings[slot].scene), "%s", scene);
            printf("[cli] scene_npc add: '%s' idx=%d map=%u (%d,%d) -> scene '%s'\n",
                   name, idx, (unsigned)wCurMap, x, y, scene);
            if (s_dsl_bank_enabled) dsl_bank_save();
        } else if (strcmp(sub, "list") == 0) {
            int any = 0;
            for (int i = 0; i < SCENE_ACTOR_MAX; i++) {
                if (!s_scene_npc_bindings[i].used) continue;
                any = 1;
                printf("[cli] scene_npc[%d]: name='%s' idx=%d map=%u tile=(%d,%d) sprite=0x%02X scene='%s'\n",
                       i, s_scene_npc_bindings[i].name, s_scene_npc_bindings[i].npc_idx,
                       (unsigned)s_scene_npc_bindings[i].map_id,
                       s_scene_npc_bindings[i].tile_x, s_scene_npc_bindings[i].tile_y,
                       (unsigned)s_scene_npc_bindings[i].sprite_id,
                       s_scene_npc_bindings[i].scene);
            }
            if (!any) printf("[cli] scene_npc list: empty\n");
        } else if (strcmp(sub, "clear") == 0) {
            char name[24] = {0};
            int cleared = 0;
            if (!cli_parse_arg(cmd, 2, name, sizeof(name))) {
                for (int i = 0; i < SCENE_ACTOR_MAX; i++) {
                    if (!s_scene_npc_bindings[i].used) continue;
                    NPC_DebugDespawn(s_scene_npc_bindings[i].npc_idx);
                    memset(&s_scene_npc_bindings[i], 0, sizeof(s_scene_npc_bindings[i]));
                    cleared++;
                }
                printf("[cli] scene_npc clear: cleared all (%d)\n", cleared);
            } else {
                for (int i = 0; i < SCENE_ACTOR_MAX; i++) {
                    if (!s_scene_npc_bindings[i].used) continue;
                    if (strcmp(s_scene_npc_bindings[i].name, name) != 0) continue;
                    NPC_DebugDespawn(s_scene_npc_bindings[i].npc_idx);
                    memset(&s_scene_npc_bindings[i], 0, sizeof(s_scene_npc_bindings[i]));
                    cleared++;
                }
                printf("[cli] scene_npc clear: name='%s' cleared=%d\n", name, cleared);
            }
            if (s_dsl_bank_enabled) dsl_bank_save();
        } else {
            printf("[cli] scene_npc usage:\n");
            printf("      scene_npc add <name> <scene> <sprite> <x_expr> <y_expr>\n");
            printf("      scene_npc list\n");
            printf("      scene_npc clear [name]\n");
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "script_trace") == 0) {
        char sub[16] = {0};
        if (!cli_parse_arg(cmd, 1, sub, sizeof(sub))) strcpy(sub, "status");
        if (strcmp(sub, "on") == 0) {
            s_script_trace_enabled = 1;
            s_script_trace_to_file = 1;
            cli_script_trace_reset_latches();
            printf("[cli] script_trace: ON (file=%s)\n", SCRIPT_TRACE_LOG_PATH);
        } else if (strcmp(sub, "off") == 0) {
            s_script_trace_enabled = 0;
            s_script_trace_to_file = 0;
            printf("[cli] script_trace: OFF\n");
        } else if (strcmp(sub, "file_on") == 0) {
            s_script_trace_to_file = 1;
            printf("[cli] script_trace: file logging ON (%s)\n", SCRIPT_TRACE_LOG_PATH);
        } else if (strcmp(sub, "file_off") == 0) {
            s_script_trace_to_file = 0;
            printf("[cli] script_trace: file logging OFF\n");
        } else if (strcmp(sub, "status") == 0) {
            printf("[cli] script_trace: %s, file=%s\n",
                   s_script_trace_enabled ? "ON" : "OFF",
                   s_script_trace_to_file ? "ON" : "OFF");
        } else {
            printf("[cli] script_trace usage: script_trace on|off|status|file_on|file_off\n");
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "scene_run") == 0) {
        char name[64] = {0};
        int ncmd;
        if (!cli_parse_arg(cmd, 1, name, sizeof(name))) {
            printf("[cli] scene_run usage: scene_run <name>  (loads debug/scenes/<name>.scene)\n");
            write_state();
            return;
        }
        scene_reset_runtime();
        ncmd = scene_load_file(name);
        if (ncmd <= 0) {
            printf("[cli] scene_run: failed to load scene '%s'\n", name);
        } else {
            s_scene_active = 1;
            s_scene_pc = 0;
            printf("[cli] scene_run: loaded '%s' (%d command(s))\n", name, ncmd);
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "scene_trigger") == 0) {
        char sub[16] = {0};
        if (!cli_parse_arg(cmd, 1, sub, sizeof(sub))) {
            printf("[cli] scene_trigger usage:\n");
            printf("      scene_trigger set <scene> trigger_point <x_expr> <y_expr> [map]\n");
            printf("      scene_trigger set <scene> trigger_point <x_expr> <y_expr> when event_set|event_clear <event> [map]\n");
            printf("      scene_trigger list\n");
            printf("      scene_trigger clear [scene]\n");
            printf("      x_expr/y_expr: number or player.x[+/-n], player.y[+/-n]\n");
            write_state();
            return;
        }
        if (strcmp(sub, "set") == 0) {
            char scene[64] = {0}, marker[24] = {0}, xexpr[32] = {0}, yexpr[32] = {0}, tok6[64] = {0}, maptok[64] = {0};
            int map_id = (int)wCurMap;
            int x = 0, y = 0;
            int ncmd, slot;
            uint8_t cond_kind = 0;
            uint16_t cond_event = 0;
            if (!cli_parse_arg(cmd, 2, scene, sizeof(scene)) ||
                !cli_parse_arg(cmd, 3, marker, sizeof(marker)) ||
                !cli_parse_arg(cmd, 4, xexpr, sizeof(xexpr)) ||
                !cli_parse_arg(cmd, 5, yexpr, sizeof(yexpr))) {
                printf("[cli] scene_trigger set usage: scene_trigger set <scene> trigger_point <x_expr> <y_expr> [map]\n");
                write_state();
                return;
            }
            if (strcmp(marker, "trigger_point") != 0) {
                printf("[cli] scene_trigger set: expected marker 'trigger_point'\n");
                write_state();
                return;
            }
            if (!scene_parse_coord_expr(xexpr, 1, &x) || !scene_parse_coord_expr(yexpr, 0, &y)) {
                printf("[cli] scene_trigger set: bad coordinate expression(s)\n");
                write_state();
                return;
            }
            if (cli_parse_arg(cmd, 6, tok6, sizeof(tok6))) {
                if (strcmp(tok6, "when") == 0 || strcmp(tok6, "if") == 0) {
                    char condtok[24] = {0}, eventtok[96] = {0}, mapafter[64] = {0};
                    if (!cli_parse_arg(cmd, 7, condtok, sizeof(condtok)) ||
                        !cli_parse_arg(cmd, 8, eventtok, sizeof(eventtok))) {
                        printf("[cli] scene_trigger set: usage ... when event_set|event_clear <event> [map]\n");
                        write_state();
                        return;
                    }
                    if (strcmp(condtok, "event_set") == 0 || strcmp(condtok, "set") == 0) cond_kind = 1;
                    else if (strcmp(condtok, "event_clear") == 0 || strcmp(condtok, "clear") == 0) cond_kind = 2;
                    else {
                        printf("[cli] scene_trigger set: expected event_set or event_clear\n");
                        write_state();
                        return;
                    }
                    if (!cli_resolve_event_token(eventtok, &cond_event)) {
                        printf("[cli] scene_trigger set: unknown event '%s'\n", eventtok);
                        write_state();
                        return;
                    }
                    if (cli_parse_arg(cmd, 9, mapafter, sizeof(mapafter))) {
                        if (!cli_resolve_map_token(mapafter, &map_id)) {
                            printf("[cli] scene_trigger set: unknown map '%s'\n", mapafter);
                            write_state();
                            return;
                        }
                    }
                } else {
                    snprintf(maptok, sizeof(maptok), "%s", tok6);
                    if (!cli_resolve_map_token(maptok, &map_id)) {
                        printf("[cli] scene_trigger set: unknown map '%s'\n", maptok);
                        write_state();
                        return;
                    }
                }
            }
            /* Validate scene exists now so trigger failures are obvious. */
            ncmd = scene_load_file(scene);
            if (ncmd <= 0) {
                printf("[cli] scene_trigger set: scene '%s' not found or empty\n", scene);
                write_state();
                return;
            }
            slot = scene_trigger_find_slot(scene, map_id, x, y);
            if (slot < 0) slot = scene_trigger_alloc_slot();
            if (slot < 0) {
                printf("[cli] scene_trigger set: no free trigger slots (max %d)\n", SCENE_TRIGGER_MAX);
                write_state();
                return;
            }
            s_scene_triggers[slot].used = 1;
            snprintf(s_scene_triggers[slot].scene, sizeof(s_scene_triggers[slot].scene), "%s", scene);
            s_scene_triggers[slot].map_id = (uint8_t)map_id;
            s_scene_triggers[slot].x = x;
            s_scene_triggers[slot].y = y;
            s_scene_triggers[slot].armed = 1;
            s_scene_triggers[slot].cond_kind = cond_kind;
            s_scene_triggers[slot].cond_event = cond_event;
            if (cond_kind == 1) {
                printf("[cli] scene_trigger set: '%s' @ map=%u (%d,%d) when event_set %s (%u)\n",
                       scene, (unsigned)map_id, x, y, EventFlagName(cond_event), (unsigned)cond_event);
            } else if (cond_kind == 2) {
                printf("[cli] scene_trigger set: '%s' @ map=%u (%d,%d) when event_clear %s (%u)\n",
                       scene, (unsigned)map_id, x, y, EventFlagName(cond_event), (unsigned)cond_event);
            } else {
                printf("[cli] scene_trigger set: '%s' @ map=%u (%d,%d)\n", scene, (unsigned)map_id, x, y);
            }
        } else if (strcmp(sub, "list") == 0) {
            int any = 0;
            for (int i = 0; i < SCENE_TRIGGER_MAX; i++) {
                if (!s_scene_triggers[i].used) continue;
                any = 1;
                printf("[cli] scene_trigger[%d]: '%s' map=%u (%d,%d) armed=%d",
                       i, s_scene_triggers[i].scene, (unsigned)s_scene_triggers[i].map_id,
                       s_scene_triggers[i].x, s_scene_triggers[i].y, s_scene_triggers[i].armed);
                if (s_scene_triggers[i].cond_kind == 1) {
                    printf(" when event_set %s (%u)",
                           EventFlagName(s_scene_triggers[i].cond_event),
                           (unsigned)s_scene_triggers[i].cond_event);
                } else if (s_scene_triggers[i].cond_kind == 2) {
                    printf(" when event_clear %s (%u)",
                           EventFlagName(s_scene_triggers[i].cond_event),
                           (unsigned)s_scene_triggers[i].cond_event);
                }
                printf("\n");
            }
            if (!any) printf("[cli] scene_trigger list: empty\n");
        } else if (strcmp(sub, "clear") == 0) {
            char scene[64] = {0};
            int cleared = 0;
            if (!cli_parse_arg(cmd, 2, scene, sizeof(scene))) {
                memset(s_scene_triggers, 0, sizeof(s_scene_triggers));
                printf("[cli] scene_trigger clear: cleared all trigger points\n");
                write_state();
                return;
            }
            for (int i = 0; i < SCENE_TRIGGER_MAX; i++) {
                if (s_scene_triggers[i].used && strcmp(s_scene_triggers[i].scene, scene) == 0) {
                    memset(&s_scene_triggers[i], 0, sizeof(s_scene_triggers[i]));
                    cleared++;
                }
            }
            printf("[cli] scene_trigger clear: cleared %d trigger(s) for '%s'\n", cleared, scene);
        } else {
            printf("[cli] scene_trigger usage:\n");
            printf("      scene_trigger set <scene> trigger_point <x_expr> <y_expr> [map]\n");
            printf("      scene_trigger set <scene> trigger_point <x_expr> <y_expr> when event_set|event_clear <event> [map]\n");
            printf("      scene_trigger list\n");
            printf("      scene_trigger clear [scene]\n");
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "scene_stop") == 0) {
        wJoyIgnore = 0;
        scene_reset_runtime();
        printf("[cli] scene_stop: stopped scene runner\n");
        write_state();
        return;
    }
    else if (strcmp(verb, "story_guard") == 0) {
        char name[32] = {0};
        int missing = 0;
#define GUARD_EVENT(ev, label) do { if (!CheckEvent((ev))) { printf("[story_guard] missing event: %s (%u)\n", (label), (unsigned)(ev)); missing++; } } while (0)
#define GUARD_BADGE(bit, label) do { if (!(wObtainedBadges & (1u << (bit)))) { printf("[story_guard] missing badge: %s\n", (label)); missing++; } } while (0)
        if (!cli_parse_arg(cmd, 1, name, sizeof(name))) {
            printf("[cli] story_guard usage: story_guard <brock|misty|surge|erika|koga|blaine|bike_gate|list>\n");
            write_state();
            return;
        }
        if (strcmp(name, "list") == 0 || strcmp(name, "help") == 0) {
            printf("[cli] story_guard targets: brock, misty, surge, erika, koga, blaine, bike_gate\n");
            write_state();
            return;
        } else if (strcmp(name, "brock") == 0) {
            GUARD_EVENT(EVENT_GOT_POKEDEX, "EVENT_GOT_POKEDEX");
        } else if (strcmp(name, "misty") == 0) {
            GUARD_EVENT(EVENT_GOT_POKEDEX, "EVENT_GOT_POKEDEX");
            GUARD_EVENT(EVENT_BEAT_BROCK, "EVENT_BEAT_BROCK");
            GUARD_BADGE(BADGE_BOULDER, "BOULDER");
        } else if (strcmp(name, "surge") == 0) {
            GUARD_EVENT(EVENT_BEAT_MISTY, "EVENT_BEAT_MISTY");
            GUARD_BADGE(BADGE_CASCADE, "CASCADE");
            GUARD_EVENT(EVENT_GOT_HM01, "EVENT_GOT_HM01");
        } else if (strcmp(name, "erika") == 0) {
            GUARD_EVENT(EVENT_BEAT_LT_SURGE, "EVENT_BEAT_LT_SURGE");
            GUARD_BADGE(BADGE_THUNDER, "THUNDER");
        } else if (strcmp(name, "koga") == 0) {
            GUARD_EVENT(EVENT_BEAT_ERIKA, "EVENT_BEAT_ERIKA");
            GUARD_BADGE(BADGE_RAINBOW, "RAINBOW");
        } else if (strcmp(name, "blaine") == 0) {
            GUARD_EVENT(EVENT_BEAT_KOGA, "EVENT_BEAT_KOGA");
            GUARD_BADGE(BADGE_SOUL, "SOUL");
        } else if (strcmp(name, "bike_gate") == 0) {
            if (Inventory_GetQty(ITEM_BICYCLE) == 0) {
                printf("[story_guard] missing item: BICYCLE\n");
                missing++;
            }
        } else {
            printf("[cli] story_guard: unknown target '%s'\n", name);
            write_state();
            return;
        }
        if (missing == 0) printf("[story_guard] %s: OK\n", name);
        else printf("[story_guard] %s: %d prerequisite(s) missing\n", name, missing);
#undef GUARD_EVENT
#undef GUARD_BADGE
        write_state();
        return;
    }
    else if (strcmp(verb, "eventdiff") == 0) {
        char sub[16] = {0};
        if (!cli_parse_arg(cmd, 1, sub, sizeof(sub))) strcpy(sub, "show");
        if (strcmp(sub, "snapshot") == 0 || strcmp(sub, "snap") == 0) {
            s_eventdiff.valid = 1;
            s_eventdiff.badges = wObtainedBadges;
            s_eventdiff.map = wCurMap;
            s_eventdiff.x = wXCoord;
            s_eventdiff.y = wYCoord;
            s_eventdiff.party_count = wPartyCount;
            for (int i = 0; i < 15; i++)
                s_eventdiff.key_flags[i] = (uint8_t)CheckEvent(s_eventdiff_keys[i]);
            printf("[cli] eventdiff: snapshot captured (map=%u pos=%u,%u badges=0x%02X)\n",
                   s_eventdiff.map, s_eventdiff.x, s_eventdiff.y, s_eventdiff.badges);
        } else if (strcmp(sub, "show") == 0 || strcmp(sub, "diff") == 0) {
            if (!s_eventdiff.valid) {
                printf("[cli] eventdiff: no snapshot; run 'eventdiff snapshot'\n");
            } else {
                printf("[cli] eventdiff: map %u->%u, pos (%u,%u)->(%u,%u), badges 0x%02X->0x%02X, party %u->%u\n",
                       s_eventdiff.map, wCurMap, s_eventdiff.x, s_eventdiff.y, wXCoord, wYCoord,
                       s_eventdiff.badges, wObtainedBadges, s_eventdiff.party_count, wPartyCount);
                for (int i = 0; i < 15; i++) {
                    uint8_t now = (uint8_t)CheckEvent(s_eventdiff_keys[i]);
                    if (now != s_eventdiff.key_flags[i]) {
                        printf("  flag %u: %u -> %u\n",
                               (unsigned)s_eventdiff_keys[i], s_eventdiff.key_flags[i], now);
                    }
                }
            }
        } else {
            printf("[cli] eventdiff usage: eventdiff snapshot | eventdiff show\n");
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "trainer_reset") == 0) {
        char tok[64] = {0};
        char mode[8] = {0};
        if (!cli_parse_arg(cmd, 1, tok, sizeof(tok))) strcpy(tok, "here");
        (void)cli_parse_arg(cmd, 2, mode, sizeof(mode));
        if (cli_is_numeric_token(tok) && (strcmp(mode, "set") == 0 || strcmp(mode, "clear") == 0 || strcmp(mode, "1") == 0 || strcmp(mode, "0") == 0)) {
            int flag = (int)strtol(tok, NULL, 0);
            if (strcmp(mode, "set") == 0 || strcmp(mode, "1") == 0) {
                SetEvent((uint16_t)flag);
                printf("[cli] trainer_reset: set event %d (trainer beaten)\n", flag);
            } else {
                ClearEvent((uint16_t)flag);
                printf("[cli] trainer_reset: cleared event %d (trainer unbeaten)\n", flag);
            }
        } else {
            int map_id = -1;
            int cleared = 0;
            if (!cli_resolve_map_token(tok, &map_id)) {
            printf("[cli] trainer_reset: unknown map '%s' (use map id, map name, or 'here')\n", tok);
                write_state();
                return;
            }
            if (map_id < 0 || map_id >= NUM_MAPS) {
                printf("[cli] trainer_reset: map out of range (%d)\n", map_id);
                write_state();
                return;
            }
            {
                const map_events_t *ev = &gMapEvents[map_id];
                for (int i = 0; i < ev->num_trainers; i++) {
                    ClearEvent(ev->trainers[i].flag_bit);
                    cleared++;
                }
            }
            if (wCurMap == (uint8_t)map_id) {
                NPC_Load();
                Trainer_LoadMap();
            }
            printf("[cli] trainer_reset: cleared %d trainer flag(s) on map %d (%s)\n",
                   cleared, map_id, gMapTable[map_id].name);
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "gym_reset") == 0) {
        char gym[16] = {0};
        if (!cli_parse_arg(cmd, 1, gym, sizeof(gym))) {
            printf("[cli] gym_reset usage: gym_reset <brock|misty|surge|erika|koga|blaine|all>\n");
            write_state();
            return;
        }
        if (strcmp(gym, "all") == 0) {
            cli_gym_clear("brock");
            cli_gym_clear("misty");
            cli_gym_clear("surge");
            cli_gym_clear("erika");
            cli_gym_clear("koga");
            cli_gym_clear("blaine");
            printf("[cli] gym_reset: cleared Brock..Blaine leader/trainer/TM progress\n");
        } else {
            cli_gym_clear(gym);
            if (strcmp(gym, "brock") == 0) { wCurMap = 0x36; wXCoord = 4; wYCoord = 2; }
            else if (strcmp(gym, "misty") == 0) { wCurMap = 0x41; wXCoord = 4; wYCoord = 8; }
            else if (strcmp(gym, "surge") == 0) { wCurMap = 0x5C; wXCoord = 5; wYCoord = 3; }
            else if (strcmp(gym, "erika") == 0) { wCurMap = 0x87; wXCoord = 4; wYCoord = 12; }
            else if (strcmp(gym, "koga") == 0) { wCurMap = 0x9D; wXCoord = 4; wYCoord = 11; }
            else if (strcmp(gym, "blaine") == 0) { wCurMap = 0xA6; wXCoord = 3; wYCoord = 4; }
            else {
                printf("[cli] gym_reset: unknown gym '%s'\n", gym);
                write_state();
                return;
            }
            Map_Load(wCurMap); NPC_Load(); Trainer_LoadMap();
            printf("[cli] gym_reset: cleared %s progress and teleported to leader\n", gym);
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "tmflow") == 0) {
        char gym[16] = {0};
        char state[16] = {0};
        if (!cli_parse_arg(cmd, 1, gym, sizeof(gym)) || !cli_parse_arg(cmd, 2, state, sizeof(state))) {
            printf("[cli] tmflow usage: tmflow <brock|misty|surge|erika|koga|blaine> <pre|post|done|reset>\n");
            write_state();
            return;
        }
        if (strcmp(state, "reset") == 0 || strcmp(state, "pre") == 0) {
            cli_gym_clear(gym);
            printf("[cli] tmflow: %s set to pre-battle/reset state\n", gym);
        } else {
            if (strcmp(gym, "brock") == 0) {
                SetEvent(EVENT_BEAT_BROCK); wObtainedBadges |= (1u << BADGE_BOULDER);
                if (strcmp(state, "done") == 0) SetEvent(EVENT_GOT_TM34); else ClearEvent(EVENT_GOT_TM34);
            } else if (strcmp(gym, "misty") == 0) {
                SetEvent(EVENT_BEAT_MISTY); wObtainedBadges |= (1u << BADGE_CASCADE);
                if (strcmp(state, "done") == 0) SetEvent(EVENT_GOT_TM11); else ClearEvent(EVENT_GOT_TM11);
            } else if (strcmp(gym, "surge") == 0) {
                SetEvent(EVENT_BEAT_LT_SURGE); wObtainedBadges |= (1u << BADGE_THUNDER);
                if (strcmp(state, "done") == 0) SetEvent(EVENT_GOT_TM24); else ClearEvent(EVENT_GOT_TM24);
            } else if (strcmp(gym, "erika") == 0) {
                SetEvent(EVENT_BEAT_ERIKA); wObtainedBadges |= (1u << BADGE_RAINBOW);
                if (strcmp(state, "done") == 0) SetEvent(EVENT_GOT_TM21); else ClearEvent(EVENT_GOT_TM21);
            } else if (strcmp(gym, "koga") == 0) {
                SetEvent(EVENT_BEAT_KOGA); wObtainedBadges |= (1u << BADGE_SOUL);
                if (strcmp(state, "done") == 0) SetEvent(EVENT_GOT_TM06); else ClearEvent(EVENT_GOT_TM06);
            } else if (strcmp(gym, "blaine") == 0) {
                SetEvent(EVENT_BEAT_BLAINE); wObtainedBadges |= (1u << BADGE_VOLCANO);
                if (strcmp(state, "done") == 0) SetEvent(EVENT_GOT_TM38); else ClearEvent(EVENT_GOT_TM38);
            }
            printf("[cli] tmflow: %s set to %s-TM state\n", gym, state);
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "gym_badges_clear") == 0) {
        int keep = 0;
        sscanf(cmd, "%*s %d", &keep);
        if (keep < 0) keep = 0;
        if (keep > 8) keep = 8;
        wObtainedBadges &= (uint8_t)((keep == 0) ? 0 : ((1u << keep) - 1u));
        if (keep <= 0) cli_gym_clear("brock");
        if (keep <= 1) cli_gym_clear("misty");
        if (keep <= 2) cli_gym_clear("surge");
        if (keep <= 3) cli_gym_clear("erika");
        if (keep <= 4) cli_gym_clear("koga");
        if (keep <= 5) cli_gym_clear("blaine");
        printf("[cli] gym_badges_clear: kept first %d badge(s), cleared later gym progress\n", keep);
        write_state();
        return;
    }
    else if (strcmp(verb, "giveitem") == 0) {
        /* giveitem <id|name> [qty] — add item to bag.
         * id may be decimal (70), hex (0x46), or a name (oaks_parcel). */
        static const struct { const char *name; uint8_t id; } kItems[] = {
            { "master_ball",   0x01 }, { "ultra_ball",    0x02 },
            { "great_ball",    0x03 }, { "poke_ball",     0x04 },
            { "town_map",      0x05 }, { "bicycle",       0x06 },
            { "pokedex",       0x09 }, { "moon_stone",    0x0A },
            { "antidote",      0x0B }, { "burn_heal",     0x0C },
            { "ice_heal",      0x0D }, { "awakening",     0x0E },
            { "parlyz_heal",   0x0F }, { "full_restore",  0x10 },
            { "max_potion",    0x11 }, { "hyper_potion",  0x12 },
            { "super_potion",  0x13 }, { "potion",        0x14 },
            { "escape_rope",   0x1D }, { "repel",         0x1E },
            { "fire_stone",    0x20 }, { "thunder_stone", 0x21 },
            { "water_stone",   0x22 }, { "rare_candy",    0x28 },
            { "poke_doll",     0x33 }, { "full_heal",     0x34 },
            { "revive",        0x35 }, { "max_revive",    0x36 },
            { "super_repel",   0x38 }, { "max_repel",     0x39 },
            { "oaks_parcel",   0x46 }, { "parcel",        0x46 },
            { "poke_flute",    0x49 }, { "pokeflute",     0x49 },
            { "hm01",          0xC4 }, { "tm01",          0xC9 },
            { NULL, 0 }
        };
        char id_str[32] = {0};
        int qty = 1;
        sscanf(cmd, "%*s %31s %d", id_str, &qty);
        if (qty < 1) qty = 1;

        int id = -1;
        /* Try name lookup first */
        char lc[32] = {0};
        for (int k = 0; id_str[k] && k < 31; k++)
            lc[k] = (id_str[k] >= 'A' && id_str[k] <= 'Z') ? id_str[k]+32 : id_str[k];
        for (int k = 0; kItems[k].name; k++) {
            if (strcmp(lc, kItems[k].name) == 0) { id = kItems[k].id; break; }
        }
        /* Fall back to numeric (handles 0x hex or decimal) */
        if (id < 0) id = (int)strtol(id_str, NULL, 0);

        if (id <= 0 || id > 255) {
            printf("[cli] giveitem: unknown item '%s'\n", id_str);
        } else if (Inventory_Add((uint8_t)id, (uint8_t)qty) == 0) {
            printf("[cli] giveitem 0x%02X x%d → added\n", id, qty);
        } else {
            printf("[cli] giveitem: bag full\n");
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "givetm") == 0) {
        /* givetm <n> — give TM n (1-50) */
        if (n < 1 || n > 50) {
            printf("[cli] givetm: n must be 1-50\n");
        } else {
            uint8_t id = (uint8_t)(TM01 + n - 1);
            if (Inventory_Add(id, 1) == 0)
                printf("[cli] givetm %d → TM%02d (0x%02X) added\n", n, n, id);
            else
                printf("[cli] givetm: bag full\n");
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "givehm") == 0) {
        /* givehm <n> — give HM n (1-5) */
        if (n < 1 || n > 5) {
            printf("[cli] givehm: n must be 1-5\n");
        } else {
            uint8_t id = (uint8_t)(HM01 + n - 1);
            if (Inventory_Add(id, 1) == 0)
                printf("[cli] givehm %d → HM%02d (0x%02X) added\n", n, n, id);
            else
                printf("[cli] givehm: bag full\n");
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "givemon") == 0) {
        /* givemon <dex 1-151> [level 1-100]
         * Appends a Pokémon to the party (or replaces slot 6 if full).
         * Uses national dex number — e.g. givemon 19 20 = Rattata Lv20. */
        int dex = 1, level = 20;
        sscanf(cmd, "%*s %d %d", &dex, &level);
        if (dex < 1 || dex > 151) {
            printf("[cli] givemon: dex must be 1-151\n");
        } else if (level < 1 || level > 100) {
            printf("[cli] givemon: level must be 1-100\n");
        } else {
            uint8_t species = gDexToSpecies[dex];
            int slot = (wPartyCount < 6) ? wPartyCount : 5;
            Pokemon_InitMon(&wPartyMons[slot], species, (uint8_t)level);
            if (wPartyCount < 6) wPartyCount++;
            printf("[cli] givemon: slot %d → %s Lv%d (species 0x%02X)\n",
                   slot + 1, Pokemon_GetName(dex), level, species);
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "bulba15") == 0 || strcmp(verb, "givemon_bulba15") == 0) {
        /* bulba15 — one-shot test setup:
         * slot 1 = Bulbasaur Lv15 with a strong grass/control moveset.
         * Preserves party slots 2-6 if present. */
        uint8_t species = gDexToSpecies[1]; /* Bulbasaur */
        Pokemon_InitMon(&wPartyMons[0], species, 15);
        if (wPartyCount < 1) wPartyCount = 1;

        /* Moves: Sleep Powder, Razor Leaf, Leech Seed, Vine Whip */
        wPartyMons[0].base.moves[0] = 79; /* SLEEP POWDER */
        wPartyMons[0].base.moves[1] = 75; /* RAZOR LEAF   */
        wPartyMons[0].base.moves[2] = 73; /* LEECH SEED   */
        wPartyMons[0].base.moves[3] = 22; /* VINE WHIP    */
        wPartyMons[0].base.pp[0] = gMoves[79].pp;
        wPartyMons[0].base.pp[1] = gMoves[75].pp;
        wPartyMons[0].base.pp[2] = gMoves[73].pp;
        wPartyMons[0].base.pp[3] = gMoves[22].pp;
        wPartyMons[0].base.hp = wPartyMons[0].max_hp;
        wPartyMons[0].base.status = 0;

        printf("[cli] bulba15: slot 1 -> BULBASAUR Lv15 (SLEEP POWDER / RAZOR LEAF / LEECH SEED / VINE WHIP)\n");
        write_state();
        return;
    }
    else if (strcmp(verb, "squirtle15") == 0 || strcmp(verb, "givemon_squirtle15") == 0) {
        /* squirtle15 — one-shot test setup:
         * slot 1 = Squirtle Lv15 with Bubble in moveset.
         * Preserves party slots 2-6 if present. */
        uint8_t species = gDexToSpecies[7]; /* Squirtle */
        Pokemon_InitMon(&wPartyMons[0], species, 15);
        if (wPartyCount < 1) wPartyCount = 1;

        /* Moves: Bubble, Water Gun, Tackle, Tail Whip */
        wPartyMons[0].base.moves[0] = 145; /* BUBBLE    */
        wPartyMons[0].base.moves[1] = 55;  /* WATER GUN */
        wPartyMons[0].base.moves[2] = 33;  /* TACKLE    */
        wPartyMons[0].base.moves[3] = 39;  /* TAIL WHIP */
        wPartyMons[0].base.pp[0] = gMoves[145].pp;
        wPartyMons[0].base.pp[1] = gMoves[55].pp;
        wPartyMons[0].base.pp[2] = gMoves[33].pp;
        wPartyMons[0].base.pp[3] = gMoves[39].pp;
        wPartyMons[0].base.hp = wPartyMons[0].max_hp;
        wPartyMons[0].base.status = 0;

        printf("[cli] squirtle15: slot 1 -> SQUIRTLE Lv15 (BUBBLE / WATER GUN / TACKLE / TAIL WHIP)\n");
        write_state();
        return;
    }
    else if (strcmp(verb, "teachmove") == 0) {
        /* teachmove <slot 1-6> <move_id hex or dec>
         * Teaches a move to the given party slot (fills first empty move slot,
         * or overwrites slot 0 if all are filled).
         * Example: teachmove 1 0x94   → teach Flash to slot 1 */
        int slot = 1;
        char move_str[16] = {0};
        sscanf(cmd, "%*s %d %15s", &slot, move_str);
        int move_id = (int)strtol(move_str, NULL, 0);
        slot--;  /* 1-based → 0-based */
        if (slot < 0 || slot >= (int)wPartyCount) {
            printf("[cli] teachmove: slot must be 1-%d\n", wPartyCount);
        } else if (move_id < 1 || move_id > 0xA5) {
            printf("[cli] teachmove: invalid move id 0x%02X\n", move_id);
        } else {
            uint8_t *moves = wPartyMons[slot].base.moves;
            int target = 0;
            for (int i = 0; i < 4; i++) {
                if (moves[i] == 0) { target = i; break; }
                if (i == 3) target = 0;  /* overwrite slot 0 if all filled */
            }
            moves[target] = (uint8_t)move_id;
            printf("[cli] teachmove: slot %d move[%d] = 0x%02X\n",
                   slot + 1, target, move_id);
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "teach") == 0) {
        /* teach <slot 1-6> <move_id|move_name>
         * Appends move to next empty slot; if all 4 full, overwrites slot 4.
         * Sets PP to move's base PP. No other restrictions.
         * Accepts numeric ID (hex or dec) or plain name (case-insensitive,
         * spaces/underscores ignored): e.g. "hyperbeam", "hyper_beam", "SURF" */
        int slot = 1;
        char move_str[32] = {0};
        sscanf(cmd, "%*s %d %31s", &slot, move_str);
        slot--;

        int move_id = cli_resolve_move_id(move_str);

        if (slot < 0 || slot >= (int)wPartyCount) {
            printf("[cli] teach: slot must be 1-%d\n", wPartyCount);
        } else if (move_id < 1 || move_id > 0xA5) {
            printf("[cli] teach: unknown move '%s'\n", move_str);
        } else {
            uint8_t *moves = wPartyMons[slot].base.moves;
            uint8_t *pp    = wPartyMons[slot].base.pp;
            int target = 3;  /* default: overwrite slot 4 */
            for (int i = 0; i < 4; i++) {
                if (moves[i] == 0) { target = i; break; }
            }
            moves[target] = (uint8_t)move_id;
            pp[target]    = gMoves[move_id].pp;
            printf("[cli] teach: slot %d move[%d] = 0x%02X (pp=%d)\n",
                   slot + 1, target, move_id, pp[target]);
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "movetestteam1") == 0) {
        /* movetestteam1 — force a 6-mon party with the first 24 move-animation
         * test moves in exact slot order:
         * mon1: moves 1-4, mon2: 5-8, ... mon6: 21-24 */
        static const char *const kMoveNames24[24] = {
            "ABSORB", "ACID", "ACID ARMOR", "AGILITY",
            "AMNESIA", "AURORA BEAM", "BARRAGE", "BARRIER",
            "BIDE", "BIND", "BITE", "BLIZZARD",
            "BODY SLAM", "BONE CLUB", "BONEMERANG", "BUBBLE",
            "BUBBLEBEAM", "CLAMP", "COMET PUNCH", "CONFUSE RAY",
            "CONFUSION", "CONSTRICT", "CONVERSION", "COUNTER"
        };
        uint8_t move_ids[24];
        int ok = 1;
        for (int i = 0; i < 24; i++) {
            int id = cli_resolve_move_id(kMoveNames24[i]);
            if (id < 1 || id > 0xA5) {
                printf("[cli] movetestteam1: failed to resolve move '%s'\n", kMoveNames24[i]);
                ok = 0;
                break;
            }
            move_ids[i] = (uint8_t)id;
        }
        if (!ok) {
            write_state();
            return;
        }

        wPartyCount = PARTY_LENGTH;
        for (int p = 0; p < PARTY_LENGTH; p++) {
            Pokemon_InitMon(&wPartyMons[p], STARTER1, 50);
            wPartyMons[p].base.status = 0;
            wPartyMons[p].base.hp = wPartyMons[p].max_hp;
            for (int m = 0; m < 4; m++) {
                uint8_t mid = move_ids[p * 4 + m];
                wPartyMons[p].base.moves[m] = mid;
                wPartyMons[p].base.pp[m] = gMoves[mid].pp;
            }
        }

        /* Keep active battle struct coherent if slot 1 is active. */
        if (wPlayerMonNumber == 0) {
            for (int m = 0; m < 4; m++) {
                wBattleMon.moves[m] = wPartyMons[0].base.moves[m];
                wBattleMon.pp[m] = wPartyMons[0].base.pp[m];
            }
        }

        printf("[cli] movetestteam1: set 6-mon party with 24 move-animation test moves (slots 1-24)\n");
        write_state();
        return;
    }
    else if (strcmp(verb, "movetestteam2") == 0) {
        /* movetestteam2 — force a 6-mon party with move-animation
         * test moves #25-48 in exact slot order:
         * mon1: moves 25-28, mon2: 29-32, ... mon6: 45-48 */
        static const char *const kMoveNames24[24] = {
            "CRABHAMMER", "CUT", "DEFENSE CURL", "DIG",
            "DISABLE", "DIZZY PUNCH", "DOUBLE KICK", "DOUBLE SLAP",
            "DOUBLE TEAM", "DOUBLE-EDGE", "DRAGON RAGE", "DREAM EATER",
            "DRILL PECK", "EARTHQUAKE", "EGG BOMB", "EMBER",
            "EXPLOSION", "FIRE BLAST", "FIRE PUNCH", "FIRE SPIN",
            "FISSURE", "FLAMETHROWER", "FLASH", "FLY"
        };
        static const uint8_t kSpecies[PARTY_LENGTH] = {
            SPECIES_BULBASAUR,
            SPECIES_CHARMANDER,
            SPECIES_SQUIRTLE,
            SPECIES_PIKACHU,
            SPECIES_EEVEE,
            SPECIES_DRATINI
        };
        uint8_t move_ids[24];
        int ok = 1;
        for (int i = 0; i < 24; i++) {
            int id = cli_resolve_move_id(kMoveNames24[i]);
            if (id < 1 || id > 0xA5) {
                printf("[cli] movetestteam2: failed to resolve move '%s'\n", kMoveNames24[i]);
                ok = 0;
                break;
            }
            move_ids[i] = (uint8_t)id;
        }
        if (!ok) {
            write_state();
            return;
        }

        wPartyCount = PARTY_LENGTH;
        for (int p = 0; p < PARTY_LENGTH; p++) {
            Pokemon_InitMon(&wPartyMons[p], kSpecies[p], 50);
            wPartyMons[p].base.status = 0;
            wPartyMons[p].base.hp = wPartyMons[p].max_hp;
            for (int m = 0; m < 4; m++) {
                uint8_t mid = move_ids[p * 4 + m];
                wPartyMons[p].base.moves[m] = mid;
                wPartyMons[p].base.pp[m] = gMoves[mid].pp;
            }
        }

        /* Keep active battle struct coherent if slot 1 is active. */
        if (wPlayerMonNumber == 0) {
            for (int m = 0; m < 4; m++) {
                wBattleMon.moves[m] = wPartyMons[0].base.moves[m];
                wBattleMon.pp[m] = wPartyMons[0].base.pp[m];
            }
        }

        printf("[cli] movetestteam2: set 6 unique mons with move-animation test moves (slots 25-48)\n");
        write_state();
        return;
    }
    else if (strcmp(verb, "sethealth") == 0) {
        /* sethealth <slot 1-6> <hp>  — set current HP; clamped to max_hp */
        int slot = 0, hp = 0;
        sscanf(cmd, "%*s %d %d", &slot, &hp);
        slot--;  /* 1-based → 0-based */
        if (slot < 0 || slot >= (int)wPartyCount) {
            printf("[cli] sethealth: slot must be 1-%d\n", wPartyCount);
        } else {
            if (hp < 0) hp = 0;
            if (hp > (int)wPartyMons[slot].max_hp) hp = (int)wPartyMons[slot].max_hp;
            wPartyMons[slot].base.hp = (uint16_t)hp;
            /* Sync active battle mon if in battle */
            if (slot == (int)wPlayerMonNumber) wBattleMon.hp = (uint16_t)hp;
            printf("[cli] sethealth: slot %d HP → %d/%d\n",
                   slot + 1, hp, (int)wPartyMons[slot].max_hp);
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "healparty") == 0) {
        /* healparty — full HP + clear status on all party mons */
        for (int i = 0; i < (int)wPartyCount; i++) {
            wPartyMons[i].base.hp     = wPartyMons[i].max_hp;
            wPartyMons[i].base.status = 0;
        }
        printf("[cli] healparty: all %d mons fully healed\n", wPartyCount);
        write_state();
        return;
    }
    else if (strcmp(verb, "setmoney") == 0) {
        /* setmoney <amount 0-999999> — set player money (stored as 3-byte BCD) */
        int amount = 0;
        sscanf(cmd, "%*s %d", &amount);
        if (amount < 0)      amount = 0;
        if (amount > 999999) amount = 999999;
        int a = amount;
        wPlayerMoney[2] = (uint8_t)((a % 10) | ((a / 10 % 10) << 4)); a /= 100;
        wPlayerMoney[1] = (uint8_t)((a % 10) | ((a / 10 % 10) << 4)); a /= 100;
        wPlayerMoney[0] = (uint8_t)((a % 10) | ((a / 10 % 10) << 4));
        printf("[cli] setmoney: $%d\n", amount);
        write_state();
        return;
    }
    else if (strcmp(verb, "listbag") == 0) {
        /* listbag — print all items currently in bag */
        printf("[cli] bag (%d items):\n", wNumBagItems);
        for (int i = 0; i < (int)wNumBagItems; i++) {
            uint8_t id  = wBagItems[i * 2];
            uint8_t qty = wBagItems[i * 2 + 1];
            char name[20];
            Inventory_DecodeASCII(id, name, sizeof(name));
            printf("  [%d] 0x%02X %-18s x%d\n", i + 1, id, name, qty);
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "checkpoint") == 0) {
        /* checkpoint <name> — set a batch of event flags for a story state.
         * Available checkpoints:
         *   parcel_ready   — has starter, battled rival, standing in Viridian
         *   pokedex_ready  — has parcel, standing outside Oak's Lab
         */
        char cp[32] = {0};
        sscanf(cmd, "%*s %31s", cp);

        if (strcmp(cp, "verify") == 0) {
            char target[32] = {0};
            char temp_path[160] = "bugs/cli_checkpoint_verify_tmp.state";
            if (!cli_parse_arg(cmd, 2, target, sizeof(target)) || target[0] == '\0') {
                printf("[cli] checkpoint verify usage: checkpoint verify <name>\n");
                write_state();
                return;
            }
            if (strcmp(target, "verify") == 0) {
                printf("[cli] checkpoint verify: refusing recursive target 'verify'\n");
                write_state();
                return;
            }
            if (Save_StateWrite(temp_path) != 0) {
                printf("[cli] checkpoint verify: failed to save temp state\n");
                write_state();
                return;
            }
            s_eventdiff.valid = 1;
            s_eventdiff.badges = wObtainedBadges;
            s_eventdiff.map = wCurMap;
            s_eventdiff.x = wXCoord;
            s_eventdiff.y = wYCoord;
            s_eventdiff.party_count = wPartyCount;
            for (int i = 0; i < 15; i++)
                s_eventdiff.key_flags[i] = (uint8_t)CheckEvent(s_eventdiff_keys[i]);

            {
                char runbuf[64];
                snprintf(runbuf, sizeof(runbuf), "checkpoint %s", target);
                process_cmd(runbuf);
            }

            printf("[cli] checkpoint verify %s:\n", target);
            printf("  map %u->%u, pos (%u,%u)->(%u,%u), badges 0x%02X->0x%02X, party %u->%u\n",
                   s_eventdiff.map, wCurMap, s_eventdiff.x, s_eventdiff.y, wXCoord, wYCoord,
                   s_eventdiff.badges, wObtainedBadges, s_eventdiff.party_count, wPartyCount);
            for (int i = 0; i < 15; i++) {
                uint8_t now = (uint8_t)CheckEvent(s_eventdiff_keys[i]);
                if (now != s_eventdiff.key_flags[i]) {
                    printf("  flag %u: %u -> %u\n",
                           (unsigned)s_eventdiff_keys[i], s_eventdiff.key_flags[i], now);
                }
            }

            if (Save_StateLoad(temp_path) == 0) cli_reload_after_state_load();
            remove(temp_path);
            write_state();
            return;
        }

        if (strcmp(cp, "parcel_ready") == 0) {
            /* All flags up to "go get parcel from Viridian Mart" */
            SetEvent(EVENT_OAK_APPEARED_IN_PALLET);
            SetEvent(EVENT_FOLLOWED_OAK_INTO_LAB);
            SetEvent(EVENT_OAK_ASKED_TO_CHOOSE_MON);
            SetEvent(EVENT_GOT_STARTER);
            SetEvent(EVENT_BATTLED_RIVAL_IN_OAKS_LAB);
            /* Give a starter if party is empty */
            if (wPartyCount == 0) {
                Pokemon_InitMon(&wPartyMons[0], STARTER1, 5);
                wPartyCount = 1;
            }
            /* Teleport to Viridian Mart entrance */
            extern void Map_Load(uint8_t map_id);
            extern void NPC_Load(void);
            extern void PalletScripts_OnMapLoad(void);
            extern void OaksLabScripts_OnMapLoad(void);
            wCurMap = 0x2a;  /* VIRIDIAN_MART */
            wXCoord = 3; wYCoord = 6;
            Map_Load(wCurMap); NPC_Load();
            PalletScripts_OnMapLoad(); OaksLabScripts_OnMapLoad();
            printf("[cli] checkpoint: parcel_ready — at Viridian Mart\n");
        } else if (strcmp(cp, "pokedex_ready") == 0) {
            /* All flags + has parcel, standing outside Oak's Lab */
            SetEvent(EVENT_OAK_APPEARED_IN_PALLET);
            SetEvent(EVENT_FOLLOWED_OAK_INTO_LAB);
            SetEvent(EVENT_OAK_ASKED_TO_CHOOSE_MON);
            SetEvent(EVENT_GOT_STARTER);
            SetEvent(EVENT_BATTLED_RIVAL_IN_OAKS_LAB);
            SetEvent(EVENT_GOT_OAKS_PARCEL);
            if (wPartyCount == 0) {
                Pokemon_InitMon(&wPartyMons[0], STARTER1, 5);
                wPartyCount = 1;
            }
            extern void Map_Load(uint8_t map_id);
            extern void NPC_Load(void);
            extern void PalletScripts_OnMapLoad(void);
            extern void OaksLabScripts_OnMapLoad(void);
            wCurMap = 0x28;  /* OAKS_LAB */
            wXCoord = 6; wYCoord = 8;
            Map_Load(wCurMap); NPC_Load();
            PalletScripts_OnMapLoad(); OaksLabScripts_OnMapLoad();
            /* Add after map load so bag isn't reset */
            if (Inventory_GetQty(ITEM_OAKS_PARCEL) == 0)
                Inventory_Add(ITEM_OAKS_PARCEL, 1);
            printf("[cli] checkpoint: pokedex_ready — at Oak's Lab with parcel\n");
        } else if (strcmp(cp, "mt_moon") == 0) {
            /* All flags through Brock, standing in Mt. Moon B2F fossil area */
            SetEvent(EVENT_OAK_APPEARED_IN_PALLET);
            SetEvent(EVENT_FOLLOWED_OAK_INTO_LAB);
            SetEvent(EVENT_OAK_ASKED_TO_CHOOSE_MON);
            SetEvent(EVENT_GOT_STARTER);
            SetEvent(EVENT_BATTLED_RIVAL_IN_OAKS_LAB);
            SetEvent(EVENT_GOT_OAKS_PARCEL);
            SetEvent(EVENT_GOT_POKEDEX);
            SetEvent(EVENT_BEAT_BROCK);
            /* Give a strong team for testing */
            if (wPartyCount == 0) {
                Pokemon_InitMon(&wPartyMons[0], STARTER1, 25);
                wPartyCount = 1;
            }
            extern void Map_Load(uint8_t map_id);
            extern void NPC_Load(void);
            extern void MtMoon_OnMapLoad(void);
            wCurMap = 0x3d;  /* MT_MOON_B2F */
            wXCoord = 13; wYCoord = 10;  /* south of Super Nerd */
            Map_Load(wCurMap); NPC_Load();
            MtMoon_OnMapLoad();
            printf("[cli] checkpoint: mt_moon — at Mt. Moon B2F, south of fossils\n");
        } else if (strcmp(cp, "cerulean") == 0) {
            /* All flags through Mt. Moon, standing at Cerulean City bridge */
            SetEvent(EVENT_OAK_APPEARED_IN_PALLET);
            SetEvent(EVENT_FOLLOWED_OAK_INTO_LAB);
            SetEvent(EVENT_OAK_ASKED_TO_CHOOSE_MON);
            SetEvent(EVENT_GOT_STARTER);
            SetEvent(EVENT_BATTLED_RIVAL_IN_OAKS_LAB);
            SetEvent(EVENT_GOT_OAKS_PARCEL);
            SetEvent(EVENT_GOT_POKEDEX);
            SetEvent(EVENT_BEAT_BROCK);
            if (wPartyCount == 0) {
                Pokemon_InitMon(&wPartyMons[0], STARTER1, 25);
                wPartyCount = 1;
            }
            extern void Map_Load(uint8_t map_id);
            extern void NPC_Load(void);
            extern void CeruleanScripts_OnMapLoad(void);
            wCurMap = 3;   /* CERULEAN_CITY */
            wXCoord = 20; wYCoord = 8;  /* two tiles south of trigger */
            Map_Load(wCurMap); NPC_Load();
            CeruleanScripts_OnMapLoad();
            printf("[cli] checkpoint: cerulean — at Cerulean City bridge (rival trigger at y=6), beat_rival=%d\n",
                   CheckEvent(EVENT_BEAT_CERULEAN_RIVAL));
        } else if (strcmp(cp, "route22") == 0) {
            /* All flags through Pokedex gift, rival ready on Route 22 */
            SetEvent(EVENT_OAK_APPEARED_IN_PALLET);
            SetEvent(EVENT_FOLLOWED_OAK_INTO_LAB);
            SetEvent(EVENT_OAK_ASKED_TO_CHOOSE_MON);
            SetEvent(EVENT_GOT_STARTER);
            SetEvent(EVENT_BATTLED_RIVAL_IN_OAKS_LAB);
            SetEvent(EVENT_GOT_OAKS_PARCEL);
            SetEvent(EVENT_GOT_POKEDEX);
            SetEvent(EVENT_1ST_ROUTE22_RIVAL_BATTLE);
            SetEvent(EVENT_ROUTE22_RIVAL_WANTS_BATTLE);
            ClearEvent(EVENT_BEAT_ROUTE22_RIVAL_1ST_BATTLE);
            if (wPartyCount == 0) {
                Pokemon_InitMon(&wPartyMons[0], STARTER1, 10);
                wPartyCount = 1;
            }
            extern void Map_Load(uint8_t map_id);
            extern void NPC_Load(void);
            extern void Route22Scripts_OnMapLoad(void);
            wCurMap = 0x21;  /* ROUTE_22 */
            wXCoord = 31; wYCoord = 5;  /* east of rival trigger (x=29) */
            Map_Load(wCurMap); NPC_Load();
            Route22Scripts_OnMapLoad();
            printf("[cli] checkpoint: route22 — on Route 22, walk left to trigger rival\n");
        } else if (strcmp(cp, "brock") == 0) {
            /* All flags through Pokedex gift, standing inside Pewter Gym */
            SetEvent(EVENT_OAK_APPEARED_IN_PALLET);
            SetEvent(EVENT_FOLLOWED_OAK_INTO_LAB);
            SetEvent(EVENT_OAK_ASKED_TO_CHOOSE_MON);
            SetEvent(EVENT_GOT_STARTER);
            SetEvent(EVENT_BATTLED_RIVAL_IN_OAKS_LAB);
            SetEvent(EVENT_GOT_OAKS_PARCEL);
            SetEvent(EVENT_GOT_POKEDEX);
            if (wPartyCount == 0) {
                Pokemon_InitMon(&wPartyMons[0], STARTER1, 15);
                wPartyCount = 1;
            }
            ClearEvent(EVENT_BEAT_PEWTER_GYM_TRAINER_0);
            ClearEvent(EVENT_BEAT_BROCK);
            extern void Map_Load(uint8_t map_id);
            extern void NPC_Load(void);
            wCurMap = 0x36;  /* PEWTER_GYM */
            wXCoord = 4; wYCoord = 2;  /* one tile below Brock (Brock is at 4,1) */
            Map_Load(wCurMap); NPC_Load();
            Trainer_LoadMap();  /* sets trainer facing directions */
            printf("[cli] checkpoint: brock — inside Pewter Gym\n");
        } else if (strcmp(cp, "misty") == 0) {
            SetEvent(EVENT_GOT_OAKS_PARCEL);
            SetEvent(EVENT_GOT_POKEDEX);
            SetEvent(EVENT_BEAT_BROCK);
            SetEvent(EVENT_BEAT_PEWTER_GYM_TRAINER_0);
            wObtainedBadges |= (1u << BADGE_BOULDER);
            SetEvent(EVENT_BEAT_CERULEAN_RIVAL);
            ClearEvent(EVENT_BEAT_MISTY);
            ClearEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_0);
            ClearEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_1);
            ClearEvent(EVENT_GOT_TM11);
            if (wPartyCount == 0) {
                Pokemon_InitMon(&wPartyMons[0], STARTER1, 25);
                wPartyCount = 1;
            }
            wCurMap = 0x41;  /* CERULEAN_GYM */
            wXCoord = 4; wYCoord = 8;
            Map_Load(wCurMap); NPC_Load();
            Trainer_LoadMap();
            printf("[cli] checkpoint: misty — inside Cerulean Gym\n");
        } else if (strcmp(cp, "cerulean_rocket") == 0) {
            /* All flags through Misty + SS Ticket obtained; Rocket thief not yet beaten */
            SetEvent(EVENT_OAK_APPEARED_IN_PALLET);
            SetEvent(EVENT_FOLLOWED_OAK_INTO_LAB);
            SetEvent(EVENT_OAK_ASKED_TO_CHOOSE_MON);
            SetEvent(EVENT_GOT_STARTER);
            SetEvent(EVENT_BATTLED_RIVAL_IN_OAKS_LAB);
            SetEvent(EVENT_GOT_OAKS_PARCEL);
            SetEvent(EVENT_GOT_POKEDEX);
            SetEvent(EVENT_BEAT_BROCK);
            SetEvent(EVENT_BEAT_PEWTER_GYM_TRAINER_0);
            wObtainedBadges |= (1u << BADGE_BOULDER);
            SetEvent(EVENT_BEAT_CERULEAN_RIVAL);
            SetEvent(EVENT_BEAT_MISTY);
            SetEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_0);
            SetEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_1);
            wObtainedBadges |= (1u << BADGE_CASCADE);
            SetEvent(EVENT_GOT_SS_TICKET);   /* guards cleared, Rocket accessible */
            ClearEvent(EVENT_BEAT_CERULEAN_ROCKET_THIEF);
            if (wPartyCount == 0) {
                Pokemon_InitMon(&wPartyMons[0], STARTER1, 25);
                wPartyCount = 1;
            }
            extern void Map_Load(uint8_t map_id);
            extern void NPC_Load(void);
            extern void CeruleanScripts_OnMapLoad(void);
            wCurMap = 3;   /* CERULEAN_CITY */
            wXCoord = 30; wYCoord = 9;  /* one tile south of Rocket NPC at (30,8) */
            Map_Load(wCurMap); NPC_Load();
            CeruleanScripts_OnMapLoad();
            printf("[cli] checkpoint: cerulean_rocket — at Rocket thief trigger (30,9) in Cerulean City\n");
        } else if (strcmp(cp, "ss_anne_hm") == 0) {
            /* ss_anne_hm — player on VermilionDock, one step from exit, HM01 obtained.
             * Tests SS Anne departure cutscene (walk north to trigger). */
            SetEvent(EVENT_GOT_POKEDEX);
            SetEvent(EVENT_BEAT_BROCK);
            SetEvent(EVENT_BEAT_PEWTER_GYM_TRAINER_0);
            wObtainedBadges |= (1u << BADGE_BOULDER);
            SetEvent(EVENT_BEAT_MISTY);
            SetEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_0);
            wObtainedBadges |= (1u << BADGE_CASCADE);
            SetEvent(EVENT_GOT_SS_TICKET);
            SetEvent(EVENT_BEAT_SS_ANNE_RIVAL);
            SetEvent(EVENT_RUBBED_CAPTAINS_BACK);
            SetEvent(EVENT_GOT_HM01);
            ClearEvent(EVENT_SS_ANNE_LEFT);
            Inventory_Add(ITEM_SS_TICKET, 1);
            Inventory_Add(HM01, 1);
            if (wPartyCount == 0) {
                Pokemon_InitMon(&wPartyMons[0], STARTER1, 25);
                wPartyCount = 1;
            }
            extern void Map_Load(uint8_t map_id);
            extern void NPC_Load(void);
            extern void SSAnneScripts_OnMapLoad(void);
            wCurMap = 0x5F;  /* SS_ANNE_1F */
            wXCoord = 26; wYCoord = 1;  /* one step south of exit warp at (26,0) to VermilionDock */
            Map_Load(wCurMap); NPC_Load();
            SSAnneScripts_OnMapLoad();
            printf("[cli] checkpoint: ss_anne_hm — inside SS Anne 1F, walk north to dock then departure\n");
        } else if (strcmp(cp, "liftkey_reset") == 0) {
            /* Reset Rocket Hideout B4F Lift Key sequence for repeat testing:
             * - Rocket3 battle not yet won
             * - Lift Key not dropped
             * - Lift Key not picked up on B4F */
            ClearEvent(EVENT_BEAT_ROCKET_HIDEOUT_4_TRAINER_2);
            ClearEvent(EVENT_ROCKET_DROPPED_LIFT_KEY);
            if (0xCA < 248)
                wPickedUpItems[0xCA] &= (uint16_t)~(1u << 4);

            /* If currently on B4F, immediately re-apply map object visibility. */
            if (wCurMap == 0xCA) {
                extern void NPC_Load(void);
                extern void RocketHideoutB4FScripts_OnMapLoad(void);
                NPC_Load();
                RocketHideoutB4FScripts_OnMapLoad();
            }
            printf("[cli] checkpoint: liftkey_reset — cleared flags 441/715 and B4F Lift Key pickup bit\n");
        } else if (strcmp(cp, "giovanni_reset") == 0) {
            /* Reset Rocket Hideout B4F Giovanni sequence for repeat testing:
             * - Giovanni battle not yet won
             * - Silph Scope not picked up on B4F */
            ClearEvent(EVENT_BEAT_ROCKET_HIDEOUT_GIOVANNI);
            if (0xCA < 248)
                wPickedUpItems[0xCA] &= (uint16_t)~(1u << 3);

            /* If currently on B4F, immediately re-apply map object visibility. */
            if (wCurMap == 0xCA) {
                extern void NPC_Load(void);
                extern void RocketHideoutB4FScripts_OnMapLoad(void);
                NPC_Load();
                RocketHideoutB4FScripts_OnMapLoad();
            }
            printf("[cli] checkpoint: giovanni_reset — cleared Giovanni flag and B4F Silph Scope pickup bit\n");
        } else if (strcmp(cp, "giovanni_ready") == 0) {
            /* Set Rocket Hideout B4F to "right before Giovanni":
             * - Hideout discovered/opened
             * - Lift Key in bag
             * - Door grunts on B4F already beaten (door open)
             * - Giovanni not yet beaten; Silph Scope not picked up
             * - Teleport inside B4F a few tiles below Giovanni */
            SetEvent(EVENT_FOUND_ROCKET_HIDEOUT);
            SetEvent(EVENT_ROCKET_DROPPED_LIFT_KEY);
            SetEvent(EVENT_BEAT_ROCKET_HIDEOUT_4_TRAINER_0);
            SetEvent(EVENT_BEAT_ROCKET_HIDEOUT_4_TRAINER_1);
            ClearEvent(EVENT_BEAT_ROCKET_HIDEOUT_GIOVANNI);
            if (Inventory_GetQty(0x4A) == 0)
                Inventory_Add(0x4A, 1); /* LIFT_KEY */
            if (0xCA < 248)
                wPickedUpItems[0xCA] &= (uint16_t)~(1u << 3); /* Silph Scope on B4F not picked */

            wCurMap = 0xCA;  /* ROCKET_HIDEOUT_B4F */
            wXCoord = 25; wYCoord = 6;
            Map_Load(wCurMap); NPC_Load();
            RocketHideoutB4FScripts_OnMapLoad();
            Trainer_LoadMap();
            printf("[cli] checkpoint: giovanni_ready — teleported to B4F in front of Giovanni, Lift Key granted\n");
        } else if (strcmp(cp, "surge") == 0) {
            /* surge — inside Vermilion Gym, puzzle solved, facing Lt. Surge */
            SetEvent(EVENT_GOT_POKEDEX);
            SetEvent(EVENT_BEAT_BROCK);
            SetEvent(EVENT_BEAT_PEWTER_GYM_TRAINER_0);
            wObtainedBadges |= (1u << BADGE_BOULDER);
            SetEvent(EVENT_BEAT_CERULEAN_RIVAL);
            SetEvent(EVENT_BEAT_MISTY);
            SetEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_0);
            SetEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_1);
            wObtainedBadges |= (1u << BADGE_CASCADE);
            SetEvent(EVENT_GOT_SS_TICKET);
            SetEvent(EVENT_BEAT_SS_ANNE_RIVAL);
            SetEvent(EVENT_RUBBED_CAPTAINS_BACK);
            SetEvent(EVENT_GOT_HM01);
            SetEvent(EVENT_SS_ANNE_LEFT);
            SetEvent(EVENT_2ND_LOCK_OPENED);           /* puzzle solved, door open */
            SetEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_0);
            SetEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_1);
            SetEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_2);
            ClearEvent(EVENT_BEAT_LT_SURGE);
            ClearEvent(EVENT_GOT_TM24);
            Inventory_Add(HM01, 1);
            if (wPartyCount == 0) {
                Pokemon_InitMon(&wPartyMons[0], STARTER1, 35);
                wPartyCount = 1;
            }
            extern void VermilionGymScripts_OnMapLoad(void);
            wCurMap = 0x5C;  /* VERMILION_GYM */
            wXCoord = 5; wYCoord = 3;  /* two tiles south of Lt. Surge at (5,1) */
            Map_Load(wCurMap); NPC_Load();
            VermilionGymScripts_OnMapLoad();
            printf("[cli] checkpoint: surge — inside Vermilion Gym, facing Lt. Surge\n");
        } else if (strcmp(cp, "erika") == 0) {
            /* erika — inside Celadon Gym, facing Erika; trainers unbeaten */
            SetEvent(EVENT_GOT_POKEDEX);
            SetEvent(EVENT_BEAT_BROCK);
            SetEvent(EVENT_BEAT_PEWTER_GYM_TRAINER_0);
            wObtainedBadges |= (1u << BADGE_BOULDER);
            SetEvent(EVENT_BEAT_CERULEAN_RIVAL);
            SetEvent(EVENT_BEAT_MISTY);
            SetEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_0);
            SetEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_1);
            wObtainedBadges |= (1u << BADGE_CASCADE);
            SetEvent(EVENT_GOT_SS_TICKET);
            SetEvent(EVENT_BEAT_SS_ANNE_RIVAL);
            SetEvent(EVENT_RUBBED_CAPTAINS_BACK);
            SetEvent(EVENT_GOT_HM01);
            SetEvent(EVENT_SS_ANNE_LEFT);
            SetEvent(EVENT_2ND_LOCK_OPENED);
            SetEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_0);
            SetEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_1);
            SetEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_2);
            SetEvent(EVENT_BEAT_LT_SURGE);
            SetEvent(EVENT_GOT_TM24);
            wObtainedBadges |= (1u << BADGE_THUNDER);
            ClearEvent(EVENT_BEAT_ERIKA);
            ClearEvent(EVENT_GOT_TM21);
            ClearEvent(EVENT_BEAT_CELADON_GYM_TRAINER_0);
            ClearEvent(EVENT_BEAT_CELADON_GYM_TRAINER_1);
            ClearEvent(EVENT_BEAT_CELADON_GYM_TRAINER_2);
            ClearEvent(EVENT_BEAT_CELADON_GYM_TRAINER_3);
            ClearEvent(EVENT_BEAT_CELADON_GYM_TRAINER_4);
            ClearEvent(EVENT_BEAT_CELADON_GYM_TRAINER_5);
            ClearEvent(EVENT_BEAT_CELADON_GYM_TRAINER_6);
            if (wPartyCount == 0) {
                Pokemon_InitMon(&wPartyMons[0], STARTER1, 40);
                wPartyCount = 1;
            }
            wCurMap = 0x86;  /* CELADON_GYM */
            wXCoord = 4; wYCoord = 4;  /* one tile south of Erika at (4,3) */
            Map_Load(wCurMap); NPC_Load();
            Trainer_LoadMap();
            printf("[cli] checkpoint: erika — inside Celadon Gym, facing Erika\n");
        } else if (strcmp(cp, "koga") == 0) {
            /* koga — inside Fuchsia Gym, facing Koga; trainers unbeaten */
            SetEvent(EVENT_GOT_POKEDEX);
            SetEvent(EVENT_BEAT_BROCK);
            SetEvent(EVENT_BEAT_PEWTER_GYM_TRAINER_0);
            wObtainedBadges |= (1u << BADGE_BOULDER);
            SetEvent(EVENT_BEAT_CERULEAN_RIVAL);
            SetEvent(EVENT_BEAT_MISTY);
            SetEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_0);
            SetEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_1);
            wObtainedBadges |= (1u << BADGE_CASCADE);
            SetEvent(EVENT_GOT_SS_TICKET);
            SetEvent(EVENT_BEAT_SS_ANNE_RIVAL);
            SetEvent(EVENT_RUBBED_CAPTAINS_BACK);
            SetEvent(EVENT_GOT_HM01);
            SetEvent(EVENT_SS_ANNE_LEFT);
            SetEvent(EVENT_2ND_LOCK_OPENED);
            SetEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_0);
            SetEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_1);
            SetEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_2);
            SetEvent(EVENT_BEAT_LT_SURGE);
            SetEvent(EVENT_GOT_TM24);
            wObtainedBadges |= (1u << BADGE_THUNDER);
            SetEvent(EVENT_BEAT_ERIKA);
            SetEvent(EVENT_GOT_TM21);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_0);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_1);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_2);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_3);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_4);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_5);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_6);
            wObtainedBadges |= (1u << BADGE_RAINBOW);
            ClearEvent(EVENT_BEAT_KOGA);
            ClearEvent(EVENT_GOT_TM06);
            ClearEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_0);
            ClearEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_1);
            ClearEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_2);
            ClearEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_3);
            ClearEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_4);
            ClearEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_5);
            if (wPartyCount == 0) {
                Pokemon_InitMon(&wPartyMons[0], STARTER1, 45);
                wPartyCount = 1;
            }
            wCurMap = 0x9D;  /* FUCHSIA_GYM */
            wXCoord = 4; wYCoord = 11;  /* one tile south of Koga at (4,10) */
            Map_Load(wCurMap); NPC_Load();
            Trainer_LoadMap();
            printf("[cli] checkpoint: koga — inside Fuchsia Gym, facing Koga\n");
        } else if (strcmp(cp, "blaine") == 0) {
            /* blaine — inside Cinnabar Gym, facing Blaine */
            SetEvent(EVENT_GOT_POKEDEX);
            SetEvent(EVENT_BEAT_BROCK);
            SetEvent(EVENT_BEAT_PEWTER_GYM_TRAINER_0);
            wObtainedBadges |= (1u << BADGE_BOULDER);
            SetEvent(EVENT_BEAT_CERULEAN_RIVAL);
            SetEvent(EVENT_BEAT_MISTY);
            SetEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_0);
            SetEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_1);
            wObtainedBadges |= (1u << BADGE_CASCADE);
            SetEvent(EVENT_GOT_SS_TICKET);
            SetEvent(EVENT_BEAT_SS_ANNE_RIVAL);
            SetEvent(EVENT_RUBBED_CAPTAINS_BACK);
            SetEvent(EVENT_GOT_HM01);
            SetEvent(EVENT_SS_ANNE_LEFT);
            SetEvent(EVENT_2ND_LOCK_OPENED);
            SetEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_0);
            SetEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_1);
            SetEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_2);
            SetEvent(EVENT_BEAT_LT_SURGE);
            SetEvent(EVENT_GOT_TM24);
            wObtainedBadges |= (1u << BADGE_THUNDER);
            SetEvent(EVENT_BEAT_ERIKA);
            SetEvent(EVENT_GOT_TM21);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_0);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_1);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_2);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_3);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_4);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_5);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_6);
            wObtainedBadges |= (1u << BADGE_RAINBOW);
            SetEvent(EVENT_BEAT_KOGA);
            SetEvent(EVENT_GOT_TM06);
            SetEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_0);
            SetEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_1);
            SetEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_2);
            SetEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_3);
            SetEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_4);
            SetEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_5);
            wObtainedBadges |= (1u << BADGE_SOUL);
            ClearEvent(EVENT_BEAT_BLAINE);
            ClearEvent(EVENT_GOT_TM38);
            if (wPartyCount == 0) {
                Pokemon_InitMon(&wPartyMons[0], STARTER1, 52);
                wPartyCount = 1;
            }
            wCurMap = 0xA6;  /* CINNABAR_GYM */
            wXCoord = 3; wYCoord = 4;  /* one tile south of Blaine at (3,3) */
            Map_Load(wCurMap); NPC_Load();
            Trainer_LoadMap();
            printf("[cli] checkpoint: blaine — inside Cinnabar Gym, facing Blaine\n");
        } else if (strcmp(cp, "erika_post") == 0) {
            /* Erika defeated, TM21 not obtained: tests post-badge TM retry branch */
            SetEvent(EVENT_GOT_POKEDEX);
            SetEvent(EVENT_BEAT_BROCK);
            SetEvent(EVENT_BEAT_PEWTER_GYM_TRAINER_0);
            wObtainedBadges |= (1u << BADGE_BOULDER);
            SetEvent(EVENT_BEAT_CERULEAN_RIVAL);
            SetEvent(EVENT_BEAT_MISTY);
            SetEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_0);
            SetEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_1);
            wObtainedBadges |= (1u << BADGE_CASCADE);
            SetEvent(EVENT_GOT_SS_TICKET);
            SetEvent(EVENT_BEAT_SS_ANNE_RIVAL);
            SetEvent(EVENT_RUBBED_CAPTAINS_BACK);
            SetEvent(EVENT_GOT_HM01);
            SetEvent(EVENT_SS_ANNE_LEFT);
            SetEvent(EVENT_2ND_LOCK_OPENED);
            SetEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_0);
            SetEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_1);
            SetEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_2);
            SetEvent(EVENT_BEAT_LT_SURGE);
            SetEvent(EVENT_GOT_TM24);
            wObtainedBadges |= (1u << BADGE_THUNDER);
            SetEvent(EVENT_BEAT_ERIKA);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_0);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_1);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_2);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_3);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_4);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_5);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_6);
            ClearEvent(EVENT_GOT_TM21);
            wObtainedBadges |= (1u << BADGE_RAINBOW);
            if (wPartyCount == 0) {
                Pokemon_InitMon(&wPartyMons[0], STARTER1, 45);
                wPartyCount = 1;
            }
            wCurMap = 0x86;  /* CELADON_GYM */
            wXCoord = 4; wYCoord = 4;
            Map_Load(wCurMap); NPC_Load();
            Trainer_LoadMap();
            printf("[cli] checkpoint: erika_post — Erika beaten, TM21 not obtained\n");
        } else if (strcmp(cp, "koga_post") == 0) {
            /* Koga defeated, TM06 not obtained: tests post-badge TM retry branch */
            SetEvent(EVENT_GOT_POKEDEX);
            SetEvent(EVENT_BEAT_BROCK);
            SetEvent(EVENT_BEAT_PEWTER_GYM_TRAINER_0);
            wObtainedBadges |= (1u << BADGE_BOULDER);
            SetEvent(EVENT_BEAT_CERULEAN_RIVAL);
            SetEvent(EVENT_BEAT_MISTY);
            SetEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_0);
            SetEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_1);
            wObtainedBadges |= (1u << BADGE_CASCADE);
            SetEvent(EVENT_GOT_SS_TICKET);
            SetEvent(EVENT_BEAT_SS_ANNE_RIVAL);
            SetEvent(EVENT_RUBBED_CAPTAINS_BACK);
            SetEvent(EVENT_GOT_HM01);
            SetEvent(EVENT_SS_ANNE_LEFT);
            SetEvent(EVENT_2ND_LOCK_OPENED);
            SetEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_0);
            SetEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_1);
            SetEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_2);
            SetEvent(EVENT_BEAT_LT_SURGE);
            SetEvent(EVENT_GOT_TM24);
            wObtainedBadges |= (1u << BADGE_THUNDER);
            SetEvent(EVENT_BEAT_ERIKA);
            SetEvent(EVENT_GOT_TM21);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_0);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_1);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_2);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_3);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_4);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_5);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_6);
            wObtainedBadges |= (1u << BADGE_RAINBOW);
            SetEvent(EVENT_BEAT_KOGA);
            SetEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_0);
            SetEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_1);
            SetEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_2);
            SetEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_3);
            SetEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_4);
            SetEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_5);
            ClearEvent(EVENT_GOT_TM06);
            wObtainedBadges |= (1u << BADGE_SOUL);
            if (wPartyCount == 0) {
                Pokemon_InitMon(&wPartyMons[0], STARTER1, 50);
                wPartyCount = 1;
            }
            wCurMap = 0x9D;  /* FUCHSIA_GYM */
            wXCoord = 4; wYCoord = 11;
            Map_Load(wCurMap); NPC_Load();
            Trainer_LoadMap();
            printf("[cli] checkpoint: koga_post — Koga beaten, TM06 not obtained\n");
        } else if (strcmp(cp, "blaine_post") == 0) {
            /* Blaine defeated, TM38 not obtained: tests post-badge TM retry branch */
            SetEvent(EVENT_GOT_POKEDEX);
            SetEvent(EVENT_BEAT_BROCK);
            SetEvent(EVENT_BEAT_PEWTER_GYM_TRAINER_0);
            wObtainedBadges |= (1u << BADGE_BOULDER);
            SetEvent(EVENT_BEAT_CERULEAN_RIVAL);
            SetEvent(EVENT_BEAT_MISTY);
            SetEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_0);
            SetEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_1);
            wObtainedBadges |= (1u << BADGE_CASCADE);
            SetEvent(EVENT_GOT_SS_TICKET);
            SetEvent(EVENT_BEAT_SS_ANNE_RIVAL);
            SetEvent(EVENT_RUBBED_CAPTAINS_BACK);
            SetEvent(EVENT_GOT_HM01);
            SetEvent(EVENT_SS_ANNE_LEFT);
            SetEvent(EVENT_2ND_LOCK_OPENED);
            SetEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_0);
            SetEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_1);
            SetEvent(EVENT_BEAT_VERMILION_GYM_TRAINER_2);
            SetEvent(EVENT_BEAT_LT_SURGE);
            SetEvent(EVENT_GOT_TM24);
            wObtainedBadges |= (1u << BADGE_THUNDER);
            SetEvent(EVENT_BEAT_ERIKA);
            SetEvent(EVENT_GOT_TM21);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_0);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_1);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_2);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_3);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_4);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_5);
            SetEvent(EVENT_BEAT_CELADON_GYM_TRAINER_6);
            wObtainedBadges |= (1u << BADGE_RAINBOW);
            SetEvent(EVENT_BEAT_KOGA);
            SetEvent(EVENT_GOT_TM06);
            SetEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_0);
            SetEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_1);
            SetEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_2);
            SetEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_3);
            SetEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_4);
            SetEvent(EVENT_BEAT_FUCHSIA_GYM_TRAINER_5);
            wObtainedBadges |= (1u << BADGE_SOUL);
            SetEvent(EVENT_BEAT_BLAINE);
            ClearEvent(EVENT_GOT_TM38);
            wObtainedBadges |= (1u << BADGE_VOLCANO);
            if (wPartyCount == 0) {
                Pokemon_InitMon(&wPartyMons[0], STARTER1, 56);
                wPartyCount = 1;
            }
            wCurMap = 0xA6;  /* CINNABAR_GYM */
            wXCoord = 3; wYCoord = 4;
            Map_Load(wCurMap); NPC_Load();
            Trainer_LoadMap();
            printf("[cli] checkpoint: blaine_post — Blaine beaten, TM38 not obtained\n");
        } else if (strcmp(cp, "gym_badges4") == 0) {
            /* Utility: exactly first 4 badges, no Fuchsia/Cinnabar progress */
            SetEvent(EVENT_BEAT_BROCK);
            SetEvent(EVENT_BEAT_MISTY);
            SetEvent(EVENT_BEAT_LT_SURGE);
            SetEvent(EVENT_BEAT_ERIKA);
            wObtainedBadges |= (1u << BADGE_BOULDER);
            wObtainedBadges |= (1u << BADGE_CASCADE);
            wObtainedBadges |= (1u << BADGE_THUNDER);
            wObtainedBadges |= (1u << BADGE_RAINBOW);
            ClearEvent(EVENT_BEAT_KOGA);
            ClearEvent(EVENT_BEAT_BLAINE);
            printf("[cli] checkpoint: gym_badges4 — set first 4 gym wins/badges\n");
        } else if (strcmp(cp, "gym_badges5") == 0) {
            /* Utility: exactly first 5 badges (through Koga) */
            SetEvent(EVENT_BEAT_BROCK);
            SetEvent(EVENT_BEAT_MISTY);
            SetEvent(EVENT_BEAT_LT_SURGE);
            SetEvent(EVENT_BEAT_ERIKA);
            SetEvent(EVENT_BEAT_KOGA);
            wObtainedBadges |= (1u << BADGE_BOULDER);
            wObtainedBadges |= (1u << BADGE_CASCADE);
            wObtainedBadges |= (1u << BADGE_THUNDER);
            wObtainedBadges |= (1u << BADGE_RAINBOW);
            wObtainedBadges |= (1u << BADGE_SOUL);
            ClearEvent(EVENT_BEAT_BLAINE);
            printf("[cli] checkpoint: gym_badges5 — set first 5 gym wins/badges\n");
        } else if (strcmp(cp, "gym_badges1") == 0) {
            SetEvent(EVENT_BEAT_BROCK);
            wObtainedBadges |= (1u << BADGE_BOULDER);
            ClearEvent(EVENT_BEAT_MISTY);
            ClearEvent(EVENT_BEAT_LT_SURGE);
            ClearEvent(EVENT_BEAT_ERIKA);
            ClearEvent(EVENT_BEAT_KOGA);
            ClearEvent(EVENT_BEAT_BLAINE);
            printf("[cli] checkpoint: gym_badges1 — set Brock win/badge only\n");
        } else if (strcmp(cp, "gym_badges2") == 0) {
            SetEvent(EVENT_BEAT_BROCK);
            SetEvent(EVENT_BEAT_MISTY);
            wObtainedBadges |= (1u << BADGE_BOULDER);
            wObtainedBadges |= (1u << BADGE_CASCADE);
            ClearEvent(EVENT_BEAT_LT_SURGE);
            ClearEvent(EVENT_BEAT_ERIKA);
            ClearEvent(EVENT_BEAT_KOGA);
            ClearEvent(EVENT_BEAT_BLAINE);
            printf("[cli] checkpoint: gym_badges2 — set Brock+Misty wins/badges\n");
        } else if (strcmp(cp, "gym_badges3") == 0) {
            SetEvent(EVENT_BEAT_BROCK);
            SetEvent(EVENT_BEAT_MISTY);
            SetEvent(EVENT_BEAT_LT_SURGE);
            wObtainedBadges |= (1u << BADGE_BOULDER);
            wObtainedBadges |= (1u << BADGE_CASCADE);
            wObtainedBadges |= (1u << BADGE_THUNDER);
            ClearEvent(EVENT_BEAT_ERIKA);
            ClearEvent(EVENT_BEAT_KOGA);
            ClearEvent(EVENT_BEAT_BLAINE);
            printf("[cli] checkpoint: gym_badges3 — set Brock+Misty+Surge wins/badges\n");
        } else if (strcmp(cp, "brock_post") == 0) {
            /* Brock defeated, TM34 not obtained: tests post-badge TM retry branch */
            SetEvent(EVENT_OAK_APPEARED_IN_PALLET);
            SetEvent(EVENT_FOLLOWED_OAK_INTO_LAB);
            SetEvent(EVENT_OAK_ASKED_TO_CHOOSE_MON);
            SetEvent(EVENT_GOT_STARTER);
            SetEvent(EVENT_BATTLED_RIVAL_IN_OAKS_LAB);
            SetEvent(EVENT_GOT_OAKS_PARCEL);
            SetEvent(EVENT_GOT_POKEDEX);
            SetEvent(EVENT_BEAT_BROCK);
            SetEvent(EVENT_BEAT_PEWTER_GYM_TRAINER_0);
            ClearEvent(EVENT_GOT_TM34);
            wObtainedBadges |= (1u << BADGE_BOULDER);
            if (wPartyCount == 0) {
                Pokemon_InitMon(&wPartyMons[0], STARTER1, 18);
                wPartyCount = 1;
            }
            wCurMap = 0x36;  /* PEWTER_GYM */
            wXCoord = 4; wYCoord = 2;
            Map_Load(wCurMap); NPC_Load();
            Trainer_LoadMap();
            printf("[cli] checkpoint: brock_post — Brock beaten, TM34 not obtained\n");
        } else if (strcmp(cp, "misty_post") == 0) {
            /* Misty defeated, TM11 not obtained: tests post-badge TM retry branch */
            SetEvent(EVENT_OAK_APPEARED_IN_PALLET);
            SetEvent(EVENT_FOLLOWED_OAK_INTO_LAB);
            SetEvent(EVENT_OAK_ASKED_TO_CHOOSE_MON);
            SetEvent(EVENT_GOT_STARTER);
            SetEvent(EVENT_BATTLED_RIVAL_IN_OAKS_LAB);
            SetEvent(EVENT_GOT_OAKS_PARCEL);
            SetEvent(EVENT_GOT_POKEDEX);
            SetEvent(EVENT_BEAT_BROCK);
            SetEvent(EVENT_BEAT_PEWTER_GYM_TRAINER_0);
            wObtainedBadges |= (1u << BADGE_BOULDER);
            SetEvent(EVENT_BEAT_MISTY);
            SetEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_0);
            SetEvent(EVENT_BEAT_CERULEAN_GYM_TRAINER_1);
            ClearEvent(EVENT_GOT_TM11);
            wObtainedBadges |= (1u << BADGE_CASCADE);
            if (wPartyCount == 0) {
                Pokemon_InitMon(&wPartyMons[0], STARTER1, 26);
                wPartyCount = 1;
            }
            wCurMap = 0x41;  /* CERULEAN_GYM */
            wXCoord = 4; wYCoord = 8;
            Map_Load(wCurMap); NPC_Load();
            Trainer_LoadMap();
            printf("[cli] checkpoint: misty_post — Misty beaten, TM11 not obtained\n");
        } else if (strcmp(cp, "list") == 0 || strcmp(cp, "help") == 0) {
            printf("[cli] checkpoint list:\n");
            printf("  parcel_ready, pokedex_ready, route22, brock, mt_moon, cerulean,\n");
            printf("  misty, cerulean_rocket, ss_anne_hm, surge, erika, koga, blaine,\n");
            printf("  brock_post, misty_post, erika_post, koga_post, blaine_post,\n");
            printf("  gym_badges1, gym_badges2, gym_badges3, gym_badges4, gym_badges5,\n");
            printf("  liftkey_reset, giovanni_reset, giovanni_ready\n");
        } else {
            printf("[cli] Unknown checkpoint: %s\n"
                   "      Use: checkpoint list\n", cp);
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "giveteam") == 0) {
        /* giveteam — replace party with a level-100 wrecking crew */
        static const uint8_t kTeam[] = {
            SPECIES_MEWTWO,    /* 0x83 — best there is */
            SPECIES_DRAGONITE, /* 0x42 */
            SPECIES_ALAKAZAM,  /* 0x95 */
            SPECIES_MACHAMP,   /* 0x7E */
            SPECIES_GENGAR,    /* 0x0E */
            SPECIES_LAPRAS,    /* 0x13 */
        };
        wPartyCount = 6;
        for (int i = 0; i < 6; i++)
            Pokemon_InitMon(&wPartyMons[i], kTeam[i], 100);
        printf("[cli] giveteam — party set to Mewtwo/Dragonite/Alakazam/Machamp/Gengar/Lapras @ lv100\n");
        write_state();
        return;
    }
    else if (strcmp(verb, "addexp") == 0) {
        /* addexp <slot 1-6> <amount>
         * Directly adds exp to a party slot.  Handles level-up stat recalc
         * and move learning.  Does not trigger evolution.
         * Example: addexp 1 5000 */
        int slot = 1, amount = 0;
        sscanf(cmd, "%*s %d %d", &slot, &amount);
        slot--;  /* 1-based → 0-based */
        if (slot < 0 || slot >= (int)wPartyCount) {
            printf("[cli] addexp: slot must be 1-%d\n", wPartyCount);
        } else if (amount <= 0) {
            printf("[cli] addexp: amount must be > 0\n");
        } else {
            Battle_AddExpDirect((uint8_t)slot, (uint32_t)amount);
        }
        write_state();
        return;
    }
    else if (strcmp(verb, "exprate") == 0) {
        /* exprate <multiplier>
         * Sets the debug exp rate applied in Battle_GainExperience.
         * Value is a percentage: 100=1×, 150=1.5×, 200=2×, 300=3×.
         * Example: exprate 200   → double exp from all battles */
        int rate = 100;
        sscanf(cmd, "%*s %d", &rate);
        if (rate < 1 || rate > 9999) {
            printf("[cli] exprate: value must be 1-9999 (100=1×, 200=2×, 300=3×)\n");
        } else {
            gDebugExpRate = rate;
            printf("[cli] exprate: exp multiplier set to %d%% (%d.%02dx)\n",
                   rate, rate / 100, rate % 100);
        }
        write_state();
        return;
    }
    else {
        printf("[cli] Unknown command: %s\n", verb);
        s_last_cmd_valid = 0;
        write_state();
        return;
    }

    if (s_seq_len > 0) {
        s_wait_remaining = 20; /* reaction frames after sequence ends */
        s_pending_write  = 1;
    } else {
        write_state();
    }
}

static void poll_cmd_file(void) {
    FILE *fp = fopen(CMD_FILE, "r");
    if (!fp) return;

    char line[128] = {0};
    fgets(line, sizeof(line), fp);
    fclose(fp);
    remove(CMD_FILE);

    char *nl = strchr(line, '\n');
    if (nl) *nl = '\0';
    if (*line == '\0') return;

    printf("[cli] cmd: %s\n", line);
    {
        char logline[CLI_HIST_WIDTH + 1];
        snprintf(logline, sizeof(logline), "> %s", line);
        s_last_cmd_hist_slot = cli_hist_push(logline);
    }
    process_cmd(line);
    if (s_last_cmd_hist_slot >= 0) {
        s_hist_color[s_last_cmd_hist_slot] =
            (uint8_t)(s_last_cmd_valid ? CLI_HIST_COLOR_OK : CLI_HIST_COLOR_ERROR);
        s_last_cmd_hist_slot = -1;
    }
}

/* ---- Public tick -------------------------------------------------- */

void DebugCLI_Tick(void) {
    dsl_bank_init_if_needed();
    if (s_dsl_bank_enabled && s_dsl_bank_last_map != wCurMap) {
        dsl_bank_ensure_current_map_spawns();
        s_dsl_bank_last_map = wCurMap;
    }

    /* Feed sequence into injection globals one frame at a time.
     * DebugCLI_Tick runs after Input_Update, so whatever we set here
     * is consumed by Input_Update on the NEXT frame. */
    if (s_seq_pos < s_seq_len) {
        gCliButtons = s_seq[s_seq_pos++];
        gCliFrames  = 1;
    }

    if (s_replay_playing) {
        if (s_replay_play_pos < s_replay_play_len) {
            gCliButtons = s_replay_play_buf[s_replay_play_pos++];
            gCliFrames  = 1;
        } else {
            replay_reset_playback();
            printf("[cli] replay: playback complete\n");
            s_pending_write = 1;
            s_wait_remaining = 4;
        }
    }

    if (s_temp_npc_walkoff_active) {
        int tx, ty;
        int sy;
        if (s_temp_npc_walkoff_idx < 0 || s_temp_npc_walkoff_idx >= NPC_GetCount()) {
            s_temp_npc_walkoff_active = 0;
            s_temp_npc_walkoff_idx = -1;
            s_temp_npc_walkoff_phase = 0;
            s_temp_npc_walkoff_pretext_frames = 0;
            wJoyIgnore = 0;
        } else if (s_temp_npc_walkoff_phase == 1) {
            NPC_SetFacing(s_temp_npc_walkoff_idx,
                (wPlayerDirection == 4) ? 1 :
                (wPlayerDirection == 8) ? 2 :
                (wPlayerDirection == 12) ? 3 : 0);
            if (s_temp_npc_walkoff_pretext_frames > 0) {
                s_temp_npc_walkoff_pretext_frames--;
            } else if (!Text_IsOpen()) {
                Text_ShowASCII("Hello! I am a\ndebug_cli NPC\ntest!\fHope this works!@");
                s_temp_npc_walkoff_phase = 2;
            }
        } else if (s_temp_npc_walkoff_phase == 2) {
            NPC_SetFacing(s_temp_npc_walkoff_idx,
                (wPlayerDirection == 4) ? 1 :
                (wPlayerDirection == 8) ? 2 :
                (wPlayerDirection == 12) ? 3 : 0);
            if (!Text_IsOpen()) {
                wJoyIgnore = 0;
                s_temp_npc_walkoff_phase = 3;
            }
        } else if (s_temp_npc_walkoff_phase == 3 && !NPC_IsWalking(s_temp_npc_walkoff_idx)) {
            NPC_GetTilePos(s_temp_npc_walkoff_idx, &tx, &ty);
            sy = ty * 2 + 1 - gCamY;
            if (sy < 0) {
                NPC_DebugDespawn(s_temp_npc_walkoff_idx);
                s_temp_npc_walkoff_active = 0;
                s_temp_npc_walkoff_idx = -1;
                s_temp_npc_walkoff_phase = 0;
                s_temp_npc_walkoff_pretext_frames = 0;
                printf("[cli] npc_walkoff: despawned after leaving top of screen\n");
            } else {
                NPC_DoScriptedStep(s_temp_npc_walkoff_idx, 1); /* up */
            }
        }
    }

    if (s_scene_active) {
        scene_track_actor_positions();
        if (s_scene_battlestart_pending) {
            if (Text_IsOpen()) {
                s_scene_battlestart_saw_text = 1;
            } else if (!s_scene_battlestart_delay) {
                /* First clean frame after text has closed (or immediately if no text). */
                s_scene_battlestart_delay = 1;
            } else {
                const char *defeat_text = s_scene_cmds[s_scene_pc].text;
                scene_start_trainer_battle(s_scene_battlestart_tc, s_scene_battlestart_tn, defeat_text);
                s_scene_wait_battle = 1;
                s_scene_battlestart_pending = 0;
                s_scene_battlestart_saw_text = 0;
                s_scene_battlestart_delay = 0;
                s_scene_pc++;
                printf("[cli] scene battlestart: class=%d no=%d\n",
                       s_scene_battlestart_tc, s_scene_battlestart_tn);
            }
        } else if (s_scene_wait_say) {
            if (Text_IsOpen()) s_scene_say_opened = 1;
            if (s_scene_say_opened && !Text_IsOpen()) {
                s_scene_wait_say = 0;
                s_scene_say_opened = 0;
            }
        } else if (s_scene_wait_battle) {
            int sc = get_scene();
            if (sc == 2 || BattleUI_IsActive())
                s_scene_battle_started = 1;
            /* Only release once we've actually entered battle at least once
             * and then returned to overworld. */
            if (s_scene_battle_started && sc == 0 && !BattleUI_IsActive()) {
                scene_restore_spawned_actors_after_battle();
                s_scene_wait_battle = 0;
                s_scene_battle_started = 0;
            }
        } else if (s_scene_wait_battleend_text) {
            if (!Text_IsOpen()) {
                s_scene_wait_battleend_text = 0;
                printf("[scene] battlend\n");
            }
        } else if (s_scene_wait_yesno) {
            YesNo_Tick();
            if (!YesNo_IsOpen()) {
                s_scene_last_yesno = YesNo_GetResult() ? 1 : 0;
                s_scene_wait_yesno = 0;
                if (s_scene_yesno_restore_joyignore) {
                    wJoyIgnore = s_scene_yesno_prev_joyignore;
                    s_scene_yesno_restore_joyignore = 0;
                }
                if (s_scene_yesno_restore_scripted_movement) {
                    gScriptedMovement = s_scene_yesno_prev_scripted_movement;
                    s_scene_yesno_restore_scripted_movement = 0;
                }
                printf("[cli] scene ask result: %s\n", s_scene_last_yesno ? "yes" : "no");
            }
        } else if (s_scene_wait > 0) {
            s_scene_wait--;
        } else if (s_scene_move_steps_left > 0) {
            if (s_scene_move_actor < 0 || s_scene_move_actor >= SCENE_ACTOR_MAX || !s_scene_actors[s_scene_move_actor].used) {
                s_scene_move_steps_left = 0;
            } else {
                int npc_idx = s_scene_actors[s_scene_move_actor].npc_idx;
                if (npc_idx < 0 || npc_idx >= NPC_GetCount()) {
                    s_scene_move_steps_left = 0;
                } else if (!NPC_IsWalking(npc_idx)) {
                    NPC_DoScriptedStep(npc_idx, s_scene_move_dir);
                    s_scene_move_steps_left--;
                }
            }
            if (s_scene_move_steps_left <= 0) s_scene_pc++;
        } else {
            if (s_scene_pc < 0 || s_scene_pc >= s_scene_cmd_count) {
                s_scene_active = 0;
                wJoyIgnore = 0;
                printf("[scene] finished\n");
            } else {
                scene_cmd_t *cmd = &s_scene_cmds[s_scene_pc];
                switch (cmd->op) {
                    case SCOP_SPAWN: {
                        int idx = -1;
                        int spawned_new = 0;
                        int ai_prev = scene_find_actor(cmd->actor);
                        if (ai_prev >= 0 && s_scene_actors[ai_prev].used) {
                            idx = s_scene_actors[ai_prev].npc_idx;
                        }
                        if (idx < 0) {
                            /* Reuse an NPC already occupying this tile to avoid
                             * stacked duplicate sprites during interaction scenes. */
                            idx = NPC_FindAtTile(cmd->b, cmd->c);
                        }
                        if (idx < 0) {
                            idx = NPC_DebugSpawn((uint8_t)cmd->a, cmd->b, cmd->c, 0, 0);
                            if (idx >= 0) spawned_new = 1;
                        }
                        {
                            int ai = scene_add_actor(cmd->actor, idx);
                            if (ai >= 0) {
                                /* Only mark scene-owned when we truly spawned one. */
                                if (spawned_new) s_scene_actors[ai].spawned_by_scene = 1;
                                s_scene_actors[ai].sprite_id = (uint8_t)cmd->a;
                                s_scene_actors[ai].last_x = cmd->b;
                                s_scene_actors[ai].last_y = cmd->c;
                            }
                        }
                        s_scene_pc++;
                    } break;
                    case SCOP_DESPAWN: {
                        int ai = scene_find_actor(cmd->actor);
                        if (ai >= 0) NPC_DebugDespawn(s_scene_actors[ai].npc_idx);
                        if (ai >= 0) s_scene_actors[ai].spawned_by_scene = 0;
                        s_scene_pc++;
                    } break;
                    case SCOP_FACE: {
                        int ai = scene_find_actor(cmd->actor);
                        if (ai >= 0) {
                            int dir = cmd->a;
                            if (dir == -2) {
                                NPC_FacePlayer(s_scene_actors[ai].npc_idx);
                            } else {
                                NPC_SetFacing(s_scene_actors[ai].npc_idx, dir);
                            }
                        }
                        s_scene_pc++;
                    } break;
                    case SCOP_MOVE: {
                        int ai = scene_find_actor(cmd->actor);
                        if (ai < 0) { s_scene_pc++; break; }
                        s_scene_move_actor = ai;
                        s_scene_move_dir = cmd->a;
                        s_scene_move_steps_left = (cmd->b < 0) ? 0 : cmd->b;
                        if (s_scene_move_steps_left <= 0) s_scene_pc++;
                    } break;
                    case SCOP_SAY:
                        if (!Text_IsOpen() && !s_scene_wait_say) {
                            Text_ShowASCII(cmd->text);
                            s_scene_wait_say = 1;
                            s_scene_say_opened = Text_IsOpen() ? 1 : 0;
                            s_scene_pc++;
                        }
                        break;
                    case SCOP_ASK:
                        if (!Text_IsOpen() && !YesNo_IsOpen()) {
                            s_scene_yesno_prev_joyignore = wJoyIgnore;
                            s_scene_yesno_restore_joyignore = 1;
                            s_scene_yesno_prev_scripted_movement = gScriptedMovement;
                            s_scene_yesno_restore_scripted_movement = 1;
                            /* Freeze player movement during ask prompt, but keep
                             * UP/DOWN available for Yes/No cursor selection. */
                            gScriptedMovement = 1;
                            wJoyIgnore = (uint8_t)(wJoyIgnore & (uint8_t)~(PAD_UP | PAD_DOWN));
                            YesNo_Show(cmd->text);
                            s_scene_wait_yesno = 1;
                            s_scene_pc++;
                        }
                        break;
                    case SCOP_BATTLESTART: {
                        if (cmd->c == 1) {
                            scene_start_custom_trainer_battle(cmd);
                            s_scene_wait_battle = 1;
                            s_scene_pc++;
                            printf("[cli] scene battlestart: custom team count=%u\n", (unsigned)cmd->team_count);
                        } else {
                            int tc = cmd->a;
                            int tn = cmd->b;
                            if (tc == -1) {
                                if (!scene_pick_random_map_trainer(&tc, &tn)) {
                                    tc = 34; tn = 1; /* fallback to Brock */
                                }
                            }
                            if (tc < 1) tc = 34;
                            if (tc > NUM_TRAINERS) tc = NUM_TRAINERS;
                            if (tn < 1) tn = 1;
                            s_scene_battlestart_tc = tc;
                            s_scene_battlestart_tn = tn;
                            s_scene_battlestart_pending = 1;
                            s_scene_battlestart_saw_text = Text_IsOpen() ? 1 : 0;
                            s_scene_battlestart_delay = 0;
                        }
                    } break;
                    case SCOP_BATTLEEND:
                        if (get_scene() == 2 || BattleUI_IsActive()) break;
                        if (Game_IsWarpFadeActive()) break;
                        if (!Text_IsOpen()) {
                            Text_ShowASCII(cmd->text);
                            s_scene_wait_battleend_text = 1;
                            s_scene_pc++;
                        }
                        break;
                    case SCOP_MUSIC:
                        Music_Play((uint8_t)cmd->a);
                        s_scene_pc++;
                        break;
                    case SCOP_WAIT:
                        s_scene_wait = (cmd->a < 0) ? 0 : cmd->a;
                        s_scene_pc++;
                        break;
                    case SCOP_WAIT_TEXT:
                        if (!Text_IsOpen()) s_scene_pc++;
                        break;
                    case SCOP_LOCK_INPUT:
                        wJoyIgnore = cmd->a ? PAD_CTRL_PAD : 0;
                        s_scene_pc++;
                        break;
                    case SCOP_END:
                        s_scene_active = 0;
                        wJoyIgnore = 0;
                        printf("[scene] end\n");
                        break;
                    default:
                        s_scene_pc++;
                        break;
                }
            }
        }
    }

    if (!s_scene_active && get_scene() == 0 && !Player_IsMoving() && !Text_IsOpen()) {
        for (int i = 0; i < SCENE_TRIGGER_MAX; i++) {
            scene_trigger_t *t = &s_scene_triggers[i];
            if (!t->used) continue;
            if ((int)t->map_id != (int)wCurMap) continue;
            if (t->cond_kind == 1 && !CheckEvent(t->cond_event)) continue;
            if (t->cond_kind == 2 && CheckEvent(t->cond_event)) continue;
            if ((int)wXCoord == t->x && (int)wYCoord == t->y) {
                if (!t->armed) continue;
                scene_reset_runtime();
                int ncmd = scene_load_file(t->scene);
                if (ncmd > 0) {
                    s_scene_active = 1;
                    s_scene_pc = 0;
                    t->armed = 0;
                    printf("[cli] scene_trigger: fired '%s' @ (%d,%d)\n", t->scene, t->x, t->y);
                } else {
                    t->armed = 0;
                    printf("[cli] scene_trigger: failed to load '%s'\n", t->scene);
                }
                break;
            } else {
                t->armed = 1;
            }
        }
    }

    if (s_script_trace_enabled) {
        uint8_t map_now = wCurMap;
        uint8_t trainer_now = (uint8_t)(Trainer_IsEngaging() ? 1 : 0);
        uint8_t text_now = (uint8_t)(Text_IsOpen() ? 1 : 0);
        uint8_t gate_now = (uint8_t)(Gate_PewterIsActive() ? 1 : 0);
        uint8_t r22_now = (uint8_t)(Route22Scripts_IsActive() ? 1 : 0);
        uint8_t r24_now = (uint8_t)(Route24Scripts_IsActive() ? 1 : 0);
        uint8_t ss_now = (uint8_t)(SSAnneScripts_IsActive() ? 1 : 0);
        uint8_t vm_now = (uint8_t)(ViridianMartScripts_IsActive() ? 1 : 0);
        uint8_t gym_now = (uint8_t)(GymScripts_IsActive() ? 1 : 0);
        uint8_t rb4f_now = (uint8_t)(RocketHideoutB4FScripts_IsActive() ? 1 : 0);

        if (map_now != s_trace_prev_map) {
            cli_script_trace_emitf("[script_trace] map=%u (%s)", map_now, gMapTable[map_now].name);
            s_trace_prev_map = map_now;
        }
        if (trainer_now != s_trace_prev_trainer_engaging) {
            cli_script_trace_emitf("[script_trace] trainer_engage=%u", trainer_now);
            s_trace_prev_trainer_engaging = trainer_now;
        }
        if (text_now != s_trace_prev_text_open) {
            cli_script_trace_emitf("[script_trace] text_open=%u", text_now);
            s_trace_prev_text_open = text_now;
        }
        if (gate_now != s_trace_prev_gate_active) {
            cli_script_trace_emitf("[script_trace] gate_active=%u", gate_now);
            s_trace_prev_gate_active = gate_now;
        }
        if (r22_now != s_trace_prev_route22_active) {
            cli_script_trace_emitf("[script_trace] route22_active=%u", r22_now);
            s_trace_prev_route22_active = r22_now;
        }
        if (r24_now != s_trace_prev_route24_active) {
            cli_script_trace_emitf("[script_trace] route24_active=%u", r24_now);
            s_trace_prev_route24_active = r24_now;
        }
        if (ss_now != s_trace_prev_ssanne_active) {
            cli_script_trace_emitf("[script_trace] ssanne_active=%u", ss_now);
            s_trace_prev_ssanne_active = ss_now;
        }
        if (vm_now != s_trace_prev_viridian_mart_active) {
            cli_script_trace_emitf("[script_trace] viridian_mart_active=%u", vm_now);
            s_trace_prev_viridian_mart_active = vm_now;
        }
        if (gym_now != s_trace_prev_gym_active) {
            cli_script_trace_emitf("[script_trace] gym_active=%u", gym_now);
            s_trace_prev_gym_active = gym_now;
        }
        if (rb4f_now != s_trace_prev_rockethideout_b4f_active) {
            cli_script_trace_emitf("[script_trace] rocket_b4f_active=%u", rb4f_now);
            s_trace_prev_rockethideout_b4f_active = rb4f_now;
        }
    }

    /* Auto-drive move animation lab while battle UI is active. */
    if (s_animlab_enabled && get_scene() == 2 /* BATTLE */) {
        /* Keep battle state stable so the sequence can run indefinitely. */
        wBattleMon.hp = wBattleMon.max_hp;
        wEnemyMon.hp  = wEnemyMon.max_hp;
        wBattleMon.status = 0;
        wEnemyMon.status  = 0;
        wPlayerBattleStatus1 = wPlayerBattleStatus2 = wPlayerBattleStatus3 = 0;
        wEnemyBattleStatus1  = wEnemyBattleStatus2  = wEnemyBattleStatus3  = 0;
        animlab_set_enemy_harmless();

        /* If dialogue is open, tap A to advance. */
        if (Text_IsOpen()) {
            gCliButtons = BTN_A;
            gCliFrames  = 1;
        } else if (s_seq_pos >= s_seq_len) {
            int bui = BattleUI_GetState();
            if (bui == 10 /* BUI_MENU */) {
                uint8_t move_id = (uint8_t)((s_animlab_move_id > 0 && s_animlab_move_id < NUM_MOVE_DEFS)
                    ? s_animlab_move_id : 1);
                animlab_set_player_move(move_id);

                seq_clear();
                seq_battle_menu(0);
                seq_move_select(1);

                s_animlab_move_id++;
                if (s_animlab_move_id >= NUM_MOVE_DEFS) {
                    s_animlab_move_id = 1;
                    s_animlab_loops++;
                }
            } else if (bui == 11 /* BUI_MOVE_SELECT */) {
                seq_clear();
                seq_push(BTN_A, PRESS, GAP);
            }
        }
    }

    /* After sequence done + reaction wait, write state */
    if (s_pending_write && s_seq_pos >= s_seq_len) {
        if (s_wait_remaining > 0) {
            s_wait_remaining--;
        } else {
            write_state();
            s_pending_write = 0;
        }
    }

    if (s_replay_recording && s_replay_rec_fp) {
        uint8_t raw = hJoyInput;
        fwrite(&raw, 1, 1, s_replay_rec_fp);
    }

    /* Poll for new command only when idle */
    if (s_seq_pos >= s_seq_len && gCliFrames == 0) {
        if (++s_poll_timer >= 30) {
            s_poll_timer = 0;
            poll_cmd_file();
        }
    }
}

int DebugCLI_OnNpcInteracted(int npc_idx) {
    int bi;
    int ncmd;
    if (npc_idx < 0) return 0;
    bi = scene_npc_binding_find_by_idx(npc_idx);
    if (bi < 0) return 0;
    if (s_scene_active) return 1;
    scene_reset_runtime();
    ncmd = scene_load_file(s_scene_npc_bindings[bi].scene);
    if (ncmd <= 0) {
        printf("[cli] scene_npc interact: failed to load scene '%s'\n",
               s_scene_npc_bindings[bi].scene);
        return 1;
    }
    s_scene_active = 1;
    s_scene_pc = 0;
    printf("[cli] scene_npc interact: '%s' -> scene '%s' (%d command(s))\n",
           s_scene_npc_bindings[bi].name, s_scene_npc_bindings[bi].scene, ncmd);
    return 1;
}

void DebugCLI_PostRender(void) {
    if (s_scene_wait_yesno && YesNo_IsOpen())
        YesNo_PostRender();
}

/* ---- Console public API ------------------------------------------ */

void DebugCLI_ConsoleOpen(void) {
    if (s_con_open) return;
    if (s_con_overlay_enabled) {
        for (int c = 0; c < SCREEN_WIDTH; c++) {
            s_con_saved[c]                = gScrollTileMap[CON_TMIDX(CON_TOP_ROW, c)];
            s_con_saved[SCREEN_WIDTH + c] = gScrollTileMap[CON_TMIDX(CON_IN_ROW,  c)];
        }
    }
    s_con_len    = 0;
    s_con_buf[0] = '\0';
    s_con_blink  = 0;
    s_con_open   = 1;
    con_draw();
}

void DebugCLI_ConsoleClose(void) {
    if (s_con_always_open) return;
    if (!s_con_open) return;
    if (s_con_overlay_enabled) {
        for (int c = 0; c < SCREEN_WIDTH; c++) {
            gScrollTileMap[CON_TMIDX(CON_TOP_ROW, c)] = s_con_saved[c];
            gScrollTileMap[CON_TMIDX(CON_IN_ROW,  c)] = s_con_saved[SCREEN_WIDTH + c];
        }
    }
    s_con_open = 0;
}

int DebugCLI_ConsoleIsOpen(void) {
    return s_con_open;
}

void DebugCLI_ConsoleAddChar(char c) {
    if (!s_con_open || s_con_len >= CON_BUFMAX) return;
    s_con_buf[s_con_len++] = c;
    s_con_buf[s_con_len]   = '\0';
    s_con_blink = 0;
    con_draw();
}

void DebugCLI_ConsoleBackspace(void) {
    if (!s_con_open || s_con_len == 0) return;
    s_con_buf[--s_con_len] = '\0';
    s_con_blink = 0;
    con_draw();
}

void DebugCLI_ConsoleExecute(void) {
    if (!s_con_open) return;
    if (s_con_len > 0) {
        printf("[console] %s\n", s_con_buf);
        {
            char logline[CLI_HIST_WIDTH + 1];
            snprintf(logline, sizeof(logline), "> %s", s_con_buf);
            s_last_cmd_hist_slot = cli_hist_push(logline);
        }
        process_cmd(s_con_buf);
        if (s_last_cmd_hist_slot >= 0) {
            s_hist_color[s_last_cmd_hist_slot] =
                (uint8_t)(s_last_cmd_valid ? CLI_HIST_COLOR_OK : CLI_HIST_COLOR_ERROR);
            s_last_cmd_hist_slot = -1;
        }
    }
    if (s_con_always_open) {
        s_con_len = 0;
        s_con_buf[0] = '\0';
        s_con_blink = 0;
        con_draw();
    } else {
        DebugCLI_ConsoleClose();
    }
}

void DebugCLI_ConsoleRender(void) {
    if (!s_con_open) return;
    s_con_blink = (s_con_blink + 1) % CON_BLINK_PERIOD;
    con_draw();
}

void DebugCLI_ConsoleSetOverlayEnabled(int enabled) {
    s_con_overlay_enabled = enabled ? 1 : 0;
}

void DebugCLI_ConsoleSetAlwaysOpen(int enabled) {
    s_con_always_open = enabled ? 1 : 0;
    if (s_con_always_open && !s_con_open) DebugCLI_ConsoleOpen();
}

const char *DebugCLI_ConsoleGetBuffer(void) {
    return s_con_buf;
}
