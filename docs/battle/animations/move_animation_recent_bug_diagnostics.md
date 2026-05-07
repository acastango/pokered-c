# Move Animation Bug Diagnostics (Recent)

## Scope
Quick postmortem of the last bug cluster (Acid/Absorb/Acid Armor), with direct prevention rules for continuing the move animation port.

## 1) Acid SFX mutes battle music (only one channel left)
### Symptom
After `Acid`, battle BGM partially/mostly disappears; only one channel keeps playing.

### Root cause
`SFX_BATTLE_2A` (Acid) runs multiple times in-script and was doing stacked `Music_SuspendChannel(0/1)` calls, but only a single resume at sequence end.

### Fix
Added explicit ownership guard for `SFX_BATTLE_2A` channel suspension:
- suspend CH0/CH1 only once per active SFX lifecycle
- resume CH0/CH1 only once when both internal sequences end

### Rule going forward
For every move SFX that can retrigger while active, use **non-stacking ownership flags** around suspend/resume pairs.

---

## 2) Move-specific SFX played wrong/default sound
### Symptom
Absorb/Acid/Acid Armor/Agility sounded generic or incorrect.

### Root cause
Move sound dispatch in `MoveAnim_PlayCommandSound` still mapped several ASM symbols to generic hit/faint helpers instead of imported move-specific routines.

### Fix
Wired symbol-accurate dispatch:
- `SFX_BATTLE_24` -> Absorb family port
- `SFX_BATTLE_2A` -> Acid family port
- `SFX_BATTLE_0D` -> Acid Armor port
- `SFX_FAINT_FALL` -> fall-only variant for Agility behavior
Plus: ensured their timers/sequencers advance in `Audio_Update`.

### Rule going forward
When adding move SFX: **import + dispatch + update-loop lifecycle** are a single unit. Missing any one causes placebo fixes.

---

## 3) Acid Armor anim affected wrong side / looked corrupted
### Symptom
Acid Armor initially hit enemy-side visuals or behaved like palette/invert corruption.

### Root cause
Port assumed enemy OAM path for an effect that, in this engine architecture, needs player-side BG tile handling for back sprite behavior.

### Fix
Implemented player-side slide/hide on the back-sprite tile area, and kept enemy-side path separate.

### Rule going forward
Always branch effect implementation by **render substrate**:
- enemy mon = mostly OAM in this port
- player mon = BG tilemap-backed back sprite

---

## 4) Acid Armor sprite stayed hidden until next input
### Symptom
Animation hides player mon correctly, but mon reappears only after selecting another move.

### Root cause
BG sprite restore depended on later UI redraw; move animation finalize didn’t force immediate player back-sprite re-stamp.

### Fix
At animation finalize, explicitly reload/re-stamp player back sprite tiles for current species.

### Rule going forward
Any effect that clears player BG sprite area must define a **deterministic restore point** (usually animation finalize).

---

## 5) Acid impact blob missing a quadrant (top-left chunk absent)
### Symptom
On enemy impact, one quarter of blob frame not rendered.

### Root cause
Frameblock destination OAM pointer behavior diverged from ASM:
- port reset destination each subentry
- ASM initializes destination once per subanimation, and `FRAMEBLOCKMODE_03/04` depend on that persistence

### Fix
Changed frameblock destination pointer lifecycle to ASM behavior:
- initialize once when subanimation loads
- do not reset each step

### Rule going forward
For subanimations using `FRAMEBLOCKMODE_02/03/04`, treat OAM destination state as **persistent subanimation state**, not per-frame scratch.

---

## Reusable Checklist (Use for each new move)
1. Confirm ASM script path (subanim IDs, SE calls, sound tokens, frameblock modes).
2. Verify render substrate per side (enemy OAM vs player BG).
3. Verify SFX lifecycle triplet:
- dispatch mapping
- sequencer/timer update
- balanced suspend/resume ownership
4. Verify frameblock destination semantics for mode `02/03/04`.
5. Verify explicit restore path for anything that hides/clears sprites.
6. Verify timing via `docs/timing_conversion_protocol.md` (no ad hoc frame guesses).

## High-value implementation principle
Most recent bugs were not “bad constants”; they were **state lifecycle mismatches** vs ASM (ownership, pointer persistence, restore timing). Prioritize state-machine fidelity over visual tweaking.
