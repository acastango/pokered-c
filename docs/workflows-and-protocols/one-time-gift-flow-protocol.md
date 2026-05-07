# One-Time Gift Flow Protocol (Reusable)

Use this for NPC/item gifts that can only be received once and are controlled by an event flag.

Current examples:
- Celadon 3F clerk -> TM18 Counter
- Celadon City gramps -> TM41
- Route 16 house -> HM02
- Diner -> Coin Case (with delayed jingle timing)

Core modules:
- `src/game/celadon_city_scripts.c`
- Event flags in `src/data/event_constants.h`

## Required Behavior

1. Gate by event flag first.
- If flag is set: show post-gift/explanation text and return.

2. For first-time interaction, split into phases.
- Phase A: show pre-gift text only.
- Phase B: after text closes, attempt inventory add.
- This avoids clobbering active text state and matches stable script sequencing.

3. Inventory add result drives branch.
- Success:
  - set event flag
  - play correct item-get SFX (`GetItem1` for TMs/items, `GetKeyItem` when appropriate)
  - show receive text
- Failure:
  - do not set event flag
  - show no-room text

4. Keep state machine ownership explicit.
- Use a script state enum entry (e.g. `WAIT_PRETEXT_CLOSE`) in the map/script tick function.
- Script callback should only enqueue/start state; tick resolves completion.

## Canonical Skeleton

```c
void NpcGiftScript(void) {
    if (script_state != IDLE) return;
    if (CheckEvent(EVENT_GOT_X)) {
        Text_ShowASCII(kExplainText);
        return;
    }
    Text_ShowASCII(kPreGiftText);
    script_state = WAIT_PRETEXT_CLOSE;
}

// in script tick:
case WAIT_PRETEXT_CLOSE:
    if (Text_IsOpen()) return;
    if (Inventory_Add(ITEM_X, 1) != 0) {
        Text_ShowASCII(kNoRoomText);
        script_state = IDLE;
        return;
    }
    SetEvent(EVENT_GOT_X);
    Audio_PlaySFX_GetItem1(); // or GetKeyItem by parity
    Text_ShowASCII(kReceivedText);
    script_state = IDLE;
    return;
```

## Parity Notes

- Do not set the one-time event flag on bag-full failure.
- Do not remove/consume prerequisite items if reward add fails.
- If ASM sequences jingle on specific text page, trigger SFX in the corresponding post-text phase.
- Keep text terminators explicit (`@`) where required by current text engine usage patterns.
