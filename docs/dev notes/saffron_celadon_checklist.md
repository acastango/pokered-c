# Saffron / Celadon Implementation Checklist
> Gen 1 Red/Blue parity pass.
> ASM is source of truth.
> Rocket Hideout intentionally omitted from this scope.

---

# Celadon City

## Core map/state
- [ ] Route7 east connection functional
- [ ] Route16 west connection functional
- [x] Gym map accessible
- [x] Game Corner accessible
- [x] Dept Store accessible
- [x] Celadon Mansion accessible
- [x] Hotel accessible
- [x] Restaurant accessible

## Mansion / Eevee
- [ ] Rear entrance path implemented
- [x] Top-room Eevee gift implemented
- [x] Eevee level set to Lv25
- [x] One-time Poké Ball interaction enforced
- [x] `EVENT_GOT_EEVEE` implemented

## Hidden items / misc
- [x] Hidden PP Up implemented near Cut-tree path
- [x] Hidden item flag persists correctly
- [x] Restaurant Coin Case NPC implemented
- [x] Coin Case is one-time only
- [x] `EVENT_GOT_COIN_CASE` implemented
- [ ] RB invisible hotel PC bug preserved (optional parity)

## Department Store

### 2F left clerk
- [x] Great Ball — 600
- [x] Super Potion — 700
- [x] Revive — 1500
- [x] Super Repel — 500
- [x] Antidote — 100
- [x] Burn Heal — 250
- [x] Ice Heal — 250
- [x] Awakening — 200
- [x] Parlyz Heal — 200

### 2F TM clerk
- [x] TM32 Double Team — 1000
- [x] TM33 Reflect — 1000
- [x] TM02 Razor Wind — 2000
- [x] TM07 Horn Drill — 2000
- [x] TM37 Egg Bomb — 2000
- [x] TM01 Mega Punch — 3000
- [x] TM05 Mega Kick — 3000
- [x] TM09 Take Down — 3000
- [x] TM17 Submission — 3000

### 3F reward NPC
- [x] TM18 Counter gift implemented
- [x] One-time reward enforced
- [x] `EVENT_GOT_TM18` implemented

### 4F items
- [x] Poké Doll — 1000
- [x] Fire Stone — 2100
- [x] Thunder Stone — 2100
- [x] Water Stone — 2100
- [x] Leaf Stone — 2100

### 5F X-items
- [x] X Accuracy — 950
- [x] Guard Spec — 700
- [x] Dire Hit — 650
- [x] X Attack — 500
- [x] X Defend — 550
- [x] X Speed — 350
- [x] X Special — 350

### 5F stat items
- [x] HP Up — 9800
- [x] Protein — 9800
- [x] Iron — 9800
- [x] Carbos — 9800
- [x] Calcium — 9800

## Roof vending
- [x] Fresh Water — 200
- [x] Soda Pop — 300
- [x] Lemonade — 350

## Roof TM girl
- [x] Fresh Water -> TM13 Ice Beam
- [x] Soda Pop -> TM48 Rock Slide
- [x] Lemonade -> TM49 Tri Attack
- [x] One-time reward logic implemented
- [x] `EVENT_GOT_TM13`
- [x] `EVENT_GOT_TM48`
- [x] `EVENT_GOT_TM49`

## Roof edge-case parity
- [ ] Bag-full trade behavior verified against ASM
- [ ] Drink-removal edge case verified
- [ ] Vending-money quirk verified
- [ ] Soda/Lemonade low-money behavior verified

## Route16 / Fly detour
- [ ] Cut-tree detour path implemented
- [ ] Gatehouse implemented
- [ ] HM02 Fly house accessible
- [x] HM02 Fly gift implemented
- [x] One-time HM02 reward enforced
- [x] `EVENT_GOT_HM02` implemented

---

# Saffron Gate Unlock Flow

- [x] Thirsty guard interaction implemented
- [x] Guard accepts Fresh Water
- [x] Guard accepts Soda Pop
- [x] Guard accepts Lemonade
- [x] Single drink unlock propagates globally
- [x] Route5 gate updated
- [x] Route6 gate updated
- [x] Route7 gate updated
- [x] Route8 gate updated
- [x] All guards become passable
- [x] `EVENT_GAVE_SAFFRON_GUARDS_DRINK` implemented

---

# Saffron City

## Core map/state
- [ ] Route5 north connection functional
- [ ] Route8 east connection functional
- [ ] Route6 south connection functional
- [ ] Route7 west connection functional
- [x] Fighting Dojo accessible
- [ ] Saffron Gym state-gated properly
- [ ] Silph exterior/state correct for current story progress

## Mart inventory
- [x] Great Ball — 600
- [x] Hyper Potion — 1500
- [x] Max Repel — 700
- [x] Escape Rope — 550
- [x] Full Heal — 600
- [x] Revive — 1500

## TM29 Psychic house
- [x] Southern-row east house implemented
- [x] TM29 Psychic reward implemented
- [x] One-time reward enforced
- [x] `EVENT_GOT_TM29` implemented

## Fighting Dojo
- [ ] Optional pre-Silph access preserved
- [ ] Trainer roster implemented
- [ ] Trainer levels verified (~Lv37 cap)
- [ ] Hitmonlee reward path implemented
- [ ] Hitmonchan reward path implemented
- [ ] Reward exclusivity enforced

## Gym gating
- [ ] Saffron Gym blocked before proper flags
- [ ] Gym logic separated from Fighting Dojo
- [ ] Story-state checks verified against ASM

---

# Fidelity / Verification

- [ ] NPC facings verified
- [ ] Sign text parity checked
- [ ] Warp placements verified
- [ ] Shop inventories diffed against ASM
- [ ] One-time rewards persist after save/load
- [ ] Hidden item persistence verified
- [ ] Cross-map flag propagation verified
- [ ] Event ordering tested against vanilla behavior
- [ ] All implemented behavior tested on fresh save
