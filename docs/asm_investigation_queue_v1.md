# ASM Investigation Queue (v1)

Date: 2026-05-05

- Source: docs/asm_script_gap_detail_v5.csv
- Output CSV: docs/asm_investigation_queue_v1.csv

## Queue A: High Keyness Partial (Prioritized)
Count: 29

- SilphCo2F.asm | score=84 | trainers=0 | owner=event_data.c
- SaffronCity.asm | score=66 | trainers=0 | owner=event_data.c
- SSAnne2F.asm | score=50 | trainers=4 | owner=event_data.c; ss_anne_scripts.c
- LancesRoom.asm | score=48 | trainers=0 | owner=event_data.c
- MtMoon1F.asm | score=46 | trainers=7 | owner=event_data.c
- SSAnne1F.asm | score=43 | trainers=4 | owner=event_data.c
- SSAnneB1F.asm | score=42 | trainers=6 | owner=event_data.c
- SSAnneB1FRooms.asm | score=42 | trainers=6 | owner=event_data.c
- SaffronGym.asm | score=41 | trainers=0 | owner=event_data.c
- Route25.asm | score=40 | trainers=9 | owner=event_data.c
- SSAnne2FRooms.asm | score=39 | trainers=4 | owner=event_data.c
- Route3.asm | score=34 | trainers=8 | owner=event_data.c
- FuchsiaCity.asm | score=32 | trainers=0 | owner=event_data.c
- ViridianForest.asm | score=32 | trainers=3 | owner=event_data.c
- CeladonCity.asm | score=31 | trainers=0 | owner=event_data.c
- MtMoonB2F.asm | score=30 | trainers=5 | owner=event_data.c; mtmoon_scripts.c
- PowerPlant.asm | score=30 | trainers=0 | owner=event_data.c
- SSAnne1FRooms.asm | score=30 | trainers=4 | owner=event_data.c
- Route6.asm | score=29 | trainers=6 | owner=event_data.c
- CeruleanCity.asm | score=27 | trainers=0 | owner=cerulean_scripts.c; event_data.c
- VictoryRoad2F.asm | score=24 | trainers=0 | owner=event_data.c
- Route16Gate1F.asm | score=22 | trainers=0 | owner=event_data.c
- VermilionCity.asm | score=22 | trainers=0 | owner=event_data.c; ss_anne_scripts.c
- SilphCo5F.asm | score=21 | trainers=0 | owner=event_data.c
- CeladonMart3F.asm | score=20 | trainers=0 | owner=event_data.c
- PalletTown.asm | score=14 | trainers=1 | owner=event_data.c; pallet_scripts.c
- SSAnneBow.asm | score=13 | trainers=2 | owner=event_data.c
- CeruleanGym.asm | score=12 | trainers=2 | owner=event_data.c
- PewterGym.asm | score=8 | trainers=1 | owner=event_data.c; gym_scripts.c

## Queue B: Missing + Nearest C Touchpoints
Count: 19

- CeladonMansion2F.asm | touchpoints=src/data/event_data.c first, then nearest grouped module in src/game/*_scripts.c
- CeladonMansionRoof.asm | touchpoints=src/data/event_data.c first, then nearest grouped module in src/game/*_scripts.c
- CeladonMartElevator.asm | touchpoints=src/data/event_data.c (map events) + grouped building script flow module
- CeruleanCave1F.asm | touchpoints=src/data/event_data.c + grouped dungeon/building modules in src/game/*_scripts.c
- CeruleanCave2F.asm | touchpoints=src/data/event_data.c + grouped dungeon/building modules in src/game/*_scripts.c
- CeruleanCity_2.asm | touchpoints=src/data/event_data.c variant map table + city grouped script module
- MtMoonB1F.asm | touchpoints=src/data/event_data.c + grouped dungeon/building modules in src/game/*_scripts.c
- RedsHouse2F.asm | touchpoints=src/data/event_data.c + src/game/pallet_scripts.c + intro/naming flow modules
- RocketHideoutElevator.asm | touchpoints=src/data/event_data.c (map events) + grouped building script flow module
- Route2.asm | touchpoints=src/data/event_data.c first, then nearest grouped module in src/game/*_scripts.c
- Route5.asm | touchpoints=src/data/event_data.c first, then nearest grouped module in src/game/*_scripts.c
- Route7.asm | touchpoints=src/data/event_data.c first, then nearest grouped module in src/game/*_scripts.c
- SafariZoneCenter.asm | touchpoints=src/data/event_data.c first, then nearest grouped module in src/game/*_scripts.c
- SafariZoneEast.asm | touchpoints=src/data/event_data.c first, then nearest grouped module in src/game/*_scripts.c
- SafariZoneNorth.asm | touchpoints=src/data/event_data.c first, then nearest grouped module in src/game/*_scripts.c
- SafariZoneWest.asm | touchpoints=src/data/event_data.c first, then nearest grouped module in src/game/*_scripts.c
- SilphCoElevator.asm | touchpoints=src/data/event_data.c (map events) + grouped building script flow module
- UndergroundPathNorthSouth.asm | touchpoints=src/data/event_data.c + src/game/gate_scripts.c + src/game/overworld.c
- UndergroundPathWestEast.asm | touchpoints=src/data/event_data.c + src/game/gate_scripts.c + src/game/overworld.c

## How to Use
1. Work Queue A top-down for highest likely impact.
2. For Queue B, open src/data/event_data.c first, then follow listed touchpoints.
3. Update master matrix row status after evidence is found.
