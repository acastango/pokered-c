/* trainer_data.c — Trainer party data.
 *
 * Direct port of data/trainers/parties.asm.
 * Party encoding: see trainer_data.h for format A / format B description.
 *
 * ALWAYS refer to pokered-master ASM before modifying anything here.
 */
#include "trainer_data.h"
#include "game/constants.h"

/* ---- Youngster (class 1) ------------------------------------------ */
static const uint8_t gYoungsterParties[] = {
    /* Route 3 */
    11, SPECIES_RATTATA, SPECIES_EKANS, 0,
    14, SPECIES_SPEAROW, 0,
    /* Mt. Moon 1F */
    10, SPECIES_RATTATA, SPECIES_RATTATA, SPECIES_ZUBAT, 0,
    /* Route 24 */
    14, SPECIES_RATTATA, SPECIES_EKANS, SPECIES_ZUBAT, 0,
    /* Route 25 */
    15, SPECIES_RATTATA, SPECIES_SPEAROW, 0,
    17, SPECIES_SLOWPOKE, 0,
    14, SPECIES_EKANS, SPECIES_SANDSHREW, 0,
    /* SS Anne 1F Rooms */
    21, SPECIES_NIDORAN_M, 0,
    /* Route 11 */
    21, SPECIES_EKANS, 0,
    19, SPECIES_SANDSHREW, SPECIES_ZUBAT, 0,
    17, SPECIES_RATTATA, SPECIES_RATTATA, SPECIES_RATICATE, 0,
    18, SPECIES_NIDORAN_M, SPECIES_NIDORINO, 0,
    /* Unused */
    17, SPECIES_SPEAROW, SPECIES_RATTATA, SPECIES_RATTATA, SPECIES_SPEAROW, 0,
};

/* ---- Bug Catcher (class 2) ---------------------------------------- */
static const uint8_t gBugCatcherParties[] = {
    /* Viridian Forest */
    6, SPECIES_WEEDLE, SPECIES_CATERPIE, 0,
    7, SPECIES_WEEDLE, SPECIES_KAKUNA, SPECIES_WEEDLE, 0,
    9, SPECIES_WEEDLE, 0,
    /* Route 3 */
    10, SPECIES_CATERPIE, SPECIES_WEEDLE, SPECIES_CATERPIE, 0,
    9, SPECIES_WEEDLE, SPECIES_KAKUNA, SPECIES_CATERPIE, SPECIES_METAPOD, 0,
    11, SPECIES_CATERPIE, SPECIES_METAPOD, 0,
    /* Mt. Moon 1F */
    11, SPECIES_WEEDLE, SPECIES_KAKUNA, 0,
    10, SPECIES_CATERPIE, SPECIES_METAPOD, SPECIES_CATERPIE, 0,
    /* Route 24 */
    14, SPECIES_CATERPIE, SPECIES_WEEDLE, 0,
    /* Route 6 */
    16, SPECIES_WEEDLE, SPECIES_CATERPIE, SPECIES_WEEDLE, 0,
    20, SPECIES_BUTTERFREE, 0,
    /* Unused */
    18, SPECIES_METAPOD, SPECIES_CATERPIE, SPECIES_VENONAT, 0,
    /* Route 9 */
    19, SPECIES_BEEDRILL, SPECIES_BEEDRILL, 0,
    20, SPECIES_CATERPIE, SPECIES_WEEDLE, SPECIES_VENONAT, 0,
};

/* ---- Lass (class 3) ----------------------------------------------- */
static const uint8_t gLassParties[] = {
    /* Route 3 */
    9, SPECIES_PIDGEY, SPECIES_PIDGEY, 0,
    10, SPECIES_RATTATA, SPECIES_NIDORAN_M, 0,
    14, SPECIES_JIGGLYPUFF, 0,
    /* Route 4 */
    31, SPECIES_PARAS, SPECIES_PARAS, SPECIES_PARASECT, 0,
    /* Mt. Moon 1F */
    11, SPECIES_ODDISH, SPECIES_BELLSPROUT, 0,
    14, SPECIES_CLEFAIRY, 0,
    /* Route 24 */
    16, SPECIES_PIDGEY, SPECIES_NIDORAN_F, 0,
    14, SPECIES_PIDGEY, SPECIES_NIDORAN_F, 0,
    /* Route 25 */
    15, SPECIES_NIDORAN_M, SPECIES_NIDORAN_F, 0,
    13, SPECIES_ODDISH, SPECIES_PIDGEY, SPECIES_ODDISH, 0,
    /* SS Anne 1F Rooms */
    18, SPECIES_PIDGEY, SPECIES_NIDORAN_F, 0,
    /* SS Anne 2F Rooms */
    18, SPECIES_RATTATA, SPECIES_PIKACHU, 0,
    /* Route 8 */
    23, SPECIES_NIDORAN_F, SPECIES_NIDORINA, 0,
    24, SPECIES_MEOWTH, SPECIES_MEOWTH, SPECIES_MEOWTH, 0,
    19, SPECIES_PIDGEY, SPECIES_RATTATA, SPECIES_NIDORAN_M, SPECIES_MEOWTH, SPECIES_PIKACHU, 0,
    22, SPECIES_CLEFAIRY, SPECIES_CLEFAIRY, 0,
    /* Celadon Gym */
    23, SPECIES_BELLSPROUT, SPECIES_WEEPINBELL, 0,
    23, SPECIES_ODDISH, SPECIES_GLOOM, 0,
};

/* ---- Sailor (class 4) --------------------------------------------- */
static const uint8_t gSailorParties[] = {
    /* SS Anne Stern */
    18, SPECIES_MACHOP, SPECIES_SHELLDER, 0,
    17, SPECIES_MACHOP, SPECIES_TENTACOOL, 0,
    /* SS Anne B1F Rooms */
    21, SPECIES_SHELLDER, 0,
    17, SPECIES_HORSEA, SPECIES_SHELLDER, SPECIES_TENTACOOL, 0,
    18, SPECIES_TENTACOOL, SPECIES_STARYU, 0,
    17, SPECIES_HORSEA, SPECIES_HORSEA, SPECIES_HORSEA, 0,
    20, SPECIES_MACHOP, 0,
    /* Vermilion Gym */
    21, SPECIES_PIKACHU, SPECIES_PIKACHU, 0,
};

/* ---- Jr. Trainer M (class 5) -------------------------------------- */
static const uint8_t gJrTrainerMParties[] = {
    /* Pewter Gym */
    11, SPECIES_DIGLETT, SPECIES_SANDSHREW, 0,
    /* Route 24/25 */
    14, SPECIES_RATTATA, SPECIES_EKANS, 0,
    /* Route 24 */
    18, SPECIES_MANKEY, 0,
    /* Route 6 */
    20, SPECIES_SQUIRTLE, 0,
    16, SPECIES_SPEAROW, SPECIES_RATICATE, 0,
    /* Unused */
    18, SPECIES_DIGLETT, SPECIES_DIGLETT, SPECIES_SANDSHREW, 0,
    /* Route 9 */
    21, SPECIES_GROWLITHE, SPECIES_CHARMANDER, 0,
    19, SPECIES_RATTATA, SPECIES_DIGLETT, SPECIES_EKANS, SPECIES_SANDSHREW, 0,
    /* Route 12 */
    29, SPECIES_NIDORAN_M, SPECIES_NIDORINO, 0,
};

/* ---- Jr. Trainer F (class 6) -------------------------------------- */
static const uint8_t gJrTrainerFParties[] = {
    /* Cerulean Gym */
    19, SPECIES_GOLDEEN, 0,
    /* Route 6 */
    16, SPECIES_RATTATA, SPECIES_PIKACHU, 0,
    16, SPECIES_PIDGEY, SPECIES_PIDGEY, SPECIES_PIDGEY, 0,
    /* Unused */
    22, SPECIES_BULBASAUR, 0,
    /* Route 9 */
    18, SPECIES_ODDISH, SPECIES_BELLSPROUT, SPECIES_ODDISH, SPECIES_BELLSPROUT, 0,
    23, SPECIES_MEOWTH, 0,
    /* Route 10 */
    20, SPECIES_PIKACHU, SPECIES_CLEFAIRY, 0,
    21, SPECIES_PIDGEY, SPECIES_PIDGEOTTO, 0,
    /* Rock Tunnel B1F */
    21, SPECIES_JIGGLYPUFF, SPECIES_PIDGEY, SPECIES_MEOWTH, 0,
    22, SPECIES_ODDISH, SPECIES_BULBASAUR, 0,
    /* Celadon Gym */
    24, SPECIES_BULBASAUR, SPECIES_IVYSAUR, 0,
    /* Route 13 */
    24, SPECIES_PIDGEY, SPECIES_MEOWTH, SPECIES_RATTATA, SPECIES_PIKACHU, SPECIES_MEOWTH, 0,
    30, SPECIES_POLIWAG, SPECIES_POLIWAG, 0,
    27, SPECIES_PIDGEY, SPECIES_MEOWTH, SPECIES_PIDGEY, SPECIES_PIDGEOTTO, 0,
    28, SPECIES_GOLDEEN, SPECIES_POLIWAG, SPECIES_HORSEA, 0,
    /* Route 20 */
    31, SPECIES_GOLDEEN, SPECIES_SEAKING, 0,
    /* Rock Tunnel 1F */
    22, SPECIES_BELLSPROUT, SPECIES_CLEFAIRY, 0,
    20, SPECIES_MEOWTH, SPECIES_ODDISH, SPECIES_PIDGEY, 0,
    19, SPECIES_PIDGEY, SPECIES_RATTATA, SPECIES_RATTATA, SPECIES_BELLSPROUT, 0,
    /* Route 15 */
    28, SPECIES_GLOOM, SPECIES_ODDISH, SPECIES_ODDISH, 0,
    29, SPECIES_PIKACHU, SPECIES_RAICHU, 0,
    33, SPECIES_CLEFAIRY, 0,
    29, SPECIES_BELLSPROUT, SPECIES_ODDISH, SPECIES_TANGELA, 0,
    /* Route 20 */
    30, SPECIES_TENTACOOL, SPECIES_HORSEA, SPECIES_SEEL, 0,
};

/* ---- Pokémaniac (class 7) ----------------------------------------- */
static const uint8_t gPokemaniacParties[] = {
    /* Route 10 */
    30, SPECIES_RHYHORN, SPECIES_LICKITUNG, 0,
    20, SPECIES_CUBONE, SPECIES_SLOWPOKE, 0,
    /* Rock Tunnel B1F */
    20, SPECIES_SLOWPOKE, SPECIES_SLOWPOKE, SPECIES_SLOWPOKE, 0,
    22, SPECIES_CHARMANDER, SPECIES_CUBONE, 0,
    25, SPECIES_SLOWPOKE, 0,
    /* Victory Road 2F */
    40, SPECIES_CHARMELEON, SPECIES_LAPRAS, SPECIES_LICKITUNG, 0,
    /* Rock Tunnel 1F */
    23, SPECIES_CUBONE, SPECIES_SLOWPOKE, 0,
};

/* ---- Super Nerd (class 8) ----------------------------------------- */
static const uint8_t gSuperNerdParties[] = {
    /* Mt. Moon 1F */
    11, SPECIES_MAGNEMITE, SPECIES_VOLTORB, 0,
    /* Mt. Moon B2F */
    12, SPECIES_GRIMER, SPECIES_VOLTORB, SPECIES_KOFFING, 0,
    /* Route 8 */
    20, SPECIES_VOLTORB, SPECIES_KOFFING, SPECIES_VOLTORB, SPECIES_MAGNEMITE, 0,
    22, SPECIES_GRIMER, SPECIES_MUK, SPECIES_GRIMER, 0,
    26, SPECIES_KOFFING, 0,
    /* Unused */
    22, SPECIES_KOFFING, SPECIES_MAGNEMITE, SPECIES_WEEZING, 0,
    20, SPECIES_MAGNEMITE, SPECIES_MAGNEMITE, SPECIES_KOFFING, SPECIES_MAGNEMITE, 0,
    24, SPECIES_MAGNEMITE, SPECIES_VOLTORB, 0,
    /* Cinnabar Gym */
    36, SPECIES_VULPIX, SPECIES_VULPIX, SPECIES_NINETALES, 0,
    34, SPECIES_PONYTA, SPECIES_CHARMANDER, SPECIES_VULPIX, SPECIES_GROWLITHE, 0,
    41, SPECIES_RAPIDASH, 0,
    37, SPECIES_GROWLITHE, SPECIES_VULPIX, 0,
};

/* ---- Hiker (class 9) ---------------------------------------------- */
static const uint8_t gHikerParties[] = {
    /* Mt. Moon 1F */
    10, SPECIES_GEODUDE, SPECIES_GEODUDE, SPECIES_ONIX, 0,
    /* Route 25 */
    15, SPECIES_MACHOP, SPECIES_GEODUDE, 0,
    13, SPECIES_GEODUDE, SPECIES_GEODUDE, SPECIES_MACHOP, SPECIES_GEODUDE, 0,
    17, SPECIES_ONIX, 0,
    /* Route 9 */
    21, SPECIES_GEODUDE, SPECIES_ONIX, 0,
    20, SPECIES_GEODUDE, SPECIES_MACHOP, SPECIES_GEODUDE, 0,
    /* Route 10 */
    21, SPECIES_GEODUDE, SPECIES_ONIX, 0,
    19, SPECIES_ONIX, SPECIES_GRAVELER, 0,
    /* Rock Tunnel B1F */
    21, SPECIES_GEODUDE, SPECIES_GEODUDE, SPECIES_GRAVELER, 0,
    25, SPECIES_GEODUDE, 0,
    /* Route 9 / Rock Tunnel B1F */
    20, SPECIES_MACHOP, SPECIES_ONIX, 0,
    /* Rock Tunnel 1F */
    19, SPECIES_GEODUDE, SPECIES_MACHOP, SPECIES_GEODUDE, SPECIES_GEODUDE, 0,
    20, SPECIES_ONIX, SPECIES_ONIX, SPECIES_GEODUDE, 0,
    21, SPECIES_GEODUDE, SPECIES_GRAVELER, 0,
};

/* ---- Biker (class 10) --------------------------------------------- */
static const uint8_t gBikerParties[] = {
    /* Route 13 */
    28, SPECIES_KOFFING, SPECIES_KOFFING, SPECIES_KOFFING, 0,
    /* Route 14 */
    29, SPECIES_KOFFING, SPECIES_GRIMER, 0,
    /* Route 15 */
    25, SPECIES_KOFFING, SPECIES_KOFFING, SPECIES_WEEZING, SPECIES_KOFFING, SPECIES_GRIMER, 0,
    28, SPECIES_KOFFING, SPECIES_GRIMER, SPECIES_WEEZING, 0,
    /* Route 16 */
    29, SPECIES_GRIMER, SPECIES_KOFFING, 0,
    33, SPECIES_WEEZING, 0,
    26, SPECIES_GRIMER, SPECIES_GRIMER, SPECIES_GRIMER, SPECIES_GRIMER, 0,
    /* Route 17 */
    28, SPECIES_WEEZING, SPECIES_KOFFING, SPECIES_WEEZING, 0,
    33, SPECIES_MUK, 0,
    29, SPECIES_VOLTORB, SPECIES_VOLTORB, 0,
    29, SPECIES_WEEZING, SPECIES_MUK, 0,
    25, SPECIES_KOFFING, SPECIES_WEEZING, SPECIES_KOFFING, SPECIES_KOFFING, SPECIES_WEEZING, 0,
    /* Route 14 */
    26, SPECIES_KOFFING, SPECIES_KOFFING, SPECIES_GRIMER, SPECIES_KOFFING, 0,
    28, SPECIES_GRIMER, SPECIES_GRIMER, SPECIES_KOFFING, 0,
    29, SPECIES_KOFFING, SPECIES_MUK, 0,
};

/* ---- Burglar (class 11) ------------------------------------------- */
static const uint8_t gBurglarParties[] = {
    /* Unused */
    29, SPECIES_GROWLITHE, SPECIES_VULPIX, 0,
    33, SPECIES_GROWLITHE, 0,
    28, SPECIES_VULPIX, SPECIES_CHARMANDER, SPECIES_PONYTA, 0,
    /* Cinnabar Gym */
    36, SPECIES_GROWLITHE, SPECIES_VULPIX, SPECIES_NINETALES, 0,
    41, SPECIES_PONYTA, 0,
    37, SPECIES_VULPIX, SPECIES_GROWLITHE, 0,
    /* Mansion 2F */
    34, SPECIES_CHARMANDER, SPECIES_CHARMELEON, 0,
    /* Mansion 3F */
    38, SPECIES_NINETALES, 0,
    /* Mansion B1F */
    34, SPECIES_GROWLITHE, SPECIES_PONYTA, 0,
};

/* ---- Engineer (class 12) ------------------------------------------ */
static const uint8_t gEngineerParties[] = {
    /* Unused */
    21, SPECIES_VOLTORB, SPECIES_MAGNEMITE, 0,
    /* Route 11 */
    21, SPECIES_MAGNEMITE, 0,
    18, SPECIES_MAGNEMITE, SPECIES_MAGNEMITE, SPECIES_MAGNETON, 0,
};

/* ---- Unused Juggler (class 13) ------------------------------------ */
/* No entries in ASM — empty sentinel so pointer table remains valid */
static const uint8_t gUnusedJugglerParties[] = {
    0,
};

/* ---- Fisher (class 14) -------------------------------------------- */
static const uint8_t gFisherParties[] = {
    /* SS Anne 2F Rooms */
    17, SPECIES_GOLDEEN, SPECIES_TENTACOOL, SPECIES_GOLDEEN, 0,
    /* SS Anne B1F Rooms */
    17, SPECIES_TENTACOOL, SPECIES_STARYU, SPECIES_SHELLDER, 0,
    /* Route 12 */
    22, SPECIES_GOLDEEN, SPECIES_POLIWAG, SPECIES_GOLDEEN, 0,
    24, SPECIES_TENTACOOL, SPECIES_GOLDEEN, 0,
    27, SPECIES_GOLDEEN, 0,
    21, SPECIES_POLIWAG, SPECIES_SHELLDER, SPECIES_GOLDEEN, SPECIES_HORSEA, 0,
    /* Route 21 */
    28, SPECIES_SEAKING, SPECIES_GOLDEEN, SPECIES_SEAKING, SPECIES_SEAKING, 0,
    31, SPECIES_SHELLDER, SPECIES_CLOYSTER, 0,
    27, SPECIES_MAGIKARP, SPECIES_MAGIKARP, SPECIES_MAGIKARP,
        SPECIES_MAGIKARP, SPECIES_MAGIKARP, SPECIES_MAGIKARP, 0,
    33, SPECIES_SEAKING, SPECIES_GOLDEEN, 0,
    /* Route 12 */
    24, SPECIES_MAGIKARP, SPECIES_MAGIKARP, 0,
};

/* ---- Swimmer (class 15) ------------------------------------------- */
static const uint8_t gSwimmerParties[] = {
    /* Cerulean Gym */
    16, SPECIES_HORSEA, SPECIES_SHELLDER, 0,
    /* Route 19 */
    30, SPECIES_TENTACOOL, SPECIES_SHELLDER, 0,
    29, SPECIES_GOLDEEN, SPECIES_HORSEA, SPECIES_STARYU, 0,
    30, SPECIES_POLIWAG, SPECIES_POLIWHIRL, 0,
    27, SPECIES_HORSEA, SPECIES_TENTACOOL, SPECIES_TENTACOOL, SPECIES_GOLDEEN, 0,
    29, SPECIES_GOLDEEN, SPECIES_SHELLDER, SPECIES_SEAKING, 0,
    30, SPECIES_HORSEA, SPECIES_HORSEA, 0,
    27, SPECIES_TENTACOOL, SPECIES_TENTACOOL, SPECIES_STARYU,
        SPECIES_HORSEA, SPECIES_TENTACRUEL, 0,
    /* Route 20 */
    31, SPECIES_SHELLDER, SPECIES_CLOYSTER, 0,
    35, SPECIES_STARYU, 0,
    28, SPECIES_HORSEA, SPECIES_HORSEA, SPECIES_SEADRA, SPECIES_HORSEA, 0,
    /* Route 21 */
    33, SPECIES_SEADRA, SPECIES_TENTACRUEL, 0,
    37, SPECIES_STARMIE, 0,
    33, SPECIES_STARYU, SPECIES_WARTORTLE, 0,
    32, SPECIES_POLIWHIRL, SPECIES_TENTACOOL, SPECIES_SEADRA, 0,
};

/* ---- Cue Ball (class 16) ------------------------------------------ */
static const uint8_t gCueBallParties[] = {
    /* Route 16 */
    28, SPECIES_MACHOP, SPECIES_MANKEY, SPECIES_MACHOP, 0,
    29, SPECIES_MANKEY, SPECIES_MACHOP, 0,
    33, SPECIES_MACHOP, 0,
    /* Route 17 */
    29, SPECIES_MANKEY, SPECIES_PRIMEAPE, 0,
    29, SPECIES_MACHOP, SPECIES_MACHOKE, 0,
    33, SPECIES_MACHOKE, 0,
    26, SPECIES_MANKEY, SPECIES_MANKEY, SPECIES_MACHOKE, SPECIES_MACHOP, 0,
    29, SPECIES_PRIMEAPE, SPECIES_MACHOKE, 0,
    /* Route 21 */
    31, SPECIES_TENTACOOL, SPECIES_TENTACOOL, SPECIES_TENTACRUEL, 0,
};

/* ---- Gambler (class 17) ------------------------------------------- */
static const uint8_t gGamblerParties[] = {
    /* Route 11 */
    18, SPECIES_POLIWAG, SPECIES_HORSEA, 0,
    18, SPECIES_BELLSPROUT, SPECIES_ODDISH, 0,
    18, SPECIES_VOLTORB, SPECIES_MAGNEMITE, 0,
    18, SPECIES_GROWLITHE, SPECIES_VULPIX, 0,
    /* Route 8 */
    22, SPECIES_POLIWAG, SPECIES_POLIWAG, SPECIES_POLIWHIRL, 0,
    /* Unused */
    22, SPECIES_ONIX, SPECIES_GEODUDE, SPECIES_GRAVELER, 0,
    /* Route 8 */
    24, SPECIES_GROWLITHE, SPECIES_VULPIX, 0,
};

/* ---- Beauty (class 18) -------------------------------------------- */
static const uint8_t gBeautyParties[] = {
    /* Celadon Gym */
    21, SPECIES_ODDISH, SPECIES_BELLSPROUT, SPECIES_ODDISH, SPECIES_BELLSPROUT, 0,
    24, SPECIES_BELLSPROUT, SPECIES_BELLSPROUT, 0,
    26, SPECIES_EXEGGCUTE, 0,
    /* Route 13 */
    27, SPECIES_RATTATA, SPECIES_PIKACHU, SPECIES_RATTATA, 0,
    29, SPECIES_CLEFAIRY, SPECIES_MEOWTH, 0,
    /* Route 20 */
    35, SPECIES_SEAKING, 0,
    30, SPECIES_SHELLDER, SPECIES_SHELLDER, SPECIES_CLOYSTER, 0,
    31, SPECIES_POLIWAG, SPECIES_SEAKING, 0,
    /* Route 15 */
    29, SPECIES_PIDGEOTTO, SPECIES_WIGGLYTUFF, 0,
    29, SPECIES_BULBASAUR, SPECIES_IVYSAUR, 0,
    /* Unused */
    33, SPECIES_WEEPINBELL, SPECIES_BELLSPROUT, SPECIES_WEEPINBELL, 0,
    /* Route 19 */
    27, SPECIES_POLIWAG, SPECIES_GOLDEEN, SPECIES_SEAKING,
        SPECIES_GOLDEEN, SPECIES_POLIWAG, 0,
    30, SPECIES_GOLDEEN, SPECIES_SEAKING, 0,
    29, SPECIES_STARYU, SPECIES_STARYU, SPECIES_STARYU, 0,
    /* Route 20 */
    30, SPECIES_SEADRA, SPECIES_HORSEA, SPECIES_SEADRA, 0,
};

/* ---- Psychic (class 19) ------------------------------------------- */
static const uint8_t gPsychicParties[] = {
    /* Saffron Gym */
    31, SPECIES_KADABRA, SPECIES_SLOWPOKE, SPECIES_MR_MIME, SPECIES_KADABRA, 0,
    34, SPECIES_MR_MIME, SPECIES_KADABRA, 0,
    33, SPECIES_SLOWPOKE, SPECIES_SLOWPOKE, SPECIES_SLOWBRO, 0,
    38, SPECIES_SLOWBRO, 0,
};

/* ---- Rocker (class 20) -------------------------------------------- */
static const uint8_t gRockerParties[] = {
    /* Vermilion Gym */
    20, SPECIES_VOLTORB, SPECIES_MAGNEMITE, SPECIES_VOLTORB, 0,
    /* Route 12 */
    29, SPECIES_VOLTORB, SPECIES_ELECTRODE, 0,
};

/* ---- Juggler (class 21) ------------------------------------------- */
static const uint8_t gJugglerParties[] = {
    /* Silph Co. 5F */
    29, SPECIES_KADABRA, SPECIES_MR_MIME, 0,
    /* Victory Road 2F */
    41, SPECIES_DROWZEE, SPECIES_HYPNO, SPECIES_KADABRA, SPECIES_KADABRA, 0,
    /* Fuchsia Gym */
    31, SPECIES_DROWZEE, SPECIES_DROWZEE, SPECIES_KADABRA, SPECIES_DROWZEE, 0,
    34, SPECIES_DROWZEE, SPECIES_HYPNO, 0,
    /* Victory Road 2F */
    48, SPECIES_MR_MIME, 0,
    /* Unused */
    33, SPECIES_HYPNO, 0,
    /* Fuchsia Gym */
    38, SPECIES_HYPNO, 0,
    34, SPECIES_DROWZEE, SPECIES_KADABRA, 0,
};

/* ---- Tamer (class 22) --------------------------------------------- */
static const uint8_t gTamerParties[] = {
    /* Fuchsia Gym */
    34, SPECIES_SANDSLASH, SPECIES_ARBOK, 0,
    33, SPECIES_ARBOK, SPECIES_SANDSLASH, SPECIES_ARBOK, 0,
    /* Viridian Gym */
    43, SPECIES_RHYHORN, 0,
    39, SPECIES_ARBOK, SPECIES_TAUROS, 0,
    /* Victory Road 2F */
    44, SPECIES_PERSIAN, SPECIES_GOLDUCK, 0,
    /* Unused */
    42, SPECIES_RHYHORN, SPECIES_PRIMEAPE, SPECIES_ARBOK, SPECIES_TAUROS, 0,
};

/* ---- Bird Keeper (class 23) --------------------------------------- */
static const uint8_t gBirdKeeperParties[] = {
    /* Route 13 */
    29, SPECIES_PIDGEY, SPECIES_PIDGEOTTO, 0,
    25, SPECIES_SPEAROW, SPECIES_PIDGEY, SPECIES_PIDGEY, SPECIES_SPEAROW, SPECIES_SPEAROW, 0,
    26, SPECIES_PIDGEY, SPECIES_PIDGEOTTO, SPECIES_SPEAROW, SPECIES_FEAROW, 0,
    /* Route 14 */
    33, SPECIES_FARFETCHD, 0,
    29, SPECIES_SPEAROW, SPECIES_FEAROW, 0,
    /* Route 15 */
    26, SPECIES_PIDGEOTTO, SPECIES_FARFETCHD, SPECIES_DODUO, SPECIES_PIDGEY, 0,
    28, SPECIES_DODRIO, SPECIES_DODUO, SPECIES_DODUO, 0,
    /* Route 18 */
    29, SPECIES_SPEAROW, SPECIES_FEAROW, 0,
    34, SPECIES_DODRIO, 0,
    26, SPECIES_SPEAROW, SPECIES_SPEAROW, SPECIES_FEAROW, SPECIES_SPEAROW, 0,
    /* Route 20 */
    30, SPECIES_FEAROW, SPECIES_FEAROW, SPECIES_PIDGEOTTO, 0,
    /* Unused */
    39, SPECIES_PIDGEOTTO, SPECIES_PIDGEOTTO, SPECIES_PIDGEY, SPECIES_PIDGEOTTO, 0,
    42, SPECIES_FARFETCHD, SPECIES_FEAROW, 0,
    /* Route 14 */
    28, SPECIES_PIDGEY, SPECIES_DODUO, SPECIES_PIDGEOTTO, 0,
    26, SPECIES_PIDGEY, SPECIES_SPEAROW, SPECIES_PIDGEY, SPECIES_FEAROW, 0,
    29, SPECIES_PIDGEOTTO, SPECIES_FEAROW, 0,
    28, SPECIES_SPEAROW, SPECIES_DODUO, SPECIES_FEAROW, 0,
};

/* ---- Blackbelt (class 24) ----------------------------------------- */
static const uint8_t gBlackbeltParties[] = {
    /* Fighting Dojo */
    37, SPECIES_HITMONLEE, SPECIES_HITMONCHAN, 0,
    31, SPECIES_MANKEY, SPECIES_MANKEY, SPECIES_PRIMEAPE, 0,
    32, SPECIES_MACHOP, SPECIES_MACHOKE, 0,
    36, SPECIES_PRIMEAPE, 0,
    31, SPECIES_MACHOP, SPECIES_MANKEY, SPECIES_PRIMEAPE, 0,
    /* Viridian Gym */
    40, SPECIES_MACHOP, SPECIES_MACHOKE, 0,
    43, SPECIES_MACHOKE, 0,
    38, SPECIES_MACHOKE, SPECIES_MACHOP, SPECIES_MACHOKE, 0,
    /* Victory Road 2F */
    43, SPECIES_MACHOKE, SPECIES_MACHOP, SPECIES_MACHOKE, 0,
};

/* ---- Rival 1 (class 25) ------------------------------------------- */
static const uint8_t gRival1Parties[] = {
    /* Route 1 / Pallet (starter choice) */
    5, SPECIES_SQUIRTLE, 0,
    5, SPECIES_BULBASAUR, 0,
    5, SPECIES_CHARMANDER, 0,
    /* Route 22 */
    0xFF, 9, SPECIES_PIDGEY, 8, SPECIES_SQUIRTLE, 0,
    0xFF, 9, SPECIES_PIDGEY, 8, SPECIES_BULBASAUR, 0,
    0xFF, 9, SPECIES_PIDGEY, 8, SPECIES_CHARMANDER, 0,
    /* Cerulean City */
    0xFF, 18, SPECIES_PIDGEOTTO, 15, SPECIES_ABRA,
          15, SPECIES_RATTATA, 17, SPECIES_SQUIRTLE, 0,
    0xFF, 18, SPECIES_PIDGEOTTO, 15, SPECIES_ABRA,
          15, SPECIES_RATTATA, 17, SPECIES_BULBASAUR, 0,
    0xFF, 18, SPECIES_PIDGEOTTO, 15, SPECIES_ABRA,
          15, SPECIES_RATTATA, 17, SPECIES_CHARMANDER, 0,
};

/* ---- Prof. Oak (class 26) ----------------------------------------- */
static const uint8_t gProfOakParties[] = {
    /* Unused */
    0xFF, 66, SPECIES_TAUROS, 67, SPECIES_EXEGGUTOR, 68, SPECIES_ARCANINE,
          69, SPECIES_BLASTOISE, 70, SPECIES_GYARADOS, 0,
    0xFF, 66, SPECIES_TAUROS, 67, SPECIES_EXEGGUTOR, 68, SPECIES_ARCANINE,
          69, SPECIES_VENUSAUR, 70, SPECIES_GYARADOS, 0,
    0xFF, 66, SPECIES_TAUROS, 67, SPECIES_EXEGGUTOR, 68, SPECIES_ARCANINE,
          69, SPECIES_CHARIZARD, 70, SPECIES_GYARADOS, 0,
};

/* ---- Chief (class 27) --------------------------------------------- */
/* No entries in ASM — empty sentinel so pointer table remains valid */
static const uint8_t gChiefParties[] = {
    0,
};

/* ---- Scientist (class 28) ----------------------------------------- */
static const uint8_t gScientistParties[] = {
    /* Unused */
    34, SPECIES_KOFFING, SPECIES_VOLTORB, 0,
    /* Silph Co. 2F */
    26, SPECIES_GRIMER, SPECIES_WEEZING, SPECIES_KOFFING, SPECIES_WEEZING, 0,
    28, SPECIES_MAGNEMITE, SPECIES_VOLTORB, SPECIES_MAGNETON, 0,
    /* Silph Co. 3F / Mansion 1F */
    29, SPECIES_ELECTRODE, SPECIES_WEEZING, 0,
    /* Silph Co. 4F */
    33, SPECIES_ELECTRODE, 0,
    /* Silph Co. 5F */
    26, SPECIES_MAGNETON, SPECIES_KOFFING, SPECIES_WEEZING, SPECIES_MAGNEMITE, 0,
    /* Silph Co. 6F */
    25, SPECIES_VOLTORB, SPECIES_KOFFING, SPECIES_MAGNETON,
        SPECIES_MAGNEMITE, SPECIES_KOFFING, 0,
    /* Silph Co. 7F */
    29, SPECIES_ELECTRODE, SPECIES_MUK, 0,
    /* Silph Co. 8F */
    29, SPECIES_GRIMER, SPECIES_ELECTRODE, 0,
    /* Silph Co. 9F */
    28, SPECIES_VOLTORB, SPECIES_KOFFING, SPECIES_MAGNETON, 0,
    /* Silph Co. 10F */
    29, SPECIES_MAGNEMITE, SPECIES_KOFFING, 0,
    /* Mansion 3F */
    33, SPECIES_MAGNEMITE, SPECIES_MAGNETON, SPECIES_VOLTORB, 0,
    /* Mansion B1F */
    34, SPECIES_MAGNEMITE, SPECIES_ELECTRODE, 0,
};

/* ---- Giovanni (class 29) ------------------------------------------ */
static const uint8_t gGiovanniParties[] = {
    /* Rocket Hideout B4F */
    0xFF, 25, SPECIES_ONIX, 24, SPECIES_RHYHORN, 29, SPECIES_KANGASKHAN, 0,
    /* Silph Co. 11F */
    0xFF, 37, SPECIES_NIDORINO, 35, SPECIES_KANGASKHAN,
          37, SPECIES_RHYHORN, 41, SPECIES_NIDOQUEEN, 0,
    /* Viridian Gym */
    0xFF, 45, SPECIES_RHYHORN, 42, SPECIES_DUGTRIO,
          44, SPECIES_NIDOQUEEN, 45, SPECIES_NIDOKING, 50, SPECIES_RHYDON, 0,
};

/* ---- Rocket (class 30) -------------------------------------------- */
static const uint8_t gRocketParties[] = {
    /* Mt. Moon B2F */
    13, SPECIES_RATTATA, SPECIES_ZUBAT, 0,
    11, SPECIES_SANDSHREW, SPECIES_RATTATA, SPECIES_ZUBAT, 0,
    12, SPECIES_ZUBAT, SPECIES_EKANS, 0,
    16, SPECIES_RATICATE, 0,
    /* Cerulean City */
    17, SPECIES_MACHOP, SPECIES_DROWZEE, 0,
    /* Route 24 */
    15, SPECIES_EKANS, SPECIES_ZUBAT, 0,
    /* Game Corner */
    20, SPECIES_RATICATE, SPECIES_ZUBAT, 0,
    /* Rocket Hideout B1F */
    21, SPECIES_DROWZEE, SPECIES_MACHOP, 0,
    21, SPECIES_RATICATE, SPECIES_RATICATE, 0,
    20, SPECIES_GRIMER, SPECIES_KOFFING, SPECIES_KOFFING, 0,
    19, SPECIES_RATTATA, SPECIES_RATICATE, SPECIES_RATICATE, SPECIES_RATTATA, 0,
    22, SPECIES_GRIMER, SPECIES_KOFFING, 0,
    /* Rocket Hideout B2F */
    17, SPECIES_ZUBAT, SPECIES_KOFFING, SPECIES_GRIMER,
        SPECIES_ZUBAT, SPECIES_RATICATE, 0,
    /* Rocket Hideout B3F */
    20, SPECIES_RATTATA, SPECIES_RATICATE, SPECIES_DROWZEE, 0,
    21, SPECIES_MACHOP, SPECIES_MACHOP, 0,
    /* Rocket Hideout B4F */
    23, SPECIES_SANDSHREW, SPECIES_EKANS, SPECIES_SANDSLASH, 0,
    23, SPECIES_EKANS, SPECIES_SANDSHREW, SPECIES_ARBOK, 0,
    21, SPECIES_KOFFING, SPECIES_ZUBAT, 0,
    /* Pokemon Tower 7F */
    25, SPECIES_ZUBAT, SPECIES_ZUBAT, SPECIES_GOLBAT, 0,
    26, SPECIES_KOFFING, SPECIES_DROWZEE, 0,
    23, SPECIES_ZUBAT, SPECIES_RATTATA, SPECIES_RATICATE, SPECIES_ZUBAT, 0,
    /* Unused */
    26, SPECIES_DROWZEE, SPECIES_KOFFING, 0,
    /* Silph Co. 2F */
    29, SPECIES_CUBONE, SPECIES_ZUBAT, 0,
    25, SPECIES_GOLBAT, SPECIES_ZUBAT, SPECIES_ZUBAT,
        SPECIES_RATICATE, SPECIES_ZUBAT, 0,
    /* Silph Co. 3F */
    28, SPECIES_RATICATE, SPECIES_HYPNO, SPECIES_RATICATE, 0,
    /* Silph Co. 4F */
    29, SPECIES_MACHOP, SPECIES_DROWZEE, 0,
    28, SPECIES_EKANS, SPECIES_ZUBAT, SPECIES_CUBONE, 0,
    /* Silph Co. 5F */
    33, SPECIES_ARBOK, 0,
    33, SPECIES_HYPNO, 0,
    /* Silph Co. 6F */
    29, SPECIES_MACHOP, SPECIES_MACHOKE, 0,
    28, SPECIES_ZUBAT, SPECIES_ZUBAT, SPECIES_GOLBAT, 0,
    /* Silph Co. 7F */
    26, SPECIES_RATICATE, SPECIES_ARBOK, SPECIES_KOFFING, SPECIES_GOLBAT, 0,
    29, SPECIES_CUBONE, SPECIES_CUBONE, 0,
    29, SPECIES_SANDSHREW, SPECIES_SANDSLASH, 0,
    /* Silph Co. 8F */
    26, SPECIES_RATICATE, SPECIES_ZUBAT, SPECIES_GOLBAT, SPECIES_RATTATA, 0,
    28, SPECIES_WEEZING, SPECIES_GOLBAT, SPECIES_KOFFING, 0,
    /* Silph Co. 9F */
    28, SPECIES_DROWZEE, SPECIES_GRIMER, SPECIES_MACHOP, 0,
    28, SPECIES_GOLBAT, SPECIES_DROWZEE, SPECIES_HYPNO, 0,
    /* Silph Co. 10F */
    33, SPECIES_MACHOKE, 0,
    /* Silph Co. 11F */
    25, SPECIES_RATTATA, SPECIES_RATTATA, SPECIES_ZUBAT,
        SPECIES_RATTATA, SPECIES_EKANS, 0,
    32, SPECIES_CUBONE, SPECIES_DROWZEE, SPECIES_MAROWAK, 0,
};

/* ---- Cooltrainer M (class 31) ------------------------------------- */
static const uint8_t gCooltrainerMParties[] = {
    /* Viridian Gym */
    39, SPECIES_NIDORINO, SPECIES_NIDOKING, 0,
    /* Victory Road 3F */
    43, SPECIES_EXEGGUTOR, SPECIES_CLOYSTER, SPECIES_ARCANINE, 0,
    43, SPECIES_KINGLER, SPECIES_TENTACRUEL, SPECIES_BLASTOISE, 0,
    /* Unused */
    45, SPECIES_KINGLER, SPECIES_STARMIE, 0,
    /* Victory Road 1F */
    42, SPECIES_IVYSAUR, SPECIES_WARTORTLE, SPECIES_CHARMELEON, SPECIES_CHARIZARD, 0,
    /* Unused */
    44, SPECIES_IVYSAUR, SPECIES_WARTORTLE, SPECIES_CHARMELEON, 0,
    49, SPECIES_NIDOKING, 0,
    44, SPECIES_KINGLER, SPECIES_CLOYSTER, 0,
    /* Viridian Gym */
    39, SPECIES_SANDSLASH, SPECIES_DUGTRIO, 0,
    43, SPECIES_RHYHORN, 0,
};

/* ---- Cooltrainer F (class 32) ------------------------------------- */
static const uint8_t gCooltrainerFParties[] = {
    /* Celadon Gym */
    24, SPECIES_WEEPINBELL, SPECIES_GLOOM, SPECIES_IVYSAUR, 0,
    /* Victory Road 3F */
    43, SPECIES_BELLSPROUT, SPECIES_WEEPINBELL, SPECIES_VICTREEBEL, 0,
    43, SPECIES_PARASECT, SPECIES_DEWGONG, SPECIES_CHANSEY, 0,
    /* Unused */
    46, SPECIES_VILEPLUME, SPECIES_BUTTERFREE, 0,
    /* Victory Road 1F */
    44, SPECIES_PERSIAN, SPECIES_NINETALES, 0,
    /* Unused */
    45, SPECIES_IVYSAUR, SPECIES_VENUSAUR, 0,
    45, SPECIES_NIDORINA, SPECIES_NIDOQUEEN, 0,
    43, SPECIES_PERSIAN, SPECIES_NINETALES, SPECIES_RAICHU, 0,
};

/* ---- Bruno (class 33) --------------------------------------------- */
static const uint8_t gBrunoParties[] = {
    0xFF, 53, SPECIES_ONIX, 55, SPECIES_HITMONCHAN, 55, SPECIES_HITMONLEE,
          56, SPECIES_ONIX, 58, SPECIES_MACHAMP, 0,
};

/* ---- Brock (class 34) --------------------------------------------- */
static const uint8_t gBrockParties[] = {
    0xFF, 12, SPECIES_GEODUDE, 14, SPECIES_ONIX, 0,
};

/* ---- Misty (class 35) --------------------------------------------- */
static const uint8_t gMistyParties[] = {
    0xFF, 18, SPECIES_STARYU, 21, SPECIES_STARMIE, 0,
};

/* ---- Lt. Surge (class 36) ----------------------------------------- */
static const uint8_t gLtSurgeParties[] = {
    0xFF, 21, SPECIES_VOLTORB, 18, SPECIES_PIKACHU, 24, SPECIES_RAICHU, 0,
};

/* ---- Erika (class 37) --------------------------------------------- */
static const uint8_t gErikaParties[] = {
    0xFF, 29, SPECIES_VICTREEBEL, 24, SPECIES_TANGELA, 29, SPECIES_VILEPLUME, 0,
};

/* ---- Koga (class 38) ---------------------------------------------- */
static const uint8_t gKogaParties[] = {
    0xFF, 37, SPECIES_KOFFING, 39, SPECIES_MUK,
          37, SPECIES_KOFFING, 43, SPECIES_WEEZING, 0,
};

/* ---- Blaine (class 39) -------------------------------------------- */
static const uint8_t gBlaineParties[] = {
    0xFF, 42, SPECIES_GROWLITHE, 40, SPECIES_PONYTA,
          42, SPECIES_RAPIDASH, 47, SPECIES_ARCANINE, 0,
};

/* ---- Sabrina (class 40) ------------------------------------------- */
static const uint8_t gSabrinaParties[] = {
    0xFF, 38, SPECIES_KADABRA, 37, SPECIES_MR_MIME,
          38, SPECIES_VENOMOTH, 43, SPECIES_ALAKAZAM, 0,
};

/* ---- Gentleman (class 41) ----------------------------------------- */
static const uint8_t gGentlemanParties[] = {
    /* SS Anne 1F Rooms */
    18, SPECIES_GROWLITHE, SPECIES_GROWLITHE, 0,
    19, SPECIES_NIDORAN_M, SPECIES_NIDORAN_F, 0,
    /* SS Anne 2F Rooms / Vermilion Gym */
    23, SPECIES_PIKACHU, 0,
    /* Unused */
    48, SPECIES_PRIMEAPE, 0,
    /* SS Anne 2F Rooms */
    17, SPECIES_GROWLITHE, SPECIES_PONYTA, 0,
};

/* ---- Rival 2 (class 42) ------------------------------------------- */
static const uint8_t gRival2Parties[] = {
    /* SS Anne 2F */
    0xFF, 19, SPECIES_PIDGEOTTO, 16, SPECIES_RATICATE,
          18, SPECIES_KADABRA, 20, SPECIES_WARTORTLE, 0,
    0xFF, 19, SPECIES_PIDGEOTTO, 16, SPECIES_RATICATE,
          18, SPECIES_KADABRA, 20, SPECIES_IVYSAUR, 0,
    0xFF, 19, SPECIES_PIDGEOTTO, 16, SPECIES_RATICATE,
          18, SPECIES_KADABRA, 20, SPECIES_CHARMELEON, 0,
    /* Pokemon Tower 2F */
    0xFF, 25, SPECIES_PIDGEOTTO, 23, SPECIES_GROWLITHE,
          22, SPECIES_EXEGGCUTE, 20, SPECIES_KADABRA, 25, SPECIES_WARTORTLE, 0,
    0xFF, 25, SPECIES_PIDGEOTTO, 23, SPECIES_GYARADOS,
          22, SPECIES_GROWLITHE, 20, SPECIES_KADABRA, 25, SPECIES_IVYSAUR, 0,
    0xFF, 25, SPECIES_PIDGEOTTO, 23, SPECIES_EXEGGCUTE,
          22, SPECIES_GYARADOS, 20, SPECIES_KADABRA, 25, SPECIES_CHARMELEON, 0,
    /* Silph Co. 7F */
    0xFF, 37, SPECIES_PIDGEOT, 38, SPECIES_GROWLITHE,
          35, SPECIES_EXEGGCUTE, 35, SPECIES_ALAKAZAM, 40, SPECIES_BLASTOISE, 0,
    0xFF, 37, SPECIES_PIDGEOT, 38, SPECIES_GYARADOS,
          35, SPECIES_GROWLITHE, 35, SPECIES_ALAKAZAM, 40, SPECIES_VENUSAUR, 0,
    0xFF, 37, SPECIES_PIDGEOT, 38, SPECIES_EXEGGCUTE,
          35, SPECIES_GYARADOS, 35, SPECIES_ALAKAZAM, 40, SPECIES_CHARIZARD, 0,
    /* Route 22 */
    0xFF, 47, SPECIES_PIDGEOT, 45, SPECIES_RHYHORN, 45, SPECIES_GROWLITHE,
          47, SPECIES_EXEGGCUTE, 50, SPECIES_ALAKAZAM, 53, SPECIES_BLASTOISE, 0,
    0xFF, 47, SPECIES_PIDGEOT, 45, SPECIES_RHYHORN, 45, SPECIES_GYARADOS,
          47, SPECIES_GROWLITHE, 50, SPECIES_ALAKAZAM, 53, SPECIES_VENUSAUR, 0,
    0xFF, 47, SPECIES_PIDGEOT, 45, SPECIES_RHYHORN, 45, SPECIES_EXEGGCUTE,
          47, SPECIES_GYARADOS, 50, SPECIES_ALAKAZAM, 53, SPECIES_CHARIZARD, 0,
};

/* ---- Rival 3 (class 43) ------------------------------------------- */
static const uint8_t gRival3Parties[] = {
    0xFF, 61, SPECIES_PIDGEOT, 59, SPECIES_ALAKAZAM, 61, SPECIES_RHYDON,
          61, SPECIES_ARCANINE, 63, SPECIES_EXEGGUTOR, 65, SPECIES_BLASTOISE, 0,
    0xFF, 61, SPECIES_PIDGEOT, 59, SPECIES_ALAKAZAM, 61, SPECIES_RHYDON,
          61, SPECIES_GYARADOS, 63, SPECIES_ARCANINE, 65, SPECIES_VENUSAUR, 0,
    0xFF, 61, SPECIES_PIDGEOT, 59, SPECIES_ALAKAZAM, 61, SPECIES_RHYDON,
          61, SPECIES_EXEGGUTOR, 63, SPECIES_GYARADOS, 65, SPECIES_CHARIZARD, 0,
};

/* ---- Lorelei (class 44) ------------------------------------------- */
static const uint8_t gLoreleiParties[] = {
    0xFF, 54, SPECIES_DEWGONG, 53, SPECIES_CLOYSTER, 54, SPECIES_SLOWBRO,
          56, SPECIES_JYNX, 56, SPECIES_LAPRAS, 0,
};

/* ---- Channeler (class 45) ----------------------------------------- */
static const uint8_t gChannelerParties[] = {
    /* Unused */
    22, SPECIES_GASTLY, 0,
    24, SPECIES_GASTLY, 0,
    23, SPECIES_GASTLY, SPECIES_GASTLY, 0,
    24, SPECIES_GASTLY, 0,
    /* Pokemon Tower 3F */
    23, SPECIES_GASTLY, 0,
    24, SPECIES_GASTLY, 0,
    /* Unused */
    24, SPECIES_HAUNTER, 0,
    /* Pokemon Tower 3F */
    22, SPECIES_GASTLY, 0,
    /* Pokemon Tower 4F */
    24, SPECIES_GASTLY, 0,
    23, SPECIES_GASTLY, SPECIES_GASTLY, 0,
    /* Unused */
    24, SPECIES_GASTLY, 0,
    /* Pokemon Tower 4F */
    22, SPECIES_GASTLY, 0,
    /* Unused */
    24, SPECIES_GASTLY, 0,
    /* Pokemon Tower 5F */
    23, SPECIES_HAUNTER, 0,
    /* Unused */
    24, SPECIES_GASTLY, 0,
    /* Pokemon Tower 5F */
    22, SPECIES_GASTLY, 0,
    24, SPECIES_GASTLY, 0,
    22, SPECIES_HAUNTER, 0,
    /* Pokemon Tower 6F */
    22, SPECIES_GASTLY, SPECIES_GASTLY, SPECIES_GASTLY, 0,
    24, SPECIES_GASTLY, 0,
    24, SPECIES_GASTLY, 0,
    /* Saffron Gym */
    34, SPECIES_GASTLY, SPECIES_HAUNTER, 0,
    38, SPECIES_HAUNTER, 0,
    33, SPECIES_GASTLY, SPECIES_GASTLY, SPECIES_HAUNTER, 0,
};

/* ---- Agatha (class 46) -------------------------------------------- */
static const uint8_t gAgathaParties[] = {
    0xFF, 56, SPECIES_GENGAR, 56, SPECIES_GOLBAT, 55, SPECIES_HAUNTER,
          58, SPECIES_ARBOK, 60, SPECIES_GENGAR, 0,
};

/* ---- Lance (class 47) --------------------------------------------- */
static const uint8_t gLanceParties[] = {
    0xFF, 58, SPECIES_GYARADOS, 56, SPECIES_DRAGONAIR, 56, SPECIES_DRAGONAIR,
          60, SPECIES_AERODACTYL, 62, SPECIES_DRAGONITE, 0,
};

/* ---- Pointer table ------------------------------------------------ */
/* Indexed by [trainer_class - 1] (0-based, classes 1..NUM_TRAINERS). */
const uint8_t * const gTrainerPartyData[NUM_TRAINERS] = {
    gYoungsterParties,      /* OPP_YOUNGSTER  = 1  */
    gBugCatcherParties,     /* OPP_BUG_CATCHER = 2  */
    gLassParties,           /* OPP_LASS       = 3  */
    gSailorParties,         /* OPP_SAILOR     = 4  */
    gJrTrainerMParties,     /* OPP_JR_TRAINER_M = 5 */
    gJrTrainerFParties,     /* OPP_JR_TRAINER_F = 6 */
    gPokemaniacParties,     /* OPP_POKEMANIAC = 7  */
    gSuperNerdParties,      /* OPP_SUPER_NERD = 8  */
    gHikerParties,          /* OPP_HIKER      = 9  */
    gBikerParties,          /* OPP_BIKER      = 10 */
    gBurglarParties,        /* OPP_BURGLAR    = 11 */
    gEngineerParties,       /* OPP_ENGINEER   = 12 */
    gUnusedJugglerParties,  /* OPP_JUGGLER_X  = 13 */
    gFisherParties,         /* OPP_FISHER     = 14 */
    gSwimmerParties,        /* OPP_SWIMMER    = 15 */
    gCueBallParties,        /* OPP_CUEIST     = 16 */
    gGamblerParties,        /* OPP_GAMER      = 17 */
    gBeautyParties,         /* OPP_BEAUTY     = 18 */
    gPsychicParties,        /* OPP_PSYCHIC_TR = 19 */
    gRockerParties,         /* OPP_ROCKER     = 20 */
    gJugglerParties,        /* OPP_JUGGLER    = 21 */
    gTamerParties,          /* OPP_TAMER      = 22 */
    gBirdKeeperParties,     /* OPP_BIRDKEEPER = 23 */
    gBlackbeltParties,      /* OPP_BLACKBELT  = 24 */
    gRival1Parties,         /* OPP_RIVAL1     = 25 */
    gProfOakParties,        /* OPP_PROF_OAK   = 26 */
    gChiefParties,          /* OPP_CHIEF      = 27 */
    gScientistParties,      /* OPP_SCIENTIST  = 28 */
    gGiovanniParties,       /* OPP_GIOVANNI   = 29 */
    gRocketParties,         /* OPP_ROCKET     = 30 */
    gCooltrainerMParties,   /* OPP_COOLTRAINER_M = 31 */
    gCooltrainerFParties,   /* OPP_COOLTRAINER_F = 32 */
    gBrunoParties,          /* OPP_BRUNO      = 33 */
    gBrockParties,          /* OPP_BROCK      = 34 */
    gMistyParties,          /* OPP_MISTY      = 35 */
    gLtSurgeParties,        /* OPP_LT_SURGE   = 36 */
    gErikaParties,          /* OPP_ERIKA      = 37 */
    gKogaParties,           /* OPP_KOGA       = 38 */
    gBlaineParties,         /* OPP_BLAINE     = 39 */
    gSabrinaParties,        /* OPP_SABRINA    = 40 */
    gGentlemanParties,      /* OPP_GENTLEMAN  = 41 */
    gRival2Parties,         /* OPP_RIVAL2     = 42 */
    gRival3Parties,         /* OPP_RIVAL3     = 43 */
    gLoreleiParties,        /* OPP_LORELEI    = 44 */
    gChannelerParties,      /* OPP_CHANNELER  = 45 */
    gAgathaParties,         /* OPP_AGATHA     = 46 */
    gLanceParties,          /* OPP_LANCE      = 47 */
};

/* Base money reward — from data/trainers/pic_pointers_money.asm.
 * Prize = base_money * sum(level_i) across all enemy mons. */
const uint16_t gTrainerBaseMoney[NUM_TRAINERS] = {
    1500, /* YOUNGSTER   */
    1000, /* BUG_CATCHER */
    1500, /* LASS        */
    3000, /* SAILOR      */
    2000, /* JR_TRAINER_M */
    2000, /* JR_TRAINER_F */
    5000, /* POKEMANIAC  */
    2500, /* SUPER_NERD  */
    3500, /* HIKER       */
    2000, /* BIKER       */
    9000, /* BURGLAR     */
    5000, /* ENGINEER    */
    3500, /* JUGGLER (1) */
    3500, /* FISHER      */
     500, /* SWIMMER     */
    2500, /* CUE_BALL    */
    7000, /* GAMBLER     */
    7000, /* BEAUTY      */
    1000, /* PSYCHIC     */
    2500, /* ROCKER      */
    3500, /* JUGGLER (2) */
    4000, /* TAMER       */
    2500, /* BIRD_KEEPER */
    2500, /* BLACKBELT   */
    3500, /* RIVAL1      */
    9900, /* PROF_OAK    */
    3000, /* CHIEF       */
    5000, /* SCIENTIST   */
    9900, /* GIOVANNI    */
    3000, /* ROCKET      */
    3500, /* COOLTRAINER_M */
    3500, /* COOLTRAINER_F */
    9900, /* BRUNO       */
    9900, /* BROCK       */
    9900, /* MISTY       */
    9900, /* LT_SURGE    */
    9900, /* ERIKA       */
    9900, /* KOGA        */
    9900, /* BLAINE      */
    9900, /* SABRINA     */
    7000, /* GENTLEMAN   */
    6500, /* RIVAL2      */
    9900, /* RIVAL3      */
    9900, /* LORELEI     */
    3000, /* CHANNELER   */
    9900, /* AGATHA      */
    9900, /* LANCE       */
};
