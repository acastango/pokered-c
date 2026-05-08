# Trainer Parity Audit: Routes/Caves vs ASM

Generated from `pokered-master/scripts/*.asm` (`TrainerHeaders`) vs `src/data/event_data.c`.

## Standard Trainer NPC Parity

All standard route/cave trainer-NPC maps are now wired with `kTrainers_*` tables and attached in `gMapEvents`.

Total missing standard trainer maps: **0**
Total missing standard trainer entries: **0**

## Scripted Wild Encounter Exceptions (Not `map_trainer_t` NPC Tables)

These use custom script flow (`TalkToTrainer`-style wild setup on object/item interactions) and are intentionally not represented as normal trainer NPC tables:

| Map | ASM pseudo-trainer headers | Reason |
|---|---:|---|
| `PowerPlant` | 9 | Voltorb/Electrode/Zapdos scripted wild encounters (mostly item-like objects), not standard trainer NPCs |
| `CeruleanCaveB1F` | 1 | Mewtwo scripted wild encounter, not a standard trainer NPC |
