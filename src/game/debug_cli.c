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
#include "trainer_sight.h"      /* Trainer_IsEngaging */
#include "pokecenter.h"         /* Pokecenter_IsWaitingYesNo */
#include "pokemon.h"            /* Pokemon_GetName, Pokemon_InitMon */
#include "battle/battle_ui.h"   /* BattleUI_GetState */
#include "../data/base_stats.h" /* gSpeciesToDex */
#include "../data/map_data.h"       /* gMapTable */
#include "../data/moves_data.h"     /* BMOVE */
#include "../data/event_constants.h"
#include "inventory.h"
#include "../platform/hardware.h"
#include "../game/constants.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../data/font_data.h"

#define CMD_FILE   "bugs/cli_cmd.txt"
#define STATE_FILE "bugs/cli_state.txt"

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

/* ---- State writers ------------------------------------------------ */

static void write_battle_state(FILE *fp) {
    int bui = BattleUI_GetState();
    const char *btype = (wIsInBattle == 2) ? "TRAINER" : "WILD";

    fprintf(fp, "=== BATTLE (%s) ===\n", btype);
    fprintf(fp, "UI State: %s\n\n", bui_state_name(bui));

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

static char tile_char(int mx, int my, int px, int py, int nc) {
    /* px/py and NPC positions are in game coords; convert to tile coords.
     * mx/my are already tile coords (from gCamX + tx loop). */
    if (mx == px * 2 && my == py * 2 + 1) return '@';
    for (int i = 0; i < nc; i++) {
        int ntx, nty;
        NPC_GetTilePos(i, &ntx, &nty);
        if (ntx * 2 == mx && nty * 2 + 1 == my) return 'N';
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
    };
    static const int LEGEND_COUNT = (int)(sizeof(legend) / sizeof(legend[0]));

    int nc = NPC_GetCount();
    int px = (int)wXCoord, py = (int)wYCoord;

    fprintf(fp, "+");
    for (int x = 0; x < SCREEN_WIDTH; x++) fprintf(fp, "-");
    fprintf(fp, "+  Legend:\n");
    for (int ty = 0; ty < SCREEN_HEIGHT; ty++) {
        fprintf(fp, "|");
        for (int tx = 0; tx < SCREEN_WIDTH; tx++)
            fprintf(fp, "%c", tile_char(gCamX + tx, gCamY + ty, px, py, nc));
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
    char verb[32] = {0};
    int  n = 1;
    sscanf(cmd, "%31s %d", verb, &n);
    if (n < 1) n = 1;

    seq_clear();

    /* ---- Text/dialogue lock: only a/b accepted while text is open ---- */
    if (Text_IsOpen() || Pokecenter_IsWaitingYesNo()) {
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
            /* Towns / Cities */
            { "pallet",           0,  10, 18 },
            { "pallet_town",      0,  10, 18 },
            { "viridian",         1,  24, 40 },
            { "viridian_city",    1,  24, 40 },
            { "pewter",           2,  16, 18 },
            { "pewter_city",      2,  16, 18 },
            { "cerulean",         3,  28, 18 },
            { "cerulean_city",    3,  28, 18 },
            { "vermilion",        5,  28, 24 },
            { "vermilion_city",   5,  28, 24 },
            { "lavender",         4,  14, 18 },
            { "lavender_town",    4,  14, 18 },
            { "celadon",          6,  44, 24 },
            { "celadon_city",     6,  44, 24 },
            { "fuchsia",          7,  28, 24 },
            { "fuchsia_city",     7,  28, 24 },
            { "cinnabar",         8,  14, 18 },
            { "cinnabar_island",  8,  14, 18 },
            { "saffron",         10,  28, 28 },
            { "saffron_city",    10,  28, 28 },
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
            /* Numeric: teleport <map_id> <x> <y> */
            map_id = 0; x = 3; y = 3;
            sscanf(cmd, "%*s %d %d %d", &map_id, &x, &y);
        }

        extern void Map_Load(uint8_t map_id);
        extern void NPC_Load(void);
        extern void PalletScripts_OnMapLoad(void);
        extern void OaksLabScripts_OnMapLoad(void);
        wCurMap  = (uint8_t)map_id;
        wXCoord  = (uint8_t)x;
        wYCoord  = (uint8_t)y;
        Map_Load(wCurMap);
        NPC_Load();
        PalletScripts_OnMapLoad();
        OaksLabScripts_OnMapLoad();
        printf("[cli] teleport → map %d (%d,%d)\n", map_id, x, y);
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
    else if (strcmp(verb, "checkpoint") == 0) {
        /* checkpoint <name> — set a batch of event flags for a story state.
         * Available checkpoints:
         *   parcel_ready   — has starter, battled rival, standing in Viridian
         *   pokedex_ready  — has parcel, standing outside Oak's Lab
         */
        char cp[32] = {0};
        sscanf(cmd, "%*s %31s", cp);

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
            printf("[cli] checkpoint: cerulean — at Cerulean City bridge (rival trigger at y=6)\n");
        } else {
            printf("[cli] Unknown checkpoint: %s\n"
                   "      Available: parcel_ready, pokedex_ready, mt_moon, cerulean\n", cp);
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
    else {
        printf("[cli] Unknown command: %s\n", verb);
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
    process_cmd(line);
}

/* ---- Public tick -------------------------------------------------- */

void DebugCLI_Tick(void) {
    /* Feed sequence into injection globals one frame at a time.
     * DebugCLI_Tick runs after Input_Update, so whatever we set here
     * is consumed by Input_Update on the NEXT frame. */
    if (s_seq_pos < s_seq_len) {
        gCliButtons = s_seq[s_seq_pos++];
        gCliFrames  = 1;
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

    /* Poll for new command only when idle */
    if (s_seq_pos >= s_seq_len && gCliFrames == 0) {
        if (++s_poll_timer >= 30) {
            s_poll_timer = 0;
            poll_cmd_file();
        }
    }
}

/* ---- Console public API ------------------------------------------ */

void DebugCLI_ConsoleOpen(void) {
    if (s_con_open) return;
    for (int c = 0; c < SCREEN_WIDTH; c++) {
        s_con_saved[c]                = gScrollTileMap[CON_TMIDX(CON_TOP_ROW, c)];
        s_con_saved[SCREEN_WIDTH + c] = gScrollTileMap[CON_TMIDX(CON_IN_ROW,  c)];
    }
    s_con_len    = 0;
    s_con_buf[0] = '\0';
    s_con_blink  = 0;
    s_con_open   = 1;
    con_draw();
}

void DebugCLI_ConsoleClose(void) {
    if (!s_con_open) return;
    for (int c = 0; c < SCREEN_WIDTH; c++) {
        gScrollTileMap[CON_TMIDX(CON_TOP_ROW, c)] = s_con_saved[c];
        gScrollTileMap[CON_TMIDX(CON_IN_ROW,  c)] = s_con_saved[SCREEN_WIDTH + c];
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
        process_cmd(s_con_buf);
    }
    DebugCLI_ConsoleClose();
}

void DebugCLI_ConsoleRender(void) {
    if (!s_con_open) return;
    s_con_blink = (s_con_blink + 1) % CON_BLINK_PERIOD;
    con_draw();
}
