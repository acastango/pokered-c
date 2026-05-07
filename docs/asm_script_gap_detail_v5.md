# ASM Script Gap Detail (v5)

Date: 2026-05-05

- Output CSV: docs/asm_script_gap_detail_v5.csv

## Counts
- total: 224
- mapped: 7
- partial: 198
- missing: 19

## Keyness Buckets
- high: 31
- medium: 86
- low: 88
- unknown: 19

## Partial: Top 25 by Structural Surface
- SilphCo2F.asm | score=84 | trainers=0 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- SaffronCity.asm | score=66 | trainers=0 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- SSAnne2F.asm | score=50 | trainers=4 | reason=covered by grouped script module via explicit asm reference (not one-file-per-map)
- LancesRoom.asm | score=48 | trainers=0 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- MtMoon1F.asm | score=46 | trainers=7 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- SSAnne1F.asm | score=43 | trainers=4 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- SSAnneB1FRooms.asm | score=42 | trainers=6 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- SSAnneB1F.asm | score=42 | trainers=6 | reason=mapped only via strict split/variant symbol(s) in event_data; no dedicated *_scripts.c owner found
- SaffronGym.asm | score=41 | trainers=0 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- Route25.asm | score=40 | trainers=9 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- SSAnne2FRooms.asm | score=39 | trainers=4 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- Route3.asm | score=34 | trainers=8 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- FuchsiaCity.asm | score=32 | trainers=0 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- ViridianForest.asm | score=32 | trainers=3 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- CeladonCity.asm | score=31 | trainers=0 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- SSAnne1FRooms.asm | score=30 | trainers=4 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- MtMoonB2F.asm | score=30 | trainers=5 | reason=covered by grouped script module via explicit asm reference (not one-file-per-map)
- PowerPlant.asm | score=30 | trainers=0 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- Route6.asm | score=29 | trainers=6 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- CeruleanCity.asm | score=27 | trainers=0 | reason=covered by grouped script module via explicit asm reference (not one-file-per-map)
- VictoryRoad2F.asm | score=24 | trainers=0 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- VermilionCity.asm | score=22 | trainers=0 | reason=covered by grouped script module via explicit asm reference (not one-file-per-map)
- Route16Gate1F.asm | score=22 | trainers=0 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- SilphCo5F.asm | score=21 | trainers=0 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found
- CeladonMart3F.asm | score=20 | trainers=0 | reason=map present in event_data dispatch only; no dedicated *_scripts.c owner found

## Missing (all)
- CeladonMansion2F.asm | keyness=unknown
- CeladonMansionRoof.asm | keyness=unknown
- CeladonMartElevator.asm | keyness=unknown
- CeruleanCave1F.asm | keyness=unknown
- CeruleanCave2F.asm | keyness=unknown
- CeruleanCity_2.asm | keyness=unknown
- MtMoonB1F.asm | keyness=unknown
- RedsHouse2F.asm | keyness=unknown
- RocketHideoutElevator.asm | keyness=unknown
- Route2.asm | keyness=unknown
- Route5.asm | keyness=unknown
- Route7.asm | keyness=unknown
- SafariZoneCenter.asm | keyness=unknown
- SafariZoneEast.asm | keyness=unknown
- SafariZoneNorth.asm | keyness=unknown
- SafariZoneWest.asm | keyness=unknown
- SilphCoElevator.asm | keyness=unknown
- UndergroundPathNorthSouth.asm | keyness=unknown
- UndergroundPathWestEast.asm | keyness=unknown

## Notes
- Prefix matching is strict in v5 and only accepts plausible split suffixes (e.g., 1F/2F/B1F, Rooms, Copy, Elevator, directional variants, Route# suffix).
- This avoids false joins like Route1 -> Route10/11.
