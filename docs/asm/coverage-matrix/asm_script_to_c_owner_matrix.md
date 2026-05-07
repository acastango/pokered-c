# ASM Script to C Owner Matrix

Date: 2026-05-05

- Source CSV: docs/asm_script_to_c_owner_matrix.csv
- Total ASM scripts: 224
- Mapped: 7
- Missing: 217

## Mapped
- BillsHouse.asm -> bills_house_scripts.c
- OaksLab.asm -> oakslab_scripts.c
- Route22.asm -> route22_scripts.c
- Route24.asm -> route24_scripts.c
- Route2Gate.asm -> route2gate_scripts.c
- VermilionGym.asm -> vermilion_gym_scripts.c
- ViridianMart.asm -> viridian_mart_scripts.c

## Missing (first 40)
- AgathasRoom.asm
- BikeShop.asm
- BluesHouse.asm
- BrunosRoom.asm
- CeladonChiefHouse.asm
- CeladonCity.asm
- CeladonDiner.asm
- CeladonGym.asm
- CeladonHotel.asm
- CeladonMansion1F.asm
- CeladonMansion2F.asm
- CeladonMansion3F.asm
- CeladonMansionRoof.asm
- CeladonMansionRoofHouse.asm
- CeladonMart1F.asm
- CeladonMart2F.asm
- CeladonMart3F.asm
- CeladonMart4F.asm
- CeladonMart5F.asm
- CeladonMartElevator.asm
- CeladonMartRoof.asm
- CeladonPokecenter.asm
- CeruleanBadgeHouse.asm
- CeruleanCave1F.asm
- CeruleanCave2F.asm
- CeruleanCaveB1F.asm
- CeruleanCity.asm
- CeruleanCity_2.asm
- CeruleanGym.asm
- CeruleanMart.asm
- CeruleanPokecenter.asm
- CeruleanTradeHouse.asm
- CeruleanTrashedHouse.asm
- ChampionsRoom.asm
- CinnabarGym.asm
- CinnabarIsland.asm
- CinnabarLab.asm
- CinnabarLabFossilRoom.asm
- CinnabarLabMetronomeRoom.asm
- CinnabarLabTradeRoom.asm

## Method
- Match rule: normalize names to lowercase alnum and compare ASM scripts/*.asm basename to C src/game/*_scripts.c basename (without _scripts).
- Statuses in CSV:
  - mapped: normalized name match found
  - missing: no normalized name match found
