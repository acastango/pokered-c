/* mtmoon_scripts.c — Mt. Moon B2F fossil choice event.
 *
 * Exact port of scripts/MtMoonB2F.asm fossil sequence:
 *   After defeating Super Nerd (NPC 0), player picks Dome or Helix fossil.
 *   Super Nerd says "All right. Then this is mine!" walks to the unchosen
 *   fossil, takes it, and both fossil sprites are hidden.
 */
#include "mtmoon_scripts.h"
#include "text.h"
#include "npc.h"
#include "player.h"
#include "inventory.h"
#include "overworld.h"
#include "trainer_sight.h"
#include "../platform/hardware.h"
#include "../platform/audio.h"
#include "../data/event_constants.h"

#include "../data/font_data.h"

#include <stdio.h>

/* ---- Constants -------------------------------------------------------- */

#define MAP_MT_MOON_B2F   0x3d

#define NERD_NPC_IDX      0
#define DOME_NPC_IDX      5
#define HELIX_NPC_IDX     6

#define EVENT_BEAT_NERD   1401u
#define EVENT_GOT_DOME    1406u
#define EVENT_GOT_HELIX   1407u

/* ---- Yes/No box (same layout as oakslab_scripts.c) -------------------- */

#define YESNO_COL  14
#define YESNO_ROW   8
#define YESNO_W     6
#define YESNO_H     4

/* Border char codes (must match oakslab_scripts.c) */
#define BC_TL  0x79u
#define BC_H   0x7Au
#define BC_TR  0x7Bu
#define BC_V   0x7Cu
#define BC_BL  0x7Du
#define BC_BR  0x7Eu
#define BC_SP  0x7Fu
#define BC_CUR 0xEDu

static int g_yesno_cursor = 0;

static uint8_t yn_char(char c) {
    if (c >= 'A' && c <= 'Z') return (uint8_t)Font_CharToTile((unsigned char)(0x80 + (c - 'A')));
    if (c >= 'a' && c <= 'z') return (uint8_t)Font_CharToTile((unsigned char)(0xA0 + (c - 'a')));
    return (uint8_t)Font_CharToTile(BC_SP);
}

static void yn_set(int col, int row, uint8_t tile) {
    if (col < 0 || col >= SCREEN_WIDTH || row < 0 || row >= SCREEN_HEIGHT) return;
    gScrollTileMap[(row + 2) * SCROLL_MAP_W + (col + 2)] = tile;
}

static void yn_draw(void) {
    yn_set(YESNO_COL,             YESNO_ROW,     (uint8_t)Font_CharToTile(BC_TL));
    for (int c = 1; c < YESNO_W - 1; c++)
        yn_set(YESNO_COL + c,     YESNO_ROW,     (uint8_t)Font_CharToTile(BC_H));
    yn_set(YESNO_COL + YESNO_W-1, YESNO_ROW,     (uint8_t)Font_CharToTile(BC_TR));

    yn_set(YESNO_COL,             YESNO_ROW + 1, (uint8_t)Font_CharToTile(BC_V));
    yn_set(YESNO_COL + 1,         YESNO_ROW + 1,
           g_yesno_cursor == 0 ? (uint8_t)Font_CharToTile(BC_CUR)
                               : (uint8_t)Font_CharToTile(BC_SP));
    yn_set(YESNO_COL + 2,         YESNO_ROW + 1, yn_char('Y'));
    yn_set(YESNO_COL + 3,         YESNO_ROW + 1, yn_char('E'));
    yn_set(YESNO_COL + 4,         YESNO_ROW + 1, yn_char('S'));
    yn_set(YESNO_COL + YESNO_W-1, YESNO_ROW + 1, (uint8_t)Font_CharToTile(BC_V));

    yn_set(YESNO_COL,             YESNO_ROW + 2, (uint8_t)Font_CharToTile(BC_V));
    yn_set(YESNO_COL + 1,         YESNO_ROW + 2,
           g_yesno_cursor == 1 ? (uint8_t)Font_CharToTile(BC_CUR)
                               : (uint8_t)Font_CharToTile(BC_SP));
    yn_set(YESNO_COL + 2,         YESNO_ROW + 2, yn_char('N'));
    yn_set(YESNO_COL + 3,         YESNO_ROW + 2, yn_char('O'));
    yn_set(YESNO_COL + 4,         YESNO_ROW + 2, (uint8_t)Font_CharToTile(BC_SP));
    yn_set(YESNO_COL + YESNO_W-1, YESNO_ROW + 2, (uint8_t)Font_CharToTile(BC_V));

    yn_set(YESNO_COL,             YESNO_ROW + 3, (uint8_t)Font_CharToTile(BC_BL));
    for (int c = 1; c < YESNO_W - 1; c++)
        yn_set(YESNO_COL + c,     YESNO_ROW + 3, (uint8_t)Font_CharToTile(BC_H));
    yn_set(YESNO_COL + YESNO_W-1, YESNO_ROW + 3, (uint8_t)Font_CharToTile(BC_BR));
}

static void yn_clear(void) {
    for (int r = 0; r < YESNO_H; r++)
        for (int c = 0; c < YESNO_W; c++)
            yn_set(YESNO_COL + c, YESNO_ROW + r, (uint8_t)Font_CharToTile(BC_SP));
}

/* ---- State machine ---------------------------------------------------- */

typedef enum {
    FS_IDLE = 0,
    FS_WAIT_TEXT,       /* waiting for question text to be dismissed */
    FS_YESNO,           /* yes/no box visible, waiting for input */
    FS_GOT_FOSSIL,      /* "got FOSSIL!" text showing */
    FS_NERD_WALK,       /* Super Nerd walks to unchosen fossil */
    FS_NERD_TAKE,       /* Super Nerd says "this is mine", hides unchosen fossil */
} FossilState;

static FossilState g_state       = FS_IDLE;
static int         g_chosen_dome = 0;  /* 1 = player chose dome, 0 = helix */

/* ---- Text strings ----------------------------------------------------- */

static const char kGotDome[]  = "{PLAYER} got\nDOME FOSSIL!";
static const char kGotHelix[] = "{PLAYER} got\nHELIX FOSSIL!";
static const char kNerdTake[] = "All right. Then\nthis is mine!";
/* Cinnabar lab text is the Super Nerd's after_text in trainer data,
 * shown automatically by the trainer system when talked to post-defeat. */

/* ---- Helpers ---------------------------------------------------------- */

static int fossils_available(void) {
    return wCurMap == MAP_MT_MOON_B2F
        && CheckEvent(EVENT_BEAT_NERD)
        && !CheckEvent(EVENT_GOT_DOME)
        && !CheckEvent(EVENT_GOT_HELIX);
}

static void start_choice(int is_dome) {
    g_chosen_dome = is_dome;
    g_yesno_cursor = 0;
    g_state = FS_WAIT_TEXT;  /* wait for text to finish printing, then show yes/no */
}

/* ---- Public API ------------------------------------------------------- */

void MtMoon_FossilCallback_Dome(void) {
    if (!fossils_available()) return;
    /* ASM sets wDoNotWaitForButtonPressAfterDisplayingText = 1 before PrintText,
     * so text prints fully then returns without waiting for A.  Text box stays
     * visible and yes/no box draws on top. */
    wDoNotWaitForButtonPress = 1;
    Text_ShowASCII("You want the\nDOME FOSSIL?");
    start_choice(1);
}

void MtMoon_FossilCallback_Helix(void) {
    if (!fossils_available()) return;
    wDoNotWaitForButtonPress = 1;
    Text_ShowASCII("You want the\nHELIX FOSSIL?");
    start_choice(0);
}

/* Step trigger: auto-engage Super Nerd when player reaches (13,8).
 * Mirrors MtMoonB2FDefaultScript lines 57-69 in the ASM. */
void MtMoon_StepCheck(void) {
    if (wCurMap != MAP_MT_MOON_B2F) return;
    if (CheckEvent(EVENT_BEAT_NERD)) return;  /* already defeated */
    if (wXCoord == 13 && wYCoord == 8) {
        printf("[mtmoon] step trigger: engaging Super Nerd at (13,8)\n");
        Trainer_EngageImmediate(NERD_NPC_IDX);
    }
}

void MtMoon_OnMapLoad(void) {
    if (wCurMap != MAP_MT_MOON_B2F) return;
    g_state = FS_IDLE;
    /* Hide fossils that have already been taken (Super Nerd stays per ASM) */
    if (CheckEvent(EVENT_GOT_DOME) || CheckEvent(EVENT_GOT_HELIX)) {
        NPC_HideSprite(DOME_NPC_IDX);
        NPC_HideSprite(HELIX_NPC_IDX);
    }
}

int MtMoon_IsActive(void) {
    return g_state != FS_IDLE;
}

void MtMoon_PostRender(void) {
    if (g_state == FS_YESNO) yn_draw();
}

void MtMoon_Tick(void) {
    if (g_state == FS_IDLE) return;

    switch (g_state) {

    case FS_WAIT_TEXT:
        /* Wait for "You want the DOME/HELIX FOSSIL?" text to be dismissed.
         * Must call Text_Update() ourselves since the normal game loop text
         * processing is skipped when MtMoon is active. */
        if (Text_IsOpen()) { Text_Update(); return; }
        /* Text dismissed — now show the yes/no box */
        yn_draw();
        g_state = FS_YESNO;
        return;

    case FS_YESNO:
        /* NPC text "You want the DOME/HELIX FOSSIL?" should already be
         * showing.  The yes/no box overlays it.  Wait for input. */
        yn_draw();
        if (hJoyPressed & PAD_UP) {
            if (g_yesno_cursor > 0) { g_yesno_cursor--; yn_draw(); }
        }
        if (hJoyPressed & PAD_DOWN) {
            if (g_yesno_cursor < 1) { g_yesno_cursor++; yn_draw(); }
        }
        if (hJoyPressed & (PAD_A | PAD_B)) {
            yn_clear();
            if ((hJoyPressed & PAD_A) && g_yesno_cursor == 0) {
                /* YES — give fossil */
                if (g_chosen_dome) {
                    Inventory_Add(ITEM_DOME_FOSSIL, 1);
                    SetEvent(EVENT_GOT_DOME);
                    NPC_HideSprite(DOME_NPC_IDX);
                    Audio_PlaySFX_GetKeyItem();
                    Text_ShowASCII(kGotDome);
                } else {
                    Inventory_Add(ITEM_HELIX_FOSSIL, 1);
                    SetEvent(EVENT_GOT_HELIX);
                    NPC_HideSprite(HELIX_NPC_IDX);
                    Audio_PlaySFX_GetKeyItem();
                    Text_ShowASCII(kGotHelix);
                }
                g_state = FS_GOT_FOSSIL;
                printf("[mtmoon] player chose %s fossil\n",
                       g_chosen_dome ? "DOME" : "HELIX");
            } else {
                /* NO or B — cancel: close the text box and return to idle */
                yn_clear();
                Text_Close();           /* clear text box tiles + reset hWY */
                Map_BuildScrollView();  /* restore map tiles under box */
                g_state = FS_IDLE;
            }
        }
        return;

    case FS_GOT_FOSSIL:
        /* Hold text open while GetKeyItem jingle plays (mirrors ASM
         * WaitForSoundToFinish before allowing text dismissal) */
        if (Audio_IsSFXPlaying_GetKeyItem()) return;
        if (Text_IsOpen()) { Text_Update(); return; }
        /* Super Nerd walks to the unchosen fossil (per ASM: walk first, text after).
         * Dome is at (12,6), Helix at (13,6), Nerd at (12,8).
         * If player took Dome: Nerd walks to Helix (13,6) → RIGHT 1, UP 2.
         * If player took Helix: Nerd walks to Dome (12,6) → UP 2. */
        g_state = FS_NERD_WALK;
        if (g_chosen_dome) {
            /* Walk to Helix fossil at (13,6): RIGHT then UP UP */
            NPC_DoScriptedStep(NERD_NPC_IDX, 3); /* RIGHT */
        } else {
            /* Walk to Dome fossil at (12,6): UP UP */
            NPC_DoScriptedStep(NERD_NPC_IDX, 1); /* UP */
        }
        return;

    case FS_NERD_WALK:
        if (NPC_IsWalking(NERD_NPC_IDX)) return;
        {
            int ntx = -1, nty = -1;
            NPC_GetTilePos(NERD_NPC_IDX, &ntx, &nty);
            /* Target: one tile south of fossil (ASM movement data stops here).
             * Dome=(12,7), Helix=(13,7) — nerd faces fossil, doesn't step on it. */
            int target_x = g_chosen_dome ? 13 : 12;
            int target_y = 7;
            if (ntx != target_x || nty != target_y) {
                /* Still need to walk — go UP */
                NPC_DoScriptedStep(NERD_NPC_IDX, 1); /* UP */
                return;
            }
        }
        /* Arrived at unchosen fossil — take it */
        g_state = FS_NERD_TAKE;
        return;

    case FS_NERD_TAKE:
        /* Super Nerd says "All right. Then this is mine!" */
        Text_ShowASCII(kNerdTake);
        Audio_PlaySFX_GetKeyItem();
        /* Hide the unchosen fossil (Super Nerd stays visible per ASM) */
        if (g_chosen_dome)
            NPC_HideSprite(HELIX_NPC_IDX);
        else
            NPC_HideSprite(DOME_NPC_IDX);
        printf("[mtmoon] Super Nerd took %s fossil\n",
               g_chosen_dome ? "HELIX" : "DOME");
        g_state = FS_IDLE;
        return;

    default:
        g_state = FS_IDLE;
        return;
    }
}
