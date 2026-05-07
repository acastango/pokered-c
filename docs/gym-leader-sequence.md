---
name: gym-leader-sequence
description: Documents the full gym leader defeat sequence pattern (battle speech, badge jingle, overworld TM/item giving) for implementing any gym leader
user-invocable: true
---

# Gym Leader Defeat Sequence — Implementation Guide

This covers the full sequence for a gym leader defeat, from battle speech through overworld item giving. Implemented for Brock (gym 1); apply the same pattern to all others.

---

## ASM Source Locations

For each gym leader, read these files from `pokeredbackup/pokered-master` before implementing:

- `scripts/<GymName>.asm` — GymLeaderBeforeScript, GymLeaderAfterScript, post-battle text labels
- `data/text/<GymName>.asm` — exact text strings for all pages
- `data/trainers/parties/<trainer_class>.asm` — party data (for trainer_no / wTrainerNo)

**Always copy text verbatim from the ASM. Do not summarize or paraphrase.**

---

## Phase 1 — Battle Defeat Speech + Badge Award

### Where it happens
`src/game/gym_scripts.c` — in the gym's pre-battle state (e.g. `GS_BROCK_PRE_WAIT`)

### What to implement

**1. Decode player name** (needed for "received" text):
```c
static char playerName[11];
decode_player_name(playerName, sizeof(playerName));
```

**2. Build defeat quote pages 1–2** (shown AFTER trainer slides off screen):
```c
static char kLeaderDefeatQuote[96];
snprintf(kLeaderDefeatQuote, sizeof(kLeaderDefeatQuote),
    "<page1 text>\f<page2 text>");
gTrainerAfterText = kLeaderDefeatQuote;
```
`gTrainerAfterText` is shown in `BUI_TRAINER_VICTORY_PAUSE` after the trainer sprite exits.

**3. Build badge received text** (page 3 — shown SIMULTANEOUSLY with key-item jingle):
```c
static char kBadgeRecvText[48];
snprintf(kBadgeRecvText, sizeof(kBadgeRecvText),
    "%s received the\n<BADGE NAME>!", playerName);
BattleUI_SetBadgeRecvText(kBadgeRecvText);
```
`BUI_TRAINER_VICTORY_TEXT` fires `Audio_PlaySFX_GetKeyItem()` and shows this text at the same time — not on A press.

**4. Set badge info pages** (pages 4–6 — shown after jingle, before returning to overworld):
```c
BattleUI_SetBadgeInfoText(
    "That's an official\nPOKEMON LEAGUE\nBADGE!"
    "\fIts bearer's\nPOKEMON become\nmore powerful!"
    "\f<HM/ability unlock line>");
```
Shown in `BUI_GYM_BADGE_JINGLE` state, after `Audio_IsSFXPlaying_GetKeyItem()` clears.

**5. Set up the battle** (at end of pre-wait state):
```c
extern uint8_t wGymLeaderNo;
wGymLeaderNo = <gym_number>;   /* 1=Brock, 2=Misty, 3=Surge, etc. */
```
`wGymLeaderNo != 0` triggers `MUSIC_GYM_LEADER_BATTLE` and `MUSIC_DEFEATED_GYM_LEADER` at the right moments.

### Battle UI state flow (battle_ui.c — already implemented, no changes needed)
```
BUI_TRAINER_VICTORY_SLIDE
  → Music_Play(MUSIC_DEFEATED_GYM_LEADER)
BUI_TRAINER_VICTORY_TEXT
  → gTrainerAfterText shown (pages 1-2)
  → if s_badge_recv_text: Audio_PlaySFX_GetKeyItem() + show recv text
  → BUI_GYM_BADGE_JINGLE
BUI_GYM_BADGE_JINGLE
  → show badge info text (pages 4-6)
  → BUI_WAIT_SOUND (waits for jingle to finish)
```

---

## Phase 2 — GymScripts_OnVictory

### Where it happens
`GymScripts_OnVictory()` in `gym_scripts.c` — called by `game.c` after battle ends

### What to implement
```c
if (wGymLeaderNo == <N>) {
    SetEvent(EVENT_BEAT_<LEADER>);
    SetEvent(EVENT_BEAT_<GYM>_TRAINER_0);  /* deactivates gym trainers */
    wObtainedBadges |= (1u << BADGE_<NAME>);
    wGymLeaderNo = 0;
    gState = GS_<LEADER>_POST_TEXT;
}
```

`SetEvent(EVENT_BEAT_<GYM>_TRAINER_0)` mirrors `SetEvent EVENT_BEAT_PEWTER_GYM_TRAINER_0` in the ASM — stops all undefeated gym trainers from engaging the player.

---

## Phase 3 — Overworld Post-Battle (TM / HM giving)

### State machine (add to gym_scripts.c GS_ enum)
```
GS_<LEADER>_POST_TEXT    → show "Wait! Take this with you!"
GS_<LEADER>_POST_WAIT    → wait for text, give TM, play GetItem1, show "[PLAYER] received TM<N>!"
GS_<LEADER>_TM_WAIT      → wait for received text
GS_<LEADER>_TM_EXPLAIN   → show TM explanation pages
GS_<LEADER>_TM_EXP_WAIT  → wait for explanation, → GS_IDLE
```

### Text to implement (all verbatim from ASM)

**Post text:** `"Wait! Take this\nwith you!"`

**Received text:**
```c
snprintf(kLeaderTMText, sizeof(kLeaderTMText),
    "%s received\nTM<NN>!", playerName);
```

**TM explanation** (4 pages from ASM — copy verbatim):
- Page 1: What a TM is
- Page 2: TMs are one-use, choose the Pokémon carefully
- Page 3: "TM<NN> contains\n<MOVE NAME>!"
- Page 4: Move description

### Giving the item
```c
Audio_PlaySFX_GetItem1();
Inventory_Add(TM01 + <offset>, 1);   /* TM01=base, offset=TM number-1 */
```
Play the SFX before showing the received text. `GetItem1` jingle is ~12 frames.

---

## Checklist for each new gym leader

- [ ] Read all text from ASM — defeat quote, badge recv, badge info, TM explanation
- [ ] `gTrainerAfterText` = pages 1–2 of defeat quote
- [ ] `BattleUI_SetBadgeRecvText` = "X received the BADGE!"
- [ ] `BattleUI_SetBadgeInfoText` = badge info pages (3 pages)
- [ ] `wGymLeaderNo` = correct gym number
- [ ] `GymScripts_OnVictory`: SetEvent for leader + trainer flag + badge bit
- [ ] Post-battle states: "Wait! Take this!" → give TM → received text → TM explanation
- [ ] TM explanation: 4 pages, verbatim from ASM

---

## Gym Leader Reference

| # | Leader   | Badge        | TM   | Move        | HM unlock |
|---|----------|--------------|------|-------------|-----------|
| 1 | Brock    | Boulder      | TM34 | Bide        | Flash     |
| 2 | Misty    | Cascade      | TM11 | Bubblebeam  | Cut       |
| 3 | Lt.Surge | Thunder      | TM24 | Thunderbolt | —         |
| 4 | Erika    | Rainbow      | TM21 | Mega Drain  | Strength  |
| 5 | Koga     | Soul         | TM06 | Toxic       | Surf      |
| 6 | Sabrina  | Marsh        | TM46 | Psywave     | —         |
| 7 | Blaine   | Volcano      | TM38 | Fire Blast  | —         |
| 8 | Giovanni | Earth        | TM27 | Fissure     | —         |
