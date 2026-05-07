# Fighting Dojo — Saffron City Implementation Checklist
> Gen 1 Red/Blue parity pass.
> ASM is source of truth.
> Use walkthrough only as gameplay/content reference.

Reference:
https://gamefaqs.gamespot.com/gameboy/367023-pokemon-red-version/faqs/64175/fighting-dojo

---

# Core Structure

- [ ] Fighting Dojo map implemented
- [x] Correct Saffron City entrance warp
- [ ] Interior warp placements verified
- [ ] NPC/object coordinates verified against ASM
- [ ] Gym statue objects absent (dojo is unofficial)
- [ ] Reward room layout verified
- [x] Hitmonlee Poké Ball object implemented
- [x] Hitmonchan Poké Ball object implemented

---

# Trainer Setup

## Blackbelt A

- [x] Trainer implemented
- [x] LOS/trainer engagement verified
- [ ] Reward money = ₽775

### Team
- [x] Machop Lv31
- [x] Mankey Lv31
- [x] Primeape Lv31

### AI
- [ ] X Attack AI behavior implemented
- [ ] Max two X Attacks per Pokémon

---

## Blackbelt B

- [x] Trainer implemented
- [x] LOS/trainer engagement verified
- [ ] Reward money = ₽800

### Team
- [x] Machop Lv32
- [x] Machoke Lv32

### AI
- [ ] X Attack AI behavior implemented
- [ ] Max two X Attacks per Pokémon

---

## Blackbelt C

- [x] Trainer implemented
- [x] LOS/trainer engagement verified
- [ ] Reward money = ₽900

### Team
- [x] Primeape Lv36

### AI
- [ ] X Attack AI behavior implemented
- [ ] Max two X Attacks per Pokémon

---

## Blackbelt D

- [x] Trainer implemented
- [x] LOS/trainer engagement verified
- [ ] Reward money = ₽775

### Team
- [x] Mankey Lv31
- [x] Mankey Lv31
- [x] Primeape Lv31

### AI
- [ ] X Attack AI behavior implemented
- [ ] Max two X Attacks per Pokémon

---

# Karate Master

- [x] Karate Master trainer implemented
- [x] Post-defeat state change implemented
- [ ] Reward money = ₽925

## Team
- [x] Hitmonlee Lv37
- [x] Hitmonchan Lv37

## Battle Logic
- [x] Trainer battle starts correctly
- [ ] Post-battle dialogue verified
- [x] Reward selection unlocks only after victory

---

# Gift Pokémon Logic

## Hitmonlee

- [x] Left Poké Ball object implemented
- [x] Gift level = 30
- [x] One-time selection enforced
- [x] Party-full behavior verified
- [ ] PC transfer behavior verified
- [x] Poké Ball disappears after selection

## Hitmonchan

- [x] Right Poké Ball object implemented
- [x] Gift level = 30
- [x] One-time selection enforced
- [x] Party-full behavior verified
- [ ] PC transfer behavior verified
- [x] Poké Ball disappears after selection

---

# Reward Exclusivity

- [x] Selecting Hitmonlee removes Hitmonchan
- [x] Selecting Hitmonchan removes Hitmonlee
- [x] Cannot receive both Pokémon
- [ ] Save/load persistence verified

## Flags

- [x] `EVENT_DEFEATED_FIGHTING_DOJO`
- [x] `EVENT_GOT_HITMONLEE`
- [x] `EVENT_GOT_HITMONCHAN`

---

# Dialogue / Text

- [ ] Entrance NPC dialogue verified
- [ ] Blackbelt defeat text verified
- [ ] Karate Master pre-battle text verified
- [ ] Karate Master defeat text verified
- [ ] Gift Pokémon selection text verified
- [x] Duplicate-interaction text verified after reward taken

---

# Fidelity / Verification

- [ ] Trainer facings verified
- [ ] Trainer movement timing verified
- [ ] Battle initiation timing feels accurate
- [ ] Warp transition timing verified
- [ ] Object visibility transitions verified
- [ ] Reward sequence tested on fresh save
- [ ] Reward sequence tested with full party
- [ ] Event ordering verified against ASM
- [ ] Trainer rosters diffed against ASM
- [ ] AI behavior diffed against ASM
- [ ] Poké Ball disappearance logic verified
- [ ] No unintended interaction after reward claimed

---

# Optional Fidelity

- [ ] Preserve original collision quirks if present
- [ ] Preserve original trainer LOS quirks if present
- [ ] Preserve original text pacing feel
- [ ] Preserve original encounter cadence



