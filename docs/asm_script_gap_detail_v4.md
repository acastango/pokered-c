# ASM Script Gap Detail (v4)

Date: 2026-05-05

- Source: docs/asm_script_to_c_owner_matrix_v3.csv + src/data/event_data.c
- Output CSV: docs/asm_script_gap_detail_v4.csv

## Counts
- total: 224
- mapped: 7
- partial: 206
- missing: 11

## Keyness Buckets
- high: 40
- medium: 80
- low: 93
- unknown: 11

## Partial: Top 30 by Structural Surface
- Route1.asm | score=220 | trainers=0 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- Route2.asm | score=121 | trainers=15 | reason=mapped only via split/variant map symbol(s) in event_data; no dedicated *_scripts.c owner found
- SilphCo2F.asm | score=84 | trainers=0 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- SaffronCity.asm | score=66 | trainers=0 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- SSAnne2F.asm | score=50 | trainers=4 | reason=covered by grouped script module via explicit asm reference (not one-file-per-map)
- Route16.asm | score=49 | trainers=0 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- LancesRoom.asm | score=48 | trainers=0 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- MtMoon1F.asm | score=46 | trainers=7 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- ViridianForest.asm | score=44 | trainers=3 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- SSAnne1F.asm | score=43 | trainers=4 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- SSAnneB1F.asm | score=42 | trainers=6 | reason=mapped only via split/variant map symbol(s) in event_data; no dedicated *_scripts.c owner found
- SSAnneB1FRooms.asm | score=42 | trainers=6 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- SaffronGym.asm | score=41 | trainers=0 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- Route25.asm | score=40 | trainers=9 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- SSAnne2FRooms.asm | score=39 | trainers=4 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- Route6.asm | score=34 | trainers=6 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- Route3.asm | score=34 | trainers=8 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- FuchsiaCity.asm | score=32 | trainers=0 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- CeladonCity.asm | score=31 | trainers=0 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- Route12.asm | score=31 | trainers=0 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- PowerPlant.asm | score=30 | trainers=0 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- SSAnne1FRooms.asm | score=30 | trainers=4 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- MtMoonB2F.asm | score=30 | trainers=5 | reason=covered by grouped script module via explicit asm reference (not one-file-per-map)
- Route11.asm | score=27 | trainers=0 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- CeruleanCity.asm | score=27 | trainers=0 | reason=covered by grouped script module via explicit asm reference (not one-file-per-map)
- Route15.asm | score=26 | trainers=0 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- CinnabarLab.asm | score=26 | trainers=0 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- VictoryRoad2F.asm | score=24 | trainers=0 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- GameCorner.asm | score=22 | trainers=0 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- VermilionCity.asm | score=22 | trainers=0 | reason=covered by grouped script module via explicit asm reference (not one-file-per-map)

## Missing (all)
- CeladonMansion2F.asm | keyness=unknown | reason=no exact name, explicit asm ref, or event_data map-symbol ownership evidence
- CeladonMartElevator.asm | keyness=unknown | reason=no exact name, explicit asm ref, or event_data map-symbol ownership evidence
- CeruleanCave1F.asm | keyness=unknown | reason=no exact name, explicit asm ref, or event_data map-symbol ownership evidence
- CeruleanCave2F.asm | keyness=unknown | reason=no exact name, explicit asm ref, or event_data map-symbol ownership evidence
- CeruleanCity_2.asm | keyness=unknown | reason=no exact name, explicit asm ref, or event_data map-symbol ownership evidence
- MtMoonB1F.asm | keyness=unknown | reason=no exact name, explicit asm ref, or event_data map-symbol ownership evidence
- RedsHouse2F.asm | keyness=unknown | reason=no exact name, explicit asm ref, or event_data map-symbol ownership evidence
- RocketHideoutElevator.asm | keyness=unknown | reason=no exact name, explicit asm ref, or event_data map-symbol ownership evidence
- SilphCoElevator.asm | keyness=unknown | reason=no exact name, explicit asm ref, or event_data map-symbol ownership evidence
- UndergroundPathNorthSouth.asm | keyness=unknown | reason=no exact name, explicit asm ref, or event_data map-symbol ownership evidence
- UndergroundPathWestEast.asm | keyness=unknown | reason=no exact name, explicit asm ref, or event_data map-symbol ownership evidence

## Scoring Method (Structural Only)
- surface_score = warps + npcs + signs + (2 * items) + (3 * trainers) + (2 * hidden_events)
- keyness:
  - high: surface_score >= 20 OR trainers > 0
  - medium: surface_score >= 8
  - low: surface_score < 8
  - unknown: missing row with no event_data map-symbol surface evidence

No behavior-level conclusions are included in this file.
