/* gate_scripts.c — NPC script callbacks for badge/event-gated guards.
 *
 * Mirrors the pokered position-script system (ViridianCity.asm,
 * Route22Gate.asm, Route23.asm) that fires when the player steps
 * to a specific coordinate.  In our port, these are wired as
 * npc_script_fn callbacks on the guard NPCs, fired on A-press.
 *
 * Text mirrors the original pokered text files exactly.
 */
#include "gate_scripts.h"
#include "badge.h"
#include "npc.h"
#include "text.h"
#include "player.h"
#include "music.h"
#include "../platform/hardware.h"
#include "../data/event_constants.h"

extern int16_t wXCoord, wYCoord;

/* Map IDs for guards that need hiding on load. */
#define MAP_VIRIDIAN_CITY   0x01
#define MAP_PEWTER_CITY     0x02
#define MAP_ROUTE23         0x22
#define MAP_ROUTE22GATE     0xc1

/* ---- helpers -------------------------------------------------------- */

/* Hide the NPC that triggered this script and set its pass event flag. */
static void pass_guard(uint16_t event_flag) {
    if (event_flag) SetEvent(event_flag);
    NPC_HideSprite(NPC_GetLastInteracted());
}

/* ---- Gate_LoadMap--------------------------------------------------- */

/* Called after NPC_Load + Trainer_LoadMap on every map load.
 * Hides badge guards whose pass-event is already set, so re-entering
 * a gate building does not restore a guard the player already passed. */
void Gate_LoadMap(void) {
    switch (wCurMap) {

    case MAP_ROUTE22GATE:
        /* Route 22 Gate: single guard, no persistent event — hide if
         * player already has Boulder Badge (they've clearly passed). */
        if (Badge_Has(BADGE_BOULDER))
            NPC_HideSprite(0);
        break;

    case MAP_ROUTE23:
        /* Seven guards from south (idx 6) to north (idx 0). */
        if (CheckEvent(EVENT_PASSED_CASCADEBADGE_CHECK)) NPC_HideSprite(6);
        if (CheckEvent(EVENT_PASSED_THUNDERBADGE_CHECK)) NPC_HideSprite(5);
        if (CheckEvent(EVENT_PASSED_RAINBOWBADGE_CHECK)) NPC_HideSprite(4);
        if (CheckEvent(EVENT_PASSED_SOULBADGE_CHECK))    NPC_HideSprite(3);
        if (CheckEvent(EVENT_PASSED_MARSHBADGE_CHECK))   NPC_HideSprite(2);
        if (CheckEvent(EVENT_PASSED_VOLCANOBADGE_CHECK)) NPC_HideSprite(1);
        if (CheckEvent(EVENT_PASSED_EARTHBADGE_CHECK))   NPC_HideSprite(0);
        break;

    case MAP_PEWTER_CITY:
        /* Hide the gym guide youngster (NPC 4) once Brock is defeated. */
        if (CheckEvent(EVENT_BEAT_BROCK))
            NPC_HideSprite(4);
        break;

    case MAP_VIRIDIAN_CITY:
        /* Two old man NPCs — sleeping (4) pre-dex, awake/walking (6) post-dex.
         * ASM uses TOGGLE_LYING_OLD_MAN / TOGGLE_OLD_MAN to swap them. */
        if (CheckEvent(EVENT_GOT_POKEDEX)) {
            NPC_HideSprite(4);  /* hide sleeping old man */
        } else {
            NPC_HideSprite(6);  /* hide awake old man */
        }
        break;

    default:
        break;
    }
}

/* ---- Viridian City old man ------------------------------------------ */

/* object_event 18, 9 (SPRITE_GAMBLER_ASLEEP) — blocks Route 2 north.
 * Mirrors ViridianCityCheckGotPokedexScript in scripts/ViridianCity.asm. */
void Gate_ViridianOldMan(void) {
    if (CheckEvent(EVENT_GOT_POKEDEX)) {
        /* Player has Pokédex — old man steps aside.
         * Text: _ViridianCityOldManHadMyCoffeeNowText */
        NPC_HideSprite(NPC_GetLastInteracted());
        Text_ShowASCII("Ahh, I've had my\ncoffee now and I\nfeel great!\fSure you can go\nthrough!\fAre you in a\nhurry?");
    } else {
        /* Text: _ViridianCityOldManSleepyPrivatePropertyText */
        Text_ShowASCII("You can't go\nthrough here!\fThis is private\nproperty!");
    }
}

/* ---- Viridian City position trigger --------------------------------- */

/* Mirrors ViridianCityCheckGotPokedexScript (scripts/ViridianCity.asm):
 * fires when the player steps to the tile just east of the sleeping old man
 * (wXCoord=19, wYCoord=9 — 1:1 ASM block coords).
 * Shows "private property" text and pushes player one step south.
 * begin_step() updates wXCoord/wYCoord immediately, so the logical position
 * advances before animation plays — re-trigger on text dismiss is prevented. */
/* ---- Pewter City east-exit escort ----------------------------------- */

/* Mirrors PewterCityCheckPlayerLeavingEastScript + PewterGuys in
 * engine/events/pewter_guys.asm and scripts/PewterCity.asm.
 * Trigger tiles (ASM block coords, 1:1 with wXCoord/wYCoord):
 *   .one   Y16,X34   LEFT DOWN DOWN RIGHT
 *   .two   Y17,X35   LEFT DOWN RIGHT LEFT
 *   .five  Y17,X36   LEFT DOWN LEFT
 *   .three Y18,X37   LEFT LEFT LEFT
 *   .four  Y19,X37   LEFT LEFT UP LEFT
 */
#define DIR_DOWN  0
#define DIR_UP    1
#define DIR_LEFT  2
#define DIR_RIGHT 3

#define PEWTER_YOUNGSTER_IDX   4
#define PEWTER_NPC_EXIT_STEPS  5

typedef enum {
    PEW_IDLE = 0,
    PEW_TEXT1,    /* first text open — waiting for dismiss  */
    PEW_WALKING,  /* auto-walking player toward gym          */
    PEW_TEXT2,    /* second text open — waiting for dismiss  */
    PEW_NPC_EXIT, /* youngster walking away to the right     */
} PewterEscortState;

static PewterEscortState g_pewter_state             = PEW_IDLE;
static int               g_pewter_dirs[52];
static int               g_pewter_dir_len            = 0;
static int               g_pewter_dir_idx            = 0;
static int               g_pewter_npc_dirs[52];
static int               g_pewter_npc_len            = 0;
static int               g_pewter_npc_idx            = 0;
static int               g_pewter_npc_step           = 0;
static int               g_pewter_npc_post_down_delay = 0;

/* Build the full escort route for the given trigger tile.
 *
 * Fine-positioning (from PewterGymGuyCoords in engine/events/pewter_guys.asm):
 *   In the original, fine-pos bytes are appended AFTER the main route in the
 *   simulated-joypad buffer.  Because the buffer is consumed LAST-FIRST, fine-pos
 *   plays first AND in reverse byte-order.  We reproduce that here by prepending
 *   the reversed fine-pos bytes before the main route in our forward dispatch.
 *
 * Main route (RLEList_PewterGymPlayer, engine/overworld/auto_movement.asm):
 *   Authored: NO_INPUT×1, RIGHT×2, DOWN×5, LEFT×11, UP×5, LEFT×15
 *   Consumed last-first → execution: LEFT×15, UP×5, LEFT×11, DOWN×5, RIGHT×2
 *
 * All five trigger tiles converge to canonical (wXCoord=34, wYCoord=18) after
 * fine-positioning before the main route begins. */
static void pewter_set_route(int16_t tx, int16_t ty) {
    int n = 0;

    /* Fine-positioning prefix (reversed authored bytes from PewterGymGuyCoords).
     * Coordinates are ASM block coords (1:1 with wXCoord/wYCoord). */
    if (tx == 34 && ty == 16) {
        /* .one Y16,X34: authored LEFT,DOWN,DOWN,RIGHT → reversed RIGHT,DOWN,DOWN,LEFT */
        g_pewter_dirs[n++] = DIR_RIGHT;
        g_pewter_dirs[n++] = DIR_DOWN;
        g_pewter_dirs[n++] = DIR_DOWN;
        g_pewter_dirs[n++] = DIR_LEFT;
    } else if (tx == 35 && ty == 17) {
        /* .two Y17,X35: authored LEFT,DOWN,RIGHT,LEFT → reversed LEFT,RIGHT,DOWN,LEFT */
        g_pewter_dirs[n++] = DIR_LEFT;
        g_pewter_dirs[n++] = DIR_RIGHT;
        g_pewter_dirs[n++] = DIR_DOWN;
        g_pewter_dirs[n++] = DIR_LEFT;
    } else if (tx == 36 && ty == 17) {
        /* .five Y17,X36: authored LEFT,DOWN,LEFT,$00×8
         * ASM $00 pauses are frame-level.
         * Use 1 NOOP for NPC head start to prevent overlap. */
        g_pewter_dirs[n++] = -1; /* 1-step head start for NPC */
        g_pewter_dirs[n++] = DIR_LEFT;
        g_pewter_dirs[n++] = DIR_DOWN;
        g_pewter_dirs[n++] = DIR_LEFT;
    } else if (tx == 37 && ty == 18) {
        /* .three Y18,X37: authored LEFT,LEFT,LEFT,$00×8
         * ASM $00 pauses are frame-level (8 frames ≈ ½ tile).
         * Use 1 NOOP to give NPC a 1-step head start, preventing
         * player/NPC overlap on the convergence path. */
        g_pewter_dirs[n++] = -1; /* 1-step head start for NPC */
        g_pewter_dirs[n++] = DIR_LEFT;
        g_pewter_dirs[n++] = DIR_LEFT;
        g_pewter_dirs[n++] = DIR_LEFT;
    } else if (tx == 37 && ty == 19) {
        /* .four Y19,X37: authored LEFT,LEFT,UP,LEFT → reversed LEFT,UP,LEFT,LEFT */
        g_pewter_dirs[n++] = DIR_LEFT;
        g_pewter_dirs[n++] = DIR_UP;
        g_pewter_dirs[n++] = DIR_LEFT;
        g_pewter_dirs[n++] = DIR_LEFT;
    }
    /* else: unknown tile, skip fine-positioning and start main route from current pos */

    /* Main route — ASM authored: NO_INPUT×1, RIGHT×2, DOWN×5, LEFT×11, UP×5, LEFT×15
     * PewterGuys `dec a` overwrites last LEFT → effective LEFT×14.
     * Executed LIFO: LEFT×14, UP×5, LEFT×11, DOWN×5, RIGHT×2, NO_INPUT×1
     * NO_INPUT = player stands still for 1 step (NPC finishes last RIGHT).
     * We use -1 as a "no-op" direction handled by the tick. */
    for (int i = 0; i < 14; i++) g_pewter_dirs[n++] = DIR_LEFT; /* 15-1: dec a overwrites last LEFT */
    for (int i = 0; i < 5;  i++) g_pewter_dirs[n++] = DIR_UP;
    for (int i = 0; i < 11; i++) g_pewter_dirs[n++] = DIR_LEFT;
    for (int i = 0; i < 5;  i++) g_pewter_dirs[n++] = DIR_DOWN;
    for (int i = 0; i < 2;  i++) g_pewter_dirs[n++] = DIR_RIGHT;
    g_pewter_dirs[n++] = -1; /* NO_INPUT×1 — player idle, NPC gets final step */

    g_pewter_dir_len = n;
    g_pewter_dir_idx = 0;

    /* NPC route — exact match of RLEList_PewterGymGuy from ASM:
     * DOWN×2, LEFT×15, UP×5, LEFT×11, DOWN×5, RIGHT×3 */
    int m = 0;
    for (int i = 0; i < 2;  i++) g_pewter_npc_dirs[m++] = DIR_DOWN;
    for (int i = 0; i < 15; i++) g_pewter_npc_dirs[m++] = DIR_LEFT;
    for (int i = 0; i < 5;  i++) g_pewter_npc_dirs[m++] = DIR_UP;
    for (int i = 0; i < 11; i++) g_pewter_npc_dirs[m++] = DIR_LEFT;
    for (int i = 0; i < 5;  i++) g_pewter_npc_dirs[m++] = DIR_DOWN;
    for (int i = 0; i < 3;  i++) g_pewter_npc_dirs[m++] = DIR_RIGHT;
    g_pewter_npc_len = m;
    g_pewter_npc_idx = 0;
}

int Gate_PewterIsActive(void) {
    return g_pewter_state != PEW_IDLE;
}

void Gate_PewterEastCheck(void) {
    if (wCurMap != MAP_PEWTER_CITY) return;
    if (CheckEvent(EVENT_BEAT_BROCK)) return;
    if (g_pewter_state != PEW_IDLE) return;
    /* Four trigger tiles from PewterCityPlayerLeavingEastCoords (scripts/PewterCity.asm):
     *   .one   Y16,X34   .two  Y17,X35   .five  Y17,X36
     *   .three Y18,X37   .four Y19,X37 */
    if (!((wXCoord == 34 && wYCoord == 16) ||
          (wXCoord == 35 && wYCoord == 17) ||
          (wXCoord == 36 && wYCoord == 17) ||
          (wXCoord == 37 && wYCoord == 18) ||
          (wXCoord == 37 && wYCoord == 19))) return;
    Text_ShowASCII("You're a trainer\nright? BROCK's\nlooking for new\nchallengers!\fFollow me!");
    pewter_set_route(wXCoord, wYCoord);
    g_pewter_state = PEW_TEXT1;
}

/* Called every frame before Player_Update.  Drives the escort state machine. */
void Gate_PewterTick(void) {
    if (g_pewter_state == PEW_IDLE) return;

    if (g_pewter_state == PEW_TEXT1) {
        if (Text_IsOpen()) return;
        Music_Play(MUSIC_MUSEUM_GUY);
        g_pewter_state = PEW_WALKING;
        return;
    }

    if (g_pewter_state == PEW_WALKING) {
        /* ASM uses a single shared counter — both player and NPC advance one
         * step per overworld tick in lockstep.  Wait for both to finish their
         * current step, then dispatch the next pair simultaneously. */
        if (Player_IsMoving() || NPC_IsWalking(PEWTER_YOUNGSTER_IDX)) return;

        int player_done = (g_pewter_dir_idx >= g_pewter_dir_len);
        int npc_done    = (g_pewter_npc_idx >= g_pewter_npc_len);

        /* Debug: log coords and direction for every step */
        {
            static const char *dname[] = {"DOWN","UP","LEFT","RIGHT","NOOP"};
            int pi = g_pewter_dir_idx, ni = g_pewter_npc_idx;
            const char *pd = player_done ? "DONE" : (g_pewter_dirs[pi] < 0 ? dname[4] : dname[g_pewter_dirs[pi]]);
            const char *nd = npc_done    ? "DONE" : dname[g_pewter_npc_dirs[ni]];
            int ntx = -1, nty = -1;
            NPC_GetTilePos(PEWTER_YOUNGSTER_IDX, &ntx, &nty);
            printf("[pewter] step P=%d/%d N=%d/%d | player(%d,%d) %s | npc(%d,%d) %s\n",
                   pi, g_pewter_dir_len, ni, g_pewter_npc_len,
                   wXCoord, wYCoord, pd,
                   ntx, nty, nd);
        }

        if (player_done && npc_done) {
            Text_ShowASCII("If you have the\nright stuff, go\ntake on BROCK!");
            g_pewter_state = PEW_TEXT2;
            return;
        }

        if (!player_done) {
            int pdir = g_pewter_dirs[g_pewter_dir_idx++];
            if (pdir >= 0)  /* -1 = NO_INPUT: player stands still this step */
                Player_DoScriptedStep(pdir);
        }
        if (!npc_done)
            NPC_DoScriptedStep(PEWTER_YOUNGSTER_IDX, g_pewter_npc_dirs[g_pewter_npc_idx++]);
        return;
    }

    if (g_pewter_state == PEW_TEXT2) {
        if (Text_IsOpen()) return;
        g_pewter_npc_step = 0;
        g_pewter_state = PEW_NPC_EXIT;
        return;
    }

    if (g_pewter_state == PEW_NPC_EXIT) {
        if (NPC_IsWalking(PEWTER_YOUNGSTER_IDX)) return;
        if (g_pewter_npc_step < PEWTER_NPC_EXIT_STEPS) {
            NPC_DoScriptedStep(PEWTER_YOUNGSTER_IDX, DIR_RIGHT);
            g_pewter_npc_step++;
        } else {
            /* ASM: HideObject → SetSpritePosition2 → ShowObject.
             * Reset youngster to original spawn (35,16) so he's visible
             * for repeat triggers until Brock is beaten. */
            NPC_HideSprite(PEWTER_YOUNGSTER_IDX);
            NPC_SetTilePos(PEWTER_YOUNGSTER_IDX, 35, 16);
            NPC_ShowSprite(PEWTER_YOUNGSTER_IDX);
            /* Restore map music (ASM: PlayDefaultMusic after escort) */
            Music_Play(Music_GetMapID(wCurMap));
            g_pewter_state = PEW_IDLE;
        }
        return;
    }
}

/* ---- Viridian City old man ------------------------------------------ */

/* Pending flag: set when the block fires; cleared when the push step runs.
 * Prevents re-showing text while the player is still on the trigger tile. */
static int g_viridian_push_pending = 0;

void Gate_ViridianStepCheck(void) {
    if (wCurMap != MAP_VIRIDIAN_CITY) return;
    if (CheckEvent(EVENT_GOT_POKEDEX)) return;
    if (g_viridian_push_pending) return;  /* already pending — wait for push */
    if (wYCoord != 9 || wXCoord != 19) return;
    Text_ShowASCII("You can't go\nthrough here!\fThis is private\nproperty!");
    g_viridian_push_pending = 1;
    /* Do NOT call Player_ForceStepDown here — deferring to Gate_ViridianDoPush
     * (called before Player_Update when text is closed) keeps the camera update
     * in sync with Map_BuildScrollView, preventing the scroll snap. */
}

/* Called every frame just before Player_Update (after text-open guard).
 * Executes the deferred south push once text is dismissed. */
void Gate_ViridianDoPush(void) {
    if (!g_viridian_push_pending) return;
    if (Text_IsOpen()) return;   /* wait until player dismisses the text */
    g_viridian_push_pending = 0;
    Player_ForceStepDown();
}

/* ---- Route 22 Gate -------------------------------------------------- */

/* object_event 6, 2 (SPRITE_GUARD) — checks BOULDER badge.
 * Mirrors Route22GateGuardText in scripts/Route22Gate.asm. */
void Gate_Route22_Guard(void) {
    if (Badge_Has(BADGE_BOULDER)) {
        /* _Route22GateGuardGoRightAheadText */
        pass_guard(0);  /* no persistent event flag for Route 22 */
        Text_ShowASCII("Oh! That is the\nBOULDERBADGE!\nGo right ahead!");
    } else {
        /* _Route22GateGuardNoBoulderbadgeText + _ICantLetYouPassText */
        Text_ShowASCII("Only truly skilled\ntrainers are\nallowed through.\fYou don't have the\nBOULDERBADGE yet!\fThe rules are\nrules. I can't\nlet you pass.");
    }
}

/* ---- Route 23 guards ------------------------------------------------ */

/* Guards are ordered in the NPC array from north (idx 0) to south (idx 6).
 * Text mirrors _Route23YouDontHaveTheBadgeYetText /
 *              _Route23OhThatIsTheBadgeText + _Route23GoRightAheadText. */

void Gate_Route23_Earth(void) {
    if (Badge_Has(BADGE_EARTH) || CheckEvent(EVENT_PASSED_EARTHBADGE_CHECK)) {
        pass_guard(EVENT_PASSED_EARTHBADGE_CHECK);
        Text_ShowASCII("Oh! That is the\nEARTHBADGE!\fOK then! Please,\ngo right ahead!");
    } else {
        Text_ShowASCII("You can pass here\nonly if you have\nthe EARTHBADGE!\fYou don't have the\nEARTHBADGE yet!\fYou have to have\nit to get to\nPOKEMON LEAGUE!");
    }
}

void Gate_Route23_Volcano(void) {
    if (Badge_Has(BADGE_VOLCANO) || CheckEvent(EVENT_PASSED_VOLCANOBADGE_CHECK)) {
        pass_guard(EVENT_PASSED_VOLCANOBADGE_CHECK);
        Text_ShowASCII("Oh! That is the\nVOLCANOBADGE!\fOK then! Please,\ngo right ahead!");
    } else {
        Text_ShowASCII("You can pass here\nonly if you have\nthe VOLCANOBADGE!\fYou don't have the\nVOLCANOBADGE yet!\fYou have to have\nit to get to\nPOKEMON LEAGUE!");
    }
}

void Gate_Route23_Marsh(void) {
    if (Badge_Has(BADGE_MARSH) || CheckEvent(EVENT_PASSED_MARSHBADGE_CHECK)) {
        pass_guard(EVENT_PASSED_MARSHBADGE_CHECK);
        Text_ShowASCII("Oh! That is the\nMARSHBADGE!\fOK then! Please,\ngo right ahead!");
    } else {
        Text_ShowASCII("You can pass here\nonly if you have\nthe MARSHBADGE!\fYou don't have the\nMARSHBADGE yet!\fYou have to have\nit to get to\nPOKEMON LEAGUE!");
    }
}

void Gate_Route23_Soul(void) {
    if (Badge_Has(BADGE_SOUL) || CheckEvent(EVENT_PASSED_SOULBADGE_CHECK)) {
        pass_guard(EVENT_PASSED_SOULBADGE_CHECK);
        Text_ShowASCII("Oh! That is the\nSOULBADGE!\fOK then! Please,\ngo right ahead!");
    } else {
        Text_ShowASCII("You can pass here\nonly if you have\nthe SOULBADGE!\fYou don't have the\nSOULBADGE yet!\fYou have to have\nit to get to\nPOKEMON LEAGUE!");
    }
}

void Gate_Route23_Rainbow(void) {
    if (Badge_Has(BADGE_RAINBOW) || CheckEvent(EVENT_PASSED_RAINBOWBADGE_CHECK)) {
        pass_guard(EVENT_PASSED_RAINBOWBADGE_CHECK);
        Text_ShowASCII("Oh! That is the\nRAINBOWBADGE!\fOK then! Please,\ngo right ahead!");
    } else {
        Text_ShowASCII("You can pass here\nonly if you have\nthe RAINBOWBADGE!\fYou don't have the\nRAINBOWBADGE yet!\fYou have to have\nit to get to\nPOKEMON LEAGUE!");
    }
}

void Gate_Route23_Thunder(void) {
    if (Badge_Has(BADGE_THUNDER) || CheckEvent(EVENT_PASSED_THUNDERBADGE_CHECK)) {
        pass_guard(EVENT_PASSED_THUNDERBADGE_CHECK);
        Text_ShowASCII("Oh! That is the\nTHUNDERBADGE!\fOK then! Please,\ngo right ahead!");
    } else {
        Text_ShowASCII("You can pass here\nonly if you have\nthe THUNDERBADGE!\fYou don't have the\nTHUNDERBADGE yet!\fYou have to have\nit to get to\nPOKEMON LEAGUE!");
    }
}

void Gate_Route23_Cascade(void) {
    if (Badge_Has(BADGE_CASCADE) || CheckEvent(EVENT_PASSED_CASCADEBADGE_CHECK)) {
        pass_guard(EVENT_PASSED_CASCADEBADGE_CHECK);
        Text_ShowASCII("Oh! That is the\nCASCADEBADGE!\fOK then! Please,\ngo right ahead!");
    } else {
        Text_ShowASCII("You can pass here\nonly if you have\nthe CASCADEBADGE!\fYou don't have the\nCASCADEBADGE yet!\fYou have to have\nit to get to\nPOKEMON LEAGUE!");
    }
}
