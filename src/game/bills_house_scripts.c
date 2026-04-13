/* bills_house_scripts.c
 *
 * Bill's House transformation sequence (map 0x58).
 * Ports scripts/BillsHouse.asm and
 * engine/events/hidden_events/bills_house_pc.asm.
 *
 * Sequence:
 *   1. Player talks to Bill_Pokemon (SPRITE_MONSTER at 6,5)
 *      → text → set POKEMON_WALK_TO_MACHINE script
 *   2. Bill_Pokemon walks UP×3 (or around player) to machine, is hidden,
 *      SetEvent EVENT_BILL_SAID_USE_CELL_SEPARATOR
 *   3. Player faces PC at (1,4) and presses A → sound sequence:
 *      TINK(32f) + SHRINK(80f) + TINK(48f) + GET_ITEM_1(32f)
 *      → SetEvent EVENT_USED_CELL_SEPARATOR_ON_BILL
 *   4. Bill1 (SPRITE_SUPER_NERD) appears at machine (1,2), delay 8f,
 *      walks DOWN,RIGHT,RIGHT,RIGHT,DOWN to final position (4,4)
 *   5. SetEvent EVENT_MET_BILL_2, EVENT_MET_BILL
 *   6. Player talks to Bill1 → SS Ticket + SetEvent EVENT_GOT_SS_TICKET
 */

#include "bills_house_scripts.h"
#include "text.h"
#include "npc.h"
#include "player.h"
#include "overworld.h"
#include "music.h"
#include "pokedex.h"
#include "constants.h"
#include "../platform/hardware.h"
#include "../platform/audio.h"
#include "../platform/display.h"
#include "../data/base_stats.h"
#include "../data/event_constants.h"
#include "../data/font_data.h"
#include "inventory.h"
#include <stdio.h>

/* ---- Constants ------------------------------------------------------- */

#define MAP_BILLS_HOUSE  0x58

/* NPC indices in kNpcs_BillsHouse (object_const_def order) */
#define BILL_POKEMON_NPC  0   /* SPRITE_MONSTER at (6,5) */
#define BILL1_NPC         1   /* SPRITE_SUPER_NERD at (4,4) — SS Ticket giver */
#define BILL2_NPC         2   /* SPRITE_SUPER_NERD at (6,5) — shows rare Pokemon */

/* Bill1 machine spawn position.
 * ASM: hSpriteMapX=5, hSpriteMapY=6; subtract 4 (display-buffer offset)
 * → object_event coords (1,2).  Movement DOWN,RIGHT,RIGHT,RIGHT,DOWN from
 * (1,2) arrives at Bill1's final object_event position (4,4). */
#define MACHINE_X  1
#define MACHINE_Y  2

/* SS Ticket item ID (S_S_TICKET = 63 = 0x3F in pokered) */
#define ITEM_SS_TICKET  0x3F

/* Direction constants (match NPC_DoScriptedStep) */
#define DIR_DOWN   0
#define DIR_UP     1
#define DIR_LEFT   2
#define DIR_RIGHT  3

/* YES/NO box layout (matches other overworld prompts) */
#define YESNO_COL  14
#define YESNO_ROW   8
#define YESNO_W     6
#define YESNO_H     4

#define BC_TL  0x79u
#define BC_H   0x7Au
#define BC_TR  0x7Bu
#define BC_V   0x7Cu
#define BC_BL  0x7Du
#define BC_BR  0x7Eu
#define BC_SP  0x7Fu
#define BC_CUR 0xEDu

/* ---- State ----------------------------------------------------------- */

typedef enum {
    BH_DEFAULT = 0,
    /* Talking to Bill_Pokemon */
    BH_POKEMON_TEXT1,     /* initial text; then show YES/NO */
    BH_POKEMON_YESNO,     /* YES/NO box active */
    BH_POKEMON_NO_TEXT,   /* "No!? Come on..." text */
    BH_POKEMON_TEXT2,     /* "use separation system" text */
    /* Bill_Pokemon walking to machine */
    BH_POKEMON_WALK,
    BH_POKEMON_ENTERED,   /* hide sprite, set event, release player */
    /* Waiting for player to use PC */
    BH_WAIT_PC,           /* IsActive() = 0; player roams freely */
    /* PC sound sequence */
    BH_PC_TEXT,           /* "PLAYER initiated…" text, waiting dismiss */
    BH_PC_DELAY_PRE,      /* ~76f pre-sequence delay (SFX_SWITCH + 60f) */
    BH_PC_SFX1,           /* TINK 1 — wait for finish */
    BH_PC_DELAY2,         /* 80f */
    BH_PC_SFX2,           /* SHRINK — wait for finish */
    BH_PC_DELAY3,         /* 48f */
    BH_PC_SFX3,           /* TINK 2 — wait for finish */
    BH_PC_DELAY4,         /* 32f */
    BH_PC_SFX4,           /* GET_ITEM_1 — wait for finish */
    /* Bill's PC rare Pokemon list (post-help revisit path) */
    BH_PC_LIST_TEXT1,     /* "BILL's favorite #MON list!" */
    BH_PC_LIST_TEXT2,     /* "Which #MON do you want to see?" */
    BH_PC_LIST_MENU,      /* list loop (EEVEE/FLAREON/JOLTEON/VAPOREON/CANCEL) */
    BH_PC_LIST_DATA,      /* while ShowPokedexData is active */
    /* Bill1 exiting machine */
    BH_BILL_APPEAR,       /* show Bill1, 8f delay */
    BH_BILL_WALK,         /* Bill1 walking to (4,4) */
    BH_BILL_CLEANUP,      /* set MET_BILL events, back to DEFAULT */
    /* Talking to Bill1 (giving SS Ticket) */
    BH_BILL1_TEXT1,       /* "Yeehah! Thanks!" text */
    BH_BILL1_JINGLE,      /* wait for GetKeyItem jingle + item text */
    BH_BILL1_TEXT2,       /* "Why don't you go" text */
    /* Talking to Bill1 (already received ticket) */
    BH_BILL1_TEXT_WHY,    /* just "Why don't you go" */
    /* Talking to Bill2 */
    BH_BILL2_TEXT,
} BHState;

static BHState g_state = BH_DEFAULT;
static int     g_frame = 0;

/* Walk sequence for Bill_Pokemon (UP×3 or around-player variant) */
#define MAX_WALK_STEPS 6
static int g_walk_seq[MAX_WALK_STEPS + 1];  /* -1 terminated */
static int g_walk_pos = 0;

/* Bill1 exit movement: DOWN, RIGHT, RIGHT, RIGHT, DOWN → ends at (4,4) */
static const int kBillExitSeq[] = {
    DIR_DOWN, DIR_RIGHT, DIR_RIGHT, DIR_RIGHT, DIR_DOWN, -1
};
static int g_exit_pos = 0;
static int g_yesno_cursor = 0;
static int g_pc_list_cursor = 0;

/* ---- Text strings ---------------------------------------------------- */

/* BillsHouseBillImNotAPokemonText */
static const char kText_ImNotAPokemon[] =
    "Hiya! I'm a\nPOKEMON...\n...No I'm not!\f"
    "Call me BILL!\nI'm a true blue\nPOKEMANIAC! Hey!\f"
    "What's with that\nskeptical look?\f"
    "I'm not joshing\nyou, I screwed up\nan experiment and\ngot combined with\na POKEMON!\f"
    "So, how about it?\nHelp me out here!";

/* BillsHouseBillUseSeparationSystemText (shown after Yes or No branch) */
static const char kText_UseSeparation[] =
    "When I'm in the\nTELEPORTER, go to\nmy PC and run the\n"
    "Cell Separation\nSystem!";

/* BillsHouseBillNoYouGottaHelpText */
static const char kText_NoYouGottaHelp[] =
    "No!? Come on, you\ngotta help a guy\nin deep trouble!\f"
    "What do you say,\nchief? Please?\nOK? All right!";

/* BillsHouseMonitorText */
static const char kText_Monitor[] =
    "TELEPORTER is\ndisplayed on\nthe PC monitor.";

/* BillsHouseInitiatedText */
static const char kText_Initiated[] =
    "{PLAYER} initiated\nTELEPORTER's\nCell Separator!";

/* BillsHouseBillThankYouText */
static const char kText_ThankYou[] =
    "BILL: Yeehah!\nThanks, bud!\nI owe you one!\f"
    "So, did you come\nto see my POKEMON\ncollection?\n"
    "That's a bummer.\f"
    "I've got to thank\nyou... Oh here,\nmaybe this'll do.";

/* SSTicketReceivedText (simplified — original uses wStringBuffer for item name) */
static const char kText_SSTicketReceived[] =
    "{PLAYER} received\nSS TICKET!";

/* SSTicketNoRoomText */
static const char kText_SSTicketNoRoom[] =
    "You've got too\nmuch stuff, bud!";

/* BillsHouseBillWhyDontYouGoInsteadOfMeText */
static const char kText_WhyDontYouGo[] =
    "That cruise ship,\nS.S.ANNE, is in\nVERMILION CITY.\n"
    "Its passengers\nare all trainers!\f"
    "They invited me\nbut I can't go.\n"
    "Why don't you\ngo instead of me?";

/* BillsHouseBillCheckOutMyRarePokemonText */
static const char kText_RarePokemon[] =
    "BILL: Look, bud,\njust check out\nsome of my rare\n"
    "POKEMON on my PC!";

/* BillsHousePokemonListText1 (simplified) */
static const char kText_PokemonList[] =
    "BILL's favorite\nPOKEMON list!";
static const char kText_PokemonListPrompt[] =
    "Which POKEMON do\nyou want to see?";

/* ---- Bill's PC list menu (engine/events/hidden_events/bills_house_pc.asm) */
#define BHPC_MENU_L  0
#define BHPC_MENU_T  0
#define BHPC_MENU_R 10  /* TextBoxBorder hlcoord 0,0 b=10 c=9 => right 10 */
#define BHPC_MENU_B 11  /* bottom 11 */

static void bhpc_set(int col, int row, uint8_t tile) {
    if ((unsigned)col >= SCREEN_WIDTH || (unsigned)row >= SCREEN_HEIGHT) return;
    gScrollTileMap[(row + 2) * SCROLL_MAP_W + (col + 2)] = tile;
}

static uint8_t bhpc_tile(unsigned char c) {
    if (c >= 'A' && c <= 'Z') return (uint8_t)Font_CharToTile((uint8_t)(0x80 + (c - 'A')));
    if (c >= 'a' && c <= 'z') return (uint8_t)Font_CharToTile((uint8_t)(0xA0 + (c - 'a')));
    if (c >= '0' && c <= '9') return (uint8_t)Font_CharToTile((uint8_t)(0xF6 + (c - '0')));
    if (c == ' ') return (uint8_t)Font_CharToTile(BC_SP);
    if (c == '\'') return (uint8_t)Font_CharToTile(0xE0);
    if (c == '!') return (uint8_t)Font_CharToTile(0xE7);
    if (c == '-') return (uint8_t)Font_CharToTile(0xE3);
    if (c == '?') return (uint8_t)Font_CharToTile(0xE6);
    if (c == '>') return (uint8_t)Font_CharToTile(0xED);
    return (uint8_t)Font_CharToTile(BC_SP);
}

static void bhpc_str(int col, int row, const char *s) {
    for (; *s; s++, col++) {
        if (*s == '#') {
            /* # => POKé token */
            bhpc_set(col++, row, (uint8_t)Font_CharToTile(0x8F));
            bhpc_set(col++, row, (uint8_t)Font_CharToTile(0x8E));
            bhpc_set(col++, row, (uint8_t)Font_CharToTile(0x8A));
            bhpc_set(col,   row, (uint8_t)Font_CharToTile(0xBA));
            continue;
        }
        bhpc_set(col, row, bhpc_tile((unsigned char)*s));
    }
}

static void bhpc_box(int l, int t, int r, int b) {
    bhpc_set(l, t, (uint8_t)Font_CharToTile(BC_TL));
    for (int c = l + 1; c < r; c++) bhpc_set(c, t, (uint8_t)Font_CharToTile(BC_H));
    bhpc_set(r, t, (uint8_t)Font_CharToTile(BC_TR));
    for (int y = t + 1; y < b; y++) {
        bhpc_set(l, y, (uint8_t)Font_CharToTile(BC_V));
        for (int c = l + 1; c < r; c++) bhpc_set(c, y, (uint8_t)Font_CharToTile(BC_SP));
        bhpc_set(r, y, (uint8_t)Font_CharToTile(BC_V));
    }
    bhpc_set(l, b, (uint8_t)Font_CharToTile(BC_BL));
    for (int c = l + 1; c < r; c++) bhpc_set(c, b, (uint8_t)Font_CharToTile(BC_H));
    bhpc_set(r, b, (uint8_t)Font_CharToTile(BC_BR));
}

static void bhpc_restore_overworld(void) {
    Map_BuildScrollView();
    NPC_BuildView(gScrollPxX, gScrollPxY);
}

static void bhpc_draw_list(void) {
    static const char *kItems[5] = {
        "EEVEE",
        "FLAREON",
        "JOLTEON",
        "VAPOREON",
        "CANCEL"
    };
    bhpc_box(BHPC_MENU_L, BHPC_MENU_T, BHPC_MENU_R, BHPC_MENU_B);
    for (int i = 0; i < 5; i++) {
        int row = 2 + i * 2;
        bhpc_set(1, row, (uint8_t)Font_CharToTile(i == g_pc_list_cursor ? 0xED : BC_SP));
        bhpc_str(2, row, kItems[i]);
    }
    NPC_BuildView(gScrollPxX, gScrollPxY);
}

static uint8_t yesno_char(char c) {
    if (c >= 'A' && c <= 'Z') return (uint8_t)Font_CharToTile((unsigned char)(0x80 + (c - 'A')));
    if (c >= 'a' && c <= 'z') return (uint8_t)Font_CharToTile((unsigned char)(0xA0 + (c - 'a')));
    return (uint8_t)Font_CharToTile(BC_SP);
}

static void yesno_set(int col, int row, uint8_t tile) {
    if (col < 0 || col >= SCREEN_WIDTH || row < 0 || row >= SCREEN_HEIGHT) return;
    gScrollTileMap[(row + 2) * SCROLL_MAP_W + (col + 2)] = tile;
}

static void yesno_draw(void) {
    yesno_set(YESNO_COL,             YESNO_ROW,     (uint8_t)Font_CharToTile(BC_TL));
    for (int c = 1; c < YESNO_W - 1; c++)
        yesno_set(YESNO_COL + c,     YESNO_ROW,     (uint8_t)Font_CharToTile(BC_H));
    yesno_set(YESNO_COL + YESNO_W-1, YESNO_ROW,     (uint8_t)Font_CharToTile(BC_TR));

    yesno_set(YESNO_COL,             YESNO_ROW + 1, (uint8_t)Font_CharToTile(BC_V));
    yesno_set(YESNO_COL + 1,         YESNO_ROW + 1,
              g_yesno_cursor == 0 ? (uint8_t)Font_CharToTile(BC_CUR)
                                  : (uint8_t)Font_CharToTile(BC_SP));
    yesno_set(YESNO_COL + 2,         YESNO_ROW + 1, yesno_char('Y'));
    yesno_set(YESNO_COL + 3,         YESNO_ROW + 1, yesno_char('E'));
    yesno_set(YESNO_COL + 4,         YESNO_ROW + 1, yesno_char('S'));
    yesno_set(YESNO_COL + YESNO_W-1, YESNO_ROW + 1, (uint8_t)Font_CharToTile(BC_V));

    yesno_set(YESNO_COL,             YESNO_ROW + 2, (uint8_t)Font_CharToTile(BC_V));
    yesno_set(YESNO_COL + 1,         YESNO_ROW + 2,
              g_yesno_cursor == 1 ? (uint8_t)Font_CharToTile(BC_CUR)
                                  : (uint8_t)Font_CharToTile(BC_SP));
    yesno_set(YESNO_COL + 2,         YESNO_ROW + 2, yesno_char('N'));
    yesno_set(YESNO_COL + 3,         YESNO_ROW + 2, yesno_char('O'));
    yesno_set(YESNO_COL + 4,         YESNO_ROW + 2, (uint8_t)Font_CharToTile(BC_SP));
    yesno_set(YESNO_COL + YESNO_W-1, YESNO_ROW + 2, (uint8_t)Font_CharToTile(BC_V));

    yesno_set(YESNO_COL,             YESNO_ROW + 3, (uint8_t)Font_CharToTile(BC_BL));
    for (int c = 1; c < YESNO_W - 1; c++)
        yesno_set(YESNO_COL + c,     YESNO_ROW + 3, (uint8_t)Font_CharToTile(BC_H));
    yesno_set(YESNO_COL + YESNO_W-1, YESNO_ROW + 3, (uint8_t)Font_CharToTile(BC_BR));
}

static void yesno_clear(void) {
    for (int r = 0; r < YESNO_H; r++)
        for (int c = 0; c < YESNO_W; c++)
            yesno_set(YESNO_COL + c, YESNO_ROW + r, (uint8_t)Font_CharToTile(BC_SP));
}

/* ---- Public API ------------------------------------------------------ */

void BillsHouseScripts_OnMapLoad(void) {
    if (wCurMap != MAP_BILLS_HOUSE) return;

    int met_bill = CheckEvent(EVENT_MET_BILL);
    int said_use = CheckEvent(EVENT_BILL_SAID_USE_CELL_SEPARATOR);
    int used_sep = CheckEvent(EVENT_USED_CELL_SEPARATOR_ON_BILL);
    int left     = CheckEvent(EVENT_LEFT_BILLS_HOUSE_AFTER_HELPING);

    if (!met_bill) {
        if (said_use && !used_sep) {
            /* Pokemon entered machine; player must use PC to continue */
            NPC_HideSprite(BILL_POKEMON_NPC);
            NPC_HideSprite(BILL1_NPC);
            NPC_HideSprite(BILL2_NPC);
            g_state = BH_WAIT_PC;
        } else {
            NPC_ShowSprite(BILL_POKEMON_NPC);
            NPC_HideSprite(BILL1_NPC);
            NPC_HideSprite(BILL2_NPC);
            g_state = BH_DEFAULT;
        }
    } else {
        /* Bill in human form */
        NPC_HideSprite(BILL_POKEMON_NPC);
        NPC_ShowSprite(BILL1_NPC);
        if (left)
            NPC_ShowSprite(BILL2_NPC);
        else
            NPC_HideSprite(BILL2_NPC);
        g_state = BH_DEFAULT;
    }

    printf("[bills_house] OnMapLoad: state=%d met=%d said=%d used=%d left=%d\n",
           g_state, met_bill, said_use, used_sep, left);
}

/* IsActive returns 0 during DEFAULT and WAIT_PC so the player can roam.
 * For all other states the overworld is blocked. */
int BillsHouseScripts_IsActive(void) {
    return g_state != BH_DEFAULT && g_state != BH_WAIT_PC;
}

void BillsHouseScripts_PostRender(void) {
    if (g_state == BH_POKEMON_YESNO) yesno_draw();
}

void BillsHouseScripts_Tick(void) {
    if (wCurMap != MAP_BILLS_HOUSE) return;

    switch (g_state) {

    case BH_DEFAULT:
    case BH_WAIT_PC:
        return;

    /* ---- Bill_Pokemon talk sequence ---------------------------------- */

    case BH_POKEMON_TEXT1:
        if (Text_IsOpen()) { Text_Update(); return; }
        yesno_draw();
        g_state = BH_POKEMON_YESNO;
        return;

    case BH_POKEMON_YESNO:
        yesno_draw();
        if (hJoyPressed & PAD_UP) {
            if (g_yesno_cursor > 0) g_yesno_cursor--;
        }
        if (hJoyPressed & PAD_DOWN) {
            if (g_yesno_cursor < 1) g_yesno_cursor++;
        }
        if (hJoyPressed & (PAD_A | PAD_B)) {
            int chose_yes = (hJoyPressed & PAD_A) && g_yesno_cursor == 0;
            yesno_clear();
            Text_Close();
            Map_BuildScrollView();
            NPC_BuildView(gScrollPxX, gScrollPxY);
            if (chose_yes) {
                Text_ShowASCII(kText_UseSeparation);
                g_state = BH_POKEMON_TEXT2;
            } else {
                Text_ShowASCII(kText_NoYouGottaHelp);
                g_state = BH_POKEMON_NO_TEXT;
            }
        }
        return;

    case BH_POKEMON_NO_TEXT:
        if (Text_IsOpen()) { Text_Update(); return; }
        Text_ShowASCII(kText_UseSeparation);
        g_state = BH_POKEMON_TEXT2;
        return;

    case BH_POKEMON_TEXT2:
        if (Text_IsOpen()) { Text_Update(); return; }
        /* Build walk sequence based on player's current facing direction */
        if (gPlayerFacing == DIR_DOWN) {
            /* Player is north of Pokemon, blocking UP path — walk around */
            g_walk_seq[0] = DIR_RIGHT;
            g_walk_seq[1] = DIR_UP;
            g_walk_seq[2] = DIR_UP;
            g_walk_seq[3] = DIR_LEFT;
            g_walk_seq[4] = DIR_UP;
            g_walk_seq[5] = -1;
        } else {
            /* Normal path: UP × 3 */
            g_walk_seq[0] = DIR_UP;
            g_walk_seq[1] = DIR_UP;
            g_walk_seq[2] = DIR_UP;
            g_walk_seq[3] = -1;
        }
        g_walk_pos = 0;
        /* Issue the first step */
        NPC_DoScriptedStep(BILL_POKEMON_NPC, g_walk_seq[g_walk_pos++]);
        g_state = BH_POKEMON_WALK;
        return;

    case BH_POKEMON_WALK:
        if (NPC_IsWalking(BILL_POKEMON_NPC)) return;
        if (g_walk_seq[g_walk_pos] != -1) {
            NPC_DoScriptedStep(BILL_POKEMON_NPC, g_walk_seq[g_walk_pos++]);
            return;
        }
        g_state = BH_POKEMON_ENTERED;
        return;

    case BH_POKEMON_ENTERED:
        NPC_HideSprite(BILL_POKEMON_NPC);
        SetEvent(EVENT_BILL_SAID_USE_CELL_SEPARATOR);
        g_state = BH_WAIT_PC;
        printf("[bills_house] Pokemon entered machine; waiting for PC\n");
        return;

    /* ---- PC sound sequence ------------------------------------------- */

    case BH_PC_TEXT:
        if (Text_IsOpen()) { Text_Update(); return; }
        /* Text dismissed — stop music, start sound sequence */
        Music_Stop();
        g_frame = 0;
        g_state = BH_PC_DELAY_PRE;
        return;

    case BH_PC_DELAY_PRE:
        /* ~76f: mirrors SFX_SWITCH(16f) + 60f delay from BillsHouseInitiatedText */
        if (++g_frame >= 76) {
            Audio_PlaySFX_HealingMachine();   /* TINK approximation */
            g_frame = 0;
            g_state = BH_PC_SFX1;
        }
        return;

    case BH_PC_SFX1:
        if (Audio_IsSFXPlaying()) return;
        g_frame = 0;
        g_state = BH_PC_DELAY2;
        return;

    case BH_PC_DELAY2:
        if (++g_frame >= 80) {
            Audio_PlaySFX_BallPoof();          /* SHRINK approximation */
            g_frame = 0;
            g_state = BH_PC_SFX2;
        }
        return;

    case BH_PC_SFX2:
        if (Audio_IsSFXPlaying()) return;
        g_frame = 0;
        g_state = BH_PC_DELAY3;
        return;

    case BH_PC_DELAY3:
        if (++g_frame >= 48) {
            Audio_PlaySFX_HealingMachine();   /* TINK approximation */
            g_frame = 0;
            g_state = BH_PC_SFX3;
        }
        return;

    case BH_PC_SFX3:
        if (Audio_IsSFXPlaying()) return;
        g_frame = 0;
        g_state = BH_PC_DELAY4;
        return;

    case BH_PC_DELAY4:
        if (++g_frame >= 32) {
            Audio_PlaySFX_GetKeyItem();        /* GET_ITEM_1 approximation */
            g_frame = 0;
            g_state = BH_PC_SFX4;
        }
        return;

    case BH_PC_SFX4:
        if (Audio_IsSFXPlaying_GetKeyItem()) return;
        /* Restore map music and mark cell separator used */
        Music_Play(Music_GetMapID(MAP_BILLS_HOUSE));
        SetEvent(EVENT_USED_CELL_SEPARATOR_ON_BILL);
        g_frame = 0;
        g_state = BH_BILL_APPEAR;
        printf("[bills_house] cell separator complete\n");
        return;

    /* ---- Bill's PC post-help list ------------------------------------ */

    case BH_PC_LIST_TEXT1:
        if (Text_IsOpen()) { Text_Update(); return; }
        Map_BuildScrollView();
        bhpc_draw_list();
        Text_ShowASCII(kText_PokemonListPrompt);
        g_state = BH_PC_LIST_TEXT2;
        return;

    case BH_PC_LIST_TEXT2:
        if (Text_IsOpen()) { Text_Update(); return; }
        bhpc_draw_list();
        g_state = BH_PC_LIST_MENU;
        return;

    case BH_PC_LIST_MENU:
        if (hJoyPressed & PAD_UP) {
            if (g_pc_list_cursor > 0) g_pc_list_cursor--;
            bhpc_draw_list();
            return;
        }
        if (hJoyPressed & PAD_DOWN) {
            if (g_pc_list_cursor < 4) g_pc_list_cursor++;
            bhpc_draw_list();
            return;
        }
        if (hJoyPressed & PAD_B) {
            bhpc_restore_overworld();
            g_state = BH_DEFAULT;
            return;
        }
        if (hJoyPressed & PAD_A) {
            /* CANCEL */
            if (g_pc_list_cursor == 4) {
                bhpc_restore_overworld();
                g_state = BH_DEFAULT;
                return;
            }
            /* EEVEE/FLAREON/JOLTEON/VAPOREON exact order from ASM list. */
            static const int kDexList[4] = {133, 136, 135, 134};
            Pokedex_ShowData(kDexList[g_pc_list_cursor]);
            g_state = BH_PC_LIST_DATA;
            return;
        }
        return;

    case BH_PC_LIST_DATA:
        if (Pokedex_IsShowingData()) {
            Pokedex_ShowDataTick();
            return;
        }
        /* Return to Bill's list loop after closing dex data. */
        Display_LoadMapPalette();
        Map_ReloadGfx();
        Font_Load();
        NPC_ReloadTiles();
        Map_BuildScrollView();
        bhpc_draw_list();
        g_state = BH_PC_LIST_MENU;
        return;

    /* ---- Bill1 exit sequence ----------------------------------------- */

    case BH_BILL_APPEAR:
        g_frame++;
        if (g_frame == 1) {
            /* Spawn Bill1 at machine position (mapX=5,mapY=6 minus 4 offset) */
            NPC_ShowSprite(BILL1_NPC);
            NPC_SetTilePos(BILL1_NPC, MACHINE_X, MACHINE_Y);
        }
        if (g_frame >= 8) {
            /* 8-frame delay done; start exit movement */
            g_exit_pos = 0;
            NPC_DoScriptedStep(BILL1_NPC, kBillExitSeq[g_exit_pos++]);
            g_state = BH_BILL_WALK;
        }
        return;

    case BH_BILL_WALK:
        if (NPC_IsWalking(BILL1_NPC)) return;
        if (kBillExitSeq[g_exit_pos] != -1) {
            NPC_DoScriptedStep(BILL1_NPC, kBillExitSeq[g_exit_pos++]);
            return;
        }
        g_state = BH_BILL_CLEANUP;
        return;

    case BH_BILL_CLEANUP:
        SetEvent(EVENT_MET_BILL_2);
        SetEvent(EVENT_MET_BILL);
        g_state = BH_DEFAULT;
        printf("[bills_house] transformation complete — Bill is human again\n");
        return;

    /* ---- Bill1 talk sequence (SS Ticket) ----------------------------- */

    case BH_BILL1_TEXT1:
        if (Text_IsOpen()) { Text_Update(); return; }
        /* Give SS Ticket */
        if (Inventory_Add(ITEM_SS_TICKET, 1) == 0) {
            SetEvent(EVENT_GOT_SS_TICKET);
            Text_ShowASCII(kText_SSTicketReceived);
            Audio_PlaySFX_GetKeyItem();
            g_state = BH_BILL1_JINGLE;
        } else {
            /* Bag full */
            Text_ShowASCII(kText_SSTicketNoRoom);
            g_state = BH_BILL1_TEXT_WHY;
        }
        return;

    case BH_BILL1_JINGLE:
        if (Audio_IsSFXPlaying_GetKeyItem()) return;
        if (Text_IsOpen()) { Text_Update(); return; }
        Text_ShowASCII(kText_WhyDontYouGo);
        g_state = BH_BILL1_TEXT2;
        return;

    case BH_BILL1_TEXT2:
    case BH_BILL1_TEXT_WHY:
        if (Text_IsOpen()) { Text_Update(); return; }
        g_state = BH_DEFAULT;
        return;

    case BH_BILL2_TEXT:
        if (Text_IsOpen()) { Text_Update(); return; }
        g_state = BH_DEFAULT;
        return;

    default:
        g_state = BH_DEFAULT;
        return;
    }
}

/* ---- NPC script callbacks -------------------------------------------- */

/* TEXT_BILLSHOUSE_BILL_POKEMON — fires when player talks to SPRITE_MONSTER */
void BillsHouseScripts_BillPokemonInteract(void) {
    if (g_state != BH_DEFAULT) return;
    g_yesno_cursor = 0;
    wDoNotWaitForButtonPress = 1;
    Text_ShowASCII(kText_ImNotAPokemon);
    g_state = BH_POKEMON_TEXT1;
}

/* TEXT_BILLSHOUSE_BILL_SS_TICKET — fires when player talks to Bill1 (4,4) */
void BillsHouseScripts_BillInteract(void) {
    if (g_state != BH_DEFAULT) return;
    if (CheckEvent(EVENT_GOT_SS_TICKET)) {
        /* Already gave the ticket — just say why don't you go */
        Text_ShowASCII(kText_WhyDontYouGo);
        g_state = BH_BILL1_TEXT_WHY;
    } else {
        Text_ShowASCII(kText_ThankYou);
        g_state = BH_BILL1_TEXT1;
    }
}

/* TEXT_BILLSHOUSE_BILL_CHECK_OUT_MY_RARE_POKEMON — fires for Bill2 (6,5) */
void BillsHouseScripts_Bill2Interact(void) {
    if (g_state != BH_DEFAULT) return;
    Text_ShowASCII(kText_RarePokemon);
    g_state = BH_BILL2_TEXT;
}

/* ---- PC hidden event callback ---------------------------------------- */

/* Fires when player faces tile (1,4) and presses A.
 * Mirrors BillsHousePC from engine/events/hidden_events/bills_house_pc.asm. */
void BillsHouseScripts_PCActivate(void) {
    /* ASM guard: BillsHousePC only responds when facing up. */
    if (gPlayerFacing != DIR_UP) return;

    /* After leaving: show Pokemon list (simplified) */
    if (CheckEvent(EVENT_LEFT_BILLS_HOUSE_AFTER_HELPING)) {
        if (g_state != BH_DEFAULT) return;
        g_pc_list_cursor = 0;
        Text_ShowASCII(kText_PokemonList);
        g_state = BH_PC_LIST_TEXT1;
        return;
    }
    /* Cell separator already used: show monitor text */
    if (CheckEvent(EVENT_USED_CELL_SEPARATOR_ON_BILL)) {
        Text_ShowASCII(kText_Monitor);
        return;
    }
    /* Bill asked player to use the PC: run cell separator sequence */
    if (CheckEvent(EVENT_BILL_SAID_USE_CELL_SEPARATOR)) {
        if (g_state != BH_WAIT_PC) return;  /* sequence already in progress */
        Text_ShowASCII(kText_Initiated);
        g_state = BH_PC_TEXT;
        return;
    }
    /* Default: just show the monitor screen */
    Text_ShowASCII(kText_Monitor);
}
