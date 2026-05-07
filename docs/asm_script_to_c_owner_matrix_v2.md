# ASM Script to C Owner Matrix (v2, Evidence-Based)

Date: 2026-05-05

- Source CSV: docs/asm_script_to_c_owner_matrix_v2.csv
- Total ASM scripts: 224
- mapped: 7
- partial: 11
- missing: 206

## Evidence Rules
- exact_name: normalized basename match between scripts/*.asm and src/game/*_scripts.c basename.
- explicit_scripts_ref: C script file contains explicit scripts/<AsmScript>.asm reference.
- No inference-based ownership was added.

## Owned (mapped + partial)
- BillsHouse.asm -> bills_house_scripts.c [mapped: exact_name; explicit_scripts_ref]
- CeruleanCity.asm -> cerulean_scripts.c [partial: explicit_scripts_ref]
- MtMoonB2F.asm -> mtmoon_scripts.c [partial: explicit_scripts_ref]
- OaksLab.asm -> oakslab_scripts.c [mapped: exact_name; explicit_scripts_ref]
- PalletTown.asm -> pallet_scripts.c [partial: explicit_scripts_ref]
- PewterCity.asm -> gate_scripts.c [partial: explicit_scripts_ref]
- PewterGym.asm -> gym_scripts.c [partial: explicit_scripts_ref]
- Route22.asm -> route22_scripts.c [mapped: exact_name; explicit_scripts_ref]
- Route22Gate.asm -> gate_scripts.c [partial: explicit_scripts_ref]
- Route24.asm -> route24_scripts.c [mapped: exact_name; explicit_scripts_ref]
- Route2Gate.asm -> route2gate_scripts.c [mapped: exact_name; explicit_scripts_ref]
- SSAnne2F.asm -> ss_anne_scripts.c [partial: explicit_scripts_ref]
- SSAnneCaptainsRoom.asm -> ss_anne_scripts.c [partial: explicit_scripts_ref]
- VermilionCity.asm -> ss_anne_scripts.c [partial: explicit_scripts_ref]
- VermilionDock.asm -> ss_anne_scripts.c [partial: explicit_scripts_ref]
- VermilionGym.asm -> vermilion_gym_scripts.c [mapped: exact_name; explicit_scripts_ref]
- ViridianCity.asm -> gate_scripts.c [partial: explicit_scripts_ref]
- ViridianMart.asm -> viridian_mart_scripts.c [mapped: exact_name; explicit_scripts_ref]

## Missing (first 50)
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
- CinnabarMart.asm
- CinnabarPokecenter.asm
- Colosseum.asm
- CopycatsHouse1F.asm
- CopycatsHouse2F.asm
- Daycare.asm
- DiglettsCave.asm
- DiglettsCaveRoute11.asm
- DiglettsCaveRoute2.asm
- FightingDojo.asm
- FuchsiaBillsGrandpasHouse.asm
