/* pokecenter.c — Pokémon Center nurse healing sequence.
 *
 * Source reference: pokered-master/engine/events/pokecenter.asm
 *                   pokered-master/engine/events/heal_party.asm
 *                   pokered-master/data/text/text_4.asm (lines 159-188)
 *
 * Sequence (mirrors Gen 1 exactly):
 *   1. "Welcome to our POKeMON CENTER! ..." (multi-page)
 *   2. "Shall we heal your POKeMON?" (kept visible) → YES/NO box
 *   3a. YES: "OK. We'll need your POKeMON." → HealParty → heal anim → "Thank you!" → farewell
 *   3b. NO: farewell
 *
 * YES/NO box: drawn at cols 14-19, rows 8-11 (above the text box which is rows 12-17).
 * Matches YesNoChoicePokeCenter (home/yes_no.asm): HEAL / CANCEL options.
 *
 * HealParty (heal_party.asm):
 *   For each party mon: status=0, HP=MaxHP, PP[i] = (pp_up_bits | base_pp) + bonus
 *   Bonus: pp_up_count * floor(base_pp/5)   (AddBonusPP in item_effects.asm:2419)
 */

#include "pokecenter.h"
#include "text.h"
#include "constants.h"
#include "music.h"
#include "npc.h"
#include "../platform/hardware.h"
#include "../platform/audio.h"
#include "../platform/display.h"
#include "../data/font_data.h"
#include "../data/moves_data.h"
#include "overworld.h"

#include <stdint.h>

/* ---- State machine ------------------------------------------------- */
typedef enum {
    PC_IDLE = 0,
    PC_WAIT_WELCOME,      /* text open: welcome + page 2 health message */
    PC_WAIT_QUESTION,     /* text open: "Shall we heal?" (kept on close) */
    PC_YESNO,             /* YES/NO box visible, awaiting input */
    PC_WAIT_NEED,         /* text open: "OK. We'll need your POKeMON." */
    PC_NURSE_TURN,        /* nurse facing UP — ~35 frame pause (mirrors Delay3) */
    PC_HEALING,           /* per-mon SFX loop (mirrors AnimateHealingMachine loop) */
    PC_HEALING_JINGLE,    /* MUSIC_PKMN_HEALED + 8-flash sprite effect (concurrent) */
    PC_HEALING_WAIT32,    /* 32 frames after jingle finishes (mirrors DelayFrames 32) */
    PC_NURSE_BOW,         /* nurse turns DOWN and bows — 20 frames */
    PC_WAIT_HEALED,       /* text open: "Thank you! Your POKeMON are fighting fit!" */
    PC_WAIT_FAREWELL,     /* text open: "We hope to see you again!" */
    PC_WAIT_DECLINE,      /* text open: farewell after NO */
} pc_state_t;

static pc_state_t g_state     = PC_IDLE;
static int        g_cursor    = 0;   /* 0=YES/HEAL, 1=NO/CANCEL */
static int        g_heal_timer = 0;  /* frames remaining in current per-mon window */
static int        g_heal_mon   = 0;  /* which party mon we're on */
static int        g_nurse_npc  = -1; /* NPC index of the nurse sprite */
static int        g_flash_count = 0; /* flashes fired in PC_HEALING_JINGLE */
static int        g_flash_timer = 0; /* frames until next flash toggle */
static int        g_flash_on    = 0; /* current flash palette state (0=normal) */
static int        g_used_pokecenter = 0; /* BIT_USED_POKECENTER mirror */
static int        g_machine_px  = 0; /* heal machine base screen X (computed at nurse-turn) */
static int        g_machine_py  = 0; /* heal machine base screen Y */
static int        g_ball_count  = 0; /* pokeballs placed so far */

/* Heal machine sprite tile data (from gfx/overworld/heal_machine.2bpp).
 * tile 0 ($7C) = monitor/desk graphic, tile 1 ($7D) = pokeball graphic.
 * Each tile = 8 rows × 2 bytes = 16 bytes; total 32 bytes. */
static const uint8_t kHealMachineTiles[32] = {
    /* tile 0 ($7C) — monitor/desk; extracted from gfx/overworld/heal_machine.png */
    0x00, 0x00,  /* row 0 */ 0x00, 0x00,  /* row 1 */
    0x7E, 0x00,  /* row 2 */ 0x7E, 0x00,  /* row 3 */
    0x7E, 0x00,  /* row 4 */ 0x00, 0x00,  /* row 5 */
    0x00, 0x00,  /* row 6 */ 0x00, 0x00,  /* row 7 */
    /* tile 1 ($7D) — pokeball; color 0 = transparent */
    0x00, 0x00,  /* row 0 */ 0x00, 0x00,  /* row 1 */
    0x0C, 0x0C,  /* row 2 */ 0x12, 0x1E,  /* row 3 */
    0x21, 0x3F,  /* row 4 */ 0x33, 0x2D,  /* row 5 */
    0x1E, 0x12,  /* row 6 */ 0x0C, 0x0C,  /* row 7 */
};

/* OAM slots 72-78: free in the overworld (player=0-3, NPCs=4-67, shadow=68-71). */
#define MACHINE_OAM_BASE  72
#define MACHINE_OAM_COUNT  7   /* 1 monitor + 6 balls */
#define OAM_PAL1          0x10 /* bit4 = use OBP1 palette */
#define OAM_XFLIP         0x20 /* bit5 = horizontal flip */

/* Normal OBP1 palette (sprite palette 1) and flash variant.
 * Original: OBP1=$E0, XOR $28 → $C8.  Matches FlashSprite8Times in healing_machine.asm */
#define OBP1_NORMAL  0xE0
#define OBP1_FLASH   0xC8

static void pc_set_obp1(uint8_t obp1) {
    Display_SetPalette(0xE4, 0xD0, obp1);
}

/* Set one heal machine OAM slot (0=monitor, 1-6=balls). */
static void pc_machine_oam_set(int idx, int spx, int spy, uint8_t tile, uint8_t flags) {
    wShadowOAM[MACHINE_OAM_BASE + idx].y     = (uint8_t)(spy + OAM_Y_OFS);
    wShadowOAM[MACHINE_OAM_BASE + idx].x     = (uint8_t)(spx + OAM_X_OFS);
    wShadowOAM[MACHINE_OAM_BASE + idx].tile  = tile;
    wShadowOAM[MACHINE_OAM_BASE + idx].flags = flags;
}

/* Hide all heal machine OAM slots (set Y=0 = off-screen). */
static void pc_machine_oam_clear(void) {
    for (int i = 0; i < MACHINE_OAM_COUNT; i++)
        wShadowOAM[MACHINE_OAM_BASE + i].y = 0;
}

/* ---- YES/NO box tile helpers --------------------------------------- */
/* Box occupies cols 14-19, rows 8-11 (6 wide × 4 tall). */
#define YESNO_COL   14
#define YESNO_ROW    8
#define YESNO_W      6
#define YESNO_H      4

/* Pokered box-drawing char codes (Font_CharToTile mapping):
 *   0x79=┌  0x7A=─  0x7B=┐  0x7C=│  0x7D=└  0x7E=┘  0x7F=space */
#define BC_TL   0x79u   /* ┌ */
#define BC_H    0x7Au   /* ─ */
#define BC_TR   0x7Bu   /* ┐ */
#define BC_V    0x7Cu   /* │ */
#define BC_BL   0x7Du   /* └ */
#define BC_BR   0x7Eu   /* ┘ */
#define BC_SP   0x7Fu   /* space */
#define BC_CUR  0xEDu   /* ▶ cursor arrow */

static void pc_set_tile(int col, int row, uint8_t tile) {
    if (col < 0 || col >= SCREEN_WIDTH || row < 0 || row >= SCREEN_HEIGHT) return;
    gScrollTileMap[(row + 2) * SCROLL_MAP_W + (col + 2)] = tile;
}

/* Convert a single ASCII char to a pokered font tile slot.
 * Mirrors bui_char_to_tile in battle_ui.c. */
static uint8_t pc_char_tile(char c) {
    if (c >= 'A' && c <= 'Z') return (uint8_t)Font_CharToTile((unsigned char)(0x80 + (c - 'A')));
    if (c >= 'a' && c <= 'z') return (uint8_t)Font_CharToTile((unsigned char)(0xA0 + (c - 'a')));
    if (c >= '0' && c <= '9') return (uint8_t)Font_CharToTile((unsigned char)(0xF6 + (c - '0')));
    if (c == '!') return (uint8_t)Font_CharToTile(0xE7u);
    return (uint8_t)Font_CharToTile(BC_SP); /* space */
}

static void pc_draw_yesno_box(void) {
    /* Top border: ┌────┐ */
    pc_set_tile(YESNO_COL,             YESNO_ROW,     (uint8_t)Font_CharToTile(BC_TL));
    for (int c = 1; c < YESNO_W - 1; c++)
        pc_set_tile(YESNO_COL + c,     YESNO_ROW,     (uint8_t)Font_CharToTile(BC_H));
    pc_set_tile(YESNO_COL + YESNO_W-1,YESNO_ROW,     (uint8_t)Font_CharToTile(BC_TR));

    /* HEAL / YES row */
    pc_set_tile(YESNO_COL,             YESNO_ROW + 1, (uint8_t)Font_CharToTile(BC_V));
    pc_set_tile(YESNO_COL + 1,         YESNO_ROW + 1,
                g_cursor == 0 ? (uint8_t)Font_CharToTile(BC_CUR)
                              : (uint8_t)Font_CharToTile(BC_SP));
    pc_set_tile(YESNO_COL + 2,         YESNO_ROW + 1, pc_char_tile('Y'));
    pc_set_tile(YESNO_COL + 3,         YESNO_ROW + 1, pc_char_tile('E'));
    pc_set_tile(YESNO_COL + 4,         YESNO_ROW + 1, pc_char_tile('S'));
    pc_set_tile(YESNO_COL + YESNO_W-1,YESNO_ROW + 1, (uint8_t)Font_CharToTile(BC_V));

    /* CANCEL / NO row */
    pc_set_tile(YESNO_COL,             YESNO_ROW + 2, (uint8_t)Font_CharToTile(BC_V));
    pc_set_tile(YESNO_COL + 1,         YESNO_ROW + 2,
                g_cursor == 1 ? (uint8_t)Font_CharToTile(BC_CUR)
                              : (uint8_t)Font_CharToTile(BC_SP));
    pc_set_tile(YESNO_COL + 2,         YESNO_ROW + 2, pc_char_tile('N'));
    pc_set_tile(YESNO_COL + 3,         YESNO_ROW + 2, pc_char_tile('O'));
    pc_set_tile(YESNO_COL + 4,         YESNO_ROW + 2, (uint8_t)Font_CharToTile(BC_SP));
    pc_set_tile(YESNO_COL + YESNO_W-1,YESNO_ROW + 2, (uint8_t)Font_CharToTile(BC_V));

    /* Bottom border: └────┘ */
    pc_set_tile(YESNO_COL,             YESNO_ROW + 3, (uint8_t)Font_CharToTile(BC_BL));
    for (int c = 1; c < YESNO_W - 1; c++)
        pc_set_tile(YESNO_COL + c,     YESNO_ROW + 3, (uint8_t)Font_CharToTile(BC_H));
    pc_set_tile(YESNO_COL + YESNO_W-1,YESNO_ROW + 3, (uint8_t)Font_CharToTile(BC_BR));
}

static void pc_clear_yesno_box(void) {
    for (int r = 0; r < YESNO_H; r++)
        for (int c = 0; c < YESNO_W; c++)
            pc_set_tile(YESNO_COL + c, YESNO_ROW + r, (uint8_t)Font_CharToTile(BC_SP));
}

/* ---- HealParty ----------------------------------------------------- */
/* Mirrors heal_party.asm:
 *   For each mon: clear status, restore PP (with PP Up bonus), restore HP=MaxHP.
 *   PP byte layout: bits 7-6 = PP Up count, bits 5-0 = current PP.
 *   After heal: pp[i] = (pp_ups << 6) | (base_pp + pp_ups * floor(base_pp/5)) */
static void heal_party(void) {
    for (int i = 0; i < wPartyCount && i < 6; i++) {
        party_mon_t *mon = &wPartyMons[i];

        /* Clear status condition */
        mon->base.status = 0;

        /* Restore PP for each move slot */
        for (int m = 0; m < 4; m++) {
            uint8_t move_id = mon->base.moves[m];
            if (move_id == 0 || move_id >= NUM_MOVE_DEFS) continue;

            uint8_t pp_ups    = (mon->base.pp[m] >> 6) & 0x03;
            uint8_t base_pp   = gMoves[move_id].pp;
            uint8_t bonus     = (uint8_t)(pp_ups * (base_pp / 5));
            uint16_t new_pp   = (uint16_t)base_pp + bonus;
            if (new_pp > 63) new_pp = 63;
            mon->base.pp[m]   = (uint8_t)((pp_ups << 6) | (uint8_t)new_pp);
        }

        /* Restore HP to MaxHP */
        mon->base.hp = mon->max_hp;
    }
}

/* ---- Public API ---------------------------------------------------- */
void Pokecenter_Start(void) {
    g_state     = PC_WAIT_WELCOME;
    g_cursor    = 0;
    g_nurse_npc = NPC_GetLastInteracted();  /* remember which NPC is the nurse */
    /* Welcome message with paragraph break (mirrors PokemonCenterWelcomeText).
     * \f = para (wait A, clear, continue). */
    Text_ShowASCII("Welcome to our\nPOKeMON CENTER!\fWe heal your\nPOKeMON to full\nhealth!");
}

int Pokecenter_IsActive(void) {
    return g_state != PC_IDLE;
}

void Pokecenter_Tick(void) {
    /* Should not be called while text is open — game.c handles that. */
    switch (g_state) {

    case PC_WAIT_WELCOME:
        /* Text just closed — show "Shall we heal?" question.
         * Use Text_KeepTilesOnClose so the text box remains visible under the YES/NO box.
         * Mirrors ShallWeHealYourPokemonText (text_pause + text_far + text_end). */
        if (!g_used_pokecenter) {
            Text_KeepTilesOnClose();
            Text_ShowASCII("Shall we heal\nyour POKeMON?");
            g_state = PC_WAIT_QUESTION;
        } else {
            /* Already visited — skip question, go straight to YES/NO */
            g_cursor = 0;
            pc_draw_yesno_box();
            g_state = PC_YESNO;
        }
        break;

    case PC_WAIT_QUESTION:
        /* "Shall we heal?" text just closed (tiles kept).
         * Draw YES/NO box on top. */
        g_cursor = 0;
        pc_draw_yesno_box();
        g_state = PC_YESNO;
        break;

    case PC_YESNO:
        /* Handle UP/DOWN cursor movement */
        if (hJoyPressed & PAD_UP) {
            if (g_cursor > 0) { g_cursor--; pc_draw_yesno_box(); }
        }
        if (hJoyPressed & PAD_DOWN) {
            if (g_cursor < 1) { g_cursor++; pc_draw_yesno_box(); }
        }

        if (hJoyPressed & PAD_A) {
            pc_clear_yesno_box();
            if (g_cursor == 0) {
                /* YES / HEAL — mirrors SetLastBlackoutMap + NeedYourPokemonText */
                g_used_pokecenter = 1;
                Map_BuildScrollView();  /* restore map tiles under YES/NO box immediately */
                Text_ShowASCII("OK. We'll need\nyour POKeMON.");
                g_state = PC_WAIT_NEED;
            } else {
                /* NO / CANCEL — mirrors .declinedHealing */
                Text_ShowASCII("We hope to see\nyou again!");
                g_state = PC_WAIT_DECLINE;
            }
        }
        if (hJoyPressed & PAD_B) {
            /* B = cancel (same as NO) */
            pc_clear_yesno_box();
            Text_ShowASCII("We hope to see\nyou again!");
            g_state = PC_WAIT_DECLINE;
        }
        break;

    case PC_WAIT_NEED:
        /* "We'll need your POKeMON." text just closed.
         * Mirror pokecenter.asm:
         *   wSprite01StateData1ImageIndex = $18  (nurse turns UP toward machine)
         *   call Delay3                          (~35 frames)
         *   predef HealParty
         *   farcall AnimateHealingMachine
         */
        /* Restore the background tilemap so the text box and YES/NO box tiles
         * (set to 0 / space by Text_Close / pc_clear_yesno_box) don't leave
         * a visible imprint during the animation. */
        Map_BuildScrollView();
        heal_party();
        Music_Stop();
        NPC_SetFacing(g_nurse_npc, 1);  /* 1 = UP: nurse turns toward machine */
        /* Load heal machine sprite tiles into VRAM slots $7C/$7D
         * (mirrors CopyVideoData for PokeCenterFlashingMonitorAndHealBall). */
        Display_LoadSpriteTile(0x7C, kHealMachineTiles);
        Display_LoadSpriteTile(0x7D, kHealMachineTiles + 16);
        /* Anchor machine sprites to nurse's current screen position.
         * Camera is player-centred (gCamX = player_x - 8, gCamY = player_y - 9),
         * so for a pokecenter (player enters at tile ~3,7) the nurse at tile (3,1)
         * lands at screen pixel (64, 24).  The original PokeCenterOAMData has the
         * monitor at screen pixel (44, 20) → offset = -20px left, -4px above nurse. */
        { int npx, npy; NPC_GetScreenPos(g_nurse_npc, &npx, &npy);
          g_machine_px = npx - 28;
          g_machine_py = npy -  8; }
        g_ball_count = 0;
        /* Show monitor sprite immediately (mirrors first CopyHealingMachineOAM call). */
        pc_machine_oam_set(0, g_machine_px, g_machine_py, 0x7C, OAM_PAL1);
        g_heal_timer = 35;              /* mirrors Delay3 (35 frames) */
        g_state = PC_NURSE_TURN;
        break;

    case PC_NURSE_TURN:
        /* Pause while nurse is turned UP, then start the per-mon SFX loop. */
        if (--g_heal_timer > 0) break;
        g_heal_mon   = 0;
        g_heal_timer = 0;  /* fire first SFX immediately on first PC_HEALING tick */
        g_state = PC_HEALING;
        break;

    case PC_HEALING:
        /* Per-mon SFX loop (mirrors AnimateHealingMachine .partyLoop):
         *   call SFX_HEALING_MACHINE
         *   ld c, 30 / call DelayFrames */
        if (--g_heal_timer > 0) break;
        if (g_heal_mon < wPartyCount && g_heal_mon < 6) {
            Audio_PlaySFX_HealingMachine();
            /* Add pokeball OAM for this mon (mirrors CopyHealingMachineOAM per loop).
             * Balls form a 2-column × 3-row grid below the monitor:
             *   col L (even mon): dx=-4, col R (odd mon): dx=+4, OAM_XFLIP
             *   row 0 (mon 0-1): dy=+7, row 1 (mon 2-3): dy=+12, row 2 (mon 4-5): dy=+17 */
            static const int8_t ball_dx[6] = {-4, +4, -4, +4, -4, +4};
            static const int8_t ball_dy[6] = { 7,  7, 12, 12, 17, 17};
            static const uint8_t ball_fl[6] = { OAM_PAL1, OAM_PAL1|OAM_XFLIP,
                                                 OAM_PAL1, OAM_PAL1|OAM_XFLIP,
                                                 OAM_PAL1, OAM_PAL1|OAM_XFLIP };
            int m = g_heal_mon;
            pc_machine_oam_set(1 + m,
                               g_machine_px + ball_dx[m],
                               g_machine_py + ball_dy[m],
                               0x7D, ball_fl[m]);
            g_heal_mon++;
            g_heal_timer = 30;
        } else {
            /* All mons done — play jingle, set up flash (mirrors FlashSprite8Times). */
            Music_Play(MUSIC_PKMN_HEALED);
            g_flash_count = 0;
            g_flash_timer = 0;  /* toggle palette immediately on first jingle tick */
            g_flash_on    = 0;
            g_state = PC_HEALING_JINGLE;
        }
        break;

    case PC_HEALING_JINGLE:
        /* Concurrent: flash OBP1 8 times × 10 frames (mirrors FlashSprite8Times d=$28).
         * Then wait for jingle to finish (mirrors .waitLoop2). */
        if (g_flash_count < 8) {
            if (--g_flash_timer <= 0) {
                g_flash_on ^= 1;
                pc_set_obp1(g_flash_on ? OBP1_FLASH : OBP1_NORMAL);
                g_flash_timer = 10;
                g_flash_count++;
            }
        } else {
            if (g_flash_on) { g_flash_on = 0; pc_set_obp1(OBP1_NORMAL); }
            if (Music_IsPlaying()) break;
            g_heal_timer = 32;
            g_state = PC_HEALING_WAIT32;
        }
        break;

    case PC_HEALING_WAIT32:
        /* 32-frame pause after jingle (mirrors DelayFrames 32 in healing_machine.asm). */
        if (--g_heal_timer > 0) break;
        pc_machine_oam_clear();         /* remove heal machine sprites (mirrors pop/UpdateSprites) */
        NPC_SetFacing(g_nurse_npc, 0);  /* 0 = DOWN: nurse bows toward player */
        g_heal_timer = 20;              /* mirrors DelayFrames 20 in pokecenter.asm */
        g_state = PC_NURSE_BOW;
        break;

    case PC_NURSE_BOW:
        /* Hold nurse bow pose 20 frames, then restore music and show "Thank you!" text. */
        if (--g_heal_timer > 0) break;
        Music_Play(Music_GetMapID(wCurMap));
        Text_ShowASCII("Thank you!\nYour POKeMON are\nfighting fit!");
        g_state = PC_WAIT_HEALED;
        break;

    case PC_WAIT_HEALED:
        /* "Fighting fit!" text just closed — show farewell (mirrors PokemonCenterFarewellText) */
        Text_ShowASCII("We hope to see\nyou again!");
        g_state = PC_WAIT_FAREWELL;
        break;

    case PC_WAIT_FAREWELL:
    case PC_WAIT_DECLINE:
        /* Farewell text just closed — sequence complete */
        g_state = PC_IDLE;
        break;

    case PC_IDLE:
    default:
        break;
    }
}
