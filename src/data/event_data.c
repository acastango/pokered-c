/* event_data.c -- Generated from pokered-master map object files. */
#include "event_data.h"
#include "../game/pokecenter.h"
#include "../game/pokemart.h"

static const map_warp_t kWarps_PalletTown[] = {
    {  10,  11, 0x25, 0 },  /* REDS_HOUSE_1F */
    {  26,  11, 0x27, 0 },  /* BLUES_HOUSE */
    {  24,  23, 0x28, 1 },  /* OAKS_LAB */
};

static const npc_event_t kNpcs_PalletTown[] = {
    {  16,  11, 0x03, 0, "OAK: Hey! Wait!\nDon't go out!@", NULL },  /* SPRITE_OAK, STAY, TEXT_PALLETTOWN_OAK */
    {   6,  17, 0x0d, 1, "I'm raising\nPOKEMON too!\fWhen they get\nstrong, they can\nprotect me!", NULL },  /* SPRITE_GIRL, WALK, TEXT_PALLETTOWN_GIRL */
    {  22,  29, 0x2f, 1, "Technology is\nincredible!\fYou can now store\nand recall items\nand POKEMON as\ndata via PC!", NULL },  /* SPRITE_FISHER, WALK, TEXT_PALLETTOWN_FISHER */
};

static const sign_event_t kSigns_PalletTown[] = {
    {  26,  27, "OAK POKEMON\nRESEARCH LAB" },  /* TEXT_PALLETTOWN_OAKSLAB_SIGN */
    {  14,  19, "PALLET TOWN\nShades of your\njourney await!" },  /* TEXT_PALLETTOWN_SIGN */
    {   6,  11, "{PLAYER}'s house " },  /* TEXT_PALLETTOWN_PLAYERSHOUSE_SIGN */
    {  22,  11, "{RIVAL}'s house " },  /* TEXT_PALLETTOWN_RIVALSHOUSE_SIGN */
};

static const map_warp_t kWarps_ViridianCity[] = {
    {  46,  51, 0x29, 0 },  /* VIRIDIAN_POKECENTER */
    {  58,  39, 0x2a, 0 },  /* VIRIDIAN_MART */
    {  42,  31, 0x2b, 0 },  /* VIRIDIAN_SCHOOL_HOUSE */
    {  42,  19, 0x2c, 0 },  /* VIRIDIAN_NICKNAME_HOUSE */
    {  64,  15, 0x2d, 0 },  /* VIRIDIAN_GYM */
};

static const npc_event_t kNpcs_ViridianCity[] = {
    {  26,  41, 0x04, 1, "Those POKE BALLs\nat your waist!\nYou have POKEMON!\fIt's great that\nyou can carry and\nuse POKEMON any\ntime, anywhere!", NULL },  /* SPRITE_YOUNGSTER, WALK, TEXT_VIRIDIANCITY_YOUNGSTER1 */
    {  60,  17, 0x0b, 0, "This POKEMON GYM\nis always closed.\fI wonder who the\nLEADER is?", NULL },  /* SPRITE_GAMBLER, STAY, TEXT_VIRIDIANCITY_GAMBLER1 */
    {  60,  51, 0x04, 1, "You want to know\nabout the 2 kinds\nof caterpillar\nPOKEMON?", NULL },  /* SPRITE_YOUNGSTER, WALK, TEXT_VIRIDIANCITY_YOUNGSTER2 */
    {  34,  19, 0x0d, 0, "Oh Grandpa! Don't\nbe so mean!\nHe hasn't had his\ncoffee yet.", NULL },  /* SPRITE_GIRL, STAY, TEXT_VIRIDIANCITY_GIRL */
    {  36,  19, 0x48, 0, "You can't go\nthrough here!\fThis is private\nproperty!", NULL },  /* SPRITE_GAMBLER_ASLEEP, STAY, TEXT_VIRIDIANCITY_OLD_MAN_SLEEPY */
    {  12,  47, 0x2f, 0, "{PLAYER} received\nTM42!@", NULL },  /* SPRITE_FISHER, STAY, TEXT_VIRIDIANCITY_FISHER */
    {  34,  11, 0x0b, 1, "Ahh, I've had my\ncoffee now and I\nfeel great!\fSure you can go\nthrough!\fAre you in a\nhurry?", NULL },  /* SPRITE_GAMBLER, WALK, TEXT_VIRIDIANCITY_OLD_MAN */
};

static const sign_event_t kSigns_ViridianCity[] = {
    {  34,  35, "VIRIDIAN CITY \nThe Eternally\nGreen Paradise" },  /* TEXT_VIRIDIANCITY_SIGN */
    {  38,   3, "TRAINER TIPS\fCatch POKEMON\nand expand your\ncollection!\fThe more you have,\nthe easier it is\nto fight!" },  /* TEXT_VIRIDIANCITY_TRAINER_TIPS1 */
    {  42,  59, "TRAINER TIPS\fThe battle moves\nof POKEMON are\nlimited by their\nPOWER POINTs, PP.\fTo replenish PP,\nrest your tired\nPOKEMON at a\nPOKEMON CENTER!" },  /* TEXT_VIRIDIANCITY_TRAINER_TIPS2 */
    {  60,  39, NULL },  /* TEXT_VIRIDIANCITY_MART_SIGN */
    {  48,  51, NULL },  /* TEXT_VIRIDIANCITY_POKECENTER_SIGN */
    {  54,  15, "VIRIDIAN CITY\nPOKEMON GYM" },  /* TEXT_VIRIDIANCITY_GYM_SIGN */
};

static const map_warp_t kWarps_PewterCity[] = {
    {  28,  15, 0x34, 0 },  /* MUSEUM_1F */
    {  38,  11, 0x34, 2 },  /* MUSEUM_1F */
    {  32,  35, 0x36, 0 },  /* PEWTER_GYM */
    {  58,  27, 0x37, 0 },  /* PEWTER_NIDORAN_HOUSE */
    {  46,  35, 0x38, 0 },  /* PEWTER_MART */
    {  14,  59, 0x39, 0 },  /* PEWTER_SPEECH_HOUSE */
    {  26,  51, 0x3a, 0 },  /* PEWTER_POKECENTER */
};

static const npc_event_t kNpcs_PewterCity[] = {
    {  16,  31, 0x06, 0, "It's rumored that\nCLEFAIRYs came\nfrom the moon!\fThey appeared \nafter MOON STONE\nfell on MT.MOON.", NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_PEWTERCITY_COOLTRAINER_F */
    {  34,  51, 0x07, 0, "There aren't many\nserious POKEMON\ntrainers here!\fThey're all like\nBUG CATCHERs,\nbut PEWTER GYM's\nBROCK is totally\ninto it!", NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_PEWTERCITY_COOLTRAINER_M */
    {  54,  35, 0x0c, 0, "Did you check out\nthe MUSEUM?", NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_PEWTERCITY_SUPER_NERD1 */
    {  52,  51, 0x0c, 1, "Psssst!\nDo you know what\nI'm doing?", NULL },  /* SPRITE_SUPER_NERD, WALK, TEXT_PEWTERCITY_SUPER_NERD2 */
    {  70,  33, 0x04, 0, "You're a trainer\nright? BROCK's\nlooking for new\nchallengers!\nFollow me!", NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_PEWTERCITY_YOUNGSTER */
};

static const sign_event_t kSigns_PewterCity[] = {
    {  38,  59, "TRAINER TIPS\fAny POKEMON that\ntakes part in\nbattle, however\nshort, earns EXP!" },  /* TEXT_PEWTERCITY_TRAINER_TIPS */
    {  66,  39, "NOTICE!\fThieves have been\nstealing POKEMON\nfossils at MT.\nMOON! Please call\nPEWTER POLICE\nwith any info!" },  /* TEXT_PEWTERCITY_POLICE_NOTICE_SIGN */
    {  48,  35, NULL },  /* TEXT_PEWTERCITY_MART_SIGN */
    {  28,  51, NULL },  /* TEXT_PEWTERCITY_POKECENTER_SIGN */
    {  30,  19, "PEWTER MUSEUM\nOF SCIENCE" },  /* TEXT_PEWTERCITY_MUSEUM_SIGN */
    {  22,  35, "PEWTER CITY\nPOKEMON GYM\nLEADER: BROCK\fThe Rock Solid\nPOKEMON Trainer!" },  /* TEXT_PEWTERCITY_GYM_SIGN */
    {  50,  47, "PEWTER CITY\nA Stone Gray\nCity" },  /* TEXT_PEWTERCITY_SIGN */
};

static const map_warp_t kWarps_CeruleanCity[] = {
    {  54,  23, 0x3e, 0 },  /* CERULEAN_TRASHED_HOUSE */
    {  26,  31, 0x3f, 0 },  /* CERULEAN_TRADE_HOUSE */
    {  38,  35, 0x40, 0 },  /* CERULEAN_POKECENTER */
    {  60,  39, 0x41, 0 },  /* CERULEAN_GYM */
    {  26,  51, 0x42, 0 },  /* BIKE_SHOP */
    {  50,  51, 0x43, 0 },  /* CERULEAN_MART */
    {   8,  23, 0xe4, 0 },  /* CERULEAN_CAVE_1F */
    {  54,  19, 0x3e, 2 },  /* CERULEAN_TRASHED_HOUSE */
    {  18,  23, 0xe6, 1 },  /* CERULEAN_BADGE_HOUSE */
    {  18,  19, 0xe6, 0 },  /* CERULEAN_BADGE_HOUSE */
};

static const npc_event_t kNpcs_CeruleanCity[] = {
    {  40,   5, 0x02, 0, "{RIVAL}: Yo!\n{PLAYER}!\fYou're still\nstruggling along\nback here?\fI'm doing great!\nI caught a bunch\nof strong and\nsmart POKEMON!\fHere, let me see\nwhat you caught,\n{PLAYER}!", NULL },  /* SPRITE_BLUE, STAY, TEXT_CERULEANCITY_RIVAL */
    {  60,  17, 0x18, 0, "Hey! Stay out!\nIt's not your\nyard! Huh? Me?\fI'm an innocent\nbystander! Don't\nyou believe me?", NULL },  /* SPRITE_ROCKET, STAY, TEXT_CERULEANCITY_ROCKET */
    {  62,  41, 0x07, 0, "You're a trainer\ntoo? Collecting,\nfighting, it's a\ntough life.", NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_CERULEANCITY_COOLTRAINER_M */
    {  30,  37, 0x0c, 1, "That bush in\nfront of the shop\nis in the way.\fThere might be a\nway around.", NULL },  /* SPRITE_SUPER_NERD, WALK, TEXT_CERULEANCITY_SUPER_NERD1 */
    {  18,  43, 0x0c, 1, "You're making an\nencyclopedia on\nPOKEMON? That\nsounds amusing.", NULL },  /* SPRITE_SUPER_NERD, WALK, TEXT_CERULEANCITY_SUPER_NERD2 */
    {  56,  25, 0x31, 0, "The people here\nwere robbed.\fIt's obvious that\nTEAM ROCKET is\nbehind this most\nheinous crime!\fEven our POLICE\nforce has trouble\nwith the ROCKETs!", NULL },  /* SPRITE_GUARD, STAY, TEXT_CERULEANCITY_GUARD1 */
    {  58,  53, 0x06, 0, "OK! SLOWBRO!\nUse SONICBOOM!\nCome on, SLOWBRO\npay attention!", NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_CERULEANCITY_COOLTRAINER_F1 */
    {  56,  53, 0x05, 0, "SLOWBRO took a\nsnooze...", NULL },  /* SPRITE_MONSTER, STAY, TEXT_CERULEANCITY_SLOWBRO */
    {  18,  55, 0x06, 1, "I want a bright\nred BICYCLE!\fI'll keep it at\nhome, so it won't\nget dirty!", NULL },  /* SPRITE_COOLTRAINER_F, WALK, TEXT_CERULEANCITY_COOLTRAINER_F2 */
    {   8,  25, 0x0c, 0, "This is CERULEAN\nCAVE! Horribly\nstrong POKEMON\nlive in there!\fThe POKEMON LEAGUE\nchampion is the\nonly person who\nis allowed in!", NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_CERULEANCITY_SUPER_NERD3 */
    {  54,  25, 0x31, 0, "The people here\nwere robbed.\fIt's obvious that\nTEAM ROCKET is\nbehind this most\nheinous crime!\fEven our POLICE\nforce has trouble\nwith the ROCKETs!", NULL },  /* SPRITE_GUARD, STAY, TEXT_CERULEANCITY_GUARD2 */
};

static const sign_event_t kSigns_CeruleanCity[] = {
    {  46,  39, "CERULEAN CITY\nA Mysterious,\nBlue Aura\nSurrounds It" },  /* TEXT_CERULEANCITY_SIGN */
    {  34,  59, "TRAINER TIPS\fPressing B Button\nduring evolution\ncancels the whole\nprocess." },  /* TEXT_CERULEANCITY_TRAINER_TIPS */
    {  52,  51, NULL },  /* TEXT_CERULEANCITY_MART_SIGN */
    {  40,  35, NULL },  /* TEXT_CERULEANCITY_POKECENTER_SIGN */
    {  22,  51, "Grass and caves\nhandled easily!\nBIKE SHOP" },  /* TEXT_CERULEANCITY_BIKESHOP_SIGN */
    {  54,  43, "CERULEAN CITY\nPOKEMON GYM\nLEADER: MISTY\fThe Tomboyish\nMermaid!" },  /* TEXT_CERULEANCITY_GYM_SIGN */
};

static const map_warp_t kWarps_LavenderTown[] = {
    {   6,  11, 0x8d, 0 },  /* LAVENDER_POKECENTER */
    {  28,  11, 0x8e, 0 },  /* POKEMON_TOWER_1F */
    {  14,  19, 0x95, 0 },  /* MR_FUJIS_HOUSE */
    {  30,  27, 0x96, 0 },  /* LAVENDER_MART */
    {   6,  27, 0x97, 0 },  /* LAVENDER_CUBONE_HOUSE */
    {  14,  27, 0xe5, 0 },  /* NAME_RATERS_HOUSE */
};

static const npc_event_t kNpcs_LavenderTown[] = {
    {  30,  19, 0x08, 1, "Do you believe in\nGHOSTs?", NULL },  /* SPRITE_LITTLE_GIRL, WALK, TEXT_LAVENDERTOWN_LITTLE_GIRL */
    {  18,  21, 0x07, 0, "This town is known\nas the grave site\nof POKEMON.\fMemorial services\nare held in\nPOKEMON TOWER.", NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_LAVENDERTOWN_COOLTRAINER_M */
    {  16,  15, 0x0c, 1, "GHOSTs appeared\nin POKEMON TOWER.\fI think they're\nthe spirits of\nPOKEMON that the\nROCKETs killed.", NULL },  /* SPRITE_SUPER_NERD, WALK, TEXT_LAVENDERTOWN_SUPER_NERD */
};

static const sign_event_t kSigns_LavenderTown[] = {
    {  22,  19, "LAVENDER TOWN\nThe Noble Purple\nTown" },  /* TEXT_LAVENDERTOWN_SIGN */
    {  18,   7, "New SILPH SCOPE!\fMake the Invisible\nPlain to See!\fSILPH CO." },  /* TEXT_LAVENDERTOWN_SILPH_SCOPE_SIGN */
    {  32,  27, NULL },  /* TEXT_LAVENDERTOWN_MART_SIGN */
    {   8,  11, NULL },  /* TEXT_LAVENDERTOWN_POKECENTER_SIGN */
    {  10,  19, "LAVENDER VOLUNTEER\nPOKEMON HOUSE" },  /* TEXT_LAVENDERTOWN_POKEMON_HOUSE_SIGN */
    {  34,  15, "May the Souls of\nPOKEMON Rest Easy\nPOKEMON TOWER" },  /* TEXT_LAVENDERTOWN_POKEMON_TOWER_SIGN */
};

static const map_warp_t kWarps_VermilionCity[] = {
    {  22,   7, 0x59, 0 },  /* VERMILION_POKECENTER */
    {  18,  27, 0x5a, 0 },  /* POKEMON_FAN_CLUB */
    {  46,  27, 0x5b, 0 },  /* VERMILION_MART */
    {  24,  39, 0x5c, 0 },  /* VERMILION_GYM */
    {  46,  39, 0x5d, 0 },  /* VERMILION_PIDGEY_HOUSE */
    {  36,  63, 0x5e, 0 },  /* VERMILION_DOCK */
    {  38,  63, 0x5e, 0 },  /* VERMILION_DOCK */
    {  30,  27, 0xc4, 0 },  /* VERMILION_TRADE_HOUSE */
    {  14,   7, 0xa3, 0 },  /* VERMILION_OLD_ROD_HOUSE */
};

static const npc_event_t kNpcs_VermilionCity[] = {
    {  38,  15, 0x0f, 1, "We're careful\nabout pollution!\fWe've heard GRIMER\nmultiplies in\ntoxic sludge!", NULL },  /* SPRITE_BEAUTY, WALK, TEXT_VERMILIONCITY_BEAUTY */
    {  28,  13, 0x0b, 0, "Did you see S.S.\nANNE moored in\nthe harbor?", NULL },  /* SPRITE_GAMBLER, STAY, TEXT_VERMILIONCITY_GAMBLER1 */
    {  38,  61, 0x13, 0, "Welcome to S.S.\nANNE!", NULL },  /* SPRITE_SAILOR, STAY, TEXT_VERMILIONCITY_SAILOR1 */
    {  60,  15, 0x0b, 0, "I'm putting up a\nbuilding on this\nplot of land.\fMy POKEMON is\ntamping the land.", NULL },  /* SPRITE_GAMBLER, STAY, TEXT_VERMILIONCITY_GAMBLER2 */
    {  58,  19, 0x05, 1, "MACHOP: Guoh!\nGogogoh!@", NULL },  /* SPRITE_MONSTER, WALK, TEXT_VERMILIONCITY_MACHOP */
    {  50,  55, 0x13, 1, "S.S.ANNE is a\nfamous luxury\ncruise ship.\fWe visit VERMILION\nonce a year.", NULL },  /* SPRITE_SAILOR, WALK, TEXT_VERMILIONCITY_SAILOR2 */
};

static const sign_event_t kSigns_VermilionCity[] = {
    {  54,   7, "VERMILION CITY\nThe Port of\nExquisite Sunsets" },  /* TEXT_VERMILIONCITY_SIGN */
    {  74,  27, "NOTICE!\fROUTE 12 may be\nblocked off by a\nsleeping POKEMON.\fDetour through\nROCK TUNNEL to\nLAVENDER TOWN.\fVERMILION POLICE" },  /* TEXT_VERMILIONCITY_NOTICE_SIGN */
    {  48,  27, NULL },  /* TEXT_VERMILIONCITY_MART_SIGN */
    {  24,   7, NULL },  /* TEXT_VERMILIONCITY_POKECENTER_SIGN */
    {  14,  27, "POKEMON FAN CLUB\nAll POKEMON fans\nwelcome!" },  /* TEXT_VERMILIONCITY_POKEMON_FAN_CLUB_SIGN */
    {  14,  39, "VERMILION CITY\nPOKEMON GYM\nLEADER: LT.SURGE\fThe Lightning \nAmerican!" },  /* TEXT_VERMILIONCITY_GYM_SIGN */
    {  58,  31, "VERMILION HARBOR" },  /* TEXT_VERMILIONCITY_HARBOR_SIGN */
};

static const map_warp_t kWarps_CeladonCity[] = {
    {  16,  27, 0x7a, 0 },  /* CELADON_MART_1F */
    {  20,  27, 0x7a, 2 },  /* CELADON_MART_1F */
    {  48,  19, 0x80, 0 },  /* CELADON_MANSION_1F */
    {  48,   7, 0x80, 2 },  /* CELADON_MANSION_1F */
    {  50,   7, 0x80, 2 },  /* CELADON_MANSION_1F */
    {  82,  19, 0x85, 0 },  /* CELADON_POKECENTER */
    {  24,  55, 0x86, 0 },  /* CELADON_GYM */
    {  56,  39, 0x87, 0 },  /* GAME_CORNER */
    {  78,  39, 0x88, 0 },  /* CELADON_MART_5F */
    {  66,  39, 0x89, 0 },  /* GAME_CORNER_PRIZE_ROOM */
    {  62,  55, 0x8a, 0 },  /* CELADON_DINER */
    {  70,  55, 0x8b, 0 },  /* CELADON_CHIEF_HOUSE */
    {  86,  55, 0x8c, 0 },  /* CELADON_HOTEL */
};

static const npc_event_t kNpcs_CeladonCity[] = {
    {  16,  35, 0x08, 1, "I got my KOFFING\nin CINNABAR!\fIt's nice, but it\nbreathes poison\nwhen it's angry!", NULL },  /* SPRITE_LITTLE_GIRL, WALK, TEXT_CELADONCITY_LITTLE_GIRL */
    {  22,  57, 0x25, 0, "Heheh! This GYM\nis great! It's\nfull of women!", NULL },  /* SPRITE_GRAMPS, STAY, TEXT_CELADONCITY_GRAMPS1 */
    {  28,  39, 0x0d, 1, "The GAME CORNER\nis bad for our\ncity's image!", NULL },  /* SPRITE_GIRL, WALK, TEXT_CELADONCITY_GIRL */
    {  50,  45, 0x25, 0, "Moan! I blew it\nall at the slots!\fI knew I should\nhave cashed in my\ncoins for prizes!", NULL },  /* SPRITE_GRAMPS, STAY, TEXT_CELADONCITY_GRAMPS2 */
    {  44,  33, 0x25, 0, "Hello, there!\fI've seen you,\nbut I never had a\nchance to talk!\fHere's a gift for\ndropping by!", NULL },  /* SPRITE_GRAMPS, STAY, TEXT_CELADONCITY_GRAMPS3 */
    {  64,  25, 0x2f, 0, "This is my trusted\npal, POLIWRATH!\fIt evolved from\nPOLIWHIRL when I\nused WATER STONE!", NULL },  /* SPRITE_FISHER, STAY, TEXT_CELADONCITY_FISHER */
    {  60,  25, 0x05, 0, "POLIWRATH: Ribi\nribit!@", NULL },  /* SPRITE_MONSTER, STAY, TEXT_CELADONCITY_POLIWRATH */
    {  64,  59, 0x18, 1, "What are you\nstaring at?", NULL },  /* SPRITE_ROCKET, WALK, TEXT_CELADONCITY_ROCKET1 */
    {  84,  29, 0x18, 1, "Keep out of TEAM\nROCKET's way!", NULL },  /* SPRITE_ROCKET, WALK, TEXT_CELADONCITY_ROCKET2 */
};

static const sign_event_t kSigns_CeladonCity[] = {
    {  54,  31, "TRAINER TIPS\fX ACCURACY boosts\nthe accuracy of\ntechniques!\fDIRE HIT jacks up\nthe likelihood of\ncritical hits!\fGet your items at\nCELADON DEPT.\nSTORE!" },  /* TEXT_CELADONCITY_TRAINER_TIPS1 */
    {  38,  31, "CELADON CITY\nThe City of\nRainbow Dreams" },  /* TEXT_CELADONCITY_SIGN */
    {  84,  19, NULL },  /* TEXT_CELADONCITY_POKECENTER_SIGN */
    {  26,  59, "CELADON CITY\nPOKEMON GYM\nLEADER: ERIKA\fThe Nature Loving\nPrincess!" },  /* TEXT_CELADONCITY_GYM_SIGN */
    {  42,  19, "CELADON MANSION" },  /* TEXT_CELADONCITY_MANSION_SIGN */
    {  24,  27, "Find what you\nneed at CELADON\nDEPT. STORE!" },  /* TEXT_CELADONCITY_DEPTSTORE_SIGN */
    {  78,  43, "TRAINER TIPS\fGUARD SPEC.\nprotects POKEMON\nagainst SPECIAL\nattacks such as\nfire and water!\fGet your items at\nCELADON DEPT.\nSTORE!" },  /* TEXT_CELADONCITY_TRAINER_TIPS2 */
    {  66,  43, "Coins exchanged\nfor prizes!\nPRIZE EXCHANGE" },  /* TEXT_CELADONCITY_PRIZEEXCHANGE_SIGN */
    {  54,  43, "ROCKET GAME CORNER\nThe playground\nfor grown-ups!" },  /* TEXT_CELADONCITY_GAMECORNER_SIGN */
};

static const map_warp_t kWarps_FuchsiaCity[] = {
    {  10,  27, 0x98, 0 },  /* FUCHSIA_MART */
    {  22,  55, 0x99, 0 },  /* FUCHSIA_BILLS_GRANDPAS_HOUSE */
    {  38,  55, 0x9a, 0 },  /* FUCHSIA_POKECENTER */
    {  54,  55, 0x9b, 0 },  /* WARDENS_HOUSE */
    {  36,   7, 0x9c, 0 },  /* SAFARI_ZONE_GATE */
    {  10,  55, 0x9d, 0 },  /* FUCHSIA_GYM */
    {  44,  27, 0x9e, 0 },  /* FUCHSIA_MEETING_ROOM */
    {  62,  55, 0xa4, 1 },  /* FUCHSIA_GOOD_ROD_HOUSE */
    {  62,  49, 0xa4, 0 },  /* FUCHSIA_GOOD_ROD_HOUSE */
};

static const npc_event_t kNpcs_FuchsiaCity[] = {
    {  20,  25, 0x04, 1, "Did you try the\nSAFARI GAME? Some\nPOKEMON can only\nbe caught there.", NULL },  /* SPRITE_YOUNGSTER, WALK, TEXT_FUCHSIACITY_YOUNGSTER1 */
    {  56,  35, 0x0b, 1, "SAFARI ZONE has a\nzoo in front of\nthe entrance.\fOut back is the\nSAFARI GAME for\ncatching POKEMON.", NULL },  /* SPRITE_GAMBLER, WALK, TEXT_FUCHSIACITY_GAMBLER */
    {  60,  29, 0x2f, 0, "ERIK: Where's\nSARA? I said I'd\nmeet her here.", NULL },  /* SPRITE_FISHER, STAY, TEXT_FUCHSIACITY_ERIK */
    {  48,  17, 0x04, 0, "That item ball in\nthere is really a\nPOKEMON.", NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_FUCHSIACITY_YOUNGSTER2 */
    {  62,  11, 0x38, 1, "!", NULL },  /* SPRITE_FAIRY, WALK, TEXT_FUCHSIACITY_CHANSEY */
    {  24,  13, 0x05, 1, "!", NULL },  /* SPRITE_MONSTER, WALK, TEXT_FUCHSIACITY_KANGASKHAN */
    {  60,  25, 0x05, 1, "!", NULL },  /* SPRITE_MONSTER, WALK, TEXT_FUCHSIACITY_SLOWPOKE */
    {  16,  35, 0x3c, 1, "!", NULL },  /* SPRITE_SEEL, WALK, TEXT_FUCHSIACITY_LAPRAS */
    {  12,  11, 0x3e, 0, "!", NULL },  /* SPRITE_FOSSIL, STAY, TEXT_FUCHSIACITY_FOSSIL */
};

static const sign_event_t kSigns_FuchsiaCity[] = {
    {  30,  47, "FUCHSIA CITY\nBehold! It's\nPassion Pink!" },  /* TEXT_FUCHSIACITY_SIGN1 */
    {  50,  31, "FUCHSIA CITY\nBehold! It's\nPassion Pink!" },  /* TEXT_FUCHSIACITY_SIGN2 */
    {  34,  11, "SAFARI GAME\nPOKEMON-U-CATCH!" },  /* TEXT_FUCHSIACITY_SAFARI_GAME_SIGN */
    {  12,  27, NULL },  /* TEXT_FUCHSIACITY_MART_SIGN */
    {  40,  55, NULL },  /* TEXT_FUCHSIACITY_POKECENTER_SIGN */
    {  54,  59, "SAFARI ZONE\nWARDEN's HOME" },  /* TEXT_FUCHSIACITY_WARDENS_HOME_SIGN */
    {  42,  31, "POKEMON PARADISE\nSAFARI ZONE" },  /* TEXT_FUCHSIACITY_SAFARI_ZONE_SIGN */
    {  10,  59, "FUCHSIA CITY\nPOKEMON GYM\nLEADER: KOGA\fThe Poisonous\nNinja Master" },  /* TEXT_FUCHSIACITY_GYM_SIGN */
    {  66,  15, "Name: CHANSEY\fCatching one is\nall up to chance." },  /* TEXT_FUCHSIACITY_CHANSEY_SIGN */
    {  54,  15, "Name: VOLTORB\fThe very image of\na POKE BALL." },  /* TEXT_FUCHSIACITY_VOLTORB_SIGN */
    {  26,  15, "Name: KANGASKHAN\fA maternal POKEMON\nthat raises its\nyoung in a pouch\non its belly." },  /* TEXT_FUCHSIACITY_KANGASKHAN_SIGN */
    {  62,  27, "Name: SLOWPOKE\fFriendly and very\nslow moving." },  /* TEXT_FUCHSIACITY_SLOWPOKE_SIGN */
    {  26,  31, "Name: LAPRAS\fA.K.A. the king\nof the seas." },  /* TEXT_FUCHSIACITY_LAPRAS_SIGN */
    {  14,  15, "Name: OMANYTE\fA POKEMON that\nwas resurrected\nfrom a fossil." },  /* TEXT_FUCHSIACITY_FOSSIL_SIGN */
};

static const map_warp_t kWarps_CinnabarIsland[] = {
    {  12,   7, 0xa5, 1 },  /* POKEMON_MANSION_1F */
    {  36,   7, 0xa6, 0 },  /* CINNABAR_GYM */
    {  12,  19, 0xa7, 0 },  /* CINNABAR_LAB */
    {  22,  23, 0xab, 0 },  /* CINNABAR_POKECENTER */
    {  30,  23, 0xac, 0 },  /* CINNABAR_MART */
};

static const npc_event_t kNpcs_CinnabarIsland[] = {
    {  24,  11, 0x0d, 1, "CINNABAR GYM's\nBLAINE is an odd\nman who has lived\nhere for decades.", NULL },  /* SPRITE_GIRL, WALK, TEXT_CINNABARISLAND_GIRL */
    {  28,  13, 0x0b, 0, "Scientists conduct\nexperiments in\nthe burned out\nbuilding.", NULL },  /* SPRITE_GAMBLER, STAY, TEXT_CINNABARISLAND_GAMBLER */
};

static const sign_event_t kSigns_CinnabarIsland[] = {
    {  18,  11, "CINNABAR ISLAND\nThe Fiery Town of\nBurning Desire" },  /* TEXT_CINNABARISLAND_SIGN */
    {  32,  23, NULL },  /* TEXT_CINNABARISLAND_MART_SIGN */
    {  24,  23, NULL },  /* TEXT_CINNABARISLAND_POKECENTER_SIGN */
    {  18,  23, "POKEMON LAB" },  /* TEXT_CINNABARISLAND_POKEMONLAB_SIGN */
    {  26,   7, "CINNABAR ISLAND\nPOKEMON GYM\nLEADER: BLAINE\fThe Hot-Headed\nQuiz Master!" },  /* TEXT_CINNABARISLAND_GYM_SIGN */
};

static const map_warp_t kWarps_IndigoPlateau[] = {
    {  18,  11, 0xae, 0 },  /* INDIGO_PLATEAU_LOBBY */
    {  20,  11, 0xae, 0 },  /* INDIGO_PLATEAU_LOBBY */
};

static const map_warp_t kWarps_SaffronCity[] = {
    {  14,  11, 0xaf, 0 },  /* COPYCATS_HOUSE_1F */
    {  52,   7, 0xb1, 0 },  /* FIGHTING_DOJO */
    {  68,   7, 0xb2, 0 },  /* SAFFRON_GYM */
    {  26,  23, 0xb3, 0 },  /* SAFFRON_PIDGEY_HOUSE */
    {  50,  23, 0xb4, 0 },  /* SAFFRON_MART */
    {  36,  43, 0xb5, 0 },  /* SILPH_CO_1F */
    {  18,  59, 0xb6, 0 },  /* SAFFRON_POKECENTER */
    {  58,  59, 0xb7, 0 },  /* MR_PSYCHICS_HOUSE */
};

static const npc_event_t kNpcs_SaffronCity[] = {
    {  14,  13, 0x18, 0, "What do you want?\nGet lost!", NULL },  /* SPRITE_ROCKET, STAY, TEXT_SAFFRONCITY_ROCKET1 */
    {  40,  17, 0x18, 1, "BOSS said he'll\ntake this town!", NULL },  /* SPRITE_ROCKET, WALK, TEXT_SAFFRONCITY_ROCKET2 */
    {  68,   9, 0x18, 0, "Get out of the\nway!", NULL },  /* SPRITE_ROCKET, STAY, TEXT_SAFFRONCITY_ROCKET3 */
    {  26,  25, 0x18, 0, "SAFFRON belongs\nto TEAM ROCKET!", NULL },  /* SPRITE_ROCKET, STAY, TEXT_SAFFRONCITY_ROCKET4 */
    {  22,  51, 0x18, 1, "Being evil makes\nme feel so alive!", NULL },  /* SPRITE_ROCKET, WALK, TEXT_SAFFRONCITY_ROCKET5 */
    {  64,  27, 0x18, 1, "Ow! Watch where\nyou're walking!", NULL },  /* SPRITE_ROCKET, WALK, TEXT_SAFFRONCITY_ROCKET6 */
    {  36,  61, 0x18, 1, "With SILPH under\ncontrol, we can\nexploit POKEMON\naround the world!", NULL },  /* SPRITE_ROCKET, WALK, TEXT_SAFFRONCITY_ROCKET7 */
    {  16,  29, 0x20, 1, "You beat TEAM\nROCKET all alone?\nThat's amazing!", NULL },  /* SPRITE_SCIENTIST, WALK, TEXT_SAFFRONCITY_SCIENTIST */
    {  46,  47, 0x2c, 0, "Yeah! TEAM ROCKET\nis gone!\nIt's safe to go\nout again!", NULL },  /* SPRITE_SILPH_WORKER_M, STAY, TEXT_SAFFRONCITY_SILPH_WORKER_M */
    {  34,  61, 0x1b, 1, "People should be\nflocking back to\nSAFFRON now.", NULL },  /* SPRITE_SILPH_WORKER_F, WALK, TEXT_SAFFRONCITY_SILPH_WORKER_F */
    {  60,  25, 0x10, 0, "I flew here on my\nPIDGEOT when I\nread about SILPH.\fIt's already over?\nI missed the\nmedia action.", NULL },  /* SPRITE_GENTLEMAN, STAY, TEXT_SAFFRONCITY_GENTLEMAN */
    {  62,  25, 0x09, 0, "PIDGEOT: Bi bibii!@", NULL },  /* SPRITE_BIRD, STAY, TEXT_SAFFRONCITY_PIDGEOT */
    {  36,  17, 0x21, 0, "I saw ROCKET\nBOSS escaping\nSILPH's building.", NULL },  /* SPRITE_ROCKER, STAY, TEXT_SAFFRONCITY_ROCKER */
    {  36,  45, 0x18, 0, "I'm a security\nguard.\fSuspicious kids I\ndon't allow in!", NULL },  /* SPRITE_ROCKET, STAY, TEXT_SAFFRONCITY_ROCKET8 */
    {  38,  45, 0x18, 0, "...\nSnore...\fHah! He's taking\na snooze!", NULL },  /* SPRITE_ROCKET, STAY, TEXT_SAFFRONCITY_ROCKET9 */
};

static const sign_event_t kSigns_SaffronCity[] = {
    {  34,  11, "SAFFRON CITY\nShining, Golden\nLand of Commerce" },  /* TEXT_SAFFRONCITY_SIGN */
    {  54,  11, "FIGHTING DOJO" },  /* TEXT_SAFFRONCITY_FIGHTING_DOJO_SIGN */
    {  70,  11, "SAFFRON CITY\nPOKEMON GYM\nLEADER: SABRINA\fThe Master of\nPsychic POKEMON!" },  /* TEXT_SAFFRONCITY_GYM_SIGN */
    {  52,  23, NULL },  /* TEXT_SAFFRONCITY_MART_SIGN */
    {  78,  39, "TRAINER TIPS\fFULL HEAL cures\nall ailments like\nsleep and burns.\fIt costs a bit\nmore, but it's\nmore convenient." },  /* TEXT_SAFFRONCITY_TRAINER_TIPS1 */
    {  10,  43, "TRAINER TIPS\fNew GREAT BALL\noffers improved\ncapture rates.\fTry it on those\nhard-to-catch\nPOKEMON." },  /* TEXT_SAFFRONCITY_TRAINER_TIPS2 */
    {  30,  43, "SILPH CO.\nOFFICE BUILDING" },  /* TEXT_SAFFRONCITY_SILPH_CO_SIGN */
    {  20,  59, NULL },  /* TEXT_SAFFRONCITY_POKECENTER_SIGN */
    {  54,  59, "MR.PSYCHIC's\nHOUSE" },  /* TEXT_SAFFRONCITY_MR_PSYCHICS_HOUSE_SIGN */
    {   2,  39, "SILPH's latest\nproduct!\fRelease to be\ndetermined..." },  /* TEXT_SAFFRONCITY_SILPH_CO_LATEST_PRODUCT_SIGN */
};

static const npc_event_t kNpcs_Route1[] = {
    {  10,  49, 0x04, 1, "Hi! I work at a\nPOKEMON MART.\fIt's a convenient\nshop, so please\nvisit us in\nVIRIDIAN CITY.\fI know, I'll give\nyou a sample!\nHere you go!", NULL },  /* SPRITE_YOUNGSTER, WALK, TEXT_ROUTE1_YOUNGSTER1 */
    {  30,  27, 0x04, 1, "See those ledges\nalong the road?\fIt's a bit scary,\nbut you can jump\nfrom them.\fYou can get back\nto PALLET TOWN\nquicker that way.", NULL },  /* SPRITE_YOUNGSTER, WALK, TEXT_ROUTE1_YOUNGSTER2 */
};

static const sign_event_t kSigns_Route1[] = {
    {  18,  55, "ROUTE 1\nPALLET TOWN -\nVIRIDIAN CITY" },  /* TEXT_ROUTE1_SIGN */
};

static const map_warp_t kWarps_Route2[] = {
    {  24,  19, 0x2e, 0 },  /* DIGLETTS_CAVE_ROUTE_2 */
    {   6,  23, 0x2f, 1 },  /* VIRIDIAN_FOREST_NORTH_GATE */
    {  30,  39, 0x30, 0 },  /* ROUTE_2_TRADE_HOUSE */
    {  32,  71, 0x31, 1 },  /* ROUTE_2_GATE */
    {  30,  79, 0x31, 2 },  /* ROUTE_2_GATE */
    {   6,  87, 0x32, 2 },  /* VIRIDIAN_FOREST_SOUTH_GATE */
};

static const sign_event_t kSigns_Route2[] = {
    {  10, 131, "ROUTE 2\nVIRIDIAN CITY -\nPEWTER CITY" },  /* TEXT_ROUTE2_SIGN */
    {  22,  23, "DIGLETT's CAVE" },  /* TEXT_ROUTE2_DIGLETTS_CAVE_SIGN */
};

static const item_event_t kItems_Route2[] = {
    {  26, 109, 0x0a },  /* MOON_STONE */
    {  26,  91, 0x23 },  /* HP_UP */
};

static const npc_event_t kNpcs_Route3[] = {
    { 114,  23, 0x0c, 0, "Whew... I better\ntake a rest...\nGroan...\fThat tunnel from\nCERULEAN takes a\nlot out of you!", NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_ROUTE3_SUPER_NERD */
    {  20,  13, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_ROUTE3_YOUNGSTER1 */
    {  28,   9, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_ROUTE3_YOUNGSTER2 */
    {  32,  19, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROUTE3_COOLTRAINER_F1 */
    {  38,  11, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_ROUTE3_YOUNGSTER3 */
    {  46,   9, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROUTE3_COOLTRAINER_F2 */
    {  44,  19, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_ROUTE3_YOUNGSTER4 */
    {  48,  13, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_ROUTE3_YOUNGSTER5 */
    {  66,  21, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROUTE3_COOLTRAINER_F3 */
};

static const sign_event_t kSigns_Route3[] = {
    { 118,  19, "ROUTE 3\nMT.MOON AHEAD" },  /* TEXT_ROUTE3_SIGN */
};

static const map_warp_t kWarps_Route4[] = {
    {  22,  11, 0x44, 0 },  /* MT_MOON_POKECENTER */
    {  36,  11, 0x3b, 0 },  /* MT_MOON_1F */
    {  48,  11, 0x3c, 7 },  /* MT_MOON_B1F */
};

static const npc_event_t kNpcs_Route4[] = {
    {  18,  17, 0x06, 1, "Ouch! I tripped\nover a rocky\nPOKEMON, GEODUDE!", NULL },  /* SPRITE_COOLTRAINER_F, WALK, TEXT_ROUTE4_COOLTRAINER_F1 */
    { 126,   7, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROUTE4_COOLTRAINER_F2 */
};

static const sign_event_t kSigns_Route4[] = {
    {  24,  11, NULL },  /* TEXT_ROUTE4_POKECENTER_SIGN */
    {  34,  15, "MT.MOON\nTunnel Entrance" },  /* TEXT_ROUTE4_MT_MOON_SIGN */
    {  54,  15, "ROUTE 4\nMT.MOON -\nCERULEAN CITY" },  /* TEXT_ROUTE4_SIGN */
};

static const item_event_t kItems_Route4[] = {
    { 114,   7, 0xcc },  /* TM_WHIRLWIND */
};

static const map_warp_t kWarps_Route5[] = {
    {  20,  59, 0x46, 3 },  /* ROUTE_5_GATE */
    {  18,  59, 0x46, 2 },  /* ROUTE_5_GATE */
    {  20,  67, 0x46, 0 },  /* ROUTE_5_GATE */
    {  34,  55, 0x47, 0 },  /* UNDERGROUND_PATH_ROUTE_5 */
    {  20,  43, 0x48, 0 },  /* DAYCARE */
};

static const sign_event_t kSigns_Route5[] = {
    {  34,  59, "UNDERGROUND PATH\nCERULEAN CITY -\nVERMILION CITY" },  /* TEXT_ROUTE5_UNDERGROUND_PATH_SIGN */
};

static const map_warp_t kWarps_Route6[] = {
    {  18,   3, 0x49, 2 },  /* ROUTE_6_GATE */
    {  20,   3, 0x49, 2 },  /* ROUTE_6_GATE */
    {  20,  15, 0x49, 0 },  /* ROUTE_6_GATE */
    {  34,  27, 0x4a, 0 },  /* UNDERGROUND_PATH_ROUTE_6 */
};

static const npc_event_t kNpcs_Route6[] = {
    {  20,  43, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_ROUTE6_COOLTRAINER_M1 */
    {  22,  43, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROUTE6_COOLTRAINER_F1 */
    {   0,  31, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_ROUTE6_YOUNGSTER1 */
    {  22,  63, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_ROUTE6_COOLTRAINER_M2 */
    {  22,  61, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROUTE6_COOLTRAINER_F2 */
    {  38,  53, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_ROUTE6_YOUNGSTER2 */
};

static const sign_event_t kSigns_Route6[] = {
    {  38,  31, "UNDERGROUND PATH\nCERULEAN CITY -\nVERMILION CITY" },  /* TEXT_ROUTE6_UNDERGROUND_PATH_SIGN */
};

static const map_warp_t kWarps_Route7[] = {
    {  36,  19, 0x4c, 2 },  /* ROUTE_7_GATE */
    {  36,  21, 0x4c, 3 },  /* ROUTE_7_GATE */
    {  22,  19, 0x4c, 0 },  /* ROUTE_7_GATE */
    {  22,  21, 0x4c, 1 },  /* ROUTE_7_GATE */
    {  10,  27, 0x4d, 0 },  /* UNDERGROUND_PATH_ROUTE_7 */
};

static const sign_event_t kSigns_Route7[] = {
    {   6,  27, "UNDERGROUND PATH\nCELADON CITY -\nLAVENDER TOWN" },  /* TEXT_ROUTE7_UNDERGROUND_PATH_SIGN */
};

static const map_warp_t kWarps_Route8[] = {
    {   2,  19, 0x4f, 0 },  /* ROUTE_8_GATE */
    {   2,  21, 0x4f, 1 },  /* ROUTE_8_GATE */
    {  16,  19, 0x4f, 2 },  /* ROUTE_8_GATE */
    {  16,  21, 0x4f, 3 },  /* ROUTE_8_GATE */
    {  26,   7, 0x50, 0 },  /* UNDERGROUND_PATH_ROUTE_8 */
};

static const npc_event_t kNpcs_Route8[] = {
    {  16,  11, 0x0c, 0, NULL, NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_ROUTE8_SUPER_NERD1 */
    {  26,  19, 0x0b, 0, NULL, NULL },  /* SPRITE_GAMBLER, STAY, TEXT_ROUTE8_GAMBLER1 */
    {  84,  13, 0x0c, 0, NULL, NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_ROUTE8_SUPER_NERD2 */
    {  52,   7, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROUTE8_COOLTRAINER_F1 */
    {  52,   9, 0x0c, 0, NULL, NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_ROUTE8_SUPER_NERD3 */
    {  52,  11, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROUTE8_COOLTRAINER_F2 */
    {  52,  13, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROUTE8_COOLTRAINER_F3 */
    {  92,  27, 0x0b, 0, NULL, NULL },  /* SPRITE_GAMBLER, STAY, TEXT_ROUTE8_GAMBLER2 */
    { 102,  25, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROUTE8_COOLTRAINER_F4 */
};

static const sign_event_t kSigns_Route8[] = {
    {  34,   7, "UNDERGROUND PATH\nCELADON CITY -\nLAVENDER TOWN" },  /* TEXT_ROUTE8_UNDERGROUND_SIGN */
};

static const npc_event_t kNpcs_Route9[] = {
    {  26,  21, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROUTE9_COOLTRAINER_F1 */
    {  48,  15, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_ROUTE9_COOLTRAINER_M1 */
    {  62,  15, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_ROUTE9_COOLTRAINER_M2 */
    {  96,  17, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROUTE9_COOLTRAINER_F2 */
    {  32,  31, 0x0e, 0, NULL, NULL },  /* SPRITE_HIKER, STAY, TEXT_ROUTE9_HIKER1 */
    {  86,   7, 0x0e, 0, NULL, NULL },  /* SPRITE_HIKER, STAY, TEXT_ROUTE9_HIKER2 */
    {  44,   5, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_ROUTE9_YOUNGSTER1 */
    {  90,  31, 0x0e, 0, NULL, NULL },  /* SPRITE_HIKER, STAY, TEXT_ROUTE9_HIKER3 */
    {  80,  17, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_ROUTE9_YOUNGSTER2 */
};

static const sign_event_t kSigns_Route9[] = {
    {  50,  15, "ROUTE 9\nCERULEAN CITY-\nROCK TUNNEL" },  /* TEXT_ROUTE9_SIGN */
};

static const item_event_t kItems_Route9[] = {
    {  20,  31, 0xe6 },  /* TM_TELEPORT */
};

static const map_warp_t kWarps_Route10[] = {
    {  22,  39, 0x51, 0 },  /* ROCK_TUNNEL_POKECENTER */
    {  16,  35, 0x52, 0 },  /* ROCK_TUNNEL_1F */
    {  16, 107, 0x52, 2 },  /* ROCK_TUNNEL_1F */
    {  12,  79, 0x53, 0 },  /* POWER_PLANT */
};

static const npc_event_t kNpcs_Route10[] = {
    {  20,  89, 0x0c, 0, NULL, NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_ROUTE10_SUPER_NERD1 */
    {   6, 115, 0x0e, 0, NULL, NULL },  /* SPRITE_HIKER, STAY, TEXT_ROUTE10_HIKER1 */
    {  28, 129, 0x0c, 0, NULL, NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_ROUTE10_SUPER_NERD2 */
    {  14,  51, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROUTE10_COOLTRAINER_F1 */
    {   6, 123, 0x0e, 0, NULL, NULL },  /* SPRITE_HIKER, STAY, TEXT_ROUTE10_HIKER2 */
    {  14, 109, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROUTE10_COOLTRAINER_F2 */
};

static const sign_event_t kSigns_Route10[] = {
    {  14,  39, "ROCK TUNNEL" },  /* TEXT_ROUTE10_ROCKTUNNEL_NORTH_SIGN */
    {  24,  39, NULL },  /* TEXT_ROUTE10_POKECENTER_SIGN */
    {  18, 111, "ROCK TUNNEL" },  /* TEXT_ROUTE10_ROCKTUNNEL_SOUTH_SIGN */
    {  10,  83, "POWER PLANT" },  /* TEXT_ROUTE10_POWERPLANT_SIGN */
};

static const map_warp_t kWarps_Route11[] = {
    {  98,  17, 0x54, 0 },  /* ROUTE_11_GATE_1F */
    {  98,  19, 0x54, 1 },  /* ROUTE_11_GATE_1F */
    { 116,  17, 0x54, 2 },  /* ROUTE_11_GATE_1F */
    { 116,  19, 0x54, 3 },  /* ROUTE_11_GATE_1F */
    {   8,  11, 0x55, 0 },  /* DIGLETTS_CAVE_ROUTE_11 */
};

static const npc_event_t kNpcs_Route11[] = {
    {  20,  29, 0x0b, 0, NULL, NULL },  /* SPRITE_GAMBLER, STAY, TEXT_ROUTE11_GAMBLER1 */
    {  52,  19, 0x0b, 0, NULL, NULL },  /* SPRITE_GAMBLER, STAY, TEXT_ROUTE11_GAMBLER2 */
    {  26,  11, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_ROUTE11_YOUNGSTER1 */
    {  72,  23, 0x0c, 0, NULL, NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_ROUTE11_SUPER_NERD1 */
    {  44,   9, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_ROUTE11_YOUNGSTER2 */
    {  90,  15, 0x0b, 0, NULL, NULL },  /* SPRITE_GAMBLER, STAY, TEXT_ROUTE11_GAMBLER3 */
    {  66,   7, 0x0b, 0, NULL, NULL },  /* SPRITE_GAMBLER, STAY, TEXT_ROUTE11_GAMBLER4 */
    {  86,  11, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_ROUTE11_YOUNGSTER3 */
    {  90,  33, 0x0c, 0, NULL, NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_ROUTE11_SUPER_NERD2 */
    {  44,  25, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_ROUTE11_YOUNGSTER4 */
};

static const sign_event_t kSigns_Route11[] = {
    {   2,  11, "DIGLETT's CAVE" },  /* TEXT_ROUTE11_DIGLETTSCAVE_SIGN */
};

static const map_warp_t kWarps_Route12[] = {
    {  20,  31, 0x57, 0 },  /* ROUTE_12_GATE_1F */
    {  22,  31, 0x57, 1 },  /* ROUTE_12_GATE_1F */
    {  20,  43, 0x57, 2 },  /* ROUTE_12_GATE_1F */
    {  22, 155, 0xbd, 0 },  /* ROUTE_12_SUPER_ROD_HOUSE */
};

static const npc_event_t kNpcs_Route12[] = {
    {  20, 125, 0x43, 0, "A sleeping POKEMON\nblocks the way!", NULL },  /* SPRITE_SNORLAX, STAY, TEXT_ROUTE12_SNORLAX */
    {  28,  63, 0x2f, 0, NULL, NULL },  /* SPRITE_FISHER, STAY, TEXT_ROUTE12_FISHER1 */
    {  10,  79, 0x2f, 0, NULL, NULL },  /* SPRITE_FISHER, STAY, TEXT_ROUTE12_FISHER2 */
    {  22, 185, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_ROUTE12_COOLTRAINER_M */
    {  28, 153, 0x0c, 0, NULL, NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_ROUTE12_SUPER_NERD */
    {  24,  81, 0x2f, 0, NULL, NULL },  /* SPRITE_FISHER, STAY, TEXT_ROUTE12_FISHER3 */
    {  18, 105, 0x2f, 0, NULL, NULL },  /* SPRITE_FISHER, STAY, TEXT_ROUTE12_FISHER4 */
    {  12, 175, 0x2f, 0, NULL, NULL },  /* SPRITE_FISHER, STAY, TEXT_ROUTE12_FISHER5 */
};

static const sign_event_t kSigns_Route12[] = {
    {  26,  27, "ROUTE 12 \nNorth to LAVENDER" },  /* TEXT_ROUTE12_SIGN */
    {  22, 127, "SPORT FISHING AREA" },  /* TEXT_ROUTE12_SPORT_FISHING_SIGN */
};

static const item_event_t kItems_Route12[] = {
    {  28,  71, 0xd8 },  /* TM_PAY_DAY */
    {  10, 179, 0x25 },  /* IRON */
};

static const npc_event_t kNpcs_Route13[] = {
    {  98,  21, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_ROUTE13_COOLTRAINER_M1 */
    {  96,  21, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROUTE13_COOLTRAINER_F1 */
    {  54,  19, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROUTE13_COOLTRAINER_F2 */
    {  46,  21, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROUTE13_COOLTRAINER_F3 */
    { 100,  11, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROUTE13_COOLTRAINER_F4 */
    {  24,   9, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_ROUTE13_COOLTRAINER_M2 */
    {  66,  13, 0x0f, 0, NULL, NULL },  /* SPRITE_BEAUTY, STAY, TEXT_ROUTE13_BEAUTY1 */
    {  64,  13, 0x0f, 0, NULL, NULL },  /* SPRITE_BEAUTY, STAY, TEXT_ROUTE13_BEAUTY2 */
    {  20,  15, 0x12, 0, NULL, NULL },  /* SPRITE_BIKER, STAY, TEXT_ROUTE13_BIKER */
    {  14,  27, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_ROUTE13_COOLTRAINER_M3 */
};

static const sign_event_t kSigns_Route13[] = {
    {  30,  27, "TRAINER TIPS\fLook to the left\nof that post!" },  /* TEXT_ROUTE13_TRAINER_TIPS1 */
    {  66,  11, "TRAINER TIPS\fUse SELECT to\nswitch items in\nthe ITEM window!" },  /* TEXT_ROUTE13_TRAINER_TIPS2 */
    {  62,  23, "ROUTE 13\nNorth to SILENCE\nBRIDGE" },  /* TEXT_ROUTE13_SIGN */
};

static const npc_event_t kNpcs_Route14[] = {
    {   8,   9, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_ROUTE14_COOLTRAINER_M1 */
    {  30,  13, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_ROUTE14_COOLTRAINER_M2 */
    {  24,  23, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_ROUTE14_COOLTRAINER_M3 */
    {  28,  31, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_ROUTE14_COOLTRAINER_M4 */
    {  30,  63, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_ROUTE14_COOLTRAINER_M5 */
    {  12,  99, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_ROUTE14_COOLTRAINER_M6 */
    {  10,  79, 0x12, 0, NULL, NULL },  /* SPRITE_BIKER, STAY, TEXT_ROUTE14_BIKER1 */
    {   8,  61, 0x12, 0, NULL, NULL },  /* SPRITE_BIKER, STAY, TEXT_ROUTE14_BIKER2 */
    {  30,  61, 0x12, 0, NULL, NULL },  /* SPRITE_BIKER, STAY, TEXT_ROUTE14_BIKER3 */
    {   8,  63, 0x12, 0, NULL, NULL },  /* SPRITE_BIKER, STAY, TEXT_ROUTE14_BIKER4 */
};

static const sign_event_t kSigns_Route14[] = {
    {  34,  27, "ROUTE 14\nWest to FUCHSIA\nCITY" },  /* TEXT_ROUTE14_SIGN */
};

static const map_warp_t kWarps_Route15[] = {
    {  14,  17, 0xb8, 0 },  /* ROUTE_15_GATE_1F */
    {  14,  19, 0xb8, 1 },  /* ROUTE_15_GATE_1F */
    {  28,  17, 0xb8, 2 },  /* ROUTE_15_GATE_1F */
    {  28,  19, 0xb8, 3 },  /* ROUTE_15_GATE_1F */
};

static const npc_event_t kNpcs_Route15[] = {
    {  82,  23, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROUTE15_COOLTRAINER_F1 */
    { 106,  21, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROUTE15_COOLTRAINER_F2 */
    {  62,  27, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_ROUTE15_COOLTRAINER_M1 */
    {  70,  27, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_ROUTE15_COOLTRAINER_M2 */
    { 106,  23, 0x0f, 0, NULL, NULL },  /* SPRITE_BEAUTY, STAY, TEXT_ROUTE15_BEAUTY1 */
    {  82,  21, 0x0f, 0, NULL, NULL },  /* SPRITE_BEAUTY, STAY, TEXT_ROUTE15_BEAUTY2 */
    {  96,  21, 0x12, 0, NULL, NULL },  /* SPRITE_BIKER, STAY, TEXT_ROUTE15_BIKER1 */
    {  92,  21, 0x12, 0, NULL, NULL },  /* SPRITE_BIKER, STAY, TEXT_ROUTE15_BIKER2 */
    {  74,  11, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROUTE15_COOLTRAINER_F3 */
    {  36,  27, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROUTE15_COOLTRAINER_F4 */
};

static const sign_event_t kSigns_Route15[] = {
    {  78,  19, "ROUTE 15\nWest to FUCHSIA\nCITY" },  /* TEXT_ROUTE15_SIGN */
};

static const item_event_t kItems_Route15[] = {
    {  36,  11, 0xdc },  /* TM_RAGE */
};

static const map_warp_t kWarps_Route16[] = {
    {  34,  21, 0xba, 0 },  /* ROUTE_16_GATE_1F */
    {  34,  23, 0xba, 1 },  /* ROUTE_16_GATE_1F */
    {  48,  21, 0xba, 2 },  /* ROUTE_16_GATE_1F */
    {  48,  23, 0xba, 3 },  /* ROUTE_16_GATE_1F */
    {  34,   9, 0xba, 4 },  /* ROUTE_16_GATE_1F */
    {  34,  11, 0xba, 5 },  /* ROUTE_16_GATE_1F */
    {  48,   9, 0xba, 6 },  /* ROUTE_16_GATE_1F */
    {  48,  11, 0xba, 7 },  /* ROUTE_16_GATE_1F */
    {  14,  11, 0xbc, 0 },  /* ROUTE_16_FLY_HOUSE */
};

static const npc_event_t kNpcs_Route16[] = {
    {  34,  25, 0x12, 0, NULL, NULL },  /* SPRITE_BIKER, STAY, TEXT_ROUTE16_BIKER1 */
    {  28,  27, 0x12, 0, NULL, NULL },  /* SPRITE_BIKER, STAY, TEXT_ROUTE16_BIKER2 */
    {  22,  25, 0x12, 0, NULL, NULL },  /* SPRITE_BIKER, STAY, TEXT_ROUTE16_BIKER3 */
    {  18,  23, 0x12, 0, NULL, NULL },  /* SPRITE_BIKER, STAY, TEXT_ROUTE16_BIKER4 */
    {  12,  21, 0x12, 0, NULL, NULL },  /* SPRITE_BIKER, STAY, TEXT_ROUTE16_BIKER5 */
    {   6,  25, 0x12, 0, NULL, NULL },  /* SPRITE_BIKER, STAY, TEXT_ROUTE16_BIKER6 */
    {  52,  21, 0x43, 0, "A sleeping POKEMON\nblocks the way!", NULL },  /* SPRITE_SNORLAX, STAY, TEXT_ROUTE16_SNORLAX */
};

static const sign_event_t kSigns_Route16[] = {
    {  54,  23, "Enjoy the slope!\nCYCLING ROAD" },  /* TEXT_ROUTE16_CYCLING_ROAD_SIGN */
    {  10,  35, "ROUTE 16\nCELADON CITY -\nFUCHSIA CITY" },  /* TEXT_ROUTE16_SIGN */
};

static const npc_event_t kNpcs_Route17[] = {
    {  24,  39, 0x12, 0, NULL, NULL },  /* SPRITE_BIKER, STAY, TEXT_ROUTE17_BIKER1 */
    {  22,  33, 0x12, 0, NULL, NULL },  /* SPRITE_BIKER, STAY, TEXT_ROUTE17_BIKER2 */
    {   8,  37, 0x12, 0, NULL, NULL },  /* SPRITE_BIKER, STAY, TEXT_ROUTE17_BIKER3 */
    {  14,  65, 0x12, 0, NULL, NULL },  /* SPRITE_BIKER, STAY, TEXT_ROUTE17_BIKER4 */
    {  28,  69, 0x12, 0, NULL, NULL },  /* SPRITE_BIKER, STAY, TEXT_ROUTE17_BIKER5 */
    {  34, 117, 0x12, 0, NULL, NULL },  /* SPRITE_BIKER, STAY, TEXT_ROUTE17_BIKER6 */
    {   4, 137, 0x12, 0, NULL, NULL },  /* SPRITE_BIKER, STAY, TEXT_ROUTE17_BIKER7 */
    {  28, 197, 0x12, 0, NULL, NULL },  /* SPRITE_BIKER, STAY, TEXT_ROUTE17_BIKER8 */
    {  10, 197, 0x12, 0, NULL, NULL },  /* SPRITE_BIKER, STAY, TEXT_ROUTE17_BIKER9 */
    {  20, 237, 0x12, 0, NULL, NULL },  /* SPRITE_BIKER, STAY, TEXT_ROUTE17_BIKER10 */
};

static const sign_event_t kSigns_Route17[] = {
    {  18, 103, "It's a notice!\fWatch out for\ndiscarded items!" },  /* TEXT_ROUTE17_NOTICE_SIGN1 */
    {  18, 127, "TRAINER TIPS\fAll POKEMON are\nunique.\fEven POKEMON of\nthe same type and\nlevel grow at\ndifferent rates." },  /* TEXT_ROUTE17_TRAINER_TIPS1 */
    {  18, 151, "TRAINER TIPS\fPress the A or B\nButton to stay in\nplace while on a\nslope." },  /* TEXT_ROUTE17_TRAINER_TIPS2 */
    {  18, 175, "ROUTE 17\nCELADON CITY -\nFUCHSIA CITY" },  /* TEXT_ROUTE17_SIGN */
    {  18, 223, "It's a notice!\fDon't throw the\ngame, throw POKE\nBALLs instead!" },  /* TEXT_ROUTE17_NOTICE_SIGN2 */
    {  18, 283, "CYCLING ROAD\nSlope ends here!" },  /* TEXT_ROUTE17_CYCLING_ROAD_ENDS_SIGN */
};

static const map_warp_t kWarps_Route18[] = {
    {  66,  17, 0xbe, 0 },  /* ROUTE_18_GATE_1F */
    {  66,  19, 0xbe, 1 },  /* ROUTE_18_GATE_1F */
    {  80,  17, 0xbe, 2 },  /* ROUTE_18_GATE_1F */
    {  80,  19, 0xbe, 3 },  /* ROUTE_18_GATE_1F */
};

static const npc_event_t kNpcs_Route18[] = {
    {  72,  23, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_ROUTE18_COOLTRAINER_M1 */
    {  80,  31, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_ROUTE18_COOLTRAINER_M2 */
    {  84,  27, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_ROUTE18_COOLTRAINER_M3 */
};

static const sign_event_t kSigns_Route18[] = {
    {  86,  15, "ROUTE 18\nCELADON CITY -\nFUCHSIA CITY" },  /* TEXT_ROUTE18_SIGN */
    {  66,  11, "CYCLING ROAD\nNo pedestrians\npermitted!" },  /* TEXT_ROUTE18_CYCLING_ROAD_SIGN */
};

static const npc_event_t kNpcs_Route19[] = {
    {  16,  15, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_ROUTE19_COOLTRAINER_M1 */
    {  26,  15, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_ROUTE19_COOLTRAINER_M2 */
    {  26,  51, 0x22, 0, NULL, NULL },  /* SPRITE_SWIMMER, STAY, TEXT_ROUTE19_SWIMMER1 */
    {   8,  55, 0x22, 0, NULL, NULL },  /* SPRITE_SWIMMER, STAY, TEXT_ROUTE19_SWIMMER2 */
    {  32,  63, 0x22, 0, NULL, NULL },  /* SPRITE_SWIMMER, STAY, TEXT_ROUTE19_SWIMMER3 */
    {  18,  23, 0x22, 0, NULL, NULL },  /* SPRITE_SWIMMER, STAY, TEXT_ROUTE19_SWIMMER4 */
    {  16,  87, 0x22, 0, NULL, NULL },  /* SPRITE_SWIMMER, STAY, TEXT_ROUTE19_SWIMMER5 */
    {  22,  87, 0x22, 0, NULL, NULL },  /* SPRITE_SWIMMER, STAY, TEXT_ROUTE19_SWIMMER6 */
    {  18,  85, 0x22, 0, NULL, NULL },  /* SPRITE_SWIMMER, STAY, TEXT_ROUTE19_SWIMMER7 */
    {  20,  89, 0x22, 0, NULL, NULL },  /* SPRITE_SWIMMER, STAY, TEXT_ROUTE19_SWIMMER8 */
};

static const sign_event_t kSigns_Route19[] = {
    {  22,  19, "SEA ROUTE 19\nFUCHSIA CITY -\nSEAFOAM ISLANDS" },  /* TEXT_ROUTE19_SIGN */
};

static const map_warp_t kWarps_Route20[] = {
    {  96,  11, 0xc0, 0 },  /* SEAFOAM_ISLANDS_1F */
    { 116,  19, 0xc0, 2 },  /* SEAFOAM_ISLANDS_1F */
};

static const npc_event_t kNpcs_Route20[] = {
    { 174,  17, 0x22, 0, NULL, NULL },  /* SPRITE_SWIMMER, STAY, TEXT_ROUTE20_SWIMMER1 */
    { 136,  23, 0x22, 0, NULL, NULL },  /* SPRITE_SWIMMER, STAY, TEXT_ROUTE20_SWIMMER2 */
    {  90,  21, 0x22, 0, NULL, NULL },  /* SPRITE_SWIMMER, STAY, TEXT_ROUTE20_SWIMMER3 */
    { 110,  29, 0x22, 0, NULL, NULL },  /* SPRITE_SWIMMER, STAY, TEXT_ROUTE20_SWIMMER4 */
    {  76,  27, 0x22, 0, NULL, NULL },  /* SPRITE_SWIMMER, STAY, TEXT_ROUTE20_SWIMMER5 */
    { 174,  27, 0x22, 0, NULL, NULL },  /* SPRITE_SWIMMER, STAY, TEXT_ROUTE20_SWIMMER6 */
    {  68,  19, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_ROUTE20_COOLTRAINER_M */
    {  50,  15, 0x22, 0, NULL, NULL },  /* SPRITE_SWIMMER, STAY, TEXT_ROUTE20_SWIMMER7 */
    {  48,  25, 0x22, 0, NULL, NULL },  /* SPRITE_SWIMMER, STAY, TEXT_ROUTE20_SWIMMER8 */
    {  30,  17, 0x22, 0, NULL, NULL },  /* SPRITE_SWIMMER, STAY, TEXT_ROUTE20_SWIMMER9 */
};

static const sign_event_t kSigns_Route20[] = {
    { 102,  15, "SEAFOAM ISLANDS" },  /* TEXT_ROUTE20_SEAFOAM_ISLANDS_WEST_SIGN */
    { 114,  23, "SEAFOAM ISLANDS" },  /* TEXT_ROUTE20_SEAFOAM_ISLANDS_EAST_SIGN */
};

static const npc_event_t kNpcs_Route21[] = {
    {   8,  49, 0x2f, 0, NULL, NULL },  /* SPRITE_FISHER, STAY, TEXT_ROUTE21_FISHER1 */
    {  12,  51, 0x2f, 0, NULL, NULL },  /* SPRITE_FISHER, STAY, TEXT_ROUTE21_FISHER2 */
    {  20,  63, 0x22, 0, NULL, NULL },  /* SPRITE_SWIMMER, STAY, TEXT_ROUTE21_SWIMMER1 */
    {  24,  61, 0x22, 0, NULL, NULL },  /* SPRITE_SWIMMER, STAY, TEXT_ROUTE21_SWIMMER2 */
    {  32, 127, 0x22, 0, NULL, NULL },  /* SPRITE_SWIMMER, STAY, TEXT_ROUTE21_SWIMMER3 */
    {  10, 143, 0x22, 0, NULL, NULL },  /* SPRITE_SWIMMER, STAY, TEXT_ROUTE21_SWIMMER4 */
    {  30, 143, 0x22, 0, NULL, NULL },  /* SPRITE_SWIMMER, STAY, TEXT_ROUTE21_SWIMMER5 */
    {  28, 113, 0x2f, 0, NULL, NULL },  /* SPRITE_FISHER, STAY, TEXT_ROUTE21_FISHER3 */
    {  34, 115, 0x2f, 0, NULL, NULL },  /* SPRITE_FISHER, STAY, TEXT_ROUTE21_FISHER4 */
};

static const map_warp_t kWarps_Route22[] = {
    {  16,  11, 0xc1, 0 },  /* ROUTE_22_GATE */
};

static const npc_event_t kNpcs_Route22[] = {
    {  50,  11, 0x02, 0, NULL, NULL },  /* SPRITE_BLUE, STAY, TEXT_ROUTE22_RIVAL1 */
    {  50,  11, 0x02, 0, NULL, NULL },  /* SPRITE_BLUE, STAY, TEXT_ROUTE22_RIVAL2 */
};

static const sign_event_t kSigns_Route22[] = {
    {  14,  23, "POKEMON LEAGUE\nFront Gate" },  /* TEXT_ROUTE22_POKEMON_LEAGUE_SIGN */
};

static const map_warp_t kWarps_Route23[] = {
    {  14, 279, 0xc1, 2 },  /* ROUTE_22_GATE */
    {  16, 279, 0xc1, 3 },  /* ROUTE_22_GATE */
    {   8,  63, 0x6c, 0 },  /* VICTORY_ROAD_1F */
    {  28,  63, 0xc2, 1 },  /* VICTORY_ROAD_2F */
};

static const npc_event_t kNpcs_Route23[] = {
    {   8,  71, 0x31, 0, NULL, NULL },  /* SPRITE_GUARD, STAY, TEXT_ROUTE23_GUARD1 */
    {  20, 113, 0x31, 0, NULL, NULL },  /* SPRITE_GUARD, STAY, TEXT_ROUTE23_GUARD2 */
    {  16, 171, 0x22, 0, NULL, NULL },  /* SPRITE_SWIMMER, STAY, TEXT_ROUTE23_SWIMMER1 */
    {  22, 193, 0x22, 0, NULL, NULL },  /* SPRITE_SWIMMER, STAY, TEXT_ROUTE23_SWIMMER2 */
    {  24, 211, 0x31, 0, NULL, NULL },  /* SPRITE_GUARD, STAY, TEXT_ROUTE23_GUARD3 */
    {  16, 239, 0x31, 0, NULL, NULL },  /* SPRITE_GUARD, STAY, TEXT_ROUTE23_GUARD4 */
    {  16, 273, 0x31, 0, NULL, NULL },  /* SPRITE_GUARD, STAY, TEXT_ROUTE23_GUARD5 */
};

static const sign_event_t kSigns_Route23[] = {
    {   6,  67, "VICTORY ROAD GATE\n- POKEMON LEAGUE" },  /* TEXT_ROUTE23_VICTORY_ROAD_GATE_SIGN */
};

static const npc_event_t kNpcs_Route24[] = {
    {  22,  31, 0x07, 0, "Congratulations!\nYou beat our 5\ncontest trainers!@", NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_ROUTE24_COOLTRAINER_M1 */
    {  10,  41, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_ROUTE24_COOLTRAINER_M2 */
    {  22,  39, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_ROUTE24_COOLTRAINER_M3 */
    {  20,  45, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROUTE24_COOLTRAINER_F1 */
    {  22,  51, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_ROUTE24_YOUNGSTER1 */
    {  20,  57, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROUTE24_COOLTRAINER_F2 */
    {  22,  63, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_ROUTE24_YOUNGSTER2 */
};

static const item_event_t kItems_Route24[] = {
    {  20,  11, 0xf5 },  /* TM_THUNDER_WAVE */
};

static const map_warp_t kWarps_Route25[] = {
    {  90,   7, 0x58, 0 },  /* BILLS_HOUSE */
};

static const npc_event_t kNpcs_Route25[] = {
    {  28,   5, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_ROUTE25_YOUNGSTER1 */
    {  36,  11, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_ROUTE25_YOUNGSTER2 */
    {  48,   9, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_ROUTE25_COOLTRAINER_M */
    {  36,  17, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROUTE25_COOLTRAINER_F1 */
    {  64,   7, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_ROUTE25_YOUNGSTER3 */
    {  74,   9, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROUTE25_COOLTRAINER_F2 */
    {  16,   9, 0x0e, 0, NULL, NULL },  /* SPRITE_HIKER, STAY, TEXT_ROUTE25_HIKER1 */
    {  46,  19, 0x0e, 0, NULL, NULL },  /* SPRITE_HIKER, STAY, TEXT_ROUTE25_HIKER2 */
    {  26,  15, 0x0e, 0, NULL, NULL },  /* SPRITE_HIKER, STAY, TEXT_ROUTE25_HIKER3 */
};

static const sign_event_t kSigns_Route25[] = {
    {  86,   7, "SEA COTTAGE\nBILL lives here!" },  /* TEXT_ROUTE25_BILL_SIGN */
};

static const item_event_t kItems_Route25[] = {
    {  44,   5, 0xdb },  /* TM_SEISMIC_TOSS */
};

static const map_warp_t kWarps_RedsHouse1F[] = {
    {   4,  15, 0xff, 0 },  /* LAST_MAP */
    {   6,  15, 0xff, 0 },  /* LAST_MAP */
    {  14,   3, 0x26, 0 },  /* REDS_HOUSE_2F */
};

static const npc_event_t kNpcs_RedsHouse1F[] = {
    {  10,   9, 0x33, 0, "MOM: Right.\nAll boys leave\nhome some day.\nIt said so on TV.\fPROF.OAK, next\ndoor, is looking\nfor you.", NULL },  /* SPRITE_MOM, STAY, TEXT_REDSHOUSE1F_MOM */
};

static const sign_event_t kSigns_RedsHouse1F[] = {
    {   6,   3, "There's a movie\non TV. Four boys\nare walking on\nrailroad tracks.\fI better go too." },  /* TEXT_REDSHOUSE1F_TV */
};

static const map_warp_t kWarps_RedsHouse2F[] = {
    {  14,   3, 0x25, 2 },  /* REDS_HOUSE_1F */
};

static const map_warp_t kWarps_BluesHouse[] = {
    {   4,  15, 0xff, 1 },  /* LAST_MAP */
    {   6,  15, 0xff, 1 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_BluesHouse[] = {
    {   4,   7, 0x11, 0, NULL, NULL },  /* SPRITE_DAISY, STAY, TEXT_BLUESHOUSE_DAISY_SITTING */
    {  12,   9, 0x11, 1, "POKEMON are living\nthings! If they\nget tired, give\nthem a rest!", NULL },  /* SPRITE_DAISY, WALK, TEXT_BLUESHOUSE_DAISY_WALKING */
    {   6,   7, 0x41, 0, "It's a big map!\nThis is useful!", NULL },  /* SPRITE_POKEDEX, STAY, TEXT_BLUESHOUSE_TOWN_MAP */
};

static const map_warp_t kWarps_OaksLab[] = {
    {   8,  23, 0xff, 2 },  /* LAST_MAP */
    {  10,  23, 0xff, 2 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_OaksLab[] = {
    {   8,   7, 0x02, 0, "{RIVAL}: Yo\n{PLAYER}! Gramps\nisn't around!", NULL },  /* SPRITE_BLUE, STAY, TEXT_OAKSLAB_RIVAL */
    {  10,   5, 0x03, 0, "OAK: Now, {PLAYER},\nwhich POKEMON do\nyou want?", NULL },  /* SPRITE_OAK, STAY, TEXT_OAKSLAB_OAK1 */
    {   4,   3, 0x41, 0, "It's encyclopedia-\nlike, but the\npages are blank!", NULL },  /* SPRITE_POKEDEX, STAY, TEXT_OAKSLAB_POKEDEX1 */
    {   6,   3, 0x41, 0, "It's encyclopedia-\nlike, but the\npages are blank!", NULL },  /* SPRITE_POKEDEX, STAY, TEXT_OAKSLAB_POKEDEX2 */
    {  10,  21, 0x03, 0, "?", NULL },  /* SPRITE_OAK, STAY, TEXT_OAKSLAB_OAK2 */
    {   2,  19, 0x0d, 1, "PROF.OAK is the\nauthority on\nPOKEMON!\fMany POKEMON\ntrainers hold him\nin high regard!", NULL },  /* SPRITE_GIRL, WALK, TEXT_OAKSLAB_GIRL */
    {   4,  21, 0x20, 0, "I study POKEMON as\nPROF.OAK's AIDE.", NULL },  /* SPRITE_SCIENTIST, STAY, TEXT_OAKSLAB_SCIENTIST1 */
    {  16,  21, 0x20, 0, "I study POKEMON as\nPROF.OAK's AIDE.", NULL },  /* SPRITE_SCIENTIST, STAY, TEXT_OAKSLAB_SCIENTIST2 */
};

static const map_warp_t kWarps_ViridianPokecenter[] = {
    {   6,  15, 0xff, 0 },  /* LAST_MAP */
    {   8,  15, 0xff, 0 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_ViridianPokecenter[] = {
    {   6,   3, 0x29, 0, NULL, Pokecenter_Start },  /* SPRITE_NURSE, STAY, TEXT_VIRIDIANPOKECENTER_NURSE */
    {  20,  11, 0x10, 1, "You can use that\nPC in the corner.\fThe receptionist\ntold me. So kind!", NULL },  /* SPRITE_GENTLEMAN, WALK, TEXT_VIRIDIANPOKECENTER_GENTLEMAN */
    {   8,   7, 0x07, 0, "There's a POKEMON\nCENTER in every\ntown ahead.\fThey don't charge\nany money either!", NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_VIRIDIANPOKECENTER_COOLTRAINER_M */
    {  22,   5, 0x2a, 0, NULL, NULL },  /* SPRITE_LINK_RECEPTIONIST, STAY, TEXT_VIRIDIANPOKECENTER_LINK_RECEPTIONIST */
};

static const map_warp_t kWarps_ViridianMart[] = {
    {   6,  15, 0xff, 1 },  /* LAST_MAP */
    {   8,  15, 0xff, 1 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_ViridianMart[] = {
    {   0,  11, 0x26, 0, NULL, ViridianMart_Start },  /* SPRITE_CLERK, STAY, TEXT_VIRIDIANMART_CLERK */
    {  10,  11, 0x04, 1, NULL, NULL },  /* SPRITE_YOUNGSTER, WALK, TEXT_VIRIDIANMART_YOUNGSTER */
    {   6,   7, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_VIRIDIANMART_COOLTRAINER_M */
};

static const map_warp_t kWarps_ViridianSchoolHouse[] = {
    {   4,  15, 0xff, 2 },  /* LAST_MAP */
    {   6,  15, 0xff, 2 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_ViridianSchoolHouse[] = {
    {   6,  11, 0x1d, 0, "Whew! I'm trying\nto memorize all\nmy notes.", NULL },  /* SPRITE_BRUNETTE_GIRL, STAY, TEXT_VIRIDIANSCHOOLHOUSE_BRUNETTE_GIRL */
    {   8,   3, 0x06, 0, "Okay!\fBe sure to read\nthe blackboard\ncarefully!", NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_VIRIDIANSCHOOLHOUSE_COOLTRAINER_F */
};

static const map_warp_t kWarps_ViridianNicknameHouse[] = {
    {   4,  15, 0xff, 3 },  /* LAST_MAP */
    {   6,  15, 0xff, 3 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_ViridianNicknameHouse[] = {
    {  10,   7, 0x34, 0, "Coming up with\nnicknames is fun,\nbut hard.\fSimple names are\nthe easiest to\nremember.", NULL },  /* SPRITE_BALDING_GUY, STAY, TEXT_VIRIDIANNICKNAMEHOUSE_BALDING_GUY */
    {   2,   9, 0x08, 1, "My Daddy loves\nPOKEMON too.", NULL },  /* SPRITE_LITTLE_GIRL, WALK, TEXT_VIRIDIANNICKNAMEHOUSE_LITTLE_GIRL */
    {  10,  11, 0x09, 1, "SPEARY: Tetweet!", NULL },  /* SPRITE_BIRD, WALK, TEXT_VIRIDIANNICKNAMEHOUSE_SPEAROW */
    {   8,   1, 0x42, 0, "SPEAROW\nName: SPEARY", NULL },  /* SPRITE_CLIPBOARD, STAY, TEXT_VIRIDIANNICKNAMEHOUSE_SPEARY_SIGN */
};

static const map_warp_t kWarps_ViridianGym[] = {
    {  32,  35, 0xff, 4 },  /* LAST_MAP */
    {  34,  35, 0xff, 4 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_ViridianGym[] = {
    {   4,   3, 0x17, 0, "Fwahahaha! This is\nmy hideout!\fI planned to\nresurrect TEAM\nROCKET here!\fBut, you have\ncaught me again!\nSo be it! This\ntime, I'm not\nholding back!\fOnce more, you\nshall face\nGIOVANNI, the\ngreatest trainer!", NULL },  /* SPRITE_GIOVANNI, STAY, TEXT_VIRIDIANGYM_GIOVANNI */
    {  24,  15, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_VIRIDIANGYM_COOLTRAINER_M1 */
    {  22,  23, 0x0e, 0, NULL, NULL },  /* SPRITE_HIKER, STAY, TEXT_VIRIDIANGYM_HIKER1 */
    {  20,  15, 0x21, 0, NULL, NULL },  /* SPRITE_ROCKER, STAY, TEXT_VIRIDIANGYM_ROCKER1 */
    {   6,  15, 0x0e, 0, NULL, NULL },  /* SPRITE_HIKER, STAY, TEXT_VIRIDIANGYM_HIKER2 */
    {  26,  11, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_VIRIDIANGYM_COOLTRAINER_M2 */
    {  20,   3, 0x0e, 0, NULL, NULL },  /* SPRITE_HIKER, STAY, TEXT_VIRIDIANGYM_HIKER3 */
    {   4,  33, 0x21, 0, NULL, NULL },  /* SPRITE_ROCKER, STAY, TEXT_VIRIDIANGYM_ROCKER2 */
    {  12,  11, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_VIRIDIANGYM_COOLTRAINER_M3 */
    {  32,  31, 0x24, 0, NULL, NULL },  /* SPRITE_GYM_GUIDE, STAY, TEXT_VIRIDIANGYM_GYM_GUIDE */
};

static const item_event_t kItems_ViridianGym[] = {
    {  32,  19, 0x35 },  /* REVIVE */
};

static const map_warp_t kWarps_DiglettsCaveRoute2[] = {
    {   4,  15, 0xff, 0 },  /* LAST_MAP */
    {   6,  15, 0xff, 0 },  /* LAST_MAP */
    {   8,   9, 0xc5, 0 },  /* DIGLETTS_CAVE */
};

static const npc_event_t kNpcs_DiglettsCaveRoute2[] = {
    {   6,   7, 0x27, 0, "I went to ROCK\nTUNNEL, but it's\ndark and scary.\fIf a POKEMON's\nFLASH could light\nit up...", NULL },  /* SPRITE_FISHING_GURU, STAY, TEXT_DIGLETTSCAVEROUTE2_FISHING_GURU */
};

static const map_warp_t kWarps_ViridianForestNorthGate[] = {
    {   8,   1, 0xff, 1 },  /* LAST_MAP */
    {  10,   1, 0xff, 1 },  /* LAST_MAP */
    {   8,  15, 0x33, 0 },  /* VIRIDIAN_FOREST */
    {  10,  15, 0x33, 0 },  /* VIRIDIAN_FOREST */
};

static const npc_event_t kNpcs_ViridianForestNorthGate[] = {
    {   6,   5, 0x0c, 0, "Many POKEMON live\nonly in forests \nand caves.\fYou need to look\neverywhere to get\ndifferent kinds!", NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_VIRIDIANFORESTNORTHGATE_SUPER_NERD */
    {   4,  11, 0x25, 0, "Have you noticed\nthe bushes on the\nroadside?\fThey can be cut\ndown by a special\nPOKEMON move.", NULL },  /* SPRITE_GRAMPS, STAY, TEXT_VIRIDIANFORESTNORTHGATE_GRAMPS */
};

static const map_warp_t kWarps_Route2TradeHouse[] = {
    {   4,  15, 0xff, 2 },  /* LAST_MAP */
    {   6,  15, 0xff, 2 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_Route2TradeHouse[] = {
    {   4,   9, 0x20, 0, "A fainted POKEMON\ncan't fight. But, \nit can still use \nmoves like CUT!", NULL },  /* SPRITE_SCIENTIST, STAY, TEXT_ROUTE2TRADEHOUSE_SCIENTIST */
    {   8,   3, 0x37, 0, NULL, NULL },  /* SPRITE_GAMEBOY_KID, STAY, TEXT_ROUTE2TRADEHOUSE_GAMEBOY_KID */
};

static const map_warp_t kWarps_Route2Gate[] = {
    {   8,   1, 0xff, 3 },  /* LAST_MAP */
    {  10,   1, 0xff, 3 },  /* LAST_MAP */
    {   8,  15, 0xff, 4 },  /* LAST_MAP */
    {  10,  15, 0xff, 4 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_Route2Gate[] = {
    {   2,   9, 0x20, 0, "The HM FLASH\nlights even the\ndarkest dungeons.", NULL },  /* SPRITE_SCIENTIST, STAY, TEXT_ROUTE2GATE_OAKS_AIDE */
    {  10,   9, 0x04, 1, "Once a POKEMON\nlearns FLASH, you\ncan get through\nROCK TUNNEL.", NULL },  /* SPRITE_YOUNGSTER, WALK, TEXT_ROUTE2GATE_YOUNGSTER */
};

static const map_warp_t kWarps_ViridianForestSouthGate[] = {
    {   8,   1, 0x33, 3 },  /* VIRIDIAN_FOREST */
    {  10,   1, 0x33, 4 },  /* VIRIDIAN_FOREST */
    {   8,  15, 0xff, 5 },  /* LAST_MAP */
    {  10,  15, 0xff, 5 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_ViridianForestSouthGate[] = {
    {  16,   9, 0x0d, 0, "Are you going to\nVIRIDIAN FOREST?\nBe careful, it's\na natural maze!", NULL },  /* SPRITE_GIRL, STAY, TEXT_VIRIDIANFORESTSOUTHGATE_GIRL */
    {   4,   9, 0x08, 1, "RATTATA may be\nsmall, but its\nbite is wicked!\nDid you get one?", NULL },  /* SPRITE_LITTLE_GIRL, WALK, TEXT_VIRIDIANFORESTSOUTHGATE_LITTLE_GIRL */
};

static const map_warp_t kWarps_ViridianForest[] = {
    {   2,   1, 0x2f, 2 },  /* VIRIDIAN_FOREST_NORTH_GATE */
    {   4,   1, 0x2f, 3 },  /* VIRIDIAN_FOREST_NORTH_GATE */
    {  30,  95, 0x32, 1 },  /* VIRIDIAN_FOREST_SOUTH_GATE */
    {  32,  95, 0x32, 1 },  /* VIRIDIAN_FOREST_SOUTH_GATE */
    {  34,  95, 0x32, 1 },  /* VIRIDIAN_FOREST_SOUTH_GATE */
    {  36,  95, 0x32, 1 },  /* VIRIDIAN_FOREST_SOUTH_GATE */
};

static const npc_event_t kNpcs_ViridianForest[] = {
    {  32,  87, 0x04, 0, "I came here with\nsome friends!\fThey're out for\nPOKEMON fights!", NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_VIRIDIANFOREST_YOUNGSTER1 */
    {  60,  67, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_VIRIDIANFOREST_YOUNGSTER2 */
    {  60,  39, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_VIRIDIANFOREST_YOUNGSTER3 */
    {   4,  37, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_VIRIDIANFOREST_YOUNGSTER4 */
    {  54,  81, 0x04, 0, "I ran out of POKE\nBALLs to catch\nPOKEMON with!\fYou should carry\nextras!", NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_VIRIDIANFOREST_YOUNGSTER5 */
};

static const sign_event_t kSigns_ViridianForest[] = {
    {  48,  81, "TRAINER TIPS\fIf you want to\navoid battles,\nstay away from\ngrassy areas!" },  /* TEXT_VIRIDIANFOREST_TRAINER_TIPS1 */
    {  32,  65, "For poison, use\nANTIDOTE! Get it\nat POKEMON MARTs!" },  /* TEXT_VIRIDIANFOREST_USE_ANTIDOTE_SIGN */
    {  52,  35, "TRAINER TIPS\fContact PROF.OAK\nvia PC to get\nyour POKEDEX\nevaluated!" },  /* TEXT_VIRIDIANFOREST_TRAINER_TIPS2 */
    {   8,  49, "TRAINER TIPS\fNo stealing of\nPOKEMON from\nother trainers!\nCatch only wild\nPOKEMON!" },  /* TEXT_VIRIDIANFOREST_TRAINER_TIPS3 */
    {  36,  91, "TRAINER TIPS\fWeaken POKEMON\nbefore attempting\ncapture!\fWhen healthy,\nthey may escape!" },  /* TEXT_VIRIDIANFOREST_TRAINER_TIPS4 */
    {   4,   3, "LEAVING\nVIRIDIAN FOREST\nPEWTER CITY AHEAD" },  /* TEXT_VIRIDIANFOREST_LEAVING_SIGN */
};

static const item_event_t kItems_ViridianForest[] = {
    {  50,  23, 0x0b },  /* ANTIDOTE */
    {  24,  59, 0x14 },  /* POTION */
    {   2,  63, 0x04 },  /* POKE_BALL */
};

static const map_warp_t kWarps_Museum1F[] = {
    {  20,  15, 0xff, 0 },  /* LAST_MAP */
    {  22,  15, 0xff, 0 },  /* LAST_MAP */
    {  32,  15, 0xff, 1 },  /* LAST_MAP */
    {  34,  15, 0xff, 1 },  /* LAST_MAP */
    {  14,  15, 0x35, 0 },  /* MUSEUM_2F */
};

static const npc_event_t kNpcs_Museum1F[] = {
    {  24,   9, 0x20, 0, "Come again!", NULL },  /* SPRITE_SCIENTIST, STAY, TEXT_MUSEUM1F_SCIENTIST1 */
    {   2,   9, 0x0b, 0, "That is one\nmagnificent\nfossil!", NULL },  /* SPRITE_GAMBLER, STAY, TEXT_MUSEUM1F_GAMBLER */
    {  30,   5, 0x20, 0, "Ssh! I think that\nthis chunk of\nAMBER contains\nPOKEMON DNA!\fIt would be great\nif POKEMON could\nbe resurrected\nfrom it!\fBut, my colleagues\njust ignore me!\fSo I have a favor\nto ask!\fTake this to a\nPOKEMON LAB and\nget it examined!", NULL },  /* SPRITE_SCIENTIST, STAY, TEXT_MUSEUM1F_SCIENTIST2 */
    {  34,   9, 0x20, 0, "We are proud of 2\nfossils of very\nrare, prehistoric\nPOKEMON!", NULL },  /* SPRITE_SCIENTIST, STAY, TEXT_MUSEUM1F_SCIENTIST3 */
    {  32,   5, 0x45, 0, "The AMBER is\nclear and gold!", NULL },  /* SPRITE_OLD_AMBER, STAY, TEXT_MUSEUM1F_OLD_AMBER */
};

static const map_warp_t kWarps_Museum2F[] = {
    {  14,  15, 0x34, 4 },  /* MUSEUM_1F */
};

static const npc_event_t kNpcs_Museum2F[] = {
    {   2,  15, 0x04, 1, "MOON STONE?\fWhat's so special\nabout it?", NULL },  /* SPRITE_YOUNGSTER, WALK, TEXT_MUSEUM2F_YOUNGSTER */
    {   0,  11, 0x25, 0, "July 20, 1969!\fThe 1st lunar\nlanding!\fI bought a color\nTV to watch it!", NULL },  /* SPRITE_GRAMPS, STAY, TEXT_MUSEUM2F_GRAMPS */
    {  14,  11, 0x20, 0, "We have a space\nexhibit now.", NULL },  /* SPRITE_SCIENTIST, STAY, TEXT_MUSEUM2F_SCIENTIST */
    {  22,  11, 0x1d, 0, "I want a PIKACHU!\nIt's so cute!\fI asked my Daddy\nto catch me one!", NULL },  /* SPRITE_BRUNETTE_GIRL, STAY, TEXT_MUSEUM2F_BRUNETTE_GIRL */
    {  24,  11, 0x0e, 0, "Yeah, a PIKACHU\nsoon, I promise!", NULL },  /* SPRITE_HIKER, STAY, TEXT_MUSEUM2F_HIKER */
};

static const sign_event_t kSigns_Museum2F[] = {
    {  22,   5, "SPACE SHUTTLE\nCOLUMBIA" },  /* TEXT_MUSEUM2F_SPACE_SHUTTLE_SIGN */
    {   4,  11, "Meteorite that\nfell on MT.MOON.\n(MOON STONE?)" },  /* TEXT_MUSEUM2F_MOON_STONE_SIGN */
};

static const map_warp_t kWarps_PewterGym[] = {
    {   8,  27, 0xff, 2 },  /* LAST_MAP */
    {  10,  27, 0xff, 2 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_PewterGym[] = {
    {   8,   3, 0x0c, 0, "I'm BROCK!\nI'm PEWTER's GYM\nLEADER!\fI believe in rock\nhard defense and\ndetermination!\fThat's why my\nPOKEMON are all\nthe rock-type!\fDo you still want\nto challenge me?\nFine then! Show\nme your best!", NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_PEWTERGYM_BROCK */
    {   6,  13, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_PEWTERGYM_COOLTRAINER_M */
    {  14,  21, 0x24, 0, NULL, NULL },  /* SPRITE_GYM_GUIDE, STAY, TEXT_PEWTERGYM_GYM_GUIDE */
};

static const map_warp_t kWarps_PewterNidoranHouse[] = {
    {   4,  15, 0xff, 3 },  /* LAST_MAP */
    {   6,  15, 0xff, 3 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_PewterNidoranHouse[] = {
    {   8,  11, 0x05, 0, "NIDORAN: Bowbow!@", NULL },  /* SPRITE_MONSTER, STAY, TEXT_PEWTERNIDORANHOUSE_NIDORAN */
    {   6,  11, 0x35, 0, "NIDORAN sit!", NULL },  /* SPRITE_LITTLE_BOY, STAY, TEXT_PEWTERNIDORANHOUSE_LITTLE_BOY */
    {   2,   5, 0x0a, 0, "Our POKEMON's an\noutsider, so it's\nhard to handle.\fAn outsider is a\nPOKEMON that you\nget in a trade.\fIt grows fast, but\nit may ignore an\nunskilled trainer\nin battle!\fIf only we had\nsome BADGEs...", NULL },  /* SPRITE_MIDDLE_AGED_MAN, STAY, TEXT_PEWTERNIDORANHOUSE_MIDDLE_AGED_MAN */
};

static const map_warp_t kWarps_PewterMart[] = {
    {   6,  15, 0xff, 4 },  /* LAST_MAP */
    {   8,  15, 0xff, 4 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_PewterMart[] = {
    {   0,  11, 0x26, 0, NULL, PewterMart_Start },  /* SPRITE_CLERK, STAY, TEXT_PEWTERMART_CLERK */
    {   6,   7, 0x04, 1, "A shady, old man\ngot me to buy\nthis really weird\nfish POKEMON!\fIt's totally weak\nand it cost ¥500!", NULL },  /* SPRITE_YOUNGSTER, WALK, TEXT_PEWTERMART_YOUNGSTER */
    {  10,  11, 0x0c, 0, "Good things can\nhappen if you\nraise POKEMON\ndiligently, even\nthe weak ones!", NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_PEWTERMART_SUPER_NERD */
};

static const map_warp_t kWarps_PewterSpeechHouse[] = {
    {   4,  15, 0xff, 5 },  /* LAST_MAP */
    {   6,  15, 0xff, 5 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_PewterSpeechHouse[] = {
    {   4,   7, 0x0b, 0, "POKEMON learn new\ntechniques as\nthey grow!\fBut, some moves\nmust be taught by\nthe trainer!", NULL },  /* SPRITE_GAMBLER, STAY, TEXT_PEWTERSPEECHHOUSE_GAMBLER */
    {   8,  11, 0x04, 0, "POKEMON become\neasier to catch\nwhen they are\nhurt or asleep!\fBut, it's not a\nsure thing!", NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_PEWTERSPEECHHOUSE_YOUNGSTER */
};

static const map_warp_t kWarps_PewterPokecenter[] = {
    {   6,  15, 0xff, 6 },  /* LAST_MAP */
    {   8,  15, 0xff, 6 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_PewterPokecenter[] = {
    {   6,   3, 0x29, 0, NULL, Pokecenter_Start },  /* SPRITE_NURSE, STAY, TEXT_PEWTERPOKECENTER_NURSE */
    {  22,  15, 0x10, 0, "What!?\fTEAM ROCKET is\nat MT.MOON? Huh?\nI'm on the phone!\fScram!", NULL },  /* SPRITE_GENTLEMAN, STAY, TEXT_PEWTERPOKECENTER_GENTLEMAN */
    {   2,   7, 0x38, 0, "JIGGLYPUFF: Puu\npupuu!", NULL },  /* SPRITE_FAIRY, STAY, TEXT_PEWTERPOKECENTER_JIGGLYPUFF */
    {  22,   5, 0x2a, 0, NULL, NULL },  /* SPRITE_LINK_RECEPTIONIST, STAY, TEXT_PEWTERPOKECENTER_LINK_RECEPTIONIST */
};

static const map_warp_t kWarps_MtMoon1F[] = {
    {  28,  71, 0xff, 1 },  /* LAST_MAP */
    {  30,  71, 0xff, 1 },  /* LAST_MAP */
    {  10,  11, 0x3c, 0 },  /* MT_MOON_B1F */
    {  34,  23, 0x3c, 2 },  /* MT_MOON_B1F */
    {  50,  31, 0x3c, 3 },  /* MT_MOON_B1F */
};

static const npc_event_t kNpcs_MtMoon1F[] = {
    {  10,  13, 0x0e, 0, NULL, NULL },  /* SPRITE_HIKER, STAY, TEXT_MTMOON1F_HIKER */
    {  24,  33, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_MTMOON1F_YOUNGSTER1 */
    {  60,   9, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_MTMOON1F_COOLTRAINER_F1 */
    {  48,  63, 0x0c, 0, NULL, NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_MTMOON1F_SUPER_NERD */
    {  32,  47, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_MTMOON1F_COOLTRAINER_F2 */
    {  14,  45, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_MTMOON1F_YOUNGSTER2 */
    {  60,  55, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_MTMOON1F_YOUNGSTER3 */
};

static const sign_event_t kSigns_MtMoon1F[] = {
    {  30,  47, "Beware! ZUBAT is\na blood sucker!" },  /* TEXT_MTMOON1F_BEWARE_ZUBAT_SIGN */
};

static const item_event_t kItems_MtMoon1F[] = {
    {   4,  41, 0x14 },  /* POTION */
    {   4,   5, 0x0a },  /* MOON_STONE */
    {  70,  63, 0x28 },  /* RARE_CANDY */
    {  72,  47, 0x1d },  /* ESCAPE_ROPE */
    {  40,  67, 0x14 },  /* POTION */
    {  10,  65, 0xd4 },  /* TM_WATER_GUN */
};

static const map_warp_t kWarps_MtMoonB1F[] = {
    {  10,  11, 0x3b, 2 },  /* MT_MOON_1F */
    {  34,  23, 0x3d, 0 },  /* MT_MOON_B2F */
    {  50,  19, 0x3b, 3 },  /* MT_MOON_1F */
    {  50,  31, 0x3b, 4 },  /* MT_MOON_1F */
    {  42,  35, 0x3d, 1 },  /* MT_MOON_B2F */
    {  26,  55, 0x3d, 2 },  /* MT_MOON_B2F */
    {  46,   7, 0x3d, 3 },  /* MT_MOON_B2F */
    {  54,   7, 0xff, 2 },  /* LAST_MAP */
};

static const map_warp_t kWarps_MtMoonB2F[] = {
    {  50,  19, 0x3c, 1 },  /* MT_MOON_B1F */
    {  42,  35, 0x3c, 4 },  /* MT_MOON_B1F */
    {  30,  55, 0x3c, 5 },  /* MT_MOON_B1F */
    {  10,  15, 0x3c, 6 },  /* MT_MOON_B1F */
};

static const npc_event_t kNpcs_MtMoonB2F[] = {
    {  24,  17, 0x0c, 0, NULL, NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_MTMOONB2F_SUPER_NERD */
    {  22,  33, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_MTMOONB2F_ROCKET1 */
    {  30,  45, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_MTMOONB2F_ROCKET2 */
    {  58,  23, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_MTMOONB2F_ROCKET3 */
    {  58,  35, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_MTMOONB2F_ROCKET4 */
    {  24,  13, 0x3e, 0, "You want the\nDOME FOSSIL?", NULL },  /* SPRITE_FOSSIL, STAY, TEXT_MTMOONB2F_DOME_FOSSIL */
    {  26,  13, 0x3e, 0, "You want the\nHELIX FOSSIL?", NULL },  /* SPRITE_FOSSIL, STAY, TEXT_MTMOONB2F_HELIX_FOSSIL */
};

static const item_event_t kItems_MtMoonB2F[] = {
    {  50,  43, 0x23 },  /* HP_UP */
    {  58,  11, 0xc9 },  /* TM_MEGA_PUNCH */
};

static const map_warp_t kWarps_CeruleanTrashedHouse[] = {
    {   4,  15, 0xff, 0 },  /* LAST_MAP */
    {   6,  15, 0xff, 0 },  /* LAST_MAP */
    {   6,   1, 0xff, 7 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_CeruleanTrashedHouse[] = {
    {   4,   3, 0x27, 0, "Those miserable\nROCKETs!\fLook what they\ndid here!\fThey stole a TM\nfor teaching\nPOKEMON how to\nDIG holes!\fThat cost me a\nbundle, it did!", NULL },  /* SPRITE_FISHING_GURU, STAY, TEXT_CERULEANTRASHEDHOUSE_FISHING_GURU */
    {  10,  13, 0x0d, 1, "TEAM ROCKET must\nbe trying to DIG\ntheir way into no\ngood!", NULL },  /* SPRITE_GIRL, WALK, TEXT_CERULEANTRASHEDHOUSE_GIRL */
};

static const sign_event_t kSigns_CeruleanTrashedHouse[] = {
    {   6,   1, "TEAM ROCKET left\na way out!" },  /* TEXT_CERULEANTRASHEDHOUSE_WALL_HOLE */
};

static const map_warp_t kWarps_CeruleanTradeHouse[] = {
    {   4,  15, 0xff, 1 },  /* LAST_MAP */
    {   6,  15, 0xff, 1 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_CeruleanTradeHouse[] = {
    {  10,   9, 0x28, 0, "My husband likes\ntrading POKEMON.\fIf you are a\ncollector, would\nyou please trade\nwith him?", NULL },  /* SPRITE_GRANNY, STAY, TEXT_CERULEANTRADEHOUSE_GRANNY */
    {   2,   5, 0x0b, 0, NULL, NULL },  /* SPRITE_GAMBLER, STAY, TEXT_CERULEANTRADEHOUSE_GAMBLER */
};

static const map_warp_t kWarps_CeruleanPokecenter[] = {
    {   6,  15, 0xff, 2 },  /* LAST_MAP */
    {   8,  15, 0xff, 2 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_CeruleanPokecenter[] = {
    {   6,   3, 0x29, 0, NULL, Pokecenter_Start },  /* SPRITE_NURSE, STAY, TEXT_CERULEANPOKECENTER_NURSE */
    {  20,  11, 0x0c, 1, "That BILL!\fI heard that\nhe'll do whatever\nit takes to get\nrare POKEMON!", NULL },  /* SPRITE_SUPER_NERD, WALK, TEXT_CERULEANPOKECENTER_SUPER_NERD */
    {   8,   7, 0x10, 0, "Have you heard\nabout BILL?\fEveryone calls\nhim a POKEMANIAC!\fI think people\nare just jealous\nof BILL, though.\fWho wouldn't want\nto boast about\ntheir POKEMON?", NULL },  /* SPRITE_GENTLEMAN, STAY, TEXT_CERULEANPOKECENTER_GENTLEMAN */
    {  22,   5, 0x2a, 0, NULL, NULL },  /* SPRITE_LINK_RECEPTIONIST, STAY, TEXT_CERULEANPOKECENTER_LINK_RECEPTIONIST */
};

static const map_warp_t kWarps_CeruleanGym[] = {
    {   8,  27, 0xff, 3 },  /* LAST_MAP */
    {  10,  27, 0xff, 3 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_CeruleanGym[] = {
    {   8,   5, 0x1d, 0, "Hi, you're a new\nface!\fTrainers who want\nto turn pro have\nto have a policy\nabout POKEMON!\fWhat is your\napproach when you\ncatch POKEMON?\fMy policy is an\nall-out offensive\nwith water-type\nPOKEMON!", NULL },  /* SPRITE_BRUNETTE_GIRL, STAY, TEXT_CERULEANGYM_MISTY */
    {   4,   7, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_CERULEANGYM_COOLTRAINER_F */
    {  16,  15, 0x22, 0, NULL, NULL },  /* SPRITE_SWIMMER, STAY, TEXT_CERULEANGYM_SWIMMER */
    {  14,  21, 0x24, 0, "Yo! Champ in\nmaking!\fHere's my advice!\fThe LEADER, MISTY,\nis a pro who uses\nwater POKEMON!\fYou can drain all\ntheir water with\nplant POKEMON!\fOr, zap them with\nelectricity!", NULL },  /* SPRITE_GYM_GUIDE, STAY, TEXT_CERULEANGYM_GYM_GUIDE */
};

static const map_warp_t kWarps_BikeShop[] = {
    {   4,  15, 0xff, 4 },  /* LAST_MAP */
    {   6,  15, 0xff, 4 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_BikeShop[] = {
    {  12,   5, 0x15, 0, NULL, NULL },  /* SPRITE_BIKE_SHOP_CLERK, STAY, TEXT_BIKESHOP_CLERK */
    {  10,  13, 0x1c, 1, "A plain city BIKE\nis good enough\nfor me!\fYou can't put a\nshopping basket\non an MTB!", NULL },  /* SPRITE_MIDDLE_AGED_WOMAN, WALK, TEXT_BIKESHOP_MIDDLE_AGED_WOMAN */
    {   2,   7, 0x04, 0, "These BIKEs are\ncool, but they're\nway expensive!", NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_BIKESHOP_YOUNGSTER */
};

static const map_warp_t kWarps_CeruleanMart[] = {
    {   6,  15, 0xff, 5 },  /* LAST_MAP */
    {   8,  15, 0xff, 5 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_CeruleanMart[] = {
    {   0,  11, 0x26, 0, NULL, CeruleanMart_Start },  /* SPRITE_CLERK, STAY, TEXT_CERULEANMART_CLERK */
    {   6,   9, 0x07, 1, "Use REPEL to keep\nbugs and weak\nPOKEMON away.\fPut your strongest\nPOKEMON at the\ntop of the list\nfor best results!", NULL },  /* SPRITE_COOLTRAINER_M, WALK, TEXT_CERULEANMART_COOLTRAINER_M */
    {  12,   5, 0x06, 1, "Have you seen any\nRARE CANDY?\fIt's supposed to\nmake POKEMON go\nup one level!", NULL },  /* SPRITE_COOLTRAINER_F, WALK, TEXT_CERULEANMART_COOLTRAINER_F */
};

static const map_warp_t kWarps_MtMoonPokecenter[] = {
    {   6,  15, 0xff, 0 },  /* LAST_MAP */
    {   8,  15, 0xff, 0 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_MtMoonPokecenter[] = {
    {   6,   3, 0x29, 0, NULL, Pokecenter_Start },  /* SPRITE_NURSE, STAY, TEXT_MTMOONPOKECENTER_NURSE */
    {   8,   7, 0x04, 0, "I've 6 POKE BALLs\nset in my belt.\fAt most, you can\ncarry 6 POKEMON.", NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_MTMOONPOKECENTER_YOUNGSTER */
    {  14,   7, 0x10, 0, "TEAM ROCKET\nattacks CERULEAN\ncitizens...\fTEAM ROCKET is\nalways in the\nnews!", NULL },  /* SPRITE_GENTLEMAN, STAY, TEXT_MTMOONPOKECENTER_GENTLEMAN */
    {  20,  13, 0x0a, 1, "MAN: Hello, there!\nHave I got a deal\njust for you!\fI'll let you have\na swell MAGIKARP\nfor just ¥500!\nWhat do you say?", NULL },  /* SPRITE_MIDDLE_AGED_MAN, WALK, TEXT_MTMOONPOKECENTER_MAGIKARP_SALESMAN */
    {  14,   5, 0x42, 0, NULL, NULL },  /* SPRITE_CLIPBOARD, STAY, TEXT_MTMOONPOKECENTER_CLIPBOARD */
    {  22,   5, 0x2a, 0, NULL, NULL },  /* SPRITE_LINK_RECEPTIONIST, STAY, TEXT_MTMOONPOKECENTER_LINK_RECEPTIONIST */
};

static const map_warp_t kWarps_Route5Gate[] = {
    {   6,  11, 0xff, 2 },  /* LAST_MAP */
    {   8,  11, 0xff, 2 },  /* LAST_MAP */
    {   6,   1, 0xff, 1 },  /* LAST_MAP */
    {   8,   1, 0xff, 0 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_Route5Gate[] = {
    {   2,   7, 0x31, 0, NULL, NULL },  /* SPRITE_GUARD, STAY, TEXT_ROUTE5GATE_GUARD */
};

static const map_warp_t kWarps_UndergroundPathRoute5[] = {
    {   6,  15, 0xff, 3 },  /* LAST_MAP */
    {   8,  15, 0xff, 3 },  /* LAST_MAP */
    {   8,   9, 0x77, 0 },  /* UNDERGROUND_PATH_NORTH_SOUTH */
};

static const npc_event_t kNpcs_UndergroundPathRoute5[] = {
    {   4,   7, 0x08, 0, NULL, NULL },  /* SPRITE_LITTLE_GIRL, STAY, TEXT_UNDERGROUNDPATHROUTE5_LITTLE_GIRL */
};

static const map_warp_t kWarps_Daycare[] = {
    {   4,  15, 0xff, 4 },  /* LAST_MAP */
    {   6,  15, 0xff, 4 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_Daycare[] = {
    {   4,   7, 0x10, 0, "I run a DAYCARE.\nWould you like me\nto raise one of\nyour POKEMON?", NULL },  /* SPRITE_GENTLEMAN, STAY, TEXT_DAYCARE_GENTLEMAN */
};

static const map_warp_t kWarps_Route6Gate[] = {
    {   6,  11, 0xff, 2 },  /* LAST_MAP */
    {   8,  11, 0xff, 2 },  /* LAST_MAP */
    {   6,   1, 0xff, 1 },  /* LAST_MAP */
    {   8,   1, 0xff, 1 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_Route6Gate[] = {
    {  12,   5, 0x31, 0, NULL, NULL },  /* SPRITE_GUARD, STAY, TEXT_ROUTE6GATE_GUARD */
};

static const map_warp_t kWarps_UndergroundPathRoute6[] = {
    {   6,  15, 0xff, 3 },  /* LAST_MAP */
    {   8,  15, 0xff, 3 },  /* LAST_MAP */
    {   8,   9, 0x77, 1 },  /* UNDERGROUND_PATH_NORTH_SOUTH */
};

static const npc_event_t kNpcs_UndergroundPathRoute6[] = {
    {   4,   7, 0x0d, 0, "People often lose\nthings in that\nUNDERGROUND PATH.", NULL },  /* SPRITE_GIRL, STAY, TEXT_UNDERGROUNDPATHROUTE6_GIRL */
};

static const map_warp_t kWarps_Route7Gate[] = {
    {   0,   7, 0xff, 3 },  /* LAST_MAP */
    {   0,   9, 0xff, 3 },  /* LAST_MAP */
    {  10,   7, 0xff, 0 },  /* LAST_MAP */
    {  10,   9, 0xff, 1 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_Route7Gate[] = {
    {   6,   3, 0x31, 0, NULL, NULL },  /* SPRITE_GUARD, STAY, TEXT_ROUTE7GATE_GUARD */
};

static const map_warp_t kWarps_UndergroundPathRoute7[] = {
    {   6,  15, 0xff, 4 },  /* LAST_MAP */
    {   8,  15, 0xff, 4 },  /* LAST_MAP */
    {   8,   9, 0x79, 0 },  /* UNDERGROUND_PATH_WEST_EAST */
};

static const npc_event_t kNpcs_UndergroundPathRoute7[] = {
    {   4,   9, 0x0a, 0, "I heard a sleepy\nPOKEMON appeared\nnear CELADON CITY.", NULL },  /* SPRITE_MIDDLE_AGED_MAN, STAY, TEXT_UNDERGROUNDPATHROUTE7_MIDDLE_AGED_MAN */
};

static const map_warp_t kWarps_UndergroundPathRoute7Copy[] = {
    {   6,  15, 0xff, 5 },  /* LAST_MAP */
    {   8,  15, 0xff, 5 },  /* LAST_MAP */
    {   8,   9, 0x79, 0 },  /* UNDERGROUND_PATH_WEST_EAST */
};

static const npc_event_t kNpcs_UndergroundPathRoute7Copy[] = {
    {   6,   5, 0x0d, 0, "I want to shop at\nthe dept. store\nin CELADON but...\fThere are so many\nrough looking\npeople there.", NULL },  /* SPRITE_GIRL, STAY, TEXT_UNDERGROUNDPATHROUTE7COPY_UNUSED_GIRL */
    {   4,   9, 0x0a, 0, "You're here to\nshop in CELADON?\fJust step outside\nand head west!", NULL },  /* SPRITE_MIDDLE_AGED_MAN, STAY, TEXT_UNDERGROUNDPATHROUTE7COPY_UNUSED_MIDDLE_AGED_MAN */
};

static const map_warp_t kWarps_Route8Gate[] = {
    {   0,   7, 0xff, 0 },  /* LAST_MAP */
    {   0,   9, 0xff, 1 },  /* LAST_MAP */
    {  10,   7, 0xff, 2 },  /* LAST_MAP */
    {  10,   9, 0xff, 3 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_Route8Gate[] = {
    {   4,   3, 0x31, 0, NULL, NULL },  /* SPRITE_GUARD, STAY, TEXT_ROUTE8GATE_GUARD */
};

static const map_warp_t kWarps_UndergroundPathRoute8[] = {
    {   6,  15, 0xff, 4 },  /* LAST_MAP */
    {   8,  15, 0xff, 4 },  /* LAST_MAP */
    {   8,   9, 0x79, 1 },  /* UNDERGROUND_PATH_WEST_EAST */
};

static const npc_event_t kNpcs_UndergroundPathRoute8[] = {
    {   6,   9, 0x0d, 0, "The dept. store\nin CELADON has a\ngreat selection!", NULL },  /* SPRITE_GIRL, STAY, TEXT_UNDERGROUNDPATHROUTE8_GIRL */
};

static const map_warp_t kWarps_RockTunnelPokecenter[] = {
    {   6,  15, 0xff, 0 },  /* LAST_MAP */
    {   8,  15, 0xff, 0 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_RockTunnelPokecenter[] = {
    {   6,   3, 0x29, 0, NULL, Pokecenter_Start },  /* SPRITE_NURSE, STAY, TEXT_ROCKTUNNELPOKECENTER_NURSE */
    {  14,   7, 0x10, 1, "The element types\nof POKEMON make\nthem stronger\nthan some types\nand weaker than\nothers!", NULL },  /* SPRITE_GENTLEMAN, WALK, TEXT_ROCKTUNNELPOKECENTER_GENTLEMAN */
    {   4,  11, 0x2f, 0, "I sold a useless\nNUGGET for ¥5000!", NULL },  /* SPRITE_FISHER, STAY, TEXT_ROCKTUNNELPOKECENTER_FISHER */
    {  22,   5, 0x2a, 0, NULL, NULL },  /* SPRITE_LINK_RECEPTIONIST, STAY, TEXT_ROCKTUNNELPOKECENTER_LINK_RECEPTIONIST */
};

static const map_warp_t kWarps_RockTunnel1F[] = {
    {  30,   7, 0xff, 1 },  /* LAST_MAP */
    {  30,   1, 0xff, 1 },  /* LAST_MAP */
    {  30,  67, 0xff, 2 },  /* LAST_MAP */
    {  30,  71, 0xff, 2 },  /* LAST_MAP */
    {  74,   7, 0xe8, 0 },  /* ROCK_TUNNEL_B1F */
    {  10,   7, 0xe8, 1 },  /* ROCK_TUNNEL_B1F */
    {  34,  23, 0xe8, 2 },  /* ROCK_TUNNEL_B1F */
    {  74,  35, 0xe8, 3 },  /* ROCK_TUNNEL_B1F */
};

static const npc_event_t kNpcs_RockTunnel1F[] = {
    {  14,  11, 0x0e, 0, NULL, NULL },  /* SPRITE_HIKER, STAY, TEXT_ROCKTUNNEL1F_HIKER1 */
    {  10,  33, 0x0e, 0, NULL, NULL },  /* SPRITE_HIKER, STAY, TEXT_ROCKTUNNEL1F_HIKER2 */
    {  34,  31, 0x0e, 0, NULL, NULL },  /* SPRITE_HIKER, STAY, TEXT_ROCKTUNNEL1F_HIKER3 */
    {  46,  17, 0x0c, 0, NULL, NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_ROCKTUNNEL1F_SUPER_NERD */
    {  74,  43, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROCKTUNNEL1F_COOLTRAINER_F1 */
    {  44,  49, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROCKTUNNEL1F_COOLTRAINER_F2 */
    {  64,  49, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROCKTUNNEL1F_COOLTRAINER_F3 */
};

static const sign_event_t kSigns_RockTunnel1F[] = {
    {  22,  59, "ROCK TUNNEL\nCERULEAN CITY -\nLAVENDER TOWN" },  /* TEXT_ROCKTUNNEL1F_SIGN */
};

static const map_warp_t kWarps_PowerPlant[] = {
    {   8,  71, 0xff, 3 },  /* LAST_MAP */
    {  10,  71, 0xff, 3 },  /* LAST_MAP */
    {   0,  23, 0xff, 3 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_PowerPlant[] = {
    {   8,  19, 0x09, 0, NULL, NULL },  /* SPRITE_BIRD, STAY, TEXT_POWERPLANT_ZAPDOS */
};

static const item_event_t kItems_PowerPlant[] = {
    {  18,  41, 0x00 },  /* VOLTORB */
    {  64,  37, 0x00 },  /* VOLTORB */
    {  42,  51, 0x00 },  /* VOLTORB */
    {  50,  37, 0x00 },  /* ELECTRODE */
    {  46,  69, 0x00 },  /* VOLTORB */
    {  52,  57, 0x00 },  /* VOLTORB */
    {  42,  29, 0x00 },  /* ELECTRODE */
    {  74,  65, 0x00 },  /* VOLTORB */
    {  14,  51, 0x26 },  /* CARBOS */
    {  56,   7, 0x23 },  /* HP_UP */
    {  68,   7, 0x28 },  /* RARE_CANDY */
    {  52,  65, 0xe1 },  /* TM_THUNDER */
    {  40,  65, 0xe9 },  /* TM_REFLECT */
};

static const map_warp_t kWarps_Route11Gate1F[] = {
    {   0,   9, 0xff, 0 },  /* LAST_MAP */
    {   0,  11, 0xff, 1 },  /* LAST_MAP */
    {  14,   9, 0xff, 2 },  /* LAST_MAP */
    {  14,  11, 0xff, 3 },  /* LAST_MAP */
    {  12,  17, 0x56, 0 },  /* ROUTE_11_GATE_2F */
};

static const npc_event_t kNpcs_Route11Gate1F[] = {
    {   8,   3, 0x31, 0, "When you catch\nlots of POKEMON,\nisn't it hard to\nthink up names?\fIn LAVENDER TOWN,\nthere's a man who\nrates POKEMON\nnicknames.\fHe'll help you\nrename them too!", NULL },  /* SPRITE_GUARD, STAY, TEXT_ROUTE11GATE1F_GUARD */
};

static const map_warp_t kWarps_DiglettsCaveRoute11[] = {
    {   4,  15, 0xff, 4 },  /* LAST_MAP */
    {   6,  15, 0xff, 4 },  /* LAST_MAP */
    {   8,   9, 0xc5, 1 },  /* DIGLETTS_CAVE */
};

static const npc_event_t kNpcs_DiglettsCaveRoute11[] = {
    {   4,   7, 0x0b, 0, "What a surprise!\nDIGLETTs dug this\nlong tunnel!\fIt goes right to\nVIRIDIAN CITY!", NULL },  /* SPRITE_GAMBLER, STAY, TEXT_DIGLETTSCAVEROUTE11_GAMBLER */
};

static const map_warp_t kWarps_Route11Gate2F[] = {
    {  14,  15, 0x54, 4 },  /* ROUTE_11_GATE_1F */
};

static const npc_event_t kNpcs_Route11Gate2F[] = {
    {   8,   5, 0x04, 1, NULL, NULL },  /* SPRITE_YOUNGSTER, WALK, TEXT_ROUTE11GATE2F_YOUNGSTER */
    {   4,  13, 0x20, 0, "There are items on\nthe ground that\ncan't be seen.\fITEMFINDER will\ndetect an item\nclose to you.\fIt can't pinpoint\nit, so you have\nto look yourself!", NULL },  /* SPRITE_SCIENTIST, STAY, TEXT_ROUTE11GATE2F_OAKS_AIDE */
};

static const sign_event_t kSigns_Route11Gate2F[] = {
    {   2,   5, "Looked into the\nbinoculars.\fA big POKEMON is\nasleep on a road!" },  /* TEXT_ROUTE11GATE2F_LEFT_BINOCULARS */
    {  12,   5, "Looked into the\nbinoculars.\fThe only way to\nget from CERULEAN\nCITY to LAVENDER\nis by way of the\nROCK TUNNEL." },  /* TEXT_ROUTE11GATE2F_RIGHT_BINOCULARS */
};

static const map_warp_t kWarps_Route12Gate1F[] = {
    {   8,   1, 0xff, 0 },  /* LAST_MAP */
    {  10,   1, 0xff, 1 },  /* LAST_MAP */
    {   8,  15, 0xff, 2 },  /* LAST_MAP */
    {  10,  15, 0xff, 2 },  /* LAST_MAP */
    {  16,  13, 0xc3, 0 },  /* ROUTE_12_GATE_2F */
};

static const npc_event_t kNpcs_Route12Gate1F[] = {
    {   2,   7, 0x31, 0, "There's a lookout\nspot upstairs.", NULL },  /* SPRITE_GUARD, STAY, TEXT_ROUTE12GATE1F_GUARD */
};

static const map_warp_t kWarps_BillsHouse[] = {
    {   4,  15, 0xff, 0 },  /* LAST_MAP */
    {   6,  15, 0xff, 0 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_BillsHouse[] = {
    {  12,  11, 0x05, 0, "Hiya! I'm a\nPOKEMON...\n...No I'm not!\fCall me BILL!\nI'm a true blue\nPOKEMANIAC! Hey!\nWhat's with that\nskeptical look?\fI'm not joshing\nyou, I screwed up\nan experiment and\ngot combined with\na POKEMON!\fSo, how about it?\nHelp me out here!", NULL },  /* SPRITE_MONSTER, STAY, TEXT_BILLSHOUSE_BILL_POKEMON */
    {   8,   9, 0x0c, 0, "BILL: Yeehah!\nThanks, bud! I\nowe you one!\fSo, did you come\nto see my POKEMON\ncollection?\nYou didn't?\nThat's a bummer.\fI've got to thank\nyou... Oh here,\nmaybe this'll do.", NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_BILLSHOUSE_BILL_SS_TICKET */
    {  12,  11, 0x0c, 0, "BILL: Look, bud,\njust check out\nsome of my rare\nPOKEMON on my PC!", NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_BILLSHOUSE_BILL_CHECK_OUT_MY_RARE_POKEMON */
};

static const map_warp_t kWarps_VermilionPokecenter[] = {
    {   6,  15, 0xff, 0 },  /* LAST_MAP */
    {   8,  15, 0xff, 0 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_VermilionPokecenter[] = {
    {   6,   3, 0x29, 0, NULL, Pokecenter_Start },  /* SPRITE_NURSE, STAY, TEXT_VERMILIONPOKECENTER_NURSE */
    {  20,  11, 0x27, 0, "Even if they are\nthe same level,\nPOKEMON can have\nvery different\nabilities.\fA POKEMON raised\nby a trainer is\nstronger than one\nin the wild.", NULL },  /* SPRITE_FISHING_GURU, STAY, TEXT_VERMILIONPOKECENTER_FISHING_GURU */
    {  10,   9, 0x13, 0, "My POKEMON was\npoisoned! It\nfainted while we\nwere walking!", NULL },  /* SPRITE_SAILOR, STAY, TEXT_VERMILIONPOKECENTER_SAILOR */
    {  22,   5, 0x2a, 0, NULL, NULL },  /* SPRITE_LINK_RECEPTIONIST, STAY, TEXT_VERMILIONPOKECENTER_LINK_RECEPTIONIST */
};

static const map_warp_t kWarps_PokemonFanClub[] = {
    {   4,  15, 0xff, 1 },  /* LAST_MAP */
    {   6,  15, 0xff, 1 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_PokemonFanClub[] = {
    {  12,   7, 0x2f, 0, "Won't you admire\nmy PIKACHU's\nadorable tail?", NULL },  /* SPRITE_FISHER, STAY, TEXT_POKEMONFANCLUB_PIKACHU_FAN */
    {   2,   7, 0x0d, 0, "I just love my\nSEEL!\fIt squeals when I\nhug it!", NULL },  /* SPRITE_GIRL, STAY, TEXT_POKEMONFANCLUB_SEEL_FAN */
    {  12,   9, 0x38, 0, "PIKACHU: Chu!\nPikachu!", NULL },  /* SPRITE_FAIRY, STAY, TEXT_POKEMONFANCLUB_PIKACHU */
    {   2,   9, 0x3c, 0, "SEEL: Kyuoo!", NULL },  /* SPRITE_SEEL, STAY, TEXT_POKEMONFANCLUB_SEEL */
    {   6,   3, 0x10, 0, "I chair the\nPOKEMON Fan Club!\fI have collected\nover 100 POKEMON!\fI'm very fussy\nwhen it comes to\nPOKEMON!\fSo...\fDid you come\nvisit to hear\nabout my POKEMON?", NULL },  /* SPRITE_GENTLEMAN, STAY, TEXT_POKEMONFANCLUB_CHAIRMAN */
    {  10,   3, 0x2a, 0, "Our Chairman is\nvery vocal about\nPOKEMON.", NULL },  /* SPRITE_LINK_RECEPTIONIST, STAY, TEXT_POKEMONFANCLUB_RECEPTIONIST */
};

static const sign_event_t kSigns_PokemonFanClub[] = {
    {   2,   1, "Let's all listen\npolitely to other\ntrainers!" },  /* TEXT_POKEMONFANCLUB_SIGN_1 */
    {  12,   1, "If someone brags,\nbrag right back!" },  /* TEXT_POKEMONFANCLUB_SIGN_2 */
};

static const map_warp_t kWarps_VermilionMart[] = {
    {   6,  15, 0xff, 2 },  /* LAST_MAP */
    {   8,  15, 0xff, 2 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_VermilionMart[] = {
    {   0,  11, 0x26, 0, NULL, VermilionMart_Start },  /* SPRITE_CLERK, STAY, TEXT_VERMILIONMART_CLERK */
    {  10,  13, 0x07, 0, "There are evil\npeople who will\nuse POKEMON for\ncriminal acts.\fTEAM ROCKET\ntraffics in rare\nPOKEMON.\fThey also abandon\nPOKEMON that they\nconsider not to\nbe popular or\nuseful.", NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_VERMILIONMART_COOLTRAINER_M */
    {   6,   7, 0x06, 1, "I think POKEMON\ncan be good or\nevil. It depends\non the trainer.", NULL },  /* SPRITE_COOLTRAINER_F, WALK, TEXT_VERMILIONMART_COOLTRAINER_F */
};

static const map_warp_t kWarps_VermilionGym[] = {
    {   8,  35, 0xff, 3 },  /* LAST_MAP */
    {  10,  35, 0xff, 3 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_VermilionGym[] = {
    {  10,   3, 0x21, 0, "Hey, kid! What do\nyou think you're\ndoing here?\fYou won't live\nlong in combat!\nThat's for sure!\fI tell you kid,\nelectric POKEMON\nsaved me during\nthe war!\fThey zapped my\nenemies into\nparalysis!\fThe same as I'll\ndo to you!", NULL },  /* SPRITE_ROCKER, STAY, TEXT_VERMILIONGYM_LT_SURGE */
    {  18,  13, 0x10, 0, NULL, NULL },  /* SPRITE_GENTLEMAN, STAY, TEXT_VERMILIONGYM_GENTLEMAN */
    {   6,  17, 0x0c, 0, NULL, NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_VERMILIONGYM_SUPER_NERD */
    {   0,  21, 0x13, 0, NULL, NULL },  /* SPRITE_SAILOR, STAY, TEXT_VERMILIONGYM_SAILOR */
    {   8,  29, 0x24, 0, "Yo! Champ in\nmaking!\fLT.SURGE has a\nnickname. People\nrefer to him as\nthe Lightning\nAmerican!\fHe's an expert on\nelectric POKEMON!\fBirds and water\nPOKEMON are at\nrisk! Beware of\nparalysis too!\fLT.SURGE is very\ncautious!\fYou'll have to\nbreak a code to\nget to him!", NULL },  /* SPRITE_GYM_GUIDE, STAY, TEXT_VERMILIONGYM_GYM_GUIDE */
};

static const map_warp_t kWarps_VermilionPidgeyHouse[] = {
    {   4,  15, 0xff, 4 },  /* LAST_MAP */
    {   6,  15, 0xff, 4 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_VermilionPidgeyHouse[] = {
    {  10,   7, 0x04, 0, "I'm getting my\nPIDGEY to fly a\nletter to SAFFRON\nin the north!", NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_VERMILIONPIDGEYHOUSE_YOUNGSTER */
    {   6,  11, 0x09, 1, "PIDGEY: Kurukkoo!@", NULL },  /* SPRITE_BIRD, WALK, TEXT_VERMILIONPIDGEYHOUSE_PIDGEY */
    {   8,   7, 0x40, 0, "Dear PIPPI, I hope\nto see you soon.\fI heard SAFFRON\nhas problems with\nTEAM ROCKET.\fVERMILION appears\nto be safe.", NULL },  /* SPRITE_PAPER, STAY, TEXT_VERMILIONPIDGEYHOUSE_LETTER */
};

static const map_warp_t kWarps_VermilionDock[] = {
    {  28,   1, 0xff, 5 },  /* LAST_MAP */
    {  28,   5, 0x5f, 1 },  /* SS_ANNE_1F */
};

static const map_warp_t kWarps_SSAnne1F[] = {
    {  52,   1, 0x5e, 1 },  /* VERMILION_DOCK */
    {  54,   1, 0x5e, 1 },  /* VERMILION_DOCK */
    {  62,  17, 0x66, 0 },  /* SS_ANNE_1F_ROOMS */
    {  46,  17, 0x66, 1 },  /* SS_ANNE_1F_ROOMS */
    {  38,  17, 0x66, 2 },  /* SS_ANNE_1F_ROOMS */
    {  30,  17, 0x66, 3 },  /* SS_ANNE_1F_ROOMS */
    {  22,  17, 0x66, 4 },  /* SS_ANNE_1F_ROOMS */
    {  14,  17, 0x66, 5 },  /* SS_ANNE_1F_ROOMS */
    {   4,  13, 0x60, 6 },  /* SS_ANNE_2F */
    {  74,  31, 0x62, 5 },  /* SS_ANNE_B1F */
    {   6,  33, 0x64, 0 },  /* SS_ANNE_KITCHEN */
};

static const npc_event_t kNpcs_SSAnne1F[] = {
    {  24,  13, 0x1a, 1, "Bonjour!\nI am le waiter on\nthis ship!\fI will be happy\nto serve you any-\nthing you please!\fAh! Le strong\nsilent type!", NULL },  /* SPRITE_WAITER, WALK, TEXT_SSANNE1F_WAITER */
    {  54,  11, 0x13, 0, "The passengers\nare restless!\fYou might be\nchallenged by the\nmore bored ones!", NULL },  /* SPRITE_SAILOR, STAY, TEXT_SSANNE1F_SAILOR */
};

static const map_warp_t kWarps_SSAnne2F[] = {
    {  18,  23, 0x67, 0 },  /* SS_ANNE_2F_ROOMS */
    {  26,  23, 0x67, 2 },  /* SS_ANNE_2F_ROOMS */
    {  34,  23, 0x67, 4 },  /* SS_ANNE_2F_ROOMS */
    {  42,  23, 0x67, 6 },  /* SS_ANNE_2F_ROOMS */
    {  50,  23, 0x67, 8 },  /* SS_ANNE_2F_ROOMS */
    {  58,  23, 0x67, 10 },  /* SS_ANNE_2F_ROOMS */
    {   4,   9, 0x5f, 8 },  /* SS_ANNE_1F */
    {   4,  25, 0x61, 1 },  /* SS_ANNE_3F */
    {  72,   9, 0x65, 0 },  /* SS_ANNE_CAPTAINS_ROOM */
};

static const npc_event_t kNpcs_SSAnne2F[] = {
    {   6,  15, 0x1a, 1, "This ship, she is\na luxury liner\nfor trainers!\fAt every port, we\nhold parties with\ninvited trainers!", NULL },  /* SPRITE_WAITER, WALK, TEXT_SSANNE2F_WAITER */
    {  72,   9, 0x02, 0, "{RIVAL}: Bonjour!\n{PLAYER}!\fImagine seeing\nyou here!\f{PLAYER}, were you\nreally invited?\fSo how's your\nPOKEDEX coming?\fI already caught\n40 kinds, pal!\fDifferent kinds\nare everywhere!\fCrawl around in\ngrassy areas!", NULL },  /* SPRITE_BLUE, STAY, TEXT_SSANNE2F_RIVAL */
};

static const map_warp_t kWarps_SSAnne3F[] = {
    {   0,   7, 0x63, 0 },  /* SS_ANNE_BOW */
    {  38,   7, 0x60, 7 },  /* SS_ANNE_2F */
};

static const npc_event_t kNpcs_SSAnne3F[] = {
    {  18,   7, 0x13, 1, "Our CAPTAIN is a\nsword master!\fHe even teaches\nCUT to POKEMON!", NULL },  /* SPRITE_SAILOR, WALK, TEXT_SSANNE3F_SAILOR */
};

static const map_warp_t kWarps_SSAnneB1F[] = {
    {  46,   7, 0x68, 8 },  /* SS_ANNE_B1F_ROOMS */
    {  38,   7, 0x68, 6 },  /* SS_ANNE_B1F_ROOMS */
    {  30,   7, 0x68, 4 },  /* SS_ANNE_B1F_ROOMS */
    {  22,   7, 0x68, 2 },  /* SS_ANNE_B1F_ROOMS */
    {  14,   7, 0x68, 0 },  /* SS_ANNE_B1F_ROOMS */
    {  54,  11, 0x5f, 9 },  /* SS_ANNE_1F */
};

static const map_warp_t kWarps_SSAnneBow[] = {
    {  26,  13, 0x61, 0 },  /* SS_ANNE_3F */
    {  26,  15, 0x61, 0 },  /* SS_ANNE_3F */
};

static const npc_event_t kNpcs_SSAnneBow[] = {
    {  10,   5, 0x0c, 0, "The party's over.\nThe ship will be\ndeparting soon.", NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_SSANNEBOW_SUPER_NERD */
    {   8,  19, 0x13, 0, "Scrubbing decks\nis hard work!", NULL },  /* SPRITE_SAILOR, STAY, TEXT_SSANNEBOW_SAILOR1 */
    {  14,  23, 0x07, 0, "Urf. I feel ill.\fI stepped out to\nget some air.", NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_SSANNEBOW_COOLTRAINER_M */
    {   8,   9, 0x13, 0, NULL, NULL },  /* SPRITE_SAILOR, STAY, TEXT_SSANNEBOW_SAILOR2 */
    {  20,  17, 0x13, 0, NULL, NULL },  /* SPRITE_SAILOR, STAY, TEXT_SSANNEBOW_SAILOR3 */
};

static const map_warp_t kWarps_SSAnneKitchen[] = {
    {  12,   1, 0x5f, 10 },  /* SS_ANNE_1F */
};

static const npc_event_t kNpcs_SSAnneKitchen[] = {
    {   2,  17, 0x14, 1, "You, mon petit!\nWe're busy here!\nOut of the way!", NULL },  /* SPRITE_COOK, WALK, TEXT_SSANNEKITCHEN_COOK1 */
    {  10,  17, 0x14, 1, "I saw an odd ball\nin the trash.", NULL },  /* SPRITE_COOK, WALK, TEXT_SSANNEKITCHEN_COOK2 */
    {  18,  15, 0x14, 1, "I'm so busy I'm\ngetting dizzy!", NULL },  /* SPRITE_COOK, WALK, TEXT_SSANNEKITCHEN_COOK3 */
    {  26,  13, 0x14, 0, "Hum-de-hum-de-\nho...\fI peel spuds\nevery day!\nHum-hum...", NULL },  /* SPRITE_COOK, STAY, TEXT_SSANNEKITCHEN_COOK4 */
    {  26,  17, 0x14, 0, "Did you hear about\nSNORLAX?\fAll it does is\neat and sleep!", NULL },  /* SPRITE_COOK, STAY, TEXT_SSANNEKITCHEN_COOK5 */
    {  26,  21, 0x14, 0, "Snivel...Sniff...\fI only get to\npeel onions...\nSnivel...", NULL },  /* SPRITE_COOK, STAY, TEXT_SSANNEKITCHEN_COOK6 */
    {  22,  27, 0x14, 0, "Er-hem! Indeed I\nam le CHEF!\fLe main course is", NULL },  /* SPRITE_COOK, STAY, TEXT_SSANNEKITCHEN_COOK7 */
};

static const map_warp_t kWarps_SSAnneCaptainsRoom[] = {
    {   0,  15, 0x60, 8 },  /* SS_ANNE_2F */
};

static const npc_event_t kNpcs_SSAnneCaptainsRoom[] = {
    {   8,   5, 0x2e, 0, NULL, NULL },  /* SPRITE_CAPTAIN, STAY, TEXT_SSANNECAPTAINSROOM_CAPTAIN */
};

static const sign_event_t kSigns_SSAnneCaptainsRoom[] = {
    {   8,   3, "Yuck! Shouldn't\nhave looked!" },  /* TEXT_SSANNECAPTAINSROOM_TRASH */
    {   2,   5, "How to Conquer\nSeasickness...\nThe CAPTAIN's\nreading this!" },  /* TEXT_SSANNECAPTAINSROOM_SEASICK_BOOK */
};

static const map_warp_t kWarps_SSAnne1FRooms[] = {
    {   0,   1, 0x5f, 2 },  /* SS_ANNE_1F */
    {  20,   1, 0x5f, 3 },  /* SS_ANNE_1F */
    {  40,   1, 0x5f, 4 },  /* SS_ANNE_1F */
    {   0,  21, 0x5f, 5 },  /* SS_ANNE_1F */
    {  20,  21, 0x5f, 6 },  /* SS_ANNE_1F */
    {  40,  21, 0x5f, 7 },  /* SS_ANNE_1F */
};

static const npc_event_t kNpcs_SSAnne1FRooms[] = {
    {   4,   7, 0x10, 0, NULL, NULL },  /* SPRITE_GENTLEMAN, STAY, TEXT_SSANNE1FROOMS_GENTLEMAN1 */
    {  22,   9, 0x10, 0, NULL, NULL },  /* SPRITE_GENTLEMAN, STAY, TEXT_SSANNE1FROOMS_GENTLEMAN2 */
    {  22,  29, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_SSANNE1FROOMS_YOUNGSTER */
    {  26,  23, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_SSANNE1FROOMS_COOLTRAINER_F */
    {  44,   7, 0x0d, 1, "Waiter, I would\nlike a cherry pie\nplease!", NULL },  /* SPRITE_GIRL, WALK, TEXT_SSANNE1FROOMS_GIRL1 */
    {   0,  29, 0x0a, 0, "A cruise is so\nelegant yet cozy!", NULL },  /* SPRITE_MIDDLE_AGED_MAN, STAY, TEXT_SSANNE1FROOMS_MIDDLE_AGED_MAN */
    {   4,  23, 0x08, 0, "I always travel\nwith WIGGLYTUFF!", NULL },  /* SPRITE_LITTLE_GIRL, STAY, TEXT_SSANNE1FROOMS_LITTLE_GIRL */
    {   6,  23, 0x38, 0, "WIGGLYTUFF: Puup\npupuu!@", NULL },  /* SPRITE_FAIRY, STAY, TEXT_SSANNE1FROOMS_WIGGLYTUFF */
    {  20,  27, 0x0d, 0, "We are cruising\naround the world.", NULL },  /* SPRITE_GIRL, STAY, TEXT_SSANNE1FROOMS_GIRL2 */
    {  42,  27, 0x10, 1, "Ssh! I'm a GLOBAL\nPOLICE agent!\fI'm on the trail\nof TEAM ROCKET!", NULL },  /* SPRITE_GENTLEMAN, WALK, TEXT_SSANNE1FROOMS_GENTLEMAN3 */
};

static const item_event_t kItems_SSAnne1FRooms[] = {
    {  24,  31, 0xd0 },  /* TM_BODY_SLAM */
};

static const map_warp_t kWarps_SSAnne2FRooms[] = {
    {   4,  11, 0x60, 0 },  /* SS_ANNE_2F */
    {   6,  11, 0x60, 0 },  /* SS_ANNE_2F */
    {  24,  11, 0x60, 1 },  /* SS_ANNE_2F */
    {  26,  11, 0x60, 1 },  /* SS_ANNE_2F */
    {  44,  11, 0x60, 2 },  /* SS_ANNE_2F */
    {  46,  11, 0x60, 2 },  /* SS_ANNE_2F */
    {   4,  31, 0x60, 3 },  /* SS_ANNE_2F */
    {   6,  31, 0x60, 3 },  /* SS_ANNE_2F */
    {  24,  31, 0x60, 4 },  /* SS_ANNE_2F */
    {  26,  31, 0x60, 4 },  /* SS_ANNE_2F */
    {  44,  31, 0x60, 5 },  /* SS_ANNE_2F */
    {  46,  31, 0x60, 5 },  /* SS_ANNE_2F */
};

static const npc_event_t kNpcs_SSAnne2FRooms[] = {
    {  20,   5, 0x10, 0, NULL, NULL },  /* SPRITE_GENTLEMAN, STAY, TEXT_SSANNE2FROOMS_GENTLEMAN1 */
    {  26,   9, 0x2f, 0, NULL, NULL },  /* SPRITE_FISHER, STAY, TEXT_SSANNE2FROOMS_FISHER */
    {   0,  29, 0x10, 0, NULL, NULL },  /* SPRITE_GENTLEMAN, STAY, TEXT_SSANNE2FROOMS_GENTLEMAN2 */
    {   4,  23, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_SSANNE2FROOMS_COOLTRAINER_F */
    {   2,   5, 0x10, 0, "In all my travels\nI've never seen\nany POKEMON sleep\nlike this one!\fIt was something\nlike this!", NULL },  /* SPRITE_GENTLEMAN, STAY, TEXT_SSANNE2FROOMS_GENTLEMAN3 */
    {  42,   5, 0x10, 0, "Ah yes, I have\nseen some POKEMON\nferry people\nacross the water!", NULL },  /* SPRITE_GENTLEMAN, STAY, TEXT_SSANNE2FROOMS_GENTLEMAN4 */
    {  44,   3, 0x25, 0, "POKEMON can CUT\ndown small bushes.", NULL },  /* SPRITE_GRAMPS, STAY, TEXT_SSANNE2FROOMS_GRAMPS */
    {  24,  25, 0x10, 0, "Have you gone to\nthe SAFARI ZONE\nin FUCHSIA CITY?\fIt had many rare\nkinds of POKEMON!!", NULL },  /* SPRITE_GENTLEMAN, STAY, TEXT_SSANNE2FROOMS_GENTLEMAN5 */
    {  22,  29, 0x35, 0, "Me and my Daddy\nthink the SAFARI\nZONE is awesome!", NULL },  /* SPRITE_LITTLE_BOY, STAY, TEXT_SSANNE2FROOMS_LITTLE_BOY */
    {  44,  25, 0x1d, 0, "The CAPTAIN looked\nreally sick and\npale!", NULL },  /* SPRITE_BRUNETTE_GIRL, STAY, TEXT_SSANNE2FROOMS_BRUNETTE_GIRL */
    {  40,  25, 0x0f, 0, "I hear many people\nget seasick!", NULL },  /* SPRITE_BEAUTY, STAY, TEXT_SSANNE2FROOMS_BEAUTY */
};

static const item_event_t kItems_SSAnne2FRooms[] = {
    {  24,   3, 0x51 },  /* MAX_ETHER */
    {   0,  25, 0x28 },  /* RARE_CANDY */
};

static const map_warp_t kWarps_SSAnneB1FRooms[] = {
    {   4,  11, 0x62, 4 },  /* SS_ANNE_B1F */
    {   6,  11, 0x62, 4 },  /* SS_ANNE_B1F */
    {  24,  11, 0x62, 3 },  /* SS_ANNE_B1F */
    {  26,  11, 0x62, 3 },  /* SS_ANNE_B1F */
    {  44,  11, 0x62, 2 },  /* SS_ANNE_B1F */
    {  46,  11, 0x62, 2 },  /* SS_ANNE_B1F */
    {   4,  31, 0x62, 1 },  /* SS_ANNE_B1F */
    {   6,  31, 0x62, 1 },  /* SS_ANNE_B1F */
    {  24,  31, 0x62, 0 },  /* SS_ANNE_B1F */
    {  26,  31, 0x62, 0 },  /* SS_ANNE_B1F */
};

static const npc_event_t kNpcs_SSAnneB1FRooms[] = {
    {   0,  27, 0x13, 0, NULL, NULL },  /* SPRITE_SAILOR, STAY, TEXT_SSANNEB1FROOMS_SAILOR1 */
    {   4,  23, 0x13, 0, NULL, NULL },  /* SPRITE_SAILOR, STAY, TEXT_SSANNEB1FROOMS_SAILOR2 */
    {  24,   7, 0x13, 0, NULL, NULL },  /* SPRITE_SAILOR, STAY, TEXT_SSANNEB1FROOMS_SAILOR3 */
    {  44,   5, 0x13, 0, NULL, NULL },  /* SPRITE_SAILOR, STAY, TEXT_SSANNEB1FROOMS_SAILOR4 */
    {   0,   5, 0x13, 0, NULL, NULL },  /* SPRITE_SAILOR, STAY, TEXT_SSANNEB1FROOMS_SAILOR5 */
    {   0,   9, 0x2f, 0, NULL, NULL },  /* SPRITE_FISHER, STAY, TEXT_SSANNEB1FROOMS_FISHER */
    {  20,  27, 0x0c, 0, "My buddy, MACHOKE,\nis super strong!\fHe has enough\nSTRENGTH to move\nbig rocks!", NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_SSANNEB1FROOMS_SUPER_NERD */
    {  22,  25, 0x05, 0, "MACHOKE: Gwoh!\nGoggoh!@", NULL },  /* SPRITE_MONSTER, STAY, TEXT_SSANNEB1FROOMS_MACHOKE */
};

static const item_event_t kItems_SSAnneB1FRooms[] = {
    {  40,   5, 0x50 },  /* ETHER */
    {  20,   5, 0xf4 },  /* TM_REST */
    {  24,  23, 0x11 },  /* MAX_POTION */
};

static const map_warp_t kWarps_LancesRoom[] = {
    {  48,  33, 0xf7, 2 },  /* AGATHAS_ROOM */
    {  10,   1, 0x78, 0 },  /* CHAMPIONS_ROOM */
    {  12,   1, 0x78, 0 },  /* CHAMPIONS_ROOM */
};

static const npc_event_t kNpcs_LancesRoom[] = {
    {  12,   3, 0x1e, 0, NULL, NULL },  /* SPRITE_LANCE, STAY, TEXT_LANCESROOM_LANCE */
};

static const map_warp_t kWarps_VictoryRoad1F[] = {
    {  16,  35, 0xff, 2 },  /* LAST_MAP */
    {  18,  35, 0xff, 2 },  /* LAST_MAP */
    {   2,   3, 0xc2, 0 },  /* VICTORY_ROAD_2F */
};

static const npc_event_t kNpcs_VictoryRoad1F[] = {
    {  14,  11, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_VICTORYROAD1F_COOLTRAINER_F */
    {   6,   5, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_VICTORYROAD1F_COOLTRAINER_M */
    {  10,  31, 0x3f, 0, NULL, NULL },  /* SPRITE_BOULDER, STAY, TEXT_VICTORYROAD1F_BOULDER1 */
    {  28,   5, 0x3f, 0, NULL, NULL },  /* SPRITE_BOULDER, STAY, TEXT_VICTORYROAD1F_BOULDER2 */
    {   4,  21, 0x3f, 0, NULL, NULL },  /* SPRITE_BOULDER, STAY, TEXT_VICTORYROAD1F_BOULDER3 */
};

static const item_event_t kItems_VictoryRoad1F[] = {
    {  22,   1, 0xf3 },  /* TM_SKY_ATTACK */
    {  18,   5, 0x28 },  /* RARE_CANDY */
};

static const map_warp_t kWarps_HallOfFame[] = {
    {   8,  15, 0x78, 2 },  /* CHAMPIONS_ROOM */
    {  10,  15, 0x78, 3 },  /* CHAMPIONS_ROOM */
};

static const npc_event_t kNpcs_HallOfFame[] = {
    {  10,   5, 0x03, 0, "OAK: Er-hem!\nCongratulations\n{PLAYER}!\fThis floor is the\nPOKEMON HALL OF\nFAME!\fPOKEMON LEAGUE\nchampions are\nhonored for their\nexploits here!\fTheir POKEMON are\nalso recorded in\nthe HALL OF FAME!\f{PLAYER}! You have\nendeavored hard\nto become the new\nLEAGUE champion!\fCongratulations,\n{PLAYER}, you and\nyour POKEMON are\nHALL OF FAMERs!", NULL },  /* SPRITE_OAK, STAY, TEXT_HALLOFFAME_OAK */
};

static const map_warp_t kWarps_UndergroundPathNorthSouth[] = {
    {  10,   9, 0x47, 2 },  /* UNDERGROUND_PATH_ROUTE_5 */
    {   4,  83, 0x4a, 2 },  /* UNDERGROUND_PATH_ROUTE_6 */
};

static const map_warp_t kWarps_ChampionsRoom[] = {
    {   6,  15, 0x71, 1 },  /* LANCES_ROOM */
    {   8,  15, 0x71, 2 },  /* LANCES_ROOM */
    {   6,   1, 0x76, 0 },  /* HALL_OF_FAME */
    {   8,   1, 0x76, 0 },  /* HALL_OF_FAME */
};

static const npc_event_t kNpcs_ChampionsRoom[] = {
    {   8,   5, 0x02, 0, "{RIVAL}: Hey!\fI was looking\nforward to seeing\nyou, {PLAYER}!\fMy rival should\nbe strong to keep\nme sharp!\fWhile working on\nPOKEDEX, I looked\nall over for\npowerful POKEMON!\fNot only that, I\nassembled teams\nthat would beat\nany POKEMON type!\fAnd now!\fI'm the POKEMON\nLEAGUE champion!\f{PLAYER}! Do you\nknow what that\nmeans?\fI'll tell you!\fI am the most\npowerful trainer\nin the world!", NULL },  /* SPRITE_BLUE, STAY, TEXT_CHAMPIONSROOM_RIVAL */
    {   6,  15, 0x03, 0, "OAK: {PLAYER}!", NULL },  /* SPRITE_OAK, STAY, TEXT_CHAMPIONSROOM_OAK */
};

static const map_warp_t kWarps_UndergroundPathWestEast[] = {
    {   4,  11, 0x4d, 2 },  /* UNDERGROUND_PATH_ROUTE_7 */
    {  94,   5, 0x50, 2 },  /* UNDERGROUND_PATH_ROUTE_8 */
};

static const map_warp_t kWarps_CeladonMart1F[] = {
    {   4,  15, 0xff, 0 },  /* LAST_MAP */
    {   6,  15, 0xff, 0 },  /* LAST_MAP */
    {  32,  15, 0xff, 1 },  /* LAST_MAP */
    {  34,  15, 0xff, 1 },  /* LAST_MAP */
    {  24,   3, 0x7b, 0 },  /* CELADON_MART_2F */
    {   2,   3, 0x7f, 0 },  /* CELADON_MART_ELEVATOR */
};

static const npc_event_t kNpcs_CeladonMart1F[] = {
    {  16,   7, 0x2a, 0, "Hello! Welcome to\nCELADON DEPT.\nSTORE.\fThe board on the\nright describes\nthe store layout.", NULL },  /* SPRITE_LINK_RECEPTIONIST, STAY, TEXT_CELADONMART1F_RECEPTIONIST */
};

static const sign_event_t kSigns_CeladonMart1F[] = {
    {  22,   9, "1F: SERVICE\n    COUNTER\f2F: TRAINER'S\n    MARKET\f3F: TV GAME SHOP\f4F: WISEMAN GIFTS\f5F: DRUG STORE\fROOFTOP SQUARE:\nVENDING MACHINES" },  /* TEXT_CELADONMART1F_DIRECTORY_SIGN */
    {  28,   3, "1F: SERVICE\n    COUNTER" },  /* TEXT_CELADONMART1F_CURRENT_FLOOR_SIGN */
};

static const map_warp_t kWarps_CeladonMart2F[] = {
    {  24,   3, 0x7a, 4 },  /* CELADON_MART_1F */
    {  32,   3, 0x7c, 1 },  /* CELADON_MART_3F */
    {   2,   3, 0x7f, 0 },  /* CELADON_MART_ELEVATOR */
};

static const npc_event_t kNpcs_CeladonMart2F[] = {
    {  10,   7, 0x26, 0, NULL, Celadon2F1Mart_Start },  /* SPRITE_CLERK, STAY, TEXT_CELADONMART2F_CLERK1 */
    {  12,   7, 0x26, 0, NULL, Celadon2F2Mart_Start },  /* SPRITE_CLERK, STAY, TEXT_CELADONMART2F_CLERK2 */
    {  38,  11, 0x0a, 0, "SUPER REPEL keeps\nweak POKEMON at\nbay...\fHmm, it's a more\npowerful REPEL!", NULL },  /* SPRITE_MIDDLE_AGED_MAN, STAY, TEXT_CELADONMART2F_MIDDLE_AGED_MAN */
    {  28,   9, 0x0d, 1, "For long outings,\nyou should buy\nREVIVE.", NULL },  /* SPRITE_GIRL, WALK, TEXT_CELADONMART2F_GIRL */
};

static const sign_event_t kSigns_CeladonMart2F[] = {
    {  28,   3, "Top Grade Items\nfor Trainers!\f2F: TRAINER'S\n    MARKET" },  /* TEXT_CELADONMART2F_CURRENT_FLOOR_SIGN */
};

static const map_warp_t kWarps_CeladonMart3F[] = {
    {  24,   3, 0x7d, 0 },  /* CELADON_MART_4F */
    {  32,   3, 0x7b, 1 },  /* CELADON_MART_2F */
    {   2,   3, 0x7f, 0 },  /* CELADON_MART_ELEVATOR */
};

static const npc_event_t kNpcs_CeladonMart3F[] = {
    {  32,  11, 0x26, 0, "Oh, hi! I finally\nfinished POKEMON!\fNot done yet?\nThis might be\nuseful!", NULL },  /* SPRITE_CLERK, STAY, TEXT_CELADONMART3F_CLERK */
    {  22,  13, 0x37, 0, "Captured POKEMON\nare registered\nwith an ID No.\nand OT, the name\nof the Original\nTrainer that\ncaught it!", NULL },  /* SPRITE_GAMEBOY_KID, STAY, TEXT_CELADONMART3F_GAMEBOY_KID1 */
    {  14,   5, 0x37, 0, "All right!\fMy buddy's going\nto trade me his\nKANGASKHAN for my\nGRAVELER!", NULL },  /* SPRITE_GAMEBOY_KID, STAY, TEXT_CELADONMART3F_GAMEBOY_KID2 */
    {  16,   5, 0x37, 0, "Come on GRAVELER!\fI love GRAVELER!\nI collect them!\fHuh?\fGRAVELER turned\ninto a different\nPOKEMON!", NULL },  /* SPRITE_GAMEBOY_KID, STAY, TEXT_CELADONMART3F_GAMEBOY_KID3 */
    {   4,  11, 0x35, 0, "You can identify\nPOKEMON you got\nin trades by\ntheir ID Numbers!", NULL },  /* SPRITE_LITTLE_BOY, STAY, TEXT_CELADONMART3F_LITTLE_BOY */
};

static const sign_event_t kSigns_CeladonMart3F[] = {
    {   4,   9, "It's an SNES!" },  /* TEXT_CELADONMART3F_SNES1 */
    {   6,   9, "An RPG! There's\nno time for that!" },  /* TEXT_CELADONMART3F_RPG */
    {  10,   9, "It's an SNES!" },  /* TEXT_CELADONMART3F_SNES2 */
    {  12,   9, "A sports game!\nDad'll like that!" },  /* TEXT_CELADONMART3F_SPORTS_GAME */
    {   4,  13, "It's an SNES!" },  /* TEXT_CELADONMART3F_SNES3 */
    {   6,  13, "A puzzle game!\nLooks addictive!" },  /* TEXT_CELADONMART3F_PUZZLE_GAME */
    {  10,  13, "It's an SNES!" },  /* TEXT_CELADONMART3F_SNES4 */
    {  12,  13, "A fighting game!\nLooks tough!" },  /* TEXT_CELADONMART3F_FIGHTING_GAME */
    {  28,   3, "3F: TV GAME SHOP" },  /* TEXT_CELADONMART3F_CURRENT_FLOOR_SIGN */
    {   8,   3, "Red and Blue!\nBoth are POKEMON!" },  /* TEXT_CELADONMART3F_POKEMON_POSTER1 */
    {  12,   3, "Red and Blue!\nBoth are POKEMON!" },  /* TEXT_CELADONMART3F_POKEMON_POSTER2 */
    {  20,   3, "Red and Blue!\nBoth are POKEMON!" },  /* TEXT_CELADONMART3F_POKEMON_POSTER3 */
};

static const map_warp_t kWarps_CeladonMart4F[] = {
    {  24,   3, 0x7c, 0 },  /* CELADON_MART_3F */
    {  32,   3, 0x88, 1 },  /* CELADON_MART_5F */
    {   2,   3, 0x7f, 0 },  /* CELADON_MART_ELEVATOR */
};

static const npc_event_t kNpcs_CeladonMart4F[] = {
    {  10,  15, 0x26, 0, NULL, Celadon4FMart_Start },  /* SPRITE_CLERK, STAY, TEXT_CELADONMART4F_CLERK */
    {  30,  11, 0x0c, 1, "I'm getting a\nPOKE DOLL for my\ngirl friend!", NULL },  /* SPRITE_SUPER_NERD, WALK, TEXT_CELADONMART4F_SUPER_NERD */
    {  10,   5, 0x04, 1, "I heard something\nuseful.\fYou can run from\nwild POKEMON by\ndistracting them\nwith a POKE DOLL!", NULL },  /* SPRITE_YOUNGSTER, WALK, TEXT_CELADONMART4F_YOUNGSTER */
};

static const sign_event_t kSigns_CeladonMart4F[] = {
    {  28,   3, "Express yourself\nwith gifts!\f4F: WISEMAN GIFTS\fEvolution Special!\nElement STONEs on\nsale now!" },  /* TEXT_CELADONMART4F_CURRENT_FLOOR_SIGN */
};

static const map_warp_t kWarps_CeladonMartRoof[] = {
    {  30,   5, 0x88, 0 },  /* CELADON_MART_5F */
};

static const npc_event_t kNpcs_CeladonMartRoof[] = {
    {  20,   9, 0x0c, 0, "My sister is a\ntrainer, believe\nit or not.\fBut, she's so\nimmature, she\ndrives me nuts!", NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_CELADONMARTROOF_SUPER_NERD */
    {  10,  11, 0x08, 1, "I'm thirsty!\nI want something\nto drink!", NULL },  /* SPRITE_LITTLE_GIRL, WALK, TEXT_CELADONMARTROOF_LITTLE_GIRL */
};

static const sign_event_t kSigns_CeladonMartRoof[] = {
    {  20,   3, NULL },  /* TEXT_CELADONMARTROOF_VENDING_MACHINE1 */
    {  22,   3, NULL },  /* TEXT_CELADONMARTROOF_VENDING_MACHINE2 */
    {  24,   5, NULL },  /* TEXT_CELADONMARTROOF_VENDING_MACHINE3 */
    {  26,   5, "ROOFTOP SQUARE:\nVENDING MACHINES" },  /* TEXT_CELADONMARTROOF_CURRENT_FLOOR_SIGN */
};

static const map_warp_t kWarps_CeladonMartElevator[] = {
    {   2,   7, 0x7a, 5 },  /* CELADON_MART_1F */
    {   4,   7, 0x7a, 5 },  /* CELADON_MART_1F */
};

static const sign_event_t kSigns_CeladonMartElevator[] = {
    {   6,   1, NULL },  /* TEXT_CELADONMARTELEVATOR */
};

static const map_warp_t kWarps_CeladonMansion1F[] = {
    {   8,  23, 0xff, 2 },  /* LAST_MAP */
    {  10,  23, 0xff, 2 },  /* LAST_MAP */
    {   8,   1, 0xff, 4 },  /* LAST_MAP */
    {  14,   3, 0x81, 1 },  /* CELADON_MANSION_2F */
    {   4,   3, 0x81, 2 },  /* CELADON_MANSION_2F */
};

static const npc_event_t kNpcs_CeladonMansion1F[] = {
    {   0,  11, 0x05, 0, "MEOWTH: Meow!@", NULL },  /* SPRITE_MONSTER, STAY, TEXT_CELADONMANSION1F_MEOWTH */
    {   2,  11, 0x28, 0, "My dear POKEMON\nkeep me company.\fMEOWTH even brings\nmoney home!", NULL },  /* SPRITE_GRANNY, STAY, TEXT_CELADONMANSION1F_GRANNY */
    {   2,  17, 0x38, 1, "CLEFAIRY: Pi\npippippi!@", NULL },  /* SPRITE_FAIRY, WALK, TEXT_CELADONMANSION1F_CLEFAIRY */
    {   8,   9, 0x05, 1, "NIDORAN: Kya\nkyaoo!@", NULL },  /* SPRITE_MONSTER, WALK, TEXT_CELADONMANSION1F_NIDORANF */
};

static const sign_event_t kSigns_CeladonMansion1F[] = {
    {   8,  19, "CELADON MANSION\nManager's Suite" },  /* TEXT_CELADONMANSION1F_MANAGERS_SUITE_SIGN */
};

static const map_warp_t kWarps_CeladonMansion2F[] = {
    {  12,   3, 0x82, 0 },  /* CELADON_MANSION_3F */
    {  14,   3, 0x80, 3 },  /* CELADON_MANSION_1F */
    {   4,   3, 0x80, 4 },  /* CELADON_MANSION_1F */
    {   8,   3, 0x82, 3 },  /* CELADON_MANSION_3F */
};

static const sign_event_t kSigns_CeladonMansion2F[] = {
    {   8,  19, "GAME FREAK\nMeeting Room" },  /* TEXT_CELADONMANSION2F_MEETING_ROOM_SIGN */
};

static const map_warp_t kWarps_CeladonMansion3F[] = {
    {  12,   3, 0x81, 0 },  /* CELADON_MANSION_2F */
    {  14,   3, 0x83, 0 },  /* CELADON_MANSION_ROOF */
    {   4,   3, 0x83, 1 },  /* CELADON_MANSION_ROOF */
    {   8,   3, 0x81, 3 },  /* CELADON_MANSION_2F */
};

static const npc_event_t kNpcs_CeladonMansion3F[] = {
    {   0,   9, 0x15, 0, "Me? I'm the\nprogrammer!", NULL },  /* SPRITE_BIKE_SHOP_CLERK, STAY, TEXT_CELADONMANSION3F_PROGRAMMER */
    {   6,   9, 0x26, 0, "I'm the graphic\nartist!\nI drew you!", NULL },  /* SPRITE_CLERK, STAY, TEXT_CELADONMANSION3F_GRAPHIC_ARTIST */
    {   0,  15, 0x0c, 0, "I wrote the story!\nIsn't ERIKA cute?\fI like MISTY a\nlot too!\fOh, and SABRINA,\nI like her!", NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_CELADONMANSION3F_WRITER */
    {   4,   7, 0x2c, 0, "Is that right?\fI'm the game\ndesigner!\fFilling up your\nPOKEDEX is tough,\nbut don't quit!\fWhen you finish,\ncome tell me!", NULL },  /* SPRITE_SILPH_WORKER_M, STAY, TEXT_CELADONMANSION3F_GAME_DESIGNER */
};

static const sign_event_t kSigns_CeladonMansion3F[] = {
    {   2,   7, "It's the game\nprogram! Messing\nwith it could bug\nout the game!" },  /* TEXT_CELADONMANSION3F_GAME_PROGRAM_PC */
    {   8,   7, "Someone's playing\na game instead of\nworking!" },  /* TEXT_CELADONMANSION3F_PLAYING_GAME_PC */
    {   2,  13, "It's the script!\nBetter not look\nat the ending!" },  /* TEXT_CELADONMANSION3F_GAME_SCRIPT_PC */
    {   8,  19, "GAME FREAK\nDevelopment Room" },  /* TEXT_CELADONMANSION3F_DEV_ROOM_SIGN */
};

static const map_warp_t kWarps_CeladonMansionRoof[] = {
    {  12,   3, 0x82, 1 },  /* CELADON_MANSION_3F */
    {   4,   3, 0x82, 2 },  /* CELADON_MANSION_3F */
    {   4,  15, 0x84, 0 },  /* CELADON_MANSION_ROOF_HOUSE */
};

static const sign_event_t kSigns_CeladonMansionRoof[] = {
    {   6,  15, "I KNOW EVERYTHING!" },  /* TEXT_CELADONMANSIONROOF_HOUSE_SIGN */
};

static const map_warp_t kWarps_CeladonMansionRoofHouse[] = {
    {   4,  15, 0x83, 2 },  /* CELADON_MANSION_ROOF */
    {   6,  15, 0x83, 2 },  /* CELADON_MANSION_ROOF */
};

static const npc_event_t kNpcs_CeladonMansionRoofHouse[] = {
    {   4,   5, 0x0e, 0, "I know everything\nabout the world\nof POKEMON in\nyour GAME BOY!\fGet together with\nyour friends and\ntrade POKEMON!", NULL },  /* SPRITE_HIKER, STAY, TEXT_CELADONMANSION_ROOF_HOUSE_HIKER */
};

static const map_warp_t kWarps_CeladonPokecenter[] = {
    {   6,  15, 0xff, 5 },  /* LAST_MAP */
    {   8,  15, 0xff, 5 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_CeladonPokecenter[] = {
    {   6,   3, 0x29, 0, NULL, Pokecenter_Start },  /* SPRITE_NURSE, STAY, TEXT_CELADONPOKECENTER_NURSE */
    {  14,   7, 0x10, 1, "POKE FLUTE awakens\nPOKEMON with a\nsound that only\nthey can hear!", NULL },  /* SPRITE_GENTLEMAN, WALK, TEXT_CELADONPOKECENTER_GENTLEMAN */
    {  20,  11, 0x0f, 1, "I rode uphill on\nCYCLING ROAD from\nFUCHSIA!", NULL },  /* SPRITE_BEAUTY, WALK, TEXT_CELADONPOKECENTER_BEAUTY */
    {  22,   5, 0x2a, 0, NULL, NULL },  /* SPRITE_LINK_RECEPTIONIST, STAY, TEXT_CELADONPOKECENTER_LINK_RECEPTIONIST */
};

static const map_warp_t kWarps_CeladonGym[] = {
    {   8,  35, 0xff, 6 },  /* LAST_MAP */
    {  10,  35, 0xff, 6 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_CeladonGym[] = {
    {   8,   7, 0x1b, 0, "Hello. Lovely\nweather isn't it?\nIt's so pleasant.\f...Oh dear...\nI must have dozed\noff. Welcome.\fMy name is ERIKA.\nI am the LEADER\nof CELADON GYM.\fI teach the art of\nflower arranging.\nMy POKEMON are of\nthe grass-type.\fOh, I'm sorry, I\nhad no idea that\nyou wished to\nchallenge me.\fVery well, but I\nshall not lose.", NULL },  /* SPRITE_SILPH_WORKER_F, STAY, TEXT_CELADONGYM_ERIKA */
    {   4,  23, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_CELADONGYM_COOLTRAINER_F1 */
    {  14,  21, 0x0f, 0, NULL, NULL },  /* SPRITE_BEAUTY, STAY, TEXT_CELADONGYM_BEAUTY1 */
    {  18,  11, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_CELADONGYM_COOLTRAINER_F2 */
    {   2,  11, 0x0f, 0, NULL, NULL },  /* SPRITE_BEAUTY, STAY, TEXT_CELADONGYM_BEAUTY2 */
    {  12,   7, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_CELADONGYM_COOLTRAINER_F3 */
    {   6,   7, 0x0f, 0, NULL, NULL },  /* SPRITE_BEAUTY, STAY, TEXT_CELADONGYM_BEAUTY3 */
    {  10,   7, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_CELADONGYM_COOLTRAINER_F4 */
};

static const map_warp_t kWarps_GameCorner[] = {
    {  30,  35, 0xff, 7 },  /* LAST_MAP */
    {  32,  35, 0xff, 7 },  /* LAST_MAP */
    {  34,   9, 0xc7, 1 },  /* ROCKET_HIDEOUT_B1F */
};

static const npc_event_t kNpcs_GameCorner[] = {
    {   4,  13, 0x0f, 0, "Welcome!\fYou can exchange\nyour coins for\nfabulous prizes\nnext door.", NULL },  /* SPRITE_BEAUTY, STAY, TEXT_GAMECORNER_BEAUTY1 */
    {  10,  13, 0x26, 0, "Welcome to ROCKET\nGAME CORNER!\fDo you need some\ngame coins?\fIt's ¥1000 for 50\ncoins. Would you\nlike some?", NULL },  /* SPRITE_CLERK, STAY, TEXT_GAMECORNER_CLERK1 */
    {   4,  21, 0x0a, 0, "Keep this quiet.\fIt's rumored that\nthis place is run\nby TEAM ROCKET.", NULL },  /* SPRITE_MIDDLE_AGED_MAN, STAY, TEXT_GAMECORNER_MIDDLE_AGED_MAN1 */
    {   4,  27, 0x0f, 0, "I think these\nmachines have\ndifferent odds.", NULL },  /* SPRITE_BEAUTY, STAY, TEXT_GAMECORNER_BEAUTY2 */
    {  10,  23, 0x27, 0, "Kid, do you want\nto play?", NULL },  /* SPRITE_FISHING_GURU, STAY, TEXT_GAMECORNER_FISHING_GURU */
    {  16,  23, 0x1c, 0, "I'm having a\nwonderful time!", NULL },  /* SPRITE_MIDDLE_AGED_WOMAN, STAY, TEXT_GAMECORNER_MIDDLE_AGED_WOMAN */
    {  16,  29, 0x24, 0, NULL, NULL },  /* SPRITE_GYM_GUIDE, STAY, TEXT_GAMECORNER_GYM_GUIDE */
    {  22,  31, 0x0b, 0, "Games are scary!\nIt's so easy to\nget hooked!", NULL },  /* SPRITE_GAMBLER, STAY, TEXT_GAMECORNER_GAMBLER */
    {  28,  23, 0x26, 0, "What's up? Want\nsome coins?", NULL },  /* SPRITE_CLERK, STAY, TEXT_GAMECORNER_CLERK2 */
    {  34,  27, 0x10, 0, "Hey, what? You're\nthrowing me off!\nHere are some\ncoins, shoo!", NULL },  /* SPRITE_GENTLEMAN, STAY, TEXT_GAMECORNER_GENTLEMAN */
    {  18,  11, 0x18, 0, "I'm guarding this\nposter!\nGo away, or else!", NULL },  /* SPRITE_ROCKET, STAY, TEXT_GAMECORNER_ROCKET */
};

static const sign_event_t kSigns_GameCorner[] = {
    {  18,   9, "Hey!\fA switch behind\nthe poster!?\nLet's push it!@" },  /* TEXT_GAMECORNER_POSTER */
};

static const map_warp_t kWarps_CeladonMart5F[] = {
    {  24,   3, 0x7e, 0 },  /* CELADON_MART_ROOF */
    {  32,   3, 0x7d, 1 },  /* CELADON_MART_4F */
    {   2,   3, 0x7f, 0 },  /* CELADON_MART_ELEVATOR */
};

static const npc_event_t kNpcs_CeladonMart5F[] = {
    {  28,  11, 0x10, 1, "POKEMON ability\nenhancers can be\nbought only here.\fUse CALCIUM to\nincrease SPECIAL\nabilities.\fUse CARBOS to\nincrease SPEED.", NULL },  /* SPRITE_GENTLEMAN, WALK, TEXT_CELADONMART5F_GENTLEMAN */
    {   4,  13, 0x13, 0, "I'm here for\nPOKEMON ability\nenhancers.\fPROTEIN increases\nATTACK power.\fIRON increases\nDEFENSE!", NULL },  /* SPRITE_SAILOR, STAY, TEXT_CELADONMART5F_SAILOR */
    {  10,   7, 0x26, 0, NULL, Celadon5F1Mart_Start },  /* SPRITE_CLERK, STAY, TEXT_CELADONMART5F_CLERK1 */
    {  12,   7, 0x26, 0, NULL, Celadon5F2Mart_Start },  /* SPRITE_CLERK, STAY, TEXT_CELADONMART5F_CLERK2 */
};

static const sign_event_t kSigns_CeladonMart5F[] = {
    {  28,   3, "5F: DRUG STORE" },  /* TEXT_CELADONMART5F_CURRENT_FLOOR_SIGN */
};

static const map_warp_t kWarps_GameCornerPrizeRoom[] = {
    {   8,  15, 0xff, 9 },  /* LAST_MAP */
    {  10,  15, 0xff, 9 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_GameCornerPrizeRoom[] = {
    {   2,   9, 0x34, 0, "I sure do fancy\nthat PORYGON!\fBut, it's hard to\nwin at slots!", NULL },  /* SPRITE_BALDING_GUY, STAY, TEXT_GAMECORNERPRIZEROOM_BALDING_GUY */
    {  14,   7, 0x0b, 1, "I had a major\nhaul today!", NULL },  /* SPRITE_GAMBLER, WALK, TEXT_GAMECORNERPRIZEROOM_GAMBLER */
};

static const sign_event_t kSigns_GameCornerPrizeRoom[] = {
    {   4,   5, NULL },  /* TEXT_GAMECORNERPRIZEROOM_PRIZE_VENDOR_1 */
    {   8,   5, NULL },  /* TEXT_GAMECORNERPRIZEROOM_PRIZE_VENDOR_2 */
    {  12,   5, NULL },  /* TEXT_GAMECORNERPRIZEROOM_PRIZE_VENDOR_3 */
};

static const map_warp_t kWarps_CeladonDiner[] = {
    {   6,  15, 0xff, 10 },  /* LAST_MAP */
    {   8,  15, 0xff, 10 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_CeladonDiner[] = {
    {  16,  11, 0x14, 1, "Hi!\fWe're taking a\nbreak now.", NULL },  /* SPRITE_COOK, WALK, TEXT_CELADONDINER_COOK */
    {  14,   5, 0x1c, 0, "My POKEMON are\nweak, so I often\nhave to go to the\nDRUG STORE.", NULL },  /* SPRITE_MIDDLE_AGED_WOMAN, STAY, TEXT_CELADONDINER_MIDDLE_AGED_WOMAN */
    {   2,   9, 0x0a, 0, "Psst! There's a\nbasement under\nthe GAME CORNER.", NULL },  /* SPRITE_MIDDLE_AGED_MAN, STAY, TEXT_CELADONDINER_MIDDLE_AGED_MAN */
    {  10,   7, 0x2f, 0, "Munch...\fThe man at that\ntable lost it all\nat the slots.", NULL },  /* SPRITE_FISHER, STAY, TEXT_CELADONDINER_FISHER */
    {   0,   3, 0x24, 0, "Go ahead! Laugh!\fI'm flat out\nbusted!\fNo more slots for\nme! I'm going\nstraight!\fHere! I won't be\nneeding this any-\nmore!", NULL },  /* SPRITE_GYM_GUIDE, STAY, TEXT_CELADONDINER_GYM_GUIDE */
};

static const map_warp_t kWarps_CeladonChiefHouse[] = {
    {   4,  15, 0xff, 11 },  /* LAST_MAP */
    {   6,  15, 0xff, 11 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_CeladonChiefHouse[] = {
    {   8,   5, 0x25, 0, "Hehehe! The slots\njust reel in the\ndough, big time!", NULL },  /* SPRITE_GRAMPS, STAY, TEXT_CELADONCHIEFHOUSE_CHIEF */
    {   2,   9, 0x18, 1, "CHIEF!\fWe just shipped\n2000 POKEMON as\nslot prizes!", NULL },  /* SPRITE_ROCKET, WALK, TEXT_CELADONCHIEFHOUSE_ROCKET */
    {  10,  13, 0x13, 0, "Don't touch the\nposter at the\nGAME CORNER!\fThere's no secret\nswitch behind it!", NULL },  /* SPRITE_SAILOR, STAY, TEXT_CELADONCHIEFHOUSE_SAILOR */
};

static const map_warp_t kWarps_CeladonHotel[] = {
    {   6,  15, 0xff, 12 },  /* LAST_MAP */
    {   8,  15, 0xff, 12 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_CeladonHotel[] = {
    {   6,   3, 0x28, 0, "POKEMON? No, this\nis a hotel for\npeople.\fWe're full up.", NULL },  /* SPRITE_GRANNY, STAY, TEXT_CELADONHOTEL_GRANNY */
    {   4,   9, 0x0f, 0, "I'm on vacation\nwith my brother\nand boy friend.\fCELADON is such a\npretty city!", NULL },  /* SPRITE_BEAUTY, STAY, TEXT_CELADONHOTEL_BEAUTY */
    {  16,   9, 0x0c, 1, "Why did she bring\nher brother?", NULL },  /* SPRITE_SUPER_NERD, WALK, TEXT_CELADONHOTEL_SUPER_NERD */
};

static const map_warp_t kWarps_LavenderPokecenter[] = {
    {   6,  15, 0xff, 0 },  /* LAST_MAP */
    {   8,  15, 0xff, 0 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_LavenderPokecenter[] = {
    {   6,   3, 0x29, 0, NULL, Pokecenter_Start },  /* SPRITE_NURSE, STAY, TEXT_LAVENDERPOKECENTER_NURSE */
    {  10,   7, 0x10, 0, "TEAM ROCKET will\ndo anything for\nthe sake of gold!", NULL },  /* SPRITE_GENTLEMAN, STAY, TEXT_LAVENDERPOKECENTER_GENTLEMAN */
    {   4,  13, 0x08, 1, "I saw CUBONE's\nmother die trying\nto escape from\nTEAM ROCKET!", NULL },  /* SPRITE_LITTLE_GIRL, WALK, TEXT_LAVENDERPOKECENTER_LITTLE_GIRL */
    {  22,   5, 0x2a, 0, NULL, NULL },  /* SPRITE_LINK_RECEPTIONIST, STAY, TEXT_LAVENDERPOKECENTER_LINK_RECEPTIONIST */
};

static const map_warp_t kWarps_PokemonTower1F[] = {
    {  20,  35, 0xff, 1 },  /* LAST_MAP */
    {  22,  35, 0xff, 1 },  /* LAST_MAP */
    {  36,  19, 0x8f, 1 },  /* POKEMON_TOWER_2F */
};

static const npc_event_t kNpcs_PokemonTower1F[] = {
    {  30,  27, 0x2a, 0, "POKEMON TOWER was\nerected in the\nmemory of POKEMON\nthat had died.", NULL },  /* SPRITE_LINK_RECEPTIONIST, STAY, TEXT_POKEMONTOWER1F_RECEPTIONIST */
    {  12,  17, 0x1c, 0, "Did you come to\npay respects?\nBless you!", NULL },  /* SPRITE_MIDDLE_AGED_WOMAN, STAY, TEXT_POKEMONTOWER1F_MIDDLE_AGED_WOMAN */
    {  16,  25, 0x34, 0, "I came to pray\nfor my CLEFAIRY.\fSniff! I can't\nstop crying...", NULL },  /* SPRITE_BALDING_GUY, STAY, TEXT_POKEMONTOWER1F_BALDING_GUY */
    {  26,  15, 0x0d, 0, "My GROWLITHE...\nWhy did you die?", NULL },  /* SPRITE_GIRL, STAY, TEXT_POKEMONTOWER1F_GIRL */
    {  34,  15, 0x19, 0, "I am a CHANNELER!\nThere are spirits\nup to mischief!", NULL },  /* SPRITE_CHANNELER, STAY, TEXT_POKEMONTOWER1F_CHANNELER */
};

static const map_warp_t kWarps_PokemonTower2F[] = {
    {   6,  19, 0x90, 0 },  /* POKEMON_TOWER_3F */
    {  36,  19, 0x8e, 2 },  /* POKEMON_TOWER_1F */
};

static const npc_event_t kNpcs_PokemonTower2F[] = {
    {  28,  11, 0x02, 0, "{RIVAL}: Hey,\n{PLAYER}! What\nbrings you here?\nYour POKEMON\ndon't look dead!\fI can at least\nmake them faint!\nLet's go, pal!", NULL },  /* SPRITE_BLUE, STAY, TEXT_POKEMONTOWER2F_RIVAL */
    {   6,  15, 0x19, 0, "Even we could not\nidentify the\nwayward GHOSTs!\fA SILPH SCOPE\nmight be able to\nunmask them.", NULL },  /* SPRITE_CHANNELER, STAY, TEXT_POKEMONTOWER2F_CHANNELER */
};

static const map_warp_t kWarps_PokemonTower3F[] = {
    {   6,  19, 0x8f, 0 },  /* POKEMON_TOWER_2F */
    {  36,  19, 0x91, 1 },  /* POKEMON_TOWER_4F */
};

static const npc_event_t kNpcs_PokemonTower3F[] = {
    {  24,   7, 0x19, 0, NULL, NULL },  /* SPRITE_CHANNELER, STAY, TEXT_POKEMONTOWER3F_CHANNELER1 */
    {  18,  17, 0x19, 0, NULL, NULL },  /* SPRITE_CHANNELER, STAY, TEXT_POKEMONTOWER3F_CHANNELER2 */
    {  20,  27, 0x19, 0, NULL, NULL },  /* SPRITE_CHANNELER, STAY, TEXT_POKEMONTOWER3F_CHANNELER3 */
};

static const item_event_t kItems_PokemonTower3F[] = {
    {  24,   3, 0x1d },  /* ESCAPE_ROPE */
};

static const map_warp_t kWarps_PokemonTower4F[] = {
    {   6,  19, 0x92, 0 },  /* POKEMON_TOWER_5F */
    {  36,  19, 0x90, 1 },  /* POKEMON_TOWER_3F */
};

static const npc_event_t kNpcs_PokemonTower4F[] = {
    {  10,  21, 0x19, 0, NULL, NULL },  /* SPRITE_CHANNELER, STAY, TEXT_POKEMONTOWER4F_CHANNELER1 */
    {  30,  15, 0x19, 0, NULL, NULL },  /* SPRITE_CHANNELER, STAY, TEXT_POKEMONTOWER4F_CHANNELER2 */
    {  28,  25, 0x19, 0, NULL, NULL },  /* SPRITE_CHANNELER, STAY, TEXT_POKEMONTOWER4F_CHANNELER3 */
};

static const item_event_t kItems_PokemonTower4F[] = {
    {  24,  21, 0x52 },  /* ELIXER */
    {  18,  21, 0x0e },  /* AWAKENING */
    {  24,  33, 0x23 },  /* HP_UP */
};

static const map_warp_t kWarps_PokemonTower5F[] = {
    {   6,  19, 0x91, 0 },  /* POKEMON_TOWER_4F */
    {  36,  19, 0x93, 0 },  /* POKEMON_TOWER_6F */
};

static const npc_event_t kNpcs_PokemonTower5F[] = {
    {  24,  17, 0x19, 0, "Come, child! I\nsealed this space\nwith white magic!\fYou can rest here!", NULL },  /* SPRITE_CHANNELER, STAY, TEXT_POKEMONTOWER5F_CHANNELER1 */
    {  34,  15, 0x19, 0, NULL, NULL },  /* SPRITE_CHANNELER, STAY, TEXT_POKEMONTOWER5F_CHANNELER2 */
    {  28,   7, 0x19, 0, NULL, NULL },  /* SPRITE_CHANNELER, STAY, TEXT_POKEMONTOWER5F_CHANNELER3 */
    {  12,  21, 0x19, 0, NULL, NULL },  /* SPRITE_CHANNELER, STAY, TEXT_POKEMONTOWER5F_CHANNELER4 */
    {  18,  33, 0x19, 0, NULL, NULL },  /* SPRITE_CHANNELER, STAY, TEXT_POKEMONTOWER5F_CHANNELER5 */
};

static const item_event_t kItems_PokemonTower5F[] = {
    {  12,  29, 0x31 },  /* NUGGET */
};

static const map_warp_t kWarps_PokemonTower6F[] = {
    {  36,  19, 0x92, 1 },  /* POKEMON_TOWER_5F */
    {  18,  33, 0x94, 0 },  /* POKEMON_TOWER_7F */
};

static const npc_event_t kNpcs_PokemonTower6F[] = {
    {  24,  21, 0x19, 0, NULL, NULL },  /* SPRITE_CHANNELER, STAY, TEXT_POKEMONTOWER6F_CHANNELER1 */
    {  18,  11, 0x19, 0, NULL, NULL },  /* SPRITE_CHANNELER, STAY, TEXT_POKEMONTOWER6F_CHANNELER2 */
    {  32,  11, 0x19, 0, NULL, NULL },  /* SPRITE_CHANNELER, STAY, TEXT_POKEMONTOWER6F_CHANNELER3 */
};

static const item_event_t kItems_PokemonTower6F[] = {
    {  12,  17, 0x28 },  /* RARE_CANDY */
    {  28,  29, 0x2e },  /* X_ACCURACY */
};

static const map_warp_t kWarps_PokemonTower7F[] = {
    {  18,  33, 0x93, 1 },  /* POKEMON_TOWER_6F */
};

static const npc_event_t kNpcs_PokemonTower7F[] = {
    {  18,  23, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_POKEMONTOWER7F_ROCKET1 */
    {  24,  19, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_POKEMONTOWER7F_ROCKET2 */
    {  18,  15, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_POKEMONTOWER7F_ROCKET3 */
    {  20,   7, 0x16, 0, "MR.FUJI: Heh? You\ncame to save me?\fThank you. But, I\ncame here of my\nown free will.\fI came to calm\nthe soul of\nCUBONE's mother.\fI think MAROWAK's\nspirit has gone\nto the afterlife.\fI must thank you\nfor your kind\nconcern!\fFollow me to my\nhome, POKEMON\nHOUSE at the foot\nof this tower.", NULL },  /* SPRITE_MR_FUJI, STAY, TEXT_POKEMONTOWER7F_MR_FUJI */
};

static const map_warp_t kWarps_MrFujisHouse[] = {
    {   4,  15, 0xff, 2 },  /* LAST_MAP */
    {   6,  15, 0xff, 2 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_MrFujisHouse[] = {
    {   6,  11, 0x0c, 0, "That's odd, MR.FUJI\nisn't here.\nWhere'd he go?", NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_MRFUJISHOUSE_SUPER_NERD */
    {  12,   7, 0x08, 0, "This is really\nMR.FUJI's house.\fHe's really kind!\fHe looks after\nabandoned and\norphaned POKEMON!", NULL },  /* SPRITE_LITTLE_GIRL, STAY, TEXT_MRFUJISHOUSE_LITTLE_GIRL */
    {  12,   9, 0x05, 0, "PSYDUCK: Gwappa!@", NULL },  /* SPRITE_MONSTER, STAY, TEXT_MRFUJISHOUSE_PSYDUCK */
    {   2,   7, 0x05, 0, "NIDORINO: Gaoo!@", NULL },  /* SPRITE_MONSTER, STAY, TEXT_MRFUJISHOUSE_NIDORINO */
    {   6,   3, 0x16, 0, "MR.FUJI: {PLAYER}.\fYour POKEDEX quest\nmay fail without\nlove for your\nPOKEMON.\fI think this may\nhelp your quest.", NULL },  /* SPRITE_MR_FUJI, STAY, TEXT_MRFUJISHOUSE_MR_FUJI */
    {   6,   7, 0x41, 0, "POKEMON Monthly\nGrand Prize\nDrawing!\fThe application\nform is...\fGone! It's been\nclipped out!", NULL },  /* SPRITE_POKEDEX, STAY, TEXT_MRFUJISHOUSE_POKEDEX */
};

static const map_warp_t kWarps_LavenderMart[] = {
    {   6,  15, 0xff, 3 },  /* LAST_MAP */
    {   8,  15, 0xff, 3 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_LavenderMart[] = {
    {   0,  11, 0x26, 0, NULL, LavenderMart_Start },  /* SPRITE_CLERK, STAY, TEXT_LAVENDERMART_CLERK */
    {   6,   9, 0x34, 0, "I'm searching for\nitems that raise\nthe abilities of\nPOKEMON during a\nsingle battle.\fX ATTACK, X\nDEFEND, X SPEED\nand X SPECIAL are\nwhat I'm after.\fDo you know where\nI can get them?", NULL },  /* SPRITE_BALDING_GUY, STAY, TEXT_LAVENDERMART_BALDING_GUY */
    {  14,   5, 0x07, 0, "You know REVIVE?\nIt revives any\nfainted POKEMON!", NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_LAVENDERMART_COOLTRAINER_M */
};

static const map_warp_t kWarps_LavenderCuboneHouse[] = {
    {   4,  15, 0xff, 4 },  /* LAST_MAP */
    {   6,  15, 0xff, 4 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_LavenderCuboneHouse[] = {
    {   6,  11, 0x05, 0, "CUBONE: Kyarugoo!@", NULL },  /* SPRITE_MONSTER, STAY, TEXT_LAVENDERCUBONEHOUSE_CUBONE */
    {   4,   9, 0x1d, 0, "I hate those\nhorrible ROCKETs!\fThat poor CUBONE's\nmother...\fIt was killed\ntrying to escape\nfrom TEAM ROCKET!", NULL },  /* SPRITE_BRUNETTE_GIRL, STAY, TEXT_LAVENDERCUBONEHOUSE_BRUNETTE_GIRL */
};

static const map_warp_t kWarps_FuchsiaMart[] = {
    {   6,  15, 0xff, 0 },  /* LAST_MAP */
    {   8,  15, 0xff, 0 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_FuchsiaMart[] = {
    {   0,  11, 0x26, 0, NULL, FuchsiaMart_Start },  /* SPRITE_CLERK, STAY, TEXT_FUCHSIAMART_CLERK */
    {   8,   5, 0x0a, 0, "Do you have a\nSAFARI ZONE flag?\fWhat about cards\nor calendars?", NULL },  /* SPRITE_MIDDLE_AGED_MAN, STAY, TEXT_FUCHSIAMART_MIDDLE_AGED_MAN */
    {  12,  11, 0x06, 1, "Did you try X\nSPEED? It speeds\nup a POKEMON in\nbattle!", NULL },  /* SPRITE_COOLTRAINER_F, WALK, TEXT_FUCHSIAMART_COOLTRAINER_F */
};

static const map_warp_t kWarps_FuchsiaBillsGrandpasHouse[] = {
    {   4,  15, 0xff, 1 },  /* LAST_MAP */
    {   6,  15, 0xff, 1 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_FuchsiaBillsGrandpasHouse[] = {
    {   4,   7, 0x1c, 0, "SAFARI ZONE's\nWARDEN is old,\nbut still active!\fAll his teeth are\nfalse, though.", NULL },  /* SPRITE_MIDDLE_AGED_WOMAN, STAY, TEXT_FUCHSIABILLSGRANDPASHOUSE_MIDDLE_AGED_WOMAN */
    {  14,   5, 0x0b, 0, "Hmm? You've met\nBILL?\fHe's my grandson!\fHe always liked\ncollecting things\neven as a child!", NULL },  /* SPRITE_GAMBLER, STAY, TEXT_FUCHSIABILLSGRANDPASHOUSE_BILLS_GRANDPA */
    {  10,  11, 0x04, 0, "BILL files his\nown POKEMON data\non his PC!\fDid he show you?", NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_FUCHSIABILLSGRANDPASHOUSE_YOUNGSTER */
};

static const map_warp_t kWarps_FuchsiaPokecenter[] = {
    {   6,  15, 0xff, 2 },  /* LAST_MAP */
    {   8,  15, 0xff, 2 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_FuchsiaPokecenter[] = {
    {   6,   3, 0x29, 0, NULL, Pokecenter_Start },  /* SPRITE_NURSE, STAY, TEXT_FUCHSIAPOKECENTER_NURSE */
    {   4,   7, 0x21, 0, "You can't win\nwith just one\nstrong POKEMON.\fIt's tough, but\nyou have to raise\nthem evenly.", NULL },  /* SPRITE_ROCKER, STAY, TEXT_FUCHSIAPOKECENTER_ROCKER */
    {  12,  11, 0x06, 1, "There's a narrow\ntrail west of\nVIRIDIAN CITY.\fIt goes to POKEMON\nLEAGUE HQ.\nThe HQ governs\nall trainers.", NULL },  /* SPRITE_COOLTRAINER_F, WALK, TEXT_FUCHSIAPOKECENTER_COOLTRAINER_F */
    {  22,   5, 0x2a, 0, NULL, NULL },  /* SPRITE_LINK_RECEPTIONIST, STAY, TEXT_FUCHSIAPOKECENTER_LINK_RECEPTIONIST */
};

static const map_warp_t kWarps_WardensHouse[] = {
    {   8,  15, 0xff, 3 },  /* LAST_MAP */
    {  10,  15, 0xff, 3 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_WardensHouse[] = {
    {   4,   7, 0x2d, 0, "WARDEN: Hif fuff\nhefifoo!\fHa lof ha feef ee\nhafahi ho. Heff\nhee fwee!", NULL },  /* SPRITE_WARDEN, STAY, TEXT_WARDENSHOUSE_WARDEN */
    {  16,   9, 0x3f, 0, NULL, NULL },  /* SPRITE_BOULDER, STAY, TEXT_WARDENSHOUSE_BOULDER */
};

static const sign_event_t kSigns_WardensHouse[] = {
    {   8,   7, "POKEMON photos\nand fossils." },  /* TEXT_WARDENSHOUSE_DISPLAY_LEFT */
    {  10,   7, "POKEMON photos\nand fossils." },  /* TEXT_WARDENSHOUSE_DISPLAY_RIGHT */
};

static const item_event_t kItems_WardensHouse[] = {
    {  16,   7, 0x28 },  /* RARE_CANDY */
};

static const map_warp_t kWarps_SafariZoneGate[] = {
    {   6,  11, 0xff, 4 },  /* LAST_MAP */
    {   8,  11, 0xff, 4 },  /* LAST_MAP */
    {   6,   1, 0xdc, 0 },  /* SAFARI_ZONE_CENTER */
    {   8,   1, 0xdc, 1 },  /* SAFARI_ZONE_CENTER */
};

static const npc_event_t kNpcs_SafariZoneGate[] = {
    {  12,   5, 0x23, 0, "Welcome to the\nSAFARI ZONE!", NULL },  /* SPRITE_SAFARI_ZONE_WORKER, STAY, TEXT_SAFARIZONEGATE_SAFARI_ZONE_WORKER1 */
    {   2,   9, 0x23, 0, "Hi! Is it your\nfirst time here?", NULL },  /* SPRITE_SAFARI_ZONE_WORKER, STAY, TEXT_SAFARIZONEGATE_SAFARI_ZONE_WORKER2 */
};

static const map_warp_t kWarps_FuchsiaGym[] = {
    {   8,  35, 0xff, 5 },  /* LAST_MAP */
    {  10,  35, 0xff, 5 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_FuchsiaGym[] = {
    {   8,  21, 0x30, 0, "KOGA: Fwahahaha!\fA mere child like\nyou dares to\nchallenge me?\fVery well, I\nshall show you\ntrue terror as a\nninja master!\fYou shall feel\nthe despair of\npoison and sleep\ntechniques!", NULL },  /* SPRITE_KOGA, STAY, TEXT_FUCHSIAGYM_KOGA */
    {  16,  27, 0x21, 0, NULL, NULL },  /* SPRITE_ROCKER, STAY, TEXT_FUCHSIAGYM_ROCKER1 */
    {  14,  17, 0x21, 0, NULL, NULL },  /* SPRITE_ROCKER, STAY, TEXT_FUCHSIAGYM_ROCKER2 */
    {   2,  25, 0x21, 0, NULL, NULL },  /* SPRITE_ROCKER, STAY, TEXT_FUCHSIAGYM_ROCKER3 */
    {   6,  11, 0x21, 0, NULL, NULL },  /* SPRITE_ROCKER, STAY, TEXT_FUCHSIAGYM_ROCKER4 */
    {  16,   5, 0x21, 0, NULL, NULL },  /* SPRITE_ROCKER, STAY, TEXT_FUCHSIAGYM_ROCKER5 */
    {   4,  15, 0x21, 0, NULL, NULL },  /* SPRITE_ROCKER, STAY, TEXT_FUCHSIAGYM_ROCKER6 */
    {  14,  31, 0x24, 0, "Yo! Champ in\nmaking!\fFUCHSIA GYM is\nriddled with\ninvisible walls!\fKOGA might appear\nclose, but he's\nblocked off!\fYou have to find\ngaps in the walls\nto reach him!", NULL },  /* SPRITE_GYM_GUIDE, STAY, TEXT_FUCHSIAGYM_GYM_GUIDE */
};

static const map_warp_t kWarps_FuchsiaMeetingRoom[] = {
    {   8,  15, 0xff, 6 },  /* LAST_MAP */
    {  10,  15, 0xff, 6 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_FuchsiaMeetingRoom[] = {
    {   8,   3, 0x23, 0, "We nicknamed the\nWARDEN SLOWPOKE.\fHe and SLOWPOKE\nboth look vacant!", NULL },  /* SPRITE_SAFARI_ZONE_WORKER, STAY, TEXT_FUCHSIAMEETINGROOM_SAFARI_ZONE_WORKER1 */
    {   0,   5, 0x23, 0, "SLOWPOKE is very\nknowledgeable\nabout POKEMON!\fHe even has some\nfossils of rare,\nextinct POKEMON!", NULL },  /* SPRITE_SAFARI_ZONE_WORKER, STAY, TEXT_FUCHSIAMEETINGROOM_SAFARI_ZONE_WORKER2 */
    {  20,   3, 0x23, 0, "SLOWPOKE came in,\nbut I couldn't\nunderstand him.\fI think he's got\na speech problem!", NULL },  /* SPRITE_SAFARI_ZONE_WORKER, STAY, TEXT_FUCHSIAMEETINGROOM_SAFARI_ZONE_WORKER3 */
};

static const map_warp_t kWarps_SeafoamIslandsB1F[] = {
    {   8,   5, 0xa0, 0 },  /* SEAFOAM_ISLANDS_B2F */
    {  14,  11, 0xc0, 4 },  /* SEAFOAM_ISLANDS_1F */
    {  26,  15, 0xa0, 2 },  /* SEAFOAM_ISLANDS_B2F */
    {  38,  31, 0xa0, 3 },  /* SEAFOAM_ISLANDS_B2F */
    {  46,  31, 0xc0, 6 },  /* SEAFOAM_ISLANDS_1F */
    {  50,  23, 0xa0, 5 },  /* SEAFOAM_ISLANDS_B2F */
    {  50,   7, 0xc0, 5 },  /* SEAFOAM_ISLANDS_1F */
};

static const npc_event_t kNpcs_SeafoamIslandsB1F[] = {
    {  34,  13, 0x3f, 0, NULL, NULL },  /* SPRITE_BOULDER, STAY, TEXT_SEAFOAMISLANDSB1F_BOULDER1 */
    {  44,  13, 0x3f, 0, NULL, NULL },  /* SPRITE_BOULDER, STAY, TEXT_SEAFOAMISLANDSB1F_BOULDER2 */
};

static const map_warp_t kWarps_SeafoamIslandsB2F[] = {
    {  10,   7, 0x9f, 0 },  /* SEAFOAM_ISLANDS_B1F */
    {  10,  27, 0xa1, 0 },  /* SEAFOAM_ISLANDS_B3F */
    {  26,  15, 0x9f, 2 },  /* SEAFOAM_ISLANDS_B1F */
    {  38,  31, 0x9f, 3 },  /* SEAFOAM_ISLANDS_B1F */
    {  50,   7, 0xa1, 3 },  /* SEAFOAM_ISLANDS_B3F */
    {  50,  23, 0x9f, 5 },  /* SEAFOAM_ISLANDS_B1F */
    {  50,  29, 0xa1, 4 },  /* SEAFOAM_ISLANDS_B3F */
};

static const npc_event_t kNpcs_SeafoamIslandsB2F[] = {
    {  36,  13, 0x3f, 0, NULL, NULL },  /* SPRITE_BOULDER, STAY, TEXT_SEAFOAMISLANDSB2F_BOULDER1 */
    {  46,  13, 0x3f, 0, NULL, NULL },  /* SPRITE_BOULDER, STAY, TEXT_SEAFOAMISLANDSB2F_BOULDER2 */
};

static const map_warp_t kWarps_SeafoamIslandsB3F[] = {
    {  10,  25, 0xa0, 1 },  /* SEAFOAM_ISLANDS_B2F */
    {  16,  13, 0xa2, 2 },  /* SEAFOAM_ISLANDS_B4F */
    {  50,   9, 0xa2, 3 },  /* SEAFOAM_ISLANDS_B4F */
    {  50,   7, 0xa0, 4 },  /* SEAFOAM_ISLANDS_B2F */
    {  50,  29, 0xa0, 6 },  /* SEAFOAM_ISLANDS_B2F */
    {  40,  35, 0xa2, 0 },  /* SEAFOAM_ISLANDS_B4F */
    {  42,  35, 0xa2, 1 },  /* SEAFOAM_ISLANDS_B4F */
};

static const npc_event_t kNpcs_SeafoamIslandsB3F[] = {
    {  10,  29, 0x3f, 0, NULL, NULL },  /* SPRITE_BOULDER, STAY, TEXT_SEAFOAMISLANDSB3F_BOULDER1 */
    {   6,  31, 0x3f, 0, NULL, NULL },  /* SPRITE_BOULDER, STAY, TEXT_SEAFOAMISLANDSB3F_BOULDER2 */
    {  16,  29, 0x3f, 0, NULL, NULL },  /* SPRITE_BOULDER, STAY, TEXT_SEAFOAMISLANDSB3F_BOULDER3 */
    {  18,  29, 0x3f, 0, NULL, NULL },  /* SPRITE_BOULDER, STAY, TEXT_SEAFOAMISLANDSB3F_BOULDER4 */
    {  36,  13, 0x3f, 0, NULL, NULL },  /* SPRITE_BOULDER, STAY, TEXT_SEAFOAMISLANDSB3F_BOULDER5 */
    {  38,  13, 0x3f, 0, NULL, NULL },  /* SPRITE_BOULDER, STAY, TEXT_SEAFOAMISLANDSB3F_BOULDER6 */
};

static const map_warp_t kWarps_SeafoamIslandsB4F[] = {
    {  40,  35, 0xa1, 5 },  /* SEAFOAM_ISLANDS_B3F */
    {  42,  35, 0xa1, 6 },  /* SEAFOAM_ISLANDS_B3F */
    {  22,  15, 0xa1, 1 },  /* SEAFOAM_ISLANDS_B3F */
    {  50,   9, 0xa1, 2 },  /* SEAFOAM_ISLANDS_B3F */
};

static const npc_event_t kNpcs_SeafoamIslandsB4F[] = {
    {   8,  31, 0x3f, 0, NULL, NULL },  /* SPRITE_BOULDER, STAY, TEXT_SEAFOAMISLANDSB4F_BOULDER1 */
    {  10,  31, 0x3f, 0, NULL, NULL },  /* SPRITE_BOULDER, STAY, TEXT_SEAFOAMISLANDSB4F_BOULDER2 */
    {  12,   3, 0x09, 0, NULL, NULL },  /* SPRITE_BIRD, STAY, TEXT_SEAFOAMISLANDSB4F_ARTICUNO */
};

static const sign_event_t kSigns_SeafoamIslandsB4F[] = {
    {  18,  31, "Boulders might\nchange the flow\nof water!" },  /* TEXT_SEAFOAMISLANDSB4F_BOULDERS_SIGN */
    {  46,   3, "DANGER\nFast current!" },  /* TEXT_SEAFOAMISLANDSB4F_DANGER_SIGN */
};

static const map_warp_t kWarps_VermilionOldRodHouse[] = {
    {   4,  15, 0xff, 8 },  /* LAST_MAP */
    {   6,  15, 0xff, 8 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_VermilionOldRodHouse[] = {
    {   4,   9, 0x27, 0, "I'm the FISHING\nGURU!\fI simply Looove\nfishing!\fDo you like to\nfish?", NULL },  /* SPRITE_FISHING_GURU, STAY, TEXT_VERMILIONOLDRODHOUSE_FISHING_GURU */
};

static const map_warp_t kWarps_FuchsiaGoodRodHouse[] = {
    {   4,   1, 0xff, 8 },  /* LAST_MAP */
    {   4,  15, 0xff, 7 },  /* LAST_MAP */
    {   6,  15, 0xff, 7 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_FuchsiaGoodRodHouse[] = {
    {  10,   7, 0x27, 0, "I'm the FISHING\nGURU's older\nbrother!\fI simply Looove\nfishing!\fDo you like to\nfish?", NULL },  /* SPRITE_FISHING_GURU, STAY, TEXT_FUCHSIAGOODRODHOUSE_FISHING_GURU */
};

static const map_warp_t kWarps_PokemonMansion1F[] = {
    {   8,  55, 0xff, 0 },  /* LAST_MAP */
    {  10,  55, 0xff, 0 },  /* LAST_MAP */
    {  12,  55, 0xff, 0 },  /* LAST_MAP */
    {  14,  55, 0xff, 0 },  /* LAST_MAP */
    {  10,  21, 0xd6, 0 },  /* POKEMON_MANSION_2F */
    {  42,  47, 0xd8, 0 },  /* POKEMON_MANSION_B1F */
    {  52,  55, 0xff, 0 },  /* LAST_MAP */
    {  54,  55, 0xff, 0 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_PokemonMansion1F[] = {
    {  34,  35, 0x20, 0, NULL, NULL },  /* SPRITE_SCIENTIST, STAY, TEXT_POKEMONMANSION1F_SCIENTIST */
};

static const item_event_t kItems_PokemonMansion1F[] = {
    {  28,   7, 0x1d },  /* ESCAPE_ROPE */
    {  36,  43, 0x26 },  /* CARBOS */
};

static const map_warp_t kWarps_CinnabarGym[] = {
    {  32,  35, 0xff, 1 },  /* LAST_MAP */
    {  34,  35, 0xff, 1 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_CinnabarGym[] = {
    {   6,   7, 0x0a, 0, "Hah!\fI am BLAINE! I\nam the LEADER of\nCINNABAR GYM!\fMy fiery POKEMON\nwill incinerate\nall challengers!\fHah! You better\nhave BURN HEAL!", NULL },  /* SPRITE_MIDDLE_AGED_MAN, STAY, TEXT_CINNABARGYM_BLAINE */
    {  34,   5, 0x0c, 0, "Do you know how\nhot POKEMON fire\nbreath can get?", NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_CINNABARGYM_SUPER_NERD1 */
    {  34,  17, 0x0c, 0, "I was a thief, but\nI became straight\nas a trainer!", NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_CINNABARGYM_SUPER_NERD2 */
    {  22,   9, 0x0c, 0, "You can't win!\nI have studied\nPOKEMON totally!", NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_CINNABARGYM_SUPER_NERD3 */
    {  22,  17, 0x0c, 0, "I just like using\nfire POKEMON!", NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_CINNABARGYM_SUPER_NERD4 */
    {  22,  29, 0x0c, 0, "I know why BLAINE\nbecame a trainer!", NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_CINNABARGYM_SUPER_NERD5 */
    {   6,  29, 0x0c, 0, "I've been to many\nGYMs, but this is\nmy favorite!", NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_CINNABARGYM_SUPER_NERD6 */
    {   6,  17, 0x0c, 0, "Fire is weak\nagainst H2O!", NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_CINNABARGYM_SUPER_NERD7 */
    {  32,  27, 0x24, 0, "Yo! Champ in\nmaking!\fThe hot-headed\nBLAINE is a fire\nPOKEMON pro!\fDouse his spirits\nwith water!\fYou better take\nsome BURN HEALs!", NULL },  /* SPRITE_GYM_GUIDE, STAY, TEXT_CINNABARGYM_GYM_GUIDE */
};

static const map_warp_t kWarps_CinnabarLab[] = {
    {   4,  15, 0xff, 2 },  /* LAST_MAP */
    {   6,  15, 0xff, 2 },  /* LAST_MAP */
    {  16,   9, 0xa8, 0 },  /* CINNABAR_LAB_TRADE_ROOM */
    {  24,   9, 0xa9, 0 },  /* CINNABAR_LAB_METRONOME_ROOM */
    {  32,   9, 0xaa, 0 },  /* CINNABAR_LAB_FOSSIL_ROOM */
};

static const npc_event_t kNpcs_CinnabarLab[] = {
    {   2,   7, 0x27, 0, "We study POKEMON\nextensively here.\fPeople often bring\nus rare POKEMON\nfor examination.", NULL },  /* SPRITE_FISHING_GURU, STAY, TEXT_CINNABARLAB_FISHING_GURU */
};

static const sign_event_t kSigns_CinnabarLab[] = {
    {   6,   5, "A photo of the\nLAB's founder,\nDR.FUJI!" },  /* TEXT_CINNABARLAB_PHOTO */
    {  18,   9, "POKEMON LAB\nMeeting Room" },  /* TEXT_CINNABARLAB_MEETING_ROOM_SIGN */
    {  26,   9, "POKEMON LAB\nR-and-D Room" },  /* TEXT_CINNABARLAB_R_AND_D_SIGN */
    {  34,   9, "POKEMON LAB\nTesting Room" },  /* TEXT_CINNABARLAB_TESTING_ROOM_SIGN */
};

static const map_warp_t kWarps_CinnabarLabTradeRoom[] = {
    {   4,  15, 0xa7, 2 },  /* CINNABAR_LAB */
    {   6,  15, 0xa7, 2 },  /* CINNABAR_LAB */
};

static const npc_event_t kNpcs_CinnabarLabTradeRoom[] = {
    {   6,   5, 0x0c, 0, "I found this very\nstrange fossil in\nMT.MOON!\fI think it's a\nrare, prehistoric\nPOKEMON!", NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_CINNABARLABTRADEROOM_SUPER_NERD */
    {   2,   9, 0x25, 0, NULL, NULL },  /* SPRITE_GRAMPS, STAY, TEXT_CINNABARLABTRADEROOM_GRAMPS */
    {  10,  11, 0x0f, 0, NULL, NULL },  /* SPRITE_BEAUTY, STAY, TEXT_CINNABARLABTRADEROOM_BEAUTY */
};

static const map_warp_t kWarps_CinnabarLabMetronomeRoom[] = {
    {   4,  15, 0xa7, 3 },  /* CINNABAR_LAB */
    {   6,  15, 0xa7, 3 },  /* CINNABAR_LAB */
};

static const npc_event_t kNpcs_CinnabarLabMetronomeRoom[] = {
    {  14,   5, 0x20, 0, "Tch-tch-tch!\nI made a cool TM!\fIt can cause all\nkinds of fun!", NULL },  /* SPRITE_SCIENTIST, STAY, TEXT_CINNABARLABMETRONOMEROOM_SCIENTIST1 */
    {   4,   7, 0x20, 1, "EEVEE can evolve\ninto 1 of 3 kinds\nof POKEMON.", NULL },  /* SPRITE_SCIENTIST, WALK, TEXT_CINNABARLABMETRONOMEROOM_SCIENTIST2 */
};

static const sign_event_t kSigns_CinnabarLabMetronomeRoom[] = {
    {   0,   9, "There's an e-mail\nmessage!\f...\fThe 3 legendary\nbird POKEMON are\nARTICUNO, ZAPDOS\nand MOLTRES.\fTheir whereabouts\nare unknown.\fWe plan to explore\nthe cavern close\nto CERULEAN.\fFrom: POKEMON\nRESEARCH TEAM\f..." },  /* TEXT_CINNABARLABMETRONOMEROOM_PC_KEYBOARD */
    {   2,   9, "There's an e-mail\nmessage!\f...\fThe 3 legendary\nbird POKEMON are\nARTICUNO, ZAPDOS\nand MOLTRES.\fTheir whereabouts\nare unknown.\fWe plan to explore\nthe cavern close\nto CERULEAN.\fFrom: POKEMON\nRESEARCH TEAM\f..." },  /* TEXT_CINNABARLABMETRONOMEROOM_PC_MONITOR */
    {   4,   3, "An amber pipe!" },  /* TEXT_CINNABARLABMETRONOMEROOM_AMBER_PIPE */
};

static const map_warp_t kWarps_CinnabarLabFossilRoom[] = {
    {   4,  15, 0xa7, 4 },  /* CINNABAR_LAB */
    {   6,  15, 0xa7, 4 },  /* CINNABAR_LAB */
};

static const npc_event_t kNpcs_CinnabarLabFossilRoom[] = {
    {  10,   5, 0x20, 1, "Hiya!\fI am important\ndoctor!\fI study here rare\nPOKEMON fossils!\fYou! Have you a\nfossil for me?", NULL },  /* SPRITE_SCIENTIST, WALK, TEXT_CINNABARLABFOSSILROOM_SCIENTIST1 */
    {  14,  13, 0x20, 0, NULL, NULL },  /* SPRITE_SCIENTIST, STAY, TEXT_CINNABARLABFOSSILROOM_SCIENTIST2 */
};

static const map_warp_t kWarps_CinnabarPokecenter[] = {
    {   6,  15, 0xff, 3 },  /* LAST_MAP */
    {   8,  15, 0xff, 3 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_CinnabarPokecenter[] = {
    {   6,   3, 0x29, 0, NULL, Pokecenter_Start },  /* SPRITE_NURSE, STAY, TEXT_CINNABARPOKECENTER_NURSE */
    {  18,   9, 0x06, 1, "You can cancel\nevolution.\fWhen a POKEMON is\nevolving, you can\nstop it and leave\nit the way it is.", NULL },  /* SPRITE_COOLTRAINER_F, WALK, TEXT_CINNABARPOKECENTER_COOLTRAINER_F */
    {   4,  13, 0x10, 0, "Do you have any\nfriends?\fPOKEMON you get\nin trades grow\nvery quickly.\fI think it's\nworth a try!", NULL },  /* SPRITE_GENTLEMAN, STAY, TEXT_CINNABARPOKECENTER_GENTLEMAN */
    {  22,   5, 0x2a, 0, NULL, NULL },  /* SPRITE_LINK_RECEPTIONIST, STAY, TEXT_CINNABARPOKECENTER_LINK_RECEPTIONIST */
};

static const map_warp_t kWarps_CinnabarMart[] = {
    {   6,  15, 0xff, 4 },  /* LAST_MAP */
    {   8,  15, 0xff, 4 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_CinnabarMart[] = {
    {   0,  11, 0x26, 0, NULL, CinnabarMart_Start },  /* SPRITE_CLERK, STAY, TEXT_CINNABARMART_CLERK */
    {  12,   5, 0x1b, 0, "Don't they have X\nATTACK? It's good\nfor battles!", NULL },  /* SPRITE_SILPH_WORKER_F, STAY, TEXT_CINNABARMART_SILPH_WORKER_F */
    {   6,   9, 0x20, 0, "It never hurts to\nhave extra items!", NULL },  /* SPRITE_SCIENTIST, STAY, TEXT_CINNABARMART_SCIENTIST */
};

static const map_warp_t kWarps_IndigoPlateauLobby[] = {
    {  14,  23, 0xff, 0 },  /* LAST_MAP */
    {  16,  23, 0xff, 1 },  /* LAST_MAP */
    {  16,   1, 0xf5, 0 },  /* LORELEIS_ROOM */
};

static const npc_event_t kNpcs_IndigoPlateauLobby[] = {
    {  14,  11, 0x29, 0, NULL, Pokecenter_Start },  /* SPRITE_NURSE, STAY, TEXT_INDIGOPLATEAULOBBY_NURSE */
    {   8,  19, 0x24, 0, "Yo! Champ in\nmaking!\fAt POKEMON LEAGUE,\nyou have to face\nthe ELITE FOUR in\nsuccession.\fIf you lose, you\nhave to start all\nover again! This\nis it! Go for it!", NULL },  /* SPRITE_GYM_GUIDE, STAY, TEXT_INDIGOPLATEAULOBBY_GYM_GUIDE */
    {  10,   3, 0x06, 0, "From here on, you\nface the ELITE\nFOUR one by one!\fIf you win, a\ndoor opens to the\nnext trainer!\nGood luck!", NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_INDIGOPLATEAULOBBY_COOLTRAINER_F */
    {   0,  11, 0x26, 0, NULL, IndigoMart_Start },  /* SPRITE_CLERK, STAY, TEXT_INDIGOPLATEAULOBBY_CLERK */
    {  26,  13, 0x2a, 0, NULL, NULL },  /* SPRITE_LINK_RECEPTIONIST, STAY, TEXT_INDIGOPLATEAULOBBY_LINK_RECEPTIONIST */
};

static const map_warp_t kWarps_CopycatsHouse1F[] = {
    {   4,  15, 0xff, 0 },  /* LAST_MAP */
    {   6,  15, 0xff, 0 },  /* LAST_MAP */
    {  14,   3, 0xb0, 0 },  /* COPYCATS_HOUSE_2F */
};

static const npc_event_t kNpcs_CopycatsHouse1F[] = {
    {   4,   5, 0x1c, 0, "My daughter is so\nself-centered.\nShe only has a\nfew friends.", NULL },  /* SPRITE_MIDDLE_AGED_WOMAN, STAY, TEXT_COPYCATSHOUSE1F_MIDDLE_AGED_WOMAN */
    {  10,   9, 0x0a, 0, "My daughter likes\nto mimic people.\fHer mimicry has\nearned her the\nnickname COPYCAT\naround here!", NULL },  /* SPRITE_MIDDLE_AGED_MAN, STAY, TEXT_COPYCATSHOUSE1F_MIDDLE_AGED_MAN */
    {   2,   9, 0x38, 1, "CHANSEY: Chaan!\nSii!@", NULL },  /* SPRITE_FAIRY, WALK, TEXT_COPYCATSHOUSE1F_CHANSEY */
};

static const map_warp_t kWarps_CopycatsHouse2F[] = {
    {  14,   3, 0xaf, 2 },  /* COPYCATS_HOUSE_1F */
};

static const npc_event_t kNpcs_CopycatsHouse2F[] = {
    {   8,   7, 0x1d, 1, "{PLAYER}: Hi! Do\nyou like POKEMON?\f{PLAYER}: Uh no, I\njust asked you.\f{PLAYER}: Huh?\nYou're strange!\fCOPYCAT: Hmm?\nQuit mimicking?\fBut, that's my\nfavorite hobby!", NULL },  /* SPRITE_BRUNETTE_GIRL, WALK, TEXT_COPYCATSHOUSE2F_COPYCAT */
    {   8,  13, 0x09, 1, "DODUO: Giiih!\fMIRROR MIRROR ON\nTHE WALL, WHO IS\nTHE FAIREST ONE\nOF ALL?", NULL },  /* SPRITE_BIRD, WALK, TEXT_COPYCATSHOUSE2F_DODUO */
    {  10,   3, 0x05, 0, "This is a rare\nPOKEMON! Huh?\nIt's only a doll!", NULL },  /* SPRITE_MONSTER, STAY, TEXT_COPYCATSHOUSE2F_MONSTER */
    {   4,   1, 0x09, 0, "This is a rare\nPOKEMON! Huh?\nIt's only a doll!", NULL },  /* SPRITE_BIRD, STAY, TEXT_COPYCATSHOUSE2F_BIRD */
    {   2,  13, 0x38, 0, "This is a rare\nPOKEMON! Huh?\nIt's only a doll!", NULL },  /* SPRITE_FAIRY, STAY, TEXT_COPYCATSHOUSE2F_FAIRY */
};

static const sign_event_t kSigns_CopycatsHouse2F[] = {
    {   6,  11, "A game with MARIO\nwearing a bucket\non his head!" },  /* TEXT_COPYCATSHOUSE2F_SNES */
    {   0,   3, "...\fMy Secrets!\fSkill: Mimicry!\nHobby: Collecting\ndolls!\nFavorite POKEMON:\nCLEFAIRY!" },  /* TEXT_COPYCATSHOUSE2F_PC */
};

static const map_warp_t kWarps_FightingDojo[] = {
    {   8,  23, 0xff, 1 },  /* LAST_MAP */
    {  10,  23, 0xff, 1 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_FightingDojo[] = {
    {  10,   7, 0x0e, 0, "Grunt!\fI am the KARATE\nMASTER! I am the\nLEADER here!\fYou wish to\nchallenge us?\nExpect no mercy!\fFwaaa!", NULL },  /* SPRITE_HIKER, STAY, TEXT_FIGHTINGDOJO_KARATE_MASTER */
    {   6,   9, 0x0e, 0, NULL, NULL },  /* SPRITE_HIKER, STAY, TEXT_FIGHTINGDOJO_BLACKBELT1 */
    {   6,  13, 0x0e, 0, NULL, NULL },  /* SPRITE_HIKER, STAY, TEXT_FIGHTINGDOJO_BLACKBELT2 */
    {  10,  11, 0x0e, 0, NULL, NULL },  /* SPRITE_HIKER, STAY, TEXT_FIGHTINGDOJO_BLACKBELT3 */
    {  10,  15, 0x0e, 0, NULL, NULL },  /* SPRITE_HIKER, STAY, TEXT_FIGHTINGDOJO_BLACKBELT4 */
};

static const map_warp_t kWarps_SaffronGym[] = {
    {  16,  35, 0xff, 2 },  /* LAST_MAP */
    {  18,  35, 0xff, 2 },  /* LAST_MAP */
    {   2,   7, 0xb2, 22 },  /* SAFFRON_GYM */
    {  10,   7, 0xb2, 15 },  /* SAFFRON_GYM */
    {   2,  11, 0xb2, 18 },  /* SAFFRON_GYM */
    {  10,  11, 0xb2, 8 },  /* SAFFRON_GYM */
    {   2,  19, 0xb2, 27 },  /* SAFFRON_GYM */
    {  10,  19, 0xb2, 16 },  /* SAFFRON_GYM */
    {   2,  23, 0xb2, 5 },  /* SAFFRON_GYM */
    {  10,  23, 0xb2, 13 },  /* SAFFRON_GYM */
    {   2,  31, 0xb2, 23 },  /* SAFFRON_GYM */
    {  10,  31, 0xb2, 30 },  /* SAFFRON_GYM */
    {   2,  35, 0xb2, 17 },  /* SAFFRON_GYM */
    {  10,  35, 0xb2, 9 },  /* SAFFRON_GYM */
    {  18,   7, 0xb2, 26 },  /* SAFFRON_GYM */
    {  22,   7, 0xb2, 3 },  /* SAFFRON_GYM */
    {  18,  11, 0xb2, 7 },  /* SAFFRON_GYM */
    {  22,  11, 0xb2, 12 },  /* SAFFRON_GYM */
    {  22,  23, 0xb2, 4 },  /* SAFFRON_GYM */
    {  22,  31, 0xb2, 31 },  /* SAFFRON_GYM */
    {  30,   7, 0xb2, 24 },  /* SAFFRON_GYM */
    {  38,   7, 0xb2, 28 },  /* SAFFRON_GYM */
    {  30,  11, 0xb2, 2 },  /* SAFFRON_GYM */
    {  38,  11, 0xb2, 10 },  /* SAFFRON_GYM */
    {  30,  19, 0xb2, 20 },  /* SAFFRON_GYM */
    {  38,  19, 0xb2, 29 },  /* SAFFRON_GYM */
    {  30,  23, 0xb2, 14 },  /* SAFFRON_GYM */
    {  38,  23, 0xb2, 6 },  /* SAFFRON_GYM */
    {  30,  31, 0xb2, 21 },  /* SAFFRON_GYM */
    {  38,  31, 0xb2, 25 },  /* SAFFRON_GYM */
    {  30,  35, 0xb2, 11 },  /* SAFFRON_GYM */
    {  38,  35, 0xb2, 19 },  /* SAFFRON_GYM */
};

static const npc_event_t kNpcs_SaffronGym[] = {
    {  18,  17, 0x0d, 0, "I had a vision of\nyour arrival!\fI have had psychic\npowers since I\nwas a child.\fI first learned\nto bend spoons\nwith my mind.\fI dislike fight-\ning, but if you\nwish, I will show\nyou my powers!", NULL },  /* SPRITE_GIRL, STAY, TEXT_SAFFRONGYM_SABRINA */
    {  20,   3, 0x19, 0, NULL, NULL },  /* SPRITE_CHANNELER, STAY, TEXT_SAFFRONGYM_CHANNELER1 */
    {  34,   3, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_SAFFRONGYM_YOUNGSTER1 */
    {   6,  15, 0x19, 0, NULL, NULL },  /* SPRITE_CHANNELER, STAY, TEXT_SAFFRONGYM_CHANNELER2 */
    {  34,  15, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_SAFFRONGYM_YOUNGSTER2 */
    {   6,  27, 0x19, 0, NULL, NULL },  /* SPRITE_CHANNELER, STAY, TEXT_SAFFRONGYM_CHANNELER3 */
    {  34,  27, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_SAFFRONGYM_YOUNGSTER3 */
    {   6,   3, 0x04, 0, NULL, NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_SAFFRONGYM_YOUNGSTER4 */
    {  20,  31, 0x24, 0, "Yo! Champ in\nmaking!\fSABRINA's POKEMON\nuse psychic power\ninstead of force!\fFighting POKEMON\nare weak against\npsychic POKEMON!\fThey get creamed\nbefore they can\neven aim a punch!", NULL },  /* SPRITE_GYM_GUIDE, STAY, TEXT_SAFFRONGYM_GYM_GUIDE */
};

static const map_warp_t kWarps_SaffronPidgeyHouse[] = {
    {   4,  15, 0xff, 3 },  /* LAST_MAP */
    {   6,  15, 0xff, 3 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_SaffronPidgeyHouse[] = {
    {   4,   7, 0x1d, 0, "Thank you for\nwriting. I hope\nto see you soon!\fHey! Don't look\nat my letter!", NULL },  /* SPRITE_BRUNETTE_GIRL, STAY, TEXT_SAFFRONPIDGEYHOUSE_BRUNETTE_GIRL */
    {   0,   9, 0x09, 1, "PIDGEY: Kurukkoo!@", NULL },  /* SPRITE_BIRD, WALK, TEXT_SAFFRONPIDGEYHOUSE_PIDGEY */
    {   8,   3, 0x04, 0, "The COPYCAT is\ncute! I'm getting\nher a POKE DOLL!", NULL },  /* SPRITE_YOUNGSTER, STAY, TEXT_SAFFRONPIDGEYHOUSE_YOUNGSTER */
    {   6,   7, 0x40, 0, "I was given a PP\nUP as a gift.\fIt's used for\nincreasing the PP\nof techniques!", NULL },  /* SPRITE_PAPER, STAY, TEXT_SAFFRONPIDGEYHOUSE_PAPER */
};

static const map_warp_t kWarps_SaffronMart[] = {
    {   6,  15, 0xff, 4 },  /* LAST_MAP */
    {   8,  15, 0xff, 4 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_SaffronMart[] = {
    {   0,  11, 0x26, 0, NULL, SaffronMart_Start },  /* SPRITE_CLERK, STAY, TEXT_SAFFRONMART_CLERK */
    {   8,   5, 0x0c, 0, "MAX REPEL lasts\nlonger than SUPER\nREPEL for keeping\nweaker POKEMON\naway!", NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_SAFFRONMART_SUPER_NERD */
    {  12,  11, 0x06, 1, "REVIVE is costly,\nbut it revives\nfainted POKEMON!", NULL },  /* SPRITE_COOLTRAINER_F, WALK, TEXT_SAFFRONMART_COOLTRAINER_F */
};

static const map_warp_t kWarps_SilphCo1F[] = {
    {  20,  35, 0xff, 5 },  /* LAST_MAP */
    {  22,  35, 0xff, 5 },  /* LAST_MAP */
    {  52,   1, 0xcf, 0 },  /* SILPH_CO_2F */
    {  40,   1, 0xec, 0 },  /* SILPH_CO_ELEVATOR */
    {  32,  21, 0xd0, 6 },  /* SILPH_CO_3F */
};

static const npc_event_t kNpcs_SilphCo1F[] = {
    {   8,   5, 0x2a, 0, "Welcome!\fThe PRESIDENT is\nin the boardroom\non 11F!", NULL },  /* SPRITE_LINK_RECEPTIONIST, STAY, TEXT_SILPHCO1F_LINK_RECEPTIONIST */
};

static const map_warp_t kWarps_SaffronPokecenter[] = {
    {   6,  15, 0xff, 6 },  /* LAST_MAP */
    {   8,  15, 0xff, 6 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_SaffronPokecenter[] = {
    {   6,   3, 0x29, 0, NULL, Pokecenter_Start },  /* SPRITE_NURSE, STAY, TEXT_SAFFRONPOKECENTER_NURSE */
    {  10,  11, 0x0f, 0, "POKEMON growth\nrates differ from\nspecie to specie.", NULL },  /* SPRITE_BEAUTY, STAY, TEXT_SAFFRONPOKECENTER_BEAUTY */
    {  16,   7, 0x10, 0, "SILPH CO. is very\nfamous. That's\nwhy it attracted\nTEAM ROCKET!", NULL },  /* SPRITE_GENTLEMAN, STAY, TEXT_SAFFRONPOKECENTER_GENTLEMAN */
    {  22,   5, 0x2a, 0, NULL, NULL },  /* SPRITE_LINK_RECEPTIONIST, STAY, TEXT_SAFFRONPOKECENTER_LINK_RECEPTIONIST */
};

static const map_warp_t kWarps_MrPsychicsHouse[] = {
    {   4,  15, 0xff, 7 },  /* LAST_MAP */
    {   6,  15, 0xff, 7 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_MrPsychicsHouse[] = {
    {  10,   7, 0x27, 0, "...Wait! Don't\nsay a word!\fYou wanted this!", NULL },  /* SPRITE_FISHING_GURU, STAY, TEXT_MRPSYCHICSHOUSE_MR_PSYCHIC */
};

static const map_warp_t kWarps_Route15Gate1F[] = {
    {   0,   9, 0xff, 0 },  /* LAST_MAP */
    {   0,  11, 0xff, 1 },  /* LAST_MAP */
    {  14,   9, 0xff, 2 },  /* LAST_MAP */
    {  14,  11, 0xff, 3 },  /* LAST_MAP */
    {  12,  17, 0xb9, 0 },  /* ROUTE_15_GATE_2F */
};

static const npc_event_t kNpcs_Route15Gate1F[] = {
    {   8,   3, 0x31, 0, "Are you working\non a POKEDEX?\fPROF.OAK's AIDE\ncame by here.", NULL },  /* SPRITE_GUARD, STAY, TEXT_ROUTE15GATE1F_GUARD */
};

static const map_warp_t kWarps_Route15Gate2F[] = {
    {  14,  15, 0xb8, 4 },  /* ROUTE_15_GATE_1F */
};

static const npc_event_t kNpcs_Route15Gate2F[] = {
    {   8,   5, 0x20, 0, "EXP.ALL gives\nEXP points to all\nthe POKEMON with\nyou, even if they\ndon't fight.\fIt does, however,\nreduce the amount\nof EXP for each\nPOKEMON.\fIf you don't need\nit, you should \nstore it via PC.", NULL },  /* SPRITE_SCIENTIST, STAY, TEXT_ROUTE15GATE2F_OAKS_AIDE */
};

static const sign_event_t kSigns_Route15Gate2F[] = {
    {  12,   5, "Looked into the\nbinoculars.\fIt looks like a\nsmall island!" },  /* TEXT_ROUTE15GATE2F_BINOCULARS */
};

static const map_warp_t kWarps_Route16Gate1F[] = {
    {   0,  17, 0xff, 0 },  /* LAST_MAP */
    {   0,  19, 0xff, 1 },  /* LAST_MAP */
    {  14,  17, 0xff, 2 },  /* LAST_MAP */
    {  14,  19, 0xff, 2 },  /* LAST_MAP */
    {   0,   5, 0xff, 4 },  /* LAST_MAP */
    {   0,   7, 0xff, 5 },  /* LAST_MAP */
    {  14,   5, 0xff, 6 },  /* LAST_MAP */
    {  14,   7, 0xff, 7 },  /* LAST_MAP */
    {  12,  25, 0xbb, 0 },  /* ROUTE_16_GATE_2F */
};

static const npc_event_t kNpcs_Route16Gate1F[] = {
    {   8,  11, 0x31, 0, "No pedestrians\nare allowed on\nCYCLING ROAD!", NULL },  /* SPRITE_GUARD, STAY, TEXT_ROUTE16GATE1F_GUARD */
    {   8,   7, 0x0b, 0, "How'd you get in?\nGood effort!", NULL },  /* SPRITE_GAMBLER, STAY, TEXT_ROUTE16GATE1F_GAMBLER */
};

static const map_warp_t kWarps_Route16Gate2F[] = {
    {  14,  15, 0xba, 8 },  /* ROUTE_16_GATE_1F */
};

static const npc_event_t kNpcs_Route16Gate2F[] = {
    {   8,   5, 0x35, 0, "I'm going for a\nride with my girl\nfriend!", NULL },  /* SPRITE_LITTLE_BOY, STAY, TEXT_ROUTE16GATE2F_LITTLE_BOY */
    {   4,  11, 0x08, 1, "We're going\nriding together!", NULL },  /* SPRITE_LITTLE_GIRL, WALK, TEXT_ROUTE16GATE2F_LITTLE_GIRL */
};

static const sign_event_t kSigns_Route16Gate2F[] = {
    {   2,   5, "Looked into the\nbinoculars.\fIt's CELADON DEPT.\nSTORE!" },  /* TEXT_ROUTE16GATE2F_LEFT_BINOCULARS */
    {  12,   5, "Looked into the\nbinoculars.\fThere's a long\npath over water!" },  /* TEXT_ROUTE16GATE2F_RIGHT_BINOCULARS */
};

static const map_warp_t kWarps_Route16FlyHouse[] = {
    {   4,  15, 0xff, 8 },  /* LAST_MAP */
    {   6,  15, 0xff, 8 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_Route16FlyHouse[] = {
    {   4,   7, 0x1d, 0, "Oh, you found my\nsecret retreat!\fPlease don't tell\nanyone I'm here.\nI'll make it up\nto you with this!", NULL },  /* SPRITE_BRUNETTE_GIRL, STAY, TEXT_ROUTE16FLYHOUSE_BRUNETTE_GIRL */
    {  12,   9, 0x09, 1, "FEAROW: Kyueen!", NULL },  /* SPRITE_BIRD, WALK, TEXT_ROUTE16FLYHOUSE_FEAROW */
};

static const map_warp_t kWarps_Route12SuperRodHouse[] = {
    {   4,  15, 0xff, 3 },  /* LAST_MAP */
    {   6,  15, 0xff, 3 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_Route12SuperRodHouse[] = {
    {   4,   9, 0x27, 0, "I'm the FISHING\nGURU's brother!\fI simply Looove\nfishing!\fDo you like to\nfish?", NULL },  /* SPRITE_FISHING_GURU, STAY, TEXT_ROUTE12SUPERRODHOUSE_FISHING_GURU */
};

static const map_warp_t kWarps_Route18Gate1F[] = {
    {   0,   9, 0xff, 0 },  /* LAST_MAP */
    {   0,  11, 0xff, 1 },  /* LAST_MAP */
    {  14,   9, 0xff, 2 },  /* LAST_MAP */
    {  14,  11, 0xff, 3 },  /* LAST_MAP */
    {  12,  17, 0xbf, 0 },  /* ROUTE_18_GATE_2F */
};

static const npc_event_t kNpcs_Route18Gate1F[] = {
    {   8,   3, 0x31, 0, "You need a BICYCLE\nfor CYCLING ROAD!", NULL },  /* SPRITE_GUARD, STAY, TEXT_ROUTE18GATE1F_GUARD */
};

static const map_warp_t kWarps_Route18Gate2F[] = {
    {  14,  15, 0xbe, 4 },  /* ROUTE_18_GATE_1F */
};

static const npc_event_t kNpcs_Route18Gate2F[] = {
    {   8,   5, 0x04, 1, NULL, NULL },  /* SPRITE_YOUNGSTER, WALK, TEXT_ROUTE18GATE2F_YOUNGSTER */
};

static const sign_event_t kSigns_Route18Gate2F[] = {
    {   2,   5, "Looked into the\nbinoculars.\fPALLET TOWN is in\nthe west!" },  /* TEXT_ROUTE18GATE2F_LEFT_BINOCULARS */
    {  12,   5, "Looked into the\nbinoculars.\fThere are people\nswimming!" },  /* TEXT_ROUTE18GATE2F_RIGHT_BINOCULARS */
};

static const map_warp_t kWarps_SeafoamIslands1F[] = {
    {   8,  35, 0xff, 0 },  /* LAST_MAP */
    {  10,  35, 0xff, 0 },  /* LAST_MAP */
    {  52,  35, 0xff, 1 },  /* LAST_MAP */
    {  54,  35, 0xff, 1 },  /* LAST_MAP */
    {  14,  11, 0x9f, 1 },  /* SEAFOAM_ISLANDS_B1F */
    {  50,   7, 0x9f, 6 },  /* SEAFOAM_ISLANDS_B1F */
    {  46,  31, 0x9f, 4 },  /* SEAFOAM_ISLANDS_B1F */
};

static const npc_event_t kNpcs_SeafoamIslands1F[] = {
    {  36,  21, 0x3f, 0, NULL, NULL },  /* SPRITE_BOULDER, STAY, TEXT_SEAFOAMISLANDS1F_BOULDER1 */
    {  52,  15, 0x3f, 0, NULL, NULL },  /* SPRITE_BOULDER, STAY, TEXT_SEAFOAMISLANDS1F_BOULDER2 */
};

static const map_warp_t kWarps_Route22Gate[] = {
    {   8,  15, 0xff, 0 },  /* LAST_MAP */
    {  10,  15, 0xff, 0 },  /* LAST_MAP */
    {   8,   1, 0xff, 0 },  /* LAST_MAP */
    {  10,   1, 0xff, 1 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_Route22Gate[] = {
    {  12,   5, 0x31, 0, NULL, NULL },  /* SPRITE_GUARD, STAY, TEXT_ROUTE22GATE_GUARD */
};

static const map_warp_t kWarps_VictoryRoad2F[] = {
    {   0,  17, 0x6c, 2 },  /* VICTORY_ROAD_1F */
    {  58,  15, 0xff, 3 },  /* LAST_MAP */
    {  58,  17, 0xff, 3 },  /* LAST_MAP */
    {  46,  15, 0xc6, 0 },  /* VICTORY_ROAD_3F */
    {  50,  29, 0xc6, 2 },  /* VICTORY_ROAD_3F */
    {  54,  15, 0xc6, 1 },  /* VICTORY_ROAD_3F */
    {   2,   3, 0xc6, 3 },  /* VICTORY_ROAD_3F */
};

static const npc_event_t kNpcs_VictoryRoad2F[] = {
    {  24,  19, 0x0e, 0, NULL, NULL },  /* SPRITE_HIKER, STAY, TEXT_VICTORYROAD2F_HIKER */
    {  42,  27, 0x0c, 0, NULL, NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_VICTORYROAD2F_SUPER_NERD1 */
    {  38,  17, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_VICTORYROAD2F_COOLTRAINER_M */
    {   8,   5, 0x0c, 0, NULL, NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_VICTORYROAD2F_SUPER_NERD2 */
    {  52,   7, 0x0c, 0, NULL, NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_VICTORYROAD2F_SUPER_NERD3 */
    {  22,  11, 0x09, 0, NULL, NULL },  /* SPRITE_BIRD, STAY, TEXT_VICTORYROAD2F_MOLTRES */
    {   8,  29, 0x3f, 0, NULL, NULL },  /* SPRITE_BOULDER, STAY, TEXT_VICTORYROAD2F_BOULDER1 */
    {  10,  11, 0x3f, 0, NULL, NULL },  /* SPRITE_BOULDER, STAY, TEXT_VICTORYROAD2F_BOULDER2 */
    {  46,  33, 0x3f, 0, NULL, NULL },  /* SPRITE_BOULDER, STAY, TEXT_VICTORYROAD2F_BOULDER3 */
};

static const item_event_t kItems_VictoryRoad2F[] = {
    {  54,  11, 0xd9 },  /* TM_SUBMISSION */
    {  36,  19, 0x34 },  /* FULL_HEAL */
    {  18,  23, 0xcd },  /* TM_MEGA_KICK */
    {  22,   1, 0x37 },  /* GUARD_SPEC */
};

static const map_warp_t kWarps_Route12Gate2F[] = {
    {  14,  15, 0x57, 4 },  /* ROUTE_12_GATE_1F */
};

static const npc_event_t kNpcs_Route12Gate2F[] = {
    {   6,   9, 0x1d, 1, "My POKEMON's\nashes are stored\nin POKEMON TOWER.\fYou can have this\nTM. I don't need\nit any more...", NULL },  /* SPRITE_BRUNETTE_GIRL, WALK, TEXT_ROUTE12GATE2F_BRUNETTE_GIRL */
};

static const sign_event_t kSigns_Route12Gate2F[] = {
    {   2,   5, "Looked into the\nbinoculars.\fA man fishing!" },  /* TEXT_ROUTE12GATE2F_LEFT_BINOCULARS */
    {  12,   5, "Looked into the\nbinoculars.\fIt's POKEMON TOWER!" },  /* TEXT_ROUTE12GATE2F_RIGHT_BINOCULARS */
};

static const map_warp_t kWarps_VermilionTradeHouse[] = {
    {   4,  15, 0xff, 7 },  /* LAST_MAP */
    {   6,  15, 0xff, 7 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_VermilionTradeHouse[] = {
    {   6,  11, 0x08, 0, NULL, NULL },  /* SPRITE_LITTLE_GIRL, STAY, TEXT_VERMILIONTRADEHOUSE_LITTLE_GIRL */
};

static const map_warp_t kWarps_DiglettsCave[] = {
    {  10,  11, 0x2e, 2 },  /* DIGLETTS_CAVE_ROUTE_2 */
    {  74,  63, 0x55, 2 },  /* DIGLETTS_CAVE_ROUTE_11 */
};

static const map_warp_t kWarps_VictoryRoad3F[] = {
    {  46,  15, 0xc2, 3 },  /* VICTORY_ROAD_2F */
    {  52,  17, 0xc2, 5 },  /* VICTORY_ROAD_2F */
    {  54,  31, 0xc2, 4 },  /* VICTORY_ROAD_2F */
    {   4,   1, 0xc2, 6 },  /* VICTORY_ROAD_2F */
};

static const npc_event_t kNpcs_VictoryRoad3F[] = {
    {  56,  11, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_VICTORYROAD3F_COOLTRAINER_M1 */
    {  14,  27, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_VICTORYROAD3F_COOLTRAINER_F1 */
    {  12,  29, 0x07, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_VICTORYROAD3F_COOLTRAINER_M2 */
    {  26,   7, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_VICTORYROAD3F_COOLTRAINER_F2 */
    {  44,   7, 0x3f, 0, NULL, NULL },  /* SPRITE_BOULDER, STAY, TEXT_VICTORYROAD3F_BOULDER1 */
    {  26,  25, 0x3f, 0, NULL, NULL },  /* SPRITE_BOULDER, STAY, TEXT_VICTORYROAD3F_BOULDER2 */
    {  48,  21, 0x3f, 0, NULL, NULL },  /* SPRITE_BOULDER, STAY, TEXT_VICTORYROAD3F_BOULDER3 */
    {  44,  31, 0x3f, 0, NULL, NULL },  /* SPRITE_BOULDER, STAY, TEXT_VICTORYROAD3F_BOULDER4 */
};

static const item_event_t kItems_VictoryRoad3F[] = {
    {  52,  11, 0x36 },  /* MAX_REVIVE */
    {  14,  15, 0xf7 },  /* TM_EXPLOSION */
};

static const map_warp_t kWarps_RocketHideoutB1F[] = {
    {  46,   5, 0xc8, 0 },  /* ROCKET_HIDEOUT_B2F */
    {  42,   5, 0x87, 2 },  /* GAME_CORNER */
    {  48,  39, 0xcb, 0 },  /* ROCKET_HIDEOUT_ELEVATOR */
    {  42,  49, 0xc8, 3 },  /* ROCKET_HIDEOUT_B2F */
    {  50,  39, 0xcb, 1 },  /* ROCKET_HIDEOUT_ELEVATOR */
};

static const npc_event_t kNpcs_RocketHideoutB1F[] = {
    {  52,  17, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_ROCKETHIDEOUTB1F_ROCKET1 */
    {  24,  13, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_ROCKETHIDEOUTB1F_ROCKET2 */
    {  36,  35, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_ROCKETHIDEOUTB1F_ROCKET3 */
    {  30,  51, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_ROCKETHIDEOUTB1F_ROCKET4 */
    {  56,  37, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_ROCKETHIDEOUTB1F_ROCKET5 */
};

static const item_event_t kItems_RocketHideoutB1F[] = {
    {  22,  29, 0x1d },  /* ESCAPE_ROPE */
    {  18,  35, 0x12 },  /* HYPER_POTION */
};

static const map_warp_t kWarps_RocketHideoutB2F[] = {
    {  54,  17, 0xc7, 0 },  /* ROCKET_HIDEOUT_B1F */
    {  42,  17, 0xc9, 0 },  /* ROCKET_HIDEOUT_B3F */
    {  48,  39, 0xcb, 0 },  /* ROCKET_HIDEOUT_ELEVATOR */
    {  42,  45, 0xc7, 3 },  /* ROCKET_HIDEOUT_B1F */
    {  50,  39, 0xcb, 1 },  /* ROCKET_HIDEOUT_ELEVATOR */
};

static const npc_event_t kNpcs_RocketHideoutB2F[] = {
    {  40,  25, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_ROCKETHIDEOUTB2F_ROCKET */
};

static const item_event_t kItems_RocketHideoutB2F[] = {
    {   2,  23, 0x0a },  /* MOON_STONE */
    {  32,  17, 0x31 },  /* NUGGET */
    {  12,  25, 0xcf },  /* TM_HORN_DRILL */
    {   6,  43, 0x13 },  /* SUPER_POTION */
};

static const map_warp_t kWarps_RocketHideoutB3F[] = {
    {  50,  13, 0xc8, 1 },  /* ROCKET_HIDEOUT_B2F */
    {  38,  37, 0xca, 0 },  /* ROCKET_HIDEOUT_B4F */
};

static const npc_event_t kNpcs_RocketHideoutB3F[] = {
    {  20,  45, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_ROCKETHIDEOUTB3F_ROCKET1 */
    {  52,  25, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_ROCKETHIDEOUTB3F_ROCKET2 */
};

static const item_event_t kItems_RocketHideoutB3F[] = {
    {  52,  35, 0xd2 },  /* TM_DOUBLE_EDGE */
    {  40,  29, 0x28 },  /* RARE_CANDY */
};

static const map_warp_t kWarps_RocketHideoutB4F[] = {
    {  38,  21, 0xc9, 1 },  /* ROCKET_HIDEOUT_B3F */
    {  48,  31, 0xcb, 0 },  /* ROCKET_HIDEOUT_ELEVATOR */
    {  50,  31, 0xcb, 1 },  /* ROCKET_HIDEOUT_ELEVATOR */
};

static const npc_event_t kNpcs_RocketHideoutB4F[] = {
    {  50,   7, 0x17, 0, "So! I must say, I\nam impressed you\ngot here!", NULL },  /* SPRITE_GIOVANNI, STAY, TEXT_ROCKETHIDEOUTB4F_GIOVANNI */
    {  46,  25, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_ROCKETHIDEOUTB4F_ROCKET1 */
    {  52,  25, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_ROCKETHIDEOUTB4F_ROCKET2 */
    {  22,   5, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_ROCKETHIDEOUTB4F_ROCKET3 */
};

static const item_event_t kItems_RocketHideoutB4F[] = {
    {  20,  25, 0x23 },  /* HP_UP */
    {  18,   9, 0xca },  /* TM_RAZOR_WIND */
    {  24,  41, 0x25 },  /* IRON */
    {  50,   5, 0x48 },  /* SILPH_SCOPE */
    {  20,   5, 0x4a },  /* LIFT_KEY */
};

static const map_warp_t kWarps_RocketHideoutElevator[] = {
    {   4,   3, 0xc7, 2 },  /* ROCKET_HIDEOUT_B1F */
    {   6,   3, 0xc7, 4 },  /* ROCKET_HIDEOUT_B1F */
};

static const sign_event_t kSigns_RocketHideoutElevator[] = {
    {   2,   3, "It appears to\nneed a key.@" },  /* TEXT_ROCKETHIDEOUTELEVATOR */
};

static const map_warp_t kWarps_SilphCo2F[] = {
    {  48,   1, 0xb5, 2 },  /* SILPH_CO_1F */
    {  52,   1, 0xd0, 0 },  /* SILPH_CO_3F */
    {  40,   1, 0xec, 0 },  /* SILPH_CO_ELEVATOR */
    {   6,   7, 0xd0, 6 },  /* SILPH_CO_3F */
    {  26,   7, 0xd5, 4 },  /* SILPH_CO_8F */
    {  54,  31, 0xd5, 5 },  /* SILPH_CO_8F */
    {  18,  31, 0xd3, 4 },  /* SILPH_CO_6F */
};

static const npc_event_t kNpcs_SilphCo2F[] = {
    {  20,   3, 0x1b, 0, "{PLAYER} got\n@", NULL },  /* SPRITE_SILPH_WORKER_F, STAY, TEXT_SILPHCO2F_SILPH_WORKER_F */
    {  10,  25, 0x20, 0, NULL, NULL },  /* SPRITE_SCIENTIST, STAY, TEXT_SILPHCO2F_SCIENTIST1 */
    {  48,  27, 0x20, 0, NULL, NULL },  /* SPRITE_SCIENTIST, STAY, TEXT_SILPHCO2F_SCIENTIST2 */
    {  32,  23, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_SILPHCO2F_ROCKET1 */
    {  48,  15, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_SILPHCO2F_ROCKET2 */
};

static const map_warp_t kWarps_SilphCo3F[] = {
    {  52,   1, 0xcf, 1 },  /* SILPH_CO_2F */
    {  48,   1, 0xd1, 0 },  /* SILPH_CO_4F */
    {  40,   1, 0xec, 0 },  /* SILPH_CO_ELEVATOR */
    {  46,  23, 0xd0, 9 },  /* SILPH_CO_3F */
    {   6,   7, 0xd2, 5 },  /* SILPH_CO_5F */
    {   6,  31, 0xd2, 6 },  /* SILPH_CO_5F */
    {  54,   7, 0xcf, 3 },  /* SILPH_CO_2F */
    {   6,  23, 0xe9, 3 },  /* SILPH_CO_9F */
    {  22,  23, 0xd4, 4 },  /* SILPH_CO_7F */
    {  54,  31, 0xd0, 3 },  /* SILPH_CO_3F */
};

static const npc_event_t kNpcs_SilphCo3F[] = {
    {  48,  17, 0x2c, 0, "I work for SILPH.\nWhat should I do?", NULL },  /* SPRITE_SILPH_WORKER_M, STAY, TEXT_SILPHCO3F_SILPH_WORKER_M */
    {  40,  15, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_SILPHCO3F_ROCKET */
    {  14,  19, 0x20, 0, NULL, NULL },  /* SPRITE_SCIENTIST, STAY, TEXT_SILPHCO3F_SCIENTIST */
};

static const item_event_t kItems_SilphCo3F[] = {
    {  16,  11, 0x12 },  /* HYPER_POTION */
};

static const map_warp_t kWarps_SilphCo4F[] = {
    {  48,   1, 0xd0, 1 },  /* SILPH_CO_3F */
    {  52,   1, 0xd2, 1 },  /* SILPH_CO_5F */
    {  40,   1, 0xec, 0 },  /* SILPH_CO_ELEVATOR */
    {  22,  15, 0xea, 3 },  /* SILPH_CO_10F */
    {  34,   7, 0xd3, 3 },  /* SILPH_CO_6F */
    {   6,  31, 0xea, 4 },  /* SILPH_CO_10F */
    {  34,  23, 0xea, 5 },  /* SILPH_CO_10F */
};

static const npc_event_t kNpcs_SilphCo4F[] = {
    {  12,   5, 0x2c, 0, "Sssh! Can't you\nsee I'm hiding?", NULL },  /* SPRITE_SILPH_WORKER_M, STAY, TEXT_SILPHCO4F_SILPH_WORKER_M */
    {  18,  29, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_SILPHCO4F_ROCKET1 */
    {  28,  13, 0x20, 0, NULL, NULL },  /* SPRITE_SCIENTIST, STAY, TEXT_SILPHCO4F_SCIENTIST */
    {  52,  21, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_SILPHCO4F_ROCKET2 */
};

static const item_event_t kItems_SilphCo4F[] = {
    {   6,  19, 0x34 },  /* FULL_HEAL */
    {   8,  15, 0x36 },  /* MAX_REVIVE */
    {  10,  17, 0x1d },  /* ESCAPE_ROPE */
};

static const map_warp_t kWarps_SilphCo5F[] = {
    {  48,   1, 0xd3, 1 },  /* SILPH_CO_6F */
    {  52,   1, 0xd1, 1 },  /* SILPH_CO_4F */
    {  40,   1, 0xec, 0 },  /* SILPH_CO_ELEVATOR */
    {  54,   7, 0xd4, 5 },  /* SILPH_CO_7F */
    {  18,  31, 0xe9, 4 },  /* SILPH_CO_9F */
    {  22,  11, 0xd0, 4 },  /* SILPH_CO_3F */
    {   6,  31, 0xd0, 5 },  /* SILPH_CO_3F */
};

static const npc_event_t kNpcs_SilphCo5F[] = {
    {  26,  19, 0x2c, 0, "TEAM ROCKET is\nin an uproar over\nsome intruder.\nThat's you right?", NULL },  /* SPRITE_SILPH_WORKER_M, STAY, TEXT_SILPHCO5F_SILPH_WORKER_M */
    {  16,  33, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_SILPHCO5F_ROCKET1 */
    {  16,   7, 0x20, 0, NULL, NULL },  /* SPRITE_SCIENTIST, STAY, TEXT_SILPHCO5F_SCIENTIST */
    {  36,  21, 0x21, 0, NULL, NULL },  /* SPRITE_ROCKER, STAY, TEXT_SILPHCO5F_ROCKER */
    {  56,   9, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_SILPHCO5F_ROCKET2 */
    {  44,  25, 0x42, 0, "It's a POKEMON\nREPORT!\fPOKEMON LAB\ncreated PORYGON,\nthe first virtual\nreality POKEMON.", NULL },  /* SPRITE_CLIPBOARD, STAY, TEXT_SILPHCO5F_POKEMON_REPORT1 */
    {  50,  21, 0x42, 0, "It's a POKEMON\nREPORT!\fOver 160 POKEMON\ntechniques have\nbeen confirmed.", NULL },  /* SPRITE_CLIPBOARD, STAY, TEXT_SILPHCO5F_POKEMON_REPORT2 */
    {  48,  13, 0x42, 0, "It's a POKEMON\nREPORT!\f4 POKEMON evolve\nonly when traded\nby link-cable.", NULL },  /* SPRITE_CLIPBOARD, STAY, TEXT_SILPHCO5F_POKEMON_REPORT3 */
};

static const item_event_t kItems_SilphCo5F[] = {
    {   4,  27, 0xd1 },  /* TM_TAKE_DOWN */
    {   8,  13, 0x24 },  /* PROTEIN */
    {  42,  33, 0x30 },  /* CARD_KEY */
};

static const map_warp_t kWarps_SilphCo6F[] = {
    {  32,   1, 0xd4, 1 },  /* SILPH_CO_7F */
    {  28,   1, 0xd2, 0 },  /* SILPH_CO_5F */
    {  36,   1, 0xec, 0 },  /* SILPH_CO_ELEVATOR */
    {   6,   7, 0xd1, 4 },  /* SILPH_CO_4F */
    {  46,   7, 0xcf, 6 },  /* SILPH_CO_2F */
};

static const npc_event_t kNpcs_SilphCo6F[] = {
    {  20,  13, 0x2c, 0, "The ROCKETs came\nand took over the\nbuilding!", NULL },  /* SPRITE_SILPH_WORKER_M, STAY, TEXT_SILPHCO6F_SILPH_WORKER_M1 */
    {  40,  13, 0x2c, 0, "Oh dear, oh dear.\nHelp me please!", NULL },  /* SPRITE_SILPH_WORKER_M, STAY, TEXT_SILPHCO6F_SILPH_WORKER_M2 */
    {  42,  13, 0x1b, 0, "Look at him! He's\nsuch a coward!", NULL },  /* SPRITE_SILPH_WORKER_F, STAY, TEXT_SILPHCO6F_SILPH_WORKER_F1 */
    {  22,  21, 0x1b, 0, "TEAM ROCKET is\ntrying to conquer\nthe world with\nPOKEMON!", NULL },  /* SPRITE_SILPH_WORKER_F, STAY, TEXT_SILPHCO6F_SILPH_WORKER_F2 */
    {  36,  27, 0x2c, 0, "They must have\ntargeted SILPH\nfor our POKEMON\nproducts.", NULL },  /* SPRITE_SILPH_WORKER_M, STAY, TEXT_SILPHCO6F_SILPH_WORKER_M3 */
    {  34,   7, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_SILPHCO6F_ROCKET1 */
    {  14,  17, 0x20, 0, NULL, NULL },  /* SPRITE_SCIENTIST, STAY, TEXT_SILPHCO6F_SCIENTIST */
    {  28,  31, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_SILPHCO6F_ROCKET2 */
};

static const item_event_t kItems_SilphCo6F[] = {
    {   6,  25, 0x23 },  /* HP_UP */
    {   4,  31, 0x2e },  /* X_ACCURACY */
};

static const map_warp_t kWarps_SilphCo7F[] = {
    {  32,   1, 0xd5, 1 },  /* SILPH_CO_8F */
    {  44,   1, 0xd3, 0 },  /* SILPH_CO_6F */
    {  36,   1, 0xec, 0 },  /* SILPH_CO_ELEVATOR */
    {  10,  15, 0xeb, 3 },  /* SILPH_CO_11F */
    {  10,   7, 0xd0, 8 },  /* SILPH_CO_3F */
    {  42,  31, 0xd2, 3 },  /* SILPH_CO_5F */
};

static const npc_event_t kNpcs_SilphCo7F[] = {
    {   2,  11, 0x2c, 0, "Oh! Hi! You're\nnot a ROCKET! You\ncame to save us?\nWhy, thank you!\fI want you to\nhave this POKEMON\nfor saving us.", NULL },  /* SPRITE_SILPH_WORKER_M, STAY, TEXT_SILPHCO7F_SILPH_WORKER_M1 */
    {  26,  27, 0x2c, 0, "TEAM ROCKET was\nafter the MASTER\nBALL which will\ncatch any POKEMON!", NULL },  /* SPRITE_SILPH_WORKER_M, STAY, TEXT_SILPHCO7F_SILPH_WORKER_M2 */
    {  14,  21, 0x2c, 0, "It would be bad\nif TEAM ROCKET\ntook over SILPH\nor our POKEMON!", NULL },  /* SPRITE_SILPH_WORKER_M, STAY, TEXT_SILPHCO7F_SILPH_WORKER_M3 */
    {  20,  17, 0x1b, 0, "You! It's really\ndangerous here!\nYou came to save\nme? You can't!", NULL },  /* SPRITE_SILPH_WORKER_F, STAY, TEXT_SILPHCO7F_SILPH_WORKER_M4 */
    {  26,   3, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_SILPHCO7F_ROCKET1 */
    {   4,  27, 0x20, 0, NULL, NULL },  /* SPRITE_SCIENTIST, STAY, TEXT_SILPHCO7F_SCIENTIST */
    {  40,   5, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_SILPHCO7F_ROCKET2 */
    {  38,  29, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_SILPHCO7F_ROCKET3 */
    {   6,  15, 0x02, 0, "{RIVAL}: What\nkept you {PLAYER}?", NULL },  /* SPRITE_BLUE, STAY, TEXT_SILPHCO7F_RIVAL */
};

static const item_event_t kItems_SilphCo7F[] = {
    {   2,  19, 0x27 },  /* CALCIUM */
    {  48,  23, 0xcb },  /* TM_SWORDS_DANCE */
};

static const map_warp_t kWarps_SilphCo8F[] = {
    {  32,   1, 0xe9, 1 },  /* SILPH_CO_9F */
    {  28,   1, 0xd4, 0 },  /* SILPH_CO_7F */
    {  36,   1, 0xec, 0 },  /* SILPH_CO_ELEVATOR */
    {   6,  23, 0xd5, 6 },  /* SILPH_CO_8F */
    {   6,  31, 0xcf, 4 },  /* SILPH_CO_2F */
    {  22,  11, 0xcf, 5 },  /* SILPH_CO_2F */
    {  22,  19, 0xd5, 3 },  /* SILPH_CO_8F */
};

static const npc_event_t kNpcs_SilphCo8F[] = {
    {   8,   5, 0x2c, 0, "I wonder if SILPH\nis finished...", NULL },  /* SPRITE_SILPH_WORKER_M, STAY, TEXT_SILPHCO8F_SILPH_WORKER_M */
    {  38,   5, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_SILPHCO8F_ROCKET1 */
    {  20,   5, 0x20, 0, NULL, NULL },  /* SPRITE_SCIENTIST, STAY, TEXT_SILPHCO8F_SCIENTIST */
    {  24,  31, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_SILPHCO8F_ROCKET2 */
};

static const map_warp_t kWarps_PokemonMansion2F[] = {
    {  10,  21, 0xa5, 4 },  /* POKEMON_MANSION_1F */
    {  14,  21, 0xd7, 0 },  /* POKEMON_MANSION_3F */
    {  50,  29, 0xd7, 2 },  /* POKEMON_MANSION_3F */
    {  12,   3, 0xd7, 1 },  /* POKEMON_MANSION_3F */
};

static const npc_event_t kNpcs_PokemonMansion2F[] = {
    {   6,  35, 0x0c, 1, NULL, NULL },  /* SPRITE_SUPER_NERD, WALK, TEXT_POKEMONMANSION2F_SUPER_NERD */
    {  36,   5, 0x41, 0, "Diary: July 5\nGuyana,\nSouth America\fA new POKEMON was\ndiscovered deep\nin the jungle.", NULL },  /* SPRITE_POKEDEX, STAY, TEXT_POKEMONMANSION2F_DIARY1 */
    {   6,  45, 0x41, 0, "Diary: July 10\nWe christened the\nnewly discovered\nPOKEMON, MEW.", NULL },  /* SPRITE_POKEDEX, STAY, TEXT_POKEMONMANSION2F_DIARY2 */
};

static const item_event_t kItems_PokemonMansion2F[] = {
    {  56,  15, 0x27 },  /* CALCIUM */
};

static const map_warp_t kWarps_PokemonMansion3F[] = {
    {  14,  21, 0xd6, 1 },  /* POKEMON_MANSION_2F */
    {  12,   3, 0xd6, 3 },  /* POKEMON_MANSION_2F */
    {  50,  29, 0xd6, 2 },  /* POKEMON_MANSION_2F */
};

static const npc_event_t kNpcs_PokemonMansion3F[] = {
    {  10,  23, 0x0c, 1, NULL, NULL },  /* SPRITE_SUPER_NERD, WALK, TEXT_POKEMONMANSION3F_SUPER_NERD */
    {  40,  23, 0x20, 0, NULL, NULL },  /* SPRITE_SCIENTIST, STAY, TEXT_POKEMONMANSION3F_SCIENTIST */
    {  12,  25, 0x41, 0, "Diary: Feb. 6\nMEW gave birth.\fWe named the\nnewborn MEWTWO.", NULL },  /* SPRITE_POKEDEX, STAY, TEXT_POKEMONMANSION3F_DIARY */
};

static const item_event_t kItems_PokemonMansion3F[] = {
    {   2,  33, 0x11 },  /* MAX_POTION */
    {  50,  11, 0x25 },  /* IRON */
};

static const map_warp_t kWarps_PokemonMansionB1F[] = {
    {  46,  45, 0xa5, 5 },  /* POKEMON_MANSION_1F */
};

static const npc_event_t kNpcs_PokemonMansionB1F[] = {
    {  32,  47, 0x0c, 0, NULL, NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_POKEMONMANSIONB1F_BURGLAR */
    {  54,  23, 0x20, 0, NULL, NULL },  /* SPRITE_SCIENTIST, STAY, TEXT_POKEMONMANSIONB1F_SCIENTIST */
    {  32,  41, 0x41, 0, "\nMEWTWO is far too\npowerful.\fWe have failed to\ncurb its vicious\ntendencies...", NULL },  /* SPRITE_POKEDEX, STAY, TEXT_POKEMONMANSIONB1F_DIARY */
};

static const item_event_t kItems_PokemonMansionB1F[] = {
    {  20,   5, 0x28 },  /* RARE_CANDY */
    {   2,  45, 0x10 },  /* FULL_RESTORE */
    {  38,  51, 0xd6 },  /* TM_BLIZZARD */
    {  10,   9, 0xde },  /* TM_SOLARBEAM */
    {  10,  27, 0x2b },  /* SECRET_KEY */
};

static const map_warp_t kWarps_SafariZoneEast[] = {
    {   0,   9, 0xda, 6 },  /* SAFARI_ZONE_NORTH */
    {   0,  11, 0xda, 7 },  /* SAFARI_ZONE_NORTH */
    {   0,  45, 0xdc, 6 },  /* SAFARI_ZONE_CENTER */
    {   0,  47, 0xdc, 6 },  /* SAFARI_ZONE_CENTER */
    {  50,  19, 0xe0, 0 },  /* SAFARI_ZONE_EAST_REST_HOUSE */
};

static const sign_event_t kSigns_SafariZoneEast[] = {
    {  52,  21, "REST HOUSE" },  /* TEXT_SAFARIZONEEAST_REST_HOUSE_SIGN */
    {  12,   9, "TRAINER TIPS\fThe remaining time\ndeclines only\nwhile you walk!" },  /* TEXT_SAFARIZONEEAST_TRAINER_TIPS */
    {  10,  47, "CENTER AREA\nNORTH: AREA 2" },  /* TEXT_SAFARIZONEEAST_SIGN */
};

static const item_event_t kItems_SafariZoneEast[] = {
    {  42,  21, 0x10 },  /* FULL_RESTORE */
    {   6,  15, 0x11 },  /* MAX_POTION */
    {  40,  27, 0x26 },  /* CARBOS */
    {  30,  25, 0xed },  /* TM_EGG_BOMB */
};

static const map_warp_t kWarps_SafariZoneNorth[] = {
    {   4,  71, 0xdb, 0 },  /* SAFARI_ZONE_WEST */
    {   6,  71, 0xdb, 1 },  /* SAFARI_ZONE_WEST */
    {  16,  71, 0xdb, 2 },  /* SAFARI_ZONE_WEST */
    {  18,  71, 0xdb, 3 },  /* SAFARI_ZONE_WEST */
    {  40,  71, 0xdc, 4 },  /* SAFARI_ZONE_CENTER */
    {  42,  71, 0xdc, 5 },  /* SAFARI_ZONE_CENTER */
    {  78,  61, 0xd9, 0 },  /* SAFARI_ZONE_EAST */
    {  78,  63, 0xd9, 1 },  /* SAFARI_ZONE_EAST */
    {  70,   7, 0xe1, 0 },  /* SAFARI_ZONE_NORTH_REST_HOUSE */
};

static const sign_event_t kSigns_SafariZoneNorth[] = {
    {  72,   9, "REST HOUSE" },  /* TEXT_SAFARIZONENORTH_REST_HOUSE_SIGN */
    {   8,  51, "TRAINER TIPS\fThe SECRET HOUSE\nis still ahead!" },  /* TEXT_SAFARIZONENORTH_TRAINER_TIPS_1 */
    {  26,  63, "AREA 2" },  /* TEXT_SAFARIZONENORTH_SIGN */
    {  38,  67, "TRAINER TIPS\fPOKEMON hide in\ntall grass!\fZigzag through\ngrassy areas to\nflush them out." },  /* TEXT_SAFARIZONENORTH_TRAINER_TIPS_2 */
    {  52,  57, "TRAINER TIPS\fWin a free HM for\nfinding the\nSECRET HOUSE!" },  /* TEXT_SAFARIZONENORTH_TRAINER_TIPS_3 */
};

static const item_event_t kItems_SafariZoneNorth[] = {
    {  50,   3, 0x24 },  /* PROTEIN */
    {  38,  15, 0xf0 },  /* TM_SKULL_BASH */
};

static const map_warp_t kWarps_SafariZoneWest[] = {
    {  40,   1, 0xda, 0 },  /* SAFARI_ZONE_NORTH */
    {  42,   1, 0xda, 1 },  /* SAFARI_ZONE_NORTH */
    {  52,   1, 0xda, 2 },  /* SAFARI_ZONE_NORTH */
    {  54,   1, 0xda, 3 },  /* SAFARI_ZONE_NORTH */
    {  58,  45, 0xdc, 2 },  /* SAFARI_ZONE_CENTER */
    {  58,  47, 0xdc, 3 },  /* SAFARI_ZONE_CENTER */
    {   6,   7, 0xde, 0 },  /* SAFARI_ZONE_SECRET_HOUSE */
    {  22,  23, 0xdf, 0 },  /* SAFARI_ZONE_WEST_REST_HOUSE */
};

static const sign_event_t kSigns_SafariZoneWest[] = {
    {  24,  25, "REST HOUSE" },  /* TEXT_SAFARIZONEWEST_REST_HOUSE_SIGN */
    {  34,   7, "REQUEST NOTICE\fPlease find the\nSAFARI WARDEN's\nlost GOLD TEETH.\nThey're around\nhere somewhere.\fReward offered!\nContact: WARDEN" },  /* TEXT_SAFARIZONEWEST_FIND_WARDENS_TEETH_SIGN */
    {  52,   9, "TRAINER TIPS\fZone Exploration\nCampaign!\fThe Search for\nthe SECRET HOUSE!" },  /* TEXT_SAFARIZONEWEST_TRAINER_TIPS */
    {  48,  45, "AREA 3\nEAST: CENTER AREA" },  /* TEXT_SAFARIZONEWEST_SIGN */
};

static const item_event_t kItems_SafariZoneWest[] = {
    {  16,  41, 0x11 },  /* MAX_POTION */
    {  18,  15, 0xe8 },  /* TM_DOUBLE_TEAM */
    {  36,  37, 0x36 },  /* MAX_REVIVE */
    {  38,  15, 0x40 },  /* GOLD_TEETH */
};

static const map_warp_t kWarps_SafariZoneCenter[] = {
    {  28,  51, 0x9c, 2 },  /* SAFARI_ZONE_GATE */
    {  30,  51, 0x9c, 3 },  /* SAFARI_ZONE_GATE */
    {   0,  21, 0xdb, 4 },  /* SAFARI_ZONE_WEST */
    {   0,  23, 0xdb, 5 },  /* SAFARI_ZONE_WEST */
    {  28,   1, 0xda, 4 },  /* SAFARI_ZONE_NORTH */
    {  30,   1, 0xda, 5 },  /* SAFARI_ZONE_NORTH */
    {  58,  21, 0xd9, 2 },  /* SAFARI_ZONE_EAST */
    {  58,  23, 0xd9, 3 },  /* SAFARI_ZONE_EAST */
    {  34,  39, 0xdd, 0 },  /* SAFARI_ZONE_CENTER_REST_HOUSE */
};

static const sign_event_t kSigns_SafariZoneCenter[] = {
    {  36,  41, "REST HOUSE" },  /* TEXT_SAFARIZONECENTER_REST_HOUSE_SIGN */
    {  28,  45, "TRAINER TIPS\fPress the START\nButton to check\nremaining time!" },  /* TEXT_SAFARIZONECENTER_TRAINER_TIPS_SIGN */
};

static const item_event_t kItems_SafariZoneCenter[] = {
    {  28,  21, 0x31 },  /* NUGGET */
};

static const map_warp_t kWarps_SafariZoneCenterRestHouse[] = {
    {   4,  15, 0xdc, 8 },  /* SAFARI_ZONE_CENTER */
    {   6,  15, 0xdc, 8 },  /* SAFARI_ZONE_CENTER */
};

static const npc_event_t kNpcs_SafariZoneCenterRestHouse[] = {
    {   6,   5, 0x0d, 0, "SARA: Where did\nmy boy friend,\nERIK, go?", NULL },  /* SPRITE_GIRL, STAY, TEXT_SAFARIZONECENTERRESTHOUSE_GIRL */
    {   2,   9, 0x20, 1, "I'm catching\nPOKEMON to take\nhome as gifts!", NULL },  /* SPRITE_SCIENTIST, WALK, TEXT_SAFARIZONECENTERRESTHOUSE_SCIENTIST */
};

static const map_warp_t kWarps_SafariZoneSecretHouse[] = {
    {   4,  15, 0xdb, 6 },  /* SAFARI_ZONE_WEST */
    {   6,  15, 0xdb, 6 },  /* SAFARI_ZONE_WEST */
};

static const npc_event_t kNpcs_SafariZoneSecretHouse[] = {
    {   6,   7, 0x27, 0, "Ah! Finally!\fYou're the first\nperson to reach\nthe SECRET HOUSE!\fI was getting\nworried that no\none would win our\ncampaign prize.\fCongratulations!\nYou have won!", NULL },  /* SPRITE_FISHING_GURU, STAY, TEXT_SAFARIZONESECRETHOUSE_FISHING_GURU */
};

static const map_warp_t kWarps_SafariZoneWestRestHouse[] = {
    {   4,  15, 0xdb, 7 },  /* SAFARI_ZONE_WEST */
    {   6,  15, 0xdb, 7 },  /* SAFARI_ZONE_WEST */
};

static const npc_event_t kNpcs_SafariZoneWestRestHouse[] = {
    {   8,   9, 0x20, 1, "Tossing ROCKs at\nPOKEMON might\nmake them run,\nbut they'll be\neasier to catch.", NULL },  /* SPRITE_SCIENTIST, WALK, TEXT_SAFARIZONEWESTRESTHOUSE_SCIENTIST */
    {   0,   5, 0x07, 0, "Using BAIT will\nmake POKEMON\neasier to catch.", NULL },  /* SPRITE_COOLTRAINER_M, STAY, TEXT_SAFARIZONEWESTRESTHOUSE_COOLTRAINER_M */
    {  12,   5, 0x1b, 0, "I hiked a lot, but\nI didn't see any\nPOKEMON I wanted.", NULL },  /* SPRITE_SILPH_WORKER_F, STAY, TEXT_SAFARIZONEWESTRESTHOUSE_SILPH_WORKER_F */
};

static const map_warp_t kWarps_SafariZoneEastRestHouse[] = {
    {   4,  15, 0xd9, 4 },  /* SAFARI_ZONE_EAST */
    {   6,  15, 0xd9, 4 },  /* SAFARI_ZONE_EAST */
};

static const npc_event_t kNpcs_SafariZoneEastRestHouse[] = {
    {   2,   7, 0x20, 1, "How many did you\ncatch? I'm bushed\nfrom the work!", NULL },  /* SPRITE_SCIENTIST, WALK, TEXT_SAFARIZONEEASTRESTHOUSE_SCIENTIST */
    {   8,   5, 0x21, 0, "I caught a\nCHANSEY!\fThat makes this\nall worthwhile!", NULL },  /* SPRITE_ROCKER, STAY, TEXT_SAFARIZONEEASTRESTHOUSE_ROCKER */
    {  10,   5, 0x2c, 0, "Whew! I'm tired\nfrom all the fun!", NULL },  /* SPRITE_SILPH_WORKER_M, STAY, TEXT_SAFARIZONEEASTRESTHOUSE_SILPH_WORKER_M */
};

static const map_warp_t kWarps_SafariZoneNorthRestHouse[] = {
    {   4,  15, 0xda, 8 },  /* SAFARI_ZONE_NORTH */
    {   6,  15, 0xda, 8 },  /* SAFARI_ZONE_NORTH */
};

static const npc_event_t kNpcs_SafariZoneNorthRestHouse[] = {
    {  12,   7, 0x20, 1, "You can keep any\nitem you find on\nthe ground here.\fBut, you'll run\nout of time if\nyou try for all\nof them at once!", NULL },  /* SPRITE_SCIENTIST, WALK, TEXT_SAFARIZONENORTHRESTHOUSE_SCIENTIST */
    {   6,   9, 0x23, 0, "Go to the deepest\npart of the\nSAFARI ZONE. You\nwill win a prize!", NULL },  /* SPRITE_SAFARI_ZONE_WORKER, STAY, TEXT_SAFARIZONENORTHRESTHOUSE_SAFARI_ZONE_WORKER */
    {   2,  11, 0x10, 1, "My EEVEE evolved\ninto FLAREON!\fBut, a friend's\nEEVEE turned into\na VAPOREON!\nI wonder why?", NULL },  /* SPRITE_GENTLEMAN, WALK, TEXT_SAFARIZONENORTHRESTHOUSE_GENTLEMAN */
};

static const map_warp_t kWarps_CeruleanCave2F[] = {
    {  58,   3, 0xe4, 2 },  /* CERULEAN_CAVE_1F */
    {  44,  13, 0xe4, 3 },  /* CERULEAN_CAVE_1F */
    {  38,  15, 0xe4, 4 },  /* CERULEAN_CAVE_1F */
    {  18,   3, 0xe4, 5 },  /* CERULEAN_CAVE_1F */
    {   2,   7, 0xe4, 6 },  /* CERULEAN_CAVE_1F */
    {   6,  23, 0xe4, 7 },  /* CERULEAN_CAVE_1F */
};

static const item_event_t kItems_CeruleanCave2F[] = {
    {  58,  19, 0x4f },  /* PP_UP */
    {   8,  31, 0x02 },  /* ULTRA_BALL */
    {  26,  13, 0x10 },  /* FULL_RESTORE */
};

static const map_warp_t kWarps_CeruleanCaveB1F[] = {
    {   6,  13, 0xe4, 8 },  /* CERULEAN_CAVE_1F */
};

static const npc_event_t kNpcs_CeruleanCaveB1F[] = {
    {  54,  27, 0x05, 0, NULL, NULL },  /* SPRITE_MONSTER, STAY, TEXT_CERULEANCAVEB1F_MEWTWO */
};

static const item_event_t kItems_CeruleanCaveB1F[] = {
    {  32,  19, 0x02 },  /* ULTRA_BALL */
    {  36,   3, 0x36 },  /* MAX_REVIVE */
};

static const map_warp_t kWarps_CeruleanCave1F[] = {
    {  48,  35, 0xff, 6 },  /* LAST_MAP */
    {  50,  35, 0xff, 6 },  /* LAST_MAP */
    {  54,   3, 0xe2, 0 },  /* CERULEAN_CAVE_2F */
    {  46,  15, 0xe2, 1 },  /* CERULEAN_CAVE_2F */
    {  36,  19, 0xe2, 2 },  /* CERULEAN_CAVE_2F */
    {  14,   3, 0xe2, 3 },  /* CERULEAN_CAVE_2F */
    {   2,   7, 0xe2, 4 },  /* CERULEAN_CAVE_2F */
    {   6,  23, 0xe2, 5 },  /* CERULEAN_CAVE_2F */
    {   0,  13, 0xe3, 0 },  /* CERULEAN_CAVE_B1F */
};

static const item_event_t kItems_CeruleanCave1F[] = {
    {  14,  27, 0x10 },  /* FULL_RESTORE */
    {  38,   7, 0x53 },  /* MAX_ELIXER */
    {  10,   1, 0x31 },  /* NUGGET */
};

static const map_warp_t kWarps_NameRatersHouse[] = {
    {   4,  15, 0xff, 5 },  /* LAST_MAP */
    {   6,  15, 0xff, 5 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_NameRatersHouse[] = {
    {  10,   7, 0x2b, 0, "Hello, hello!\nI am the official\nNAME RATER!\fWant me to rate\nthe nicknames of\nyour POKEMON?", NULL },  /* SPRITE_SILPH_PRESIDENT, STAY, TEXT_NAMERATERSHOUSE_NAME_RATER */
};

static const map_warp_t kWarps_CeruleanBadgeHouse[] = {
    {   4,   1, 0xff, 9 },  /* LAST_MAP */
    {   4,  15, 0xff, 8 },  /* LAST_MAP */
    {   6,  15, 0xff, 8 },  /* LAST_MAP */
};

static const npc_event_t kNpcs_CeruleanBadgeHouse[] = {
    {  10,   7, 0x0a, 0, "POKEMON BADGEs\nare owned only by\nskilled trainers.\fI see you have\nat least one.\fThose BADGEs have\namazing secrets!", NULL },  /* SPRITE_MIDDLE_AGED_MAN, STAY, TEXT_CERULEANBADGEHOUSE_MIDDLE_AGED_MAN */
};

static const map_warp_t kWarps_RockTunnelB1F[] = {
    {  66,  51, 0x52, 4 },  /* ROCK_TUNNEL_1F */
    {  54,   7, 0x52, 5 },  /* ROCK_TUNNEL_1F */
    {  46,  23, 0x52, 6 },  /* ROCK_TUNNEL_1F */
    {   6,   7, 0x52, 7 },  /* ROCK_TUNNEL_1F */
};

static const npc_event_t kNpcs_RockTunnelB1F[] = {
    {  22,  27, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROCKTUNNELB1F_COOLTRAINER_F1 */
    {  12,  21, 0x0e, 0, NULL, NULL },  /* SPRITE_HIKER, STAY, TEXT_ROCKTUNNELB1F_HIKER1 */
    {   6,  11, 0x0c, 0, NULL, NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_ROCKTUNNELB1F_SUPER_NERD1 */
    {  40,  43, 0x0c, 0, NULL, NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_ROCKTUNNELB1F_SUPER_NERD2 */
    {  60,  21, 0x0e, 0, NULL, NULL },  /* SPRITE_HIKER, STAY, TEXT_ROCKTUNNELB1F_HIKER2 */
    {  28,  57, 0x06, 0, NULL, NULL },  /* SPRITE_COOLTRAINER_F, STAY, TEXT_ROCKTUNNELB1F_COOLTRAINER_F2 */
    {  66,  11, 0x0e, 0, NULL, NULL },  /* SPRITE_HIKER, STAY, TEXT_ROCKTUNNELB1F_HIKER3 */
    {  52,  61, 0x0c, 0, NULL, NULL },  /* SPRITE_SUPER_NERD, STAY, TEXT_ROCKTUNNELB1F_SUPER_NERD3 */
};

static const map_warp_t kWarps_SilphCo9F[] = {
    {  28,   1, 0xea, 0 },  /* SILPH_CO_10F */
    {  32,   1, 0xd5, 0 },  /* SILPH_CO_8F */
    {  36,   1, 0xec, 0 },  /* SILPH_CO_ELEVATOR */
    {  18,   7, 0xd0, 7 },  /* SILPH_CO_3F */
    {  34,  31, 0xd2, 4 },  /* SILPH_CO_5F */
};

static const npc_event_t kNpcs_SilphCo9F[] = {
    {   6,  29, 0x29, 0, NULL, Pokecenter_Start },  /* SPRITE_NURSE, STAY, TEXT_SILPHCO9F_NURSE */
    {   4,   9, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_SILPHCO9F_ROCKET1 */
    {  42,  27, 0x20, 0, NULL, NULL },  /* SPRITE_SCIENTIST, STAY, TEXT_SILPHCO9F_SCIENTIST */
    {  26,  33, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_SILPHCO9F_ROCKET2 */
};

static const map_warp_t kWarps_SilphCo10F[] = {
    {  16,   1, 0xe9, 0 },  /* SILPH_CO_9F */
    {  20,   1, 0xeb, 0 },  /* SILPH_CO_11F */
    {  24,   1, 0xec, 0 },  /* SILPH_CO_ELEVATOR */
    {  18,  23, 0xd1, 3 },  /* SILPH_CO_4F */
    {  26,  31, 0xd1, 5 },  /* SILPH_CO_4F */
    {  26,  15, 0xd1, 6 },  /* SILPH_CO_4F */
};

static const npc_event_t kNpcs_SilphCo10F[] = {
    {   2,  19, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_SILPHCO10F_ROCKET */
    {  20,   5, 0x20, 0, NULL, NULL },  /* SPRITE_SCIENTIST, STAY, TEXT_SILPHCO10F_SCIENTIST */
    {  18,  31, 0x1b, 1, "Waaaaa!\nI'm scared!", NULL },  /* SPRITE_SILPH_WORKER_F, WALK, TEXT_SILPHCO10F_SILPH_WORKER_F */
};

static const item_event_t kItems_SilphCo10F[] = {
    {   4,  25, 0xe2 },  /* TM_EARTHQUAKE */
    {   8,  29, 0x28 },  /* RARE_CANDY */
    {  10,  23, 0x26 },  /* CARBOS */
};

static const map_warp_t kWarps_SilphCo11F[] = {
    {  18,   1, 0xea, 1 },  /* SILPH_CO_10F */
    {  26,   1, 0xec, 0 },  /* SILPH_CO_ELEVATOR */
    {  10,  11, 0xff, 9 },  /* LAST_MAP */
    {   6,   5, 0xd4, 3 },  /* SILPH_CO_7F */
};

static const npc_event_t kNpcs_SilphCo11F[] = {
    {  14,  11, 0x2b, 0, "PRESIDENT: Thank\nyou for saving\nSILPH!\fI will never\nforget you saved\nus in our moment\nof peril!\fI have to thank\nyou in some way!\fBecause I am rich,\nI can give you\nanything!\fHere, maybe this\nwill do!", NULL },  /* SPRITE_SILPH_PRESIDENT, STAY, TEXT_SILPHCO11F_SILPH_PRESIDENT */
    {  20,  11, 0x0f, 0, "SECRETARY: Thank\nyou for rescuing\nall of us!\fWe admire your\ncourage.", NULL },  /* SPRITE_BEAUTY, STAY, TEXT_SILPHCO11F_BEAUTY */
    {  12,  19, 0x17, 0, "Ah {PLAYER}!\nSo we meet again!\fThe PRESIDENT and\nI are discussing\na vital business\nproposition.\fKeep your nose\nout of grown-up\nmatters...\fOr, experience a\nworld of pain!", NULL },  /* SPRITE_GIOVANNI, STAY, TEXT_SILPHCO11F_GIOVANNI */
    {   6,  33, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_SILPHCO11F_ROCKET1 */
    {  30,  19, 0x18, 0, NULL, NULL },  /* SPRITE_ROCKET, STAY, TEXT_SILPHCO11F_ROCKET2 */
};

static const map_warp_t kWarps_SilphCoElevator[] = {
    {   2,   7, 0xed, 0 },  /* UNUSED_MAP_ED */
    {   4,   7, 0xed, 0 },  /* UNUSED_MAP_ED */
};

static const sign_event_t kSigns_SilphCoElevator[] = {
    {   6,   1, NULL },  /* TEXT_SILPHCOELEVATOR_ELEVATOR */
};

static const npc_event_t kNpcs_TradeCenter[] = {
    {   4,   5, 0x01, 0, NULL, NULL },  /* SPRITE_RED, STAY, TEXT_TRADECENTER_OPPONENT */
};

static const npc_event_t kNpcs_Colosseum[] = {
    {   4,   5, 0x01, 0, NULL, NULL },  /* SPRITE_RED, STAY, TEXT_COLOSSEUM_OPPONENT */
};

static const map_warp_t kWarps_LoreleisRoom[] = {
    {   8,  23, 0xae, 2 },  /* INDIGO_PLATEAU_LOBBY */
    {  10,  23, 0xae, 2 },  /* INDIGO_PLATEAU_LOBBY */
    {   8,   1, 0xf6, 0 },  /* BRUNOS_ROOM */
    {  10,   1, 0xf6, 1 },  /* BRUNOS_ROOM */
};

static const npc_event_t kNpcs_LoreleisRoom[] = {
    {  10,   5, 0x3b, 0, NULL, NULL },  /* SPRITE_LORELEI, STAY, TEXT_LORELEISROOM_LORELEI */
};

static const map_warp_t kWarps_BrunosRoom[] = {
    {   8,  23, 0xf5, 2 },  /* LORELEIS_ROOM */
    {  10,  23, 0xf5, 3 },  /* LORELEIS_ROOM */
    {   8,   1, 0xf7, 0 },  /* AGATHAS_ROOM */
    {  10,   1, 0xf7, 1 },  /* AGATHAS_ROOM */
};

static const npc_event_t kNpcs_BrunosRoom[] = {
    {  10,   5, 0x3a, 0, NULL, NULL },  /* SPRITE_BRUNO, STAY, TEXT_BRUNOSROOM_BRUNO */
};

static const map_warp_t kWarps_AgathasRoom[] = {
    {   8,  23, 0xf6, 2 },  /* BRUNOS_ROOM */
    {  10,  23, 0xf6, 3 },  /* BRUNOS_ROOM */
    {   8,   1, 0x71, 0 },  /* LANCES_ROOM */
    {  10,   1, 0x71, 0 },  /* LANCES_ROOM */
};

static const npc_event_t kNpcs_AgathasRoom[] = {
    {  10,   5, 0x39, 0, NULL, NULL },  /* SPRITE_AGATHA, STAY, TEXT_AGATHASROOM_AGATHA */
};

const map_events_t gMapEvents[NUM_MAPS] = {
    [0x00] = { kWarps_PalletTown, 3, kNpcs_PalletTown, 3, kSigns_PalletTown, 4, NULL, 0, 0x0b },
    [0x01] = { kWarps_ViridianCity, 5, kNpcs_ViridianCity, 7, kSigns_ViridianCity, 6, NULL, 0, 0x0f },
    [0x02] = { kWarps_PewterCity, 7, kNpcs_PewterCity, 5, kSigns_PewterCity, 7, NULL, 0, 0x0a },
    [0x03] = { kWarps_CeruleanCity, 10, kNpcs_CeruleanCity, 11, kSigns_CeruleanCity, 6, NULL, 0, 0x0f },
    [0x04] = { kWarps_LavenderTown, 6, kNpcs_LavenderTown, 3, kSigns_LavenderTown, 6, NULL, 0, 0x2c },
    [0x05] = { kWarps_VermilionCity, 9, kNpcs_VermilionCity, 6, kSigns_VermilionCity, 7, NULL, 0, 0x43 },
    [0x06] = { kWarps_CeladonCity, 13, kNpcs_CeladonCity, 9, kSigns_CeladonCity, 9, NULL, 0, 0x0f },
    [0x07] = { kWarps_FuchsiaCity, 9, kNpcs_FuchsiaCity, 9, kSigns_FuchsiaCity, 14, NULL, 0, 0x0f },
    [0x08] = { kWarps_CinnabarIsland, 5, kNpcs_CinnabarIsland, 2, kSigns_CinnabarIsland, 5, NULL, 0, 0x43 },
    [0x09] = { kWarps_IndigoPlateau, 2, NULL, 0, NULL, 0, NULL, 0, 0x0e },
    [0x0a] = { kWarps_SaffronCity, 8, kNpcs_SaffronCity, 15, kSigns_SaffronCity, 10, NULL, 0, 0x0f },
    [0x0b] = { kWarps_SaffronCity, 8, kNpcs_SaffronCity, 15, kSigns_SaffronCity, 10, NULL, 0, 0x0f },
    [0x0c] = { NULL, 0, kNpcs_Route1, 2, kSigns_Route1, 1, NULL, 0, 0x0b },
    [0x0d] = { kWarps_Route2, 6, NULL, 0, kSigns_Route2, 2, kItems_Route2, 2, 0x0f },
    [0x0e] = { NULL, 0, kNpcs_Route3, 9, kSigns_Route3, 1, NULL, 0, 0x2c },
    [0x0f] = { kWarps_Route4, 3, kNpcs_Route4, 2, kSigns_Route4, 3, kItems_Route4, 1, 0x2c },
    [0x10] = { kWarps_Route5, 5, NULL, 0, kSigns_Route5, 1, NULL, 0, 0x0a },
    [0x11] = { kWarps_Route6, 4, kNpcs_Route6, 6, kSigns_Route6, 1, NULL, 0, 0x0f },
    [0x12] = { kWarps_Route7, 5, NULL, 0, kSigns_Route7, 1, NULL, 0, 0x0f },
    [0x13] = { kWarps_Route8, 5, kNpcs_Route8, 9, kSigns_Route8, 1, NULL, 0, 0x2c },
    [0x14] = { NULL, 0, kNpcs_Route9, 9, kSigns_Route9, 1, kItems_Route9, 1, 0x2c },
    [0x15] = { kWarps_Route10, 4, kNpcs_Route10, 6, kSigns_Route10, 4, NULL, 0, 0x2c },
    [0x16] = { kWarps_Route11, 5, kNpcs_Route11, 10, kSigns_Route11, 1, NULL, 0, 0x0f },
    [0x17] = { kWarps_Route12, 4, kNpcs_Route12, 8, kSigns_Route12, 2, kItems_Route12, 2, 0x43 },
    [0x18] = { NULL, 0, kNpcs_Route13, 10, kSigns_Route13, 3, NULL, 0, 0x43 },
    [0x19] = { NULL, 0, kNpcs_Route14, 10, kSigns_Route14, 1, NULL, 0, 0x43 },
    [0x1a] = { kWarps_Route15, 4, kNpcs_Route15, 10, kSigns_Route15, 1, kItems_Route15, 1, 0x43 },
    [0x1b] = { kWarps_Route16, 9, kNpcs_Route16, 7, kSigns_Route16, 2, NULL, 0, 0x0f },
    [0x1c] = { NULL, 0, kNpcs_Route17, 10, kSigns_Route17, 6, NULL, 0, 0x43 },
    [0x1d] = { kWarps_Route18, 4, kNpcs_Route18, 3, kSigns_Route18, 2, NULL, 0, 0x43 },
    [0x1e] = { NULL, 0, kNpcs_Route19, 10, kSigns_Route19, 1, NULL, 0, 0x43 },
    [0x1f] = { kWarps_Route20, 2, kNpcs_Route20, 10, kSigns_Route20, 2, NULL, 0, 0x43 },
    [0x20] = { NULL, 0, kNpcs_Route21, 9, NULL, 0, NULL, 0, 0x43 },
    [0x21] = { kWarps_Route22, 1, kNpcs_Route22, 2, kSigns_Route22, 1, NULL, 0, 0x2c },
    [0x22] = { kWarps_Route23, 4, kNpcs_Route23, 7, kSigns_Route23, 1, NULL, 0, 0x0f },
    [0x23] = { NULL, 0, kNpcs_Route24, 7, NULL, 0, kItems_Route24, 1, 0x2c },
    [0x24] = { kWarps_Route25, 1, kNpcs_Route25, 9, kSigns_Route25, 1, kItems_Route25, 1, 0x2c },
    [0x25] = { kWarps_RedsHouse1F, 3, kNpcs_RedsHouse1F, 1, kSigns_RedsHouse1F, 1, NULL, 0, 0x0a },
    [0x26] = { kWarps_RedsHouse2F, 1, NULL, 0, NULL, 0, NULL, 0, 0x0a },
    [0x27] = { kWarps_BluesHouse, 2, kNpcs_BluesHouse, 3, NULL, 0, NULL, 0, 0x0a },
    [0x28] = { kWarps_OaksLab, 2, kNpcs_OaksLab, 8, NULL, 0, NULL, 0, 0x03 },
    [0x29] = { kWarps_ViridianPokecenter, 2, kNpcs_ViridianPokecenter, 4, NULL, 0, NULL, 0, 0x00 },
    [0x2a] = { kWarps_ViridianMart, 2, kNpcs_ViridianMart, 3, NULL, 0, NULL, 0, 0x00 },
    [0x2b] = { kWarps_ViridianSchoolHouse, 2, kNpcs_ViridianSchoolHouse, 2, NULL, 0, NULL, 0, 0x0a },
    [0x2c] = { kWarps_ViridianNicknameHouse, 2, kNpcs_ViridianNicknameHouse, 4, NULL, 0, NULL, 0, 0x0a },
    [0x2d] = { kWarps_ViridianGym, 2, kNpcs_ViridianGym, 10, NULL, 0, kItems_ViridianGym, 1, 0x03 },
    [0x2e] = { kWarps_DiglettsCaveRoute2, 3, kNpcs_DiglettsCaveRoute2, 1, NULL, 0, NULL, 0, 0x7d },
    [0x2f] = { kWarps_ViridianForestNorthGate, 4, kNpcs_ViridianForestNorthGate, 2, NULL, 0, NULL, 0, 0x0a },
    [0x30] = { kWarps_Route2TradeHouse, 2, kNpcs_Route2TradeHouse, 2, NULL, 0, NULL, 0, 0x0a },
    [0x31] = { kWarps_Route2Gate, 4, kNpcs_Route2Gate, 2, NULL, 0, NULL, 0, 0x0a },
    [0x32] = { kWarps_ViridianForestSouthGate, 4, kNpcs_ViridianForestSouthGate, 2, NULL, 0, NULL, 0, 0x0a },
    [0x33] = { kWarps_ViridianForest, 6, kNpcs_ViridianForest, 5, kSigns_ViridianForest, 6, kItems_ViridianForest, 3, 0x03 },
    [0x34] = { kWarps_Museum1F, 5, kNpcs_Museum1F, 5, NULL, 0, NULL, 0, 0x0a },
    [0x35] = { kWarps_Museum2F, 1, kNpcs_Museum2F, 5, kSigns_Museum2F, 2, NULL, 0, 0x0a },
    [0x36] = { kWarps_PewterGym, 2, kNpcs_PewterGym, 3, NULL, 0, NULL, 0, 0x03 },
    [0x37] = { kWarps_PewterNidoranHouse, 2, kNpcs_PewterNidoranHouse, 3, NULL, 0, NULL, 0, 0x0a },
    [0x38] = { kWarps_PewterMart, 2, kNpcs_PewterMart, 3, NULL, 0, NULL, 0, 0x00 },
    [0x39] = { kWarps_PewterSpeechHouse, 2, kNpcs_PewterSpeechHouse, 2, NULL, 0, NULL, 0, 0x0a },
    [0x3a] = { kWarps_PewterPokecenter, 2, kNpcs_PewterPokecenter, 4, NULL, 0, NULL, 0, 0x00 },
    [0x3b] = { kWarps_MtMoon1F, 5, kNpcs_MtMoon1F, 7, kSigns_MtMoon1F, 1, kItems_MtMoon1F, 6, 0x03 },
    [0x3c] = { kWarps_MtMoonB1F, 8, NULL, 0, NULL, 0, NULL, 0, 0x03 },
    [0x3d] = { kWarps_MtMoonB2F, 4, kNpcs_MtMoonB2F, 7, NULL, 0, kItems_MtMoonB2F, 2, 0x03 },
    [0x3e] = { kWarps_CeruleanTrashedHouse, 3, kNpcs_CeruleanTrashedHouse, 2, kSigns_CeruleanTrashedHouse, 1, NULL, 0, 0x0a },
    [0x3f] = { kWarps_CeruleanTradeHouse, 2, kNpcs_CeruleanTradeHouse, 2, NULL, 0, NULL, 0, 0x0a },
    [0x40] = { kWarps_CeruleanPokecenter, 2, kNpcs_CeruleanPokecenter, 4, NULL, 0, NULL, 0, 0x00 },
    [0x41] = { kWarps_CeruleanGym, 2, kNpcs_CeruleanGym, 4, NULL, 0, NULL, 0, 0x03 },
    [0x42] = { kWarps_BikeShop, 2, kNpcs_BikeShop, 3, NULL, 0, NULL, 0, 0x0e },
    [0x43] = { kWarps_CeruleanMart, 2, kNpcs_CeruleanMart, 3, NULL, 0, NULL, 0, 0x00 },
    [0x44] = { kWarps_MtMoonPokecenter, 2, kNpcs_MtMoonPokecenter, 6, NULL, 0, NULL, 0, 0x00 },
    [0x45] = { kWarps_CeruleanTrashedHouse, 3, kNpcs_CeruleanTrashedHouse, 2, kSigns_CeruleanTrashedHouse, 1, NULL, 0, 0x0a },
    [0x46] = { kWarps_Route5Gate, 4, kNpcs_Route5Gate, 1, NULL, 0, NULL, 0, 0x0a },
    [0x47] = { kWarps_UndergroundPathRoute5, 3, kNpcs_UndergroundPathRoute5, 1, NULL, 0, NULL, 0, 0x0a },
    [0x48] = { kWarps_Daycare, 2, kNpcs_Daycare, 1, NULL, 0, NULL, 0, 0x0a },
    [0x49] = { kWarps_Route6Gate, 4, kNpcs_Route6Gate, 1, NULL, 0, NULL, 0, 0x0a },
    [0x4a] = { kWarps_UndergroundPathRoute6, 3, kNpcs_UndergroundPathRoute6, 1, NULL, 0, NULL, 0, 0x0a },
    [0x4b] = { kWarps_UndergroundPathRoute6, 3, kNpcs_UndergroundPathRoute6, 1, NULL, 0, NULL, 0, 0x0a },
    [0x4c] = { kWarps_Route7Gate, 4, kNpcs_Route7Gate, 1, NULL, 0, NULL, 0, 0x0a },
    [0x4d] = { kWarps_UndergroundPathRoute7, 3, kNpcs_UndergroundPathRoute7, 1, NULL, 0, NULL, 0, 0x0a },
    [0x4e] = { kWarps_UndergroundPathRoute7Copy, 3, kNpcs_UndergroundPathRoute7Copy, 2, NULL, 0, NULL, 0, 0x0a },
    [0x4f] = { kWarps_Route8Gate, 4, kNpcs_Route8Gate, 1, NULL, 0, NULL, 0, 0x0a },
    [0x50] = { kWarps_UndergroundPathRoute8, 3, kNpcs_UndergroundPathRoute8, 1, NULL, 0, NULL, 0, 0x0a },
    [0x51] = { kWarps_RockTunnelPokecenter, 2, kNpcs_RockTunnelPokecenter, 4, NULL, 0, NULL, 0, 0x00 },
    [0x52] = { kWarps_RockTunnel1F, 8, kNpcs_RockTunnel1F, 7, kSigns_RockTunnel1F, 1, NULL, 0, 0x03 },
    [0x53] = { kWarps_PowerPlant, 3, kNpcs_PowerPlant, 1, NULL, 0, kItems_PowerPlant, 13, 0x2e },
    [0x54] = { kWarps_Route11Gate1F, 5, kNpcs_Route11Gate1F, 1, NULL, 0, NULL, 0, 0x0a },
    [0x55] = { kWarps_DiglettsCaveRoute11, 3, kNpcs_DiglettsCaveRoute11, 1, NULL, 0, NULL, 0, 0x7d },
    [0x56] = { kWarps_Route11Gate2F, 1, kNpcs_Route11Gate2F, 2, kSigns_Route11Gate2F, 2, NULL, 0, 0x0a },
    [0x57] = { kWarps_Route12Gate1F, 5, kNpcs_Route12Gate1F, 1, NULL, 0, NULL, 0, 0x0a },
    [0x58] = { kWarps_BillsHouse, 2, kNpcs_BillsHouse, 3, NULL, 0, NULL, 0, 0x0d },
    [0x59] = { kWarps_VermilionPokecenter, 2, kNpcs_VermilionPokecenter, 4, NULL, 0, NULL, 0, 0x00 },
    [0x5a] = { kWarps_PokemonFanClub, 2, kNpcs_PokemonFanClub, 6, kSigns_PokemonFanClub, 2, NULL, 0, 0x0d },
    [0x5b] = { kWarps_VermilionMart, 2, kNpcs_VermilionMart, 3, NULL, 0, NULL, 0, 0x00 },
    [0x5c] = { kWarps_VermilionGym, 2, kNpcs_VermilionGym, 5, NULL, 0, NULL, 0, 0x03 },
    [0x5d] = { kWarps_VermilionPidgeyHouse, 2, kNpcs_VermilionPidgeyHouse, 3, NULL, 0, NULL, 0, 0x0a },
    [0x5e] = { kWarps_VermilionDock, 2, NULL, 0, NULL, 0, NULL, 0, 0x0f },
    [0x5f] = { kWarps_SSAnne1F, 11, kNpcs_SSAnne1F, 2, NULL, 0, NULL, 0, 0x0c },
    [0x60] = { kWarps_SSAnne2F, 9, kNpcs_SSAnne2F, 2, NULL, 0, NULL, 0, 0x0c },
    [0x61] = { kWarps_SSAnne3F, 2, kNpcs_SSAnne3F, 1, NULL, 0, NULL, 0, 0x0c },
    [0x62] = { kWarps_SSAnneB1F, 6, NULL, 0, NULL, 0, NULL, 0, 0x0c },
    [0x63] = { kWarps_SSAnneBow, 2, kNpcs_SSAnneBow, 5, NULL, 0, NULL, 0, 0x23 },
    [0x64] = { kWarps_SSAnneKitchen, 1, kNpcs_SSAnneKitchen, 7, NULL, 0, NULL, 0, 0x0c },
    [0x65] = { kWarps_SSAnneCaptainsRoom, 1, kNpcs_SSAnneCaptainsRoom, 1, kSigns_SSAnneCaptainsRoom, 2, NULL, 0, 0x0c },
    [0x66] = { kWarps_SSAnne1FRooms, 6, kNpcs_SSAnne1FRooms, 10, NULL, 0, kItems_SSAnne1FRooms, 1, 0x0c },
    [0x67] = { kWarps_SSAnne2FRooms, 12, kNpcs_SSAnne2FRooms, 11, NULL, 0, kItems_SSAnne2FRooms, 2, 0x0c },
    [0x68] = { kWarps_SSAnneB1FRooms, 10, kNpcs_SSAnneB1FRooms, 8, NULL, 0, kItems_SSAnneB1FRooms, 3, 0x0c },
    [0x69] = { kWarps_LancesRoom, 3, kNpcs_LancesRoom, 1, NULL, 0, NULL, 0, 0x03 },
    [0x6a] = { kWarps_LancesRoom, 3, kNpcs_LancesRoom, 1, NULL, 0, NULL, 0, 0x03 },
    [0x6b] = { kWarps_LancesRoom, 3, kNpcs_LancesRoom, 1, NULL, 0, NULL, 0, 0x03 },
    [0x6c] = { kWarps_VictoryRoad1F, 3, kNpcs_VictoryRoad1F, 5, NULL, 0, kItems_VictoryRoad1F, 2, 0x7d },
    [0x6d] = { kWarps_LancesRoom, 3, kNpcs_LancesRoom, 1, NULL, 0, NULL, 0, 0x03 },
    [0x6e] = { kWarps_LancesRoom, 3, kNpcs_LancesRoom, 1, NULL, 0, NULL, 0, 0x03 },
    [0x6f] = { kWarps_LancesRoom, 3, kNpcs_LancesRoom, 1, NULL, 0, NULL, 0, 0x03 },
    [0x70] = { kWarps_LancesRoom, 3, kNpcs_LancesRoom, 1, NULL, 0, NULL, 0, 0x03 },
    [0x71] = { kWarps_LancesRoom, 3, kNpcs_LancesRoom, 1, NULL, 0, NULL, 0, 0x03 },
    [0x72] = { kWarps_LancesRoom, 3, kNpcs_LancesRoom, 1, NULL, 0, NULL, 0, 0x03 },
    [0x73] = { kWarps_LancesRoom, 3, kNpcs_LancesRoom, 1, NULL, 0, NULL, 0, 0x03 },
    [0x74] = { kWarps_LancesRoom, 3, kNpcs_LancesRoom, 1, NULL, 0, NULL, 0, 0x03 },
    [0x75] = { kWarps_LancesRoom, 3, kNpcs_LancesRoom, 1, NULL, 0, NULL, 0, 0x03 },
    [0x76] = { kWarps_HallOfFame, 2, kNpcs_HallOfFame, 1, NULL, 0, NULL, 0, 0x03 },
    [0x77] = { kWarps_UndergroundPathNorthSouth, 2, NULL, 0, NULL, 0, NULL, 0, 0x01 },
    [0x78] = { kWarps_ChampionsRoom, 4, kNpcs_ChampionsRoom, 2, NULL, 0, NULL, 0, 0x03 },
    [0x79] = { kWarps_UndergroundPathWestEast, 2, NULL, 0, NULL, 0, NULL, 0, 0x01 },
    [0x7a] = { kWarps_CeladonMart1F, 6, kNpcs_CeladonMart1F, 1, kSigns_CeladonMart1F, 2, NULL, 0, 0x0f },
    [0x7b] = { kWarps_CeladonMart2F, 3, kNpcs_CeladonMart2F, 4, kSigns_CeladonMart2F, 1, NULL, 0, 0x0f },
    [0x7c] = { kWarps_CeladonMart3F, 3, kNpcs_CeladonMart3F, 5, kSigns_CeladonMart3F, 12, NULL, 0, 0x0f },
    [0x7d] = { kWarps_CeladonMart4F, 3, kNpcs_CeladonMart4F, 3, kSigns_CeladonMart4F, 1, NULL, 0, 0x0f },
    [0x7e] = { kWarps_CeladonMartRoof, 1, kNpcs_CeladonMartRoof, 2, kSigns_CeladonMartRoof, 4, NULL, 0, 0x42 },
    [0x7f] = { kWarps_CeladonMartElevator, 2, NULL, 0, kSigns_CeladonMartElevator, 1, NULL, 0, 0x0f },
    [0x80] = { kWarps_CeladonMansion1F, 5, kNpcs_CeladonMansion1F, 4, kSigns_CeladonMansion1F, 1, NULL, 0, 0x0f },
    [0x81] = { kWarps_CeladonMansion2F, 4, NULL, 0, kSigns_CeladonMansion2F, 1, NULL, 0, 0x0f },
    [0x82] = { kWarps_CeladonMansion3F, 4, kNpcs_CeladonMansion3F, 4, kSigns_CeladonMansion3F, 4, NULL, 0, 0x0f },
    [0x83] = { kWarps_CeladonMansionRoof, 3, NULL, 0, kSigns_CeladonMansionRoof, 1, NULL, 0, 0x09 },
    [0x84] = { kWarps_CeladonMansionRoofHouse, 2, kNpcs_CeladonMansionRoofHouse, 1, NULL, 0, NULL, 0, 0x0a },
    [0x85] = { kWarps_CeladonPokecenter, 2, kNpcs_CeladonPokecenter, 4, NULL, 0, NULL, 0, 0x00 },
    [0x86] = { kWarps_CeladonGym, 2, kNpcs_CeladonGym, 8, NULL, 0, NULL, 0, 0x03 },
    [0x87] = { kWarps_GameCorner, 3, kNpcs_GameCorner, 11, kSigns_GameCorner, 1, NULL, 0, 0x0f },
    [0x88] = { kWarps_CeladonMart5F, 3, kNpcs_CeladonMart5F, 4, kSigns_CeladonMart5F, 1, NULL, 0, 0x0f },
    [0x89] = { kWarps_GameCornerPrizeRoom, 2, kNpcs_GameCornerPrizeRoom, 2, kSigns_GameCornerPrizeRoom, 3, NULL, 0, 0x0f },
    [0x8a] = { kWarps_CeladonDiner, 2, kNpcs_CeladonDiner, 5, NULL, 0, NULL, 0, 0x0f },
    [0x8b] = { kWarps_CeladonChiefHouse, 2, kNpcs_CeladonChiefHouse, 3, NULL, 0, NULL, 0, 0x0f },
    [0x8c] = { kWarps_CeladonHotel, 2, kNpcs_CeladonHotel, 3, NULL, 0, NULL, 0, 0x00 },
    [0x8d] = { kWarps_LavenderPokecenter, 2, kNpcs_LavenderPokecenter, 4, NULL, 0, NULL, 0, 0x00 },
    [0x8e] = { kWarps_PokemonTower1F, 3, kNpcs_PokemonTower1F, 5, NULL, 0, NULL, 0, 0x01 },
    [0x8f] = { kWarps_PokemonTower2F, 2, kNpcs_PokemonTower2F, 2, NULL, 0, NULL, 0, 0x01 },
    [0x90] = { kWarps_PokemonTower3F, 2, kNpcs_PokemonTower3F, 3, NULL, 0, kItems_PokemonTower3F, 1, 0x01 },
    [0x91] = { kWarps_PokemonTower4F, 2, kNpcs_PokemonTower4F, 3, NULL, 0, kItems_PokemonTower4F, 3, 0x01 },
    [0x92] = { kWarps_PokemonTower5F, 2, kNpcs_PokemonTower5F, 5, NULL, 0, kItems_PokemonTower5F, 1, 0x01 },
    [0x93] = { kWarps_PokemonTower6F, 2, kNpcs_PokemonTower6F, 3, NULL, 0, kItems_PokemonTower6F, 2, 0x01 },
    [0x94] = { kWarps_PokemonTower7F, 1, kNpcs_PokemonTower7F, 4, NULL, 0, NULL, 0, 0x01 },
    [0x95] = { kWarps_MrFujisHouse, 2, kNpcs_MrFujisHouse, 6, NULL, 0, NULL, 0, 0x0a },
    [0x96] = { kWarps_LavenderMart, 2, kNpcs_LavenderMart, 3, NULL, 0, NULL, 0, 0x00 },
    [0x97] = { kWarps_LavenderCuboneHouse, 2, kNpcs_LavenderCuboneHouse, 2, NULL, 0, NULL, 0, 0x0a },
    [0x98] = { kWarps_FuchsiaMart, 2, kNpcs_FuchsiaMart, 3, NULL, 0, NULL, 0, 0x00 },
    [0x99] = { kWarps_FuchsiaBillsGrandpasHouse, 2, kNpcs_FuchsiaBillsGrandpasHouse, 3, NULL, 0, NULL, 0, 0x0a },
    [0x9a] = { kWarps_FuchsiaPokecenter, 2, kNpcs_FuchsiaPokecenter, 4, NULL, 0, NULL, 0, 0x00 },
    [0x9b] = { kWarps_WardensHouse, 2, kNpcs_WardensHouse, 2, kSigns_WardensHouse, 2, kItems_WardensHouse, 1, 0x17 },
    [0x9c] = { kWarps_SafariZoneGate, 4, kNpcs_SafariZoneGate, 2, NULL, 0, NULL, 0, 0x0a },
    [0x9d] = { kWarps_FuchsiaGym, 2, kNpcs_FuchsiaGym, 8, NULL, 0, NULL, 0, 0x03 },
    [0x9e] = { kWarps_FuchsiaMeetingRoom, 2, kNpcs_FuchsiaMeetingRoom, 3, NULL, 0, NULL, 0, 0x17 },
    [0x9f] = { kWarps_SeafoamIslandsB1F, 7, kNpcs_SeafoamIslandsB1F, 2, NULL, 0, NULL, 0, 0x7d },
    [0xa0] = { kWarps_SeafoamIslandsB2F, 7, kNpcs_SeafoamIslandsB2F, 2, NULL, 0, NULL, 0, 0x7d },
    [0xa1] = { kWarps_SeafoamIslandsB3F, 7, kNpcs_SeafoamIslandsB3F, 6, NULL, 0, NULL, 0, 0x7d },
    [0xa2] = { kWarps_SeafoamIslandsB4F, 4, kNpcs_SeafoamIslandsB4F, 3, kSigns_SeafoamIslandsB4F, 2, NULL, 0, 0x7d },
    [0xa3] = { kWarps_VermilionOldRodHouse, 2, kNpcs_VermilionOldRodHouse, 1, NULL, 0, NULL, 0, 0x0a },
    [0xa4] = { kWarps_FuchsiaGoodRodHouse, 3, kNpcs_FuchsiaGoodRodHouse, 1, NULL, 0, NULL, 0, 0x0c },
    [0xa5] = { kWarps_PokemonMansion1F, 8, kNpcs_PokemonMansion1F, 1, NULL, 0, kItems_PokemonMansion1F, 2, 0x2e },
    [0xa6] = { kWarps_CinnabarGym, 2, kNpcs_CinnabarGym, 9, NULL, 0, NULL, 0, 0x2e },
    [0xa7] = { kWarps_CinnabarLab, 5, kNpcs_CinnabarLab, 1, kSigns_CinnabarLab, 4, NULL, 0, 0x17 },
    [0xa8] = { kWarps_CinnabarLabTradeRoom, 2, kNpcs_CinnabarLabTradeRoom, 3, NULL, 0, NULL, 0, 0x17 },
    [0xa9] = { kWarps_CinnabarLabMetronomeRoom, 2, kNpcs_CinnabarLabMetronomeRoom, 2, kSigns_CinnabarLabMetronomeRoom, 3, NULL, 0, 0x17 },
    [0xaa] = { kWarps_CinnabarLabFossilRoom, 2, kNpcs_CinnabarLabFossilRoom, 2, NULL, 0, NULL, 0, 0x17 },
    [0xab] = { kWarps_CinnabarPokecenter, 2, kNpcs_CinnabarPokecenter, 4, NULL, 0, NULL, 0, 0x00 },
    [0xac] = { kWarps_CinnabarMart, 2, kNpcs_CinnabarMart, 3, NULL, 0, NULL, 0, 0x00 },
    [0xad] = { kWarps_CinnabarMart, 2, kNpcs_CinnabarMart, 3, NULL, 0, NULL, 0, 0x00 },
    [0xae] = { kWarps_IndigoPlateauLobby, 3, kNpcs_IndigoPlateauLobby, 5, NULL, 0, NULL, 0, 0x00 },
    [0xaf] = { kWarps_CopycatsHouse1F, 3, kNpcs_CopycatsHouse1F, 3, NULL, 0, NULL, 0, 0x0a },
    [0xb0] = { kWarps_CopycatsHouse2F, 1, kNpcs_CopycatsHouse2F, 5, kSigns_CopycatsHouse2F, 2, NULL, 0, 0x0a },
    [0xb1] = { kWarps_FightingDojo, 2, kNpcs_FightingDojo, 5, NULL, 0, NULL, 0, 0x03 },
    [0xb2] = { kWarps_SaffronGym, 32, kNpcs_SaffronGym, 9, NULL, 0, NULL, 0, 0x2e },
    [0xb3] = { kWarps_SaffronPidgeyHouse, 2, kNpcs_SaffronPidgeyHouse, 4, NULL, 0, NULL, 0, 0x0a },
    [0xb4] = { kWarps_SaffronMart, 2, kNpcs_SaffronMart, 3, NULL, 0, NULL, 0, 0x00 },
    [0xb5] = { kWarps_SilphCo1F, 5, kNpcs_SilphCo1F, 1, NULL, 0, NULL, 0, 0x2e },
    [0xb6] = { kWarps_SaffronPokecenter, 2, kNpcs_SaffronPokecenter, 4, NULL, 0, NULL, 0, 0x00 },
    [0xb7] = { kWarps_MrPsychicsHouse, 2, kNpcs_MrPsychicsHouse, 1, NULL, 0, NULL, 0, 0x0a },
    [0xb8] = { kWarps_Route15Gate1F, 5, kNpcs_Route15Gate1F, 1, NULL, 0, NULL, 0, 0x0a },
    [0xb9] = { kWarps_Route15Gate2F, 1, kNpcs_Route15Gate2F, 1, kSigns_Route15Gate2F, 1, NULL, 0, 0x0a },
    [0xba] = { kWarps_Route16Gate1F, 9, kNpcs_Route16Gate1F, 2, NULL, 0, NULL, 0, 0x0a },
    [0xbb] = { kWarps_Route16Gate2F, 1, kNpcs_Route16Gate2F, 2, kSigns_Route16Gate2F, 2, NULL, 0, 0x0a },
    [0xbc] = { kWarps_Route16FlyHouse, 2, kNpcs_Route16FlyHouse, 2, NULL, 0, NULL, 0, 0x0a },
    [0xbd] = { kWarps_Route12SuperRodHouse, 2, kNpcs_Route12SuperRodHouse, 1, NULL, 0, NULL, 0, 0x0a },
    [0xbe] = { kWarps_Route18Gate1F, 5, kNpcs_Route18Gate1F, 1, NULL, 0, NULL, 0, 0x0a },
    [0xbf] = { kWarps_Route18Gate2F, 1, kNpcs_Route18Gate2F, 1, kSigns_Route18Gate2F, 2, NULL, 0, 0x0a },
    [0xc0] = { kWarps_SeafoamIslands1F, 7, kNpcs_SeafoamIslands1F, 2, NULL, 0, NULL, 0, 0x7d },
    [0xc1] = { kWarps_Route22Gate, 4, kNpcs_Route22Gate, 1, NULL, 0, NULL, 0, 0x0a },
    [0xc2] = { kWarps_VictoryRoad2F, 7, kNpcs_VictoryRoad2F, 9, NULL, 0, kItems_VictoryRoad2F, 4, 0x7d },
    [0xc3] = { kWarps_Route12Gate2F, 1, kNpcs_Route12Gate2F, 1, kSigns_Route12Gate2F, 2, NULL, 0, 0x0a },
    [0xc4] = { kWarps_VermilionTradeHouse, 2, kNpcs_VermilionTradeHouse, 1, NULL, 0, NULL, 0, 0x0a },
    [0xc5] = { kWarps_DiglettsCave, 2, NULL, 0, NULL, 0, NULL, 0, 0x19 },
    [0xc6] = { kWarps_VictoryRoad3F, 4, kNpcs_VictoryRoad3F, 8, NULL, 0, kItems_VictoryRoad3F, 2, 0x7d },
    [0xc7] = { kWarps_RocketHideoutB1F, 5, kNpcs_RocketHideoutB1F, 5, NULL, 0, kItems_RocketHideoutB1F, 2, 0x2e },
    [0xc8] = { kWarps_RocketHideoutB2F, 5, kNpcs_RocketHideoutB2F, 1, NULL, 0, kItems_RocketHideoutB2F, 4, 0x2e },
    [0xc9] = { kWarps_RocketHideoutB3F, 2, kNpcs_RocketHideoutB3F, 2, NULL, 0, kItems_RocketHideoutB3F, 2, 0x2e },
    [0xca] = { kWarps_RocketHideoutB4F, 3, kNpcs_RocketHideoutB4F, 4, NULL, 0, kItems_RocketHideoutB4F, 5, 0x2e },
    [0xcb] = { kWarps_RocketHideoutElevator, 2, NULL, 0, kSigns_RocketHideoutElevator, 1, NULL, 0, 0x0f },
    [0xcc] = { kWarps_RocketHideoutElevator, 2, NULL, 0, kSigns_RocketHideoutElevator, 1, NULL, 0, 0x0f },
    [0xcd] = { kWarps_RocketHideoutElevator, 2, NULL, 0, kSigns_RocketHideoutElevator, 1, NULL, 0, 0x0f },
    [0xce] = { kWarps_RocketHideoutElevator, 2, NULL, 0, kSigns_RocketHideoutElevator, 1, NULL, 0, 0x0f },
    [0xcf] = { kWarps_SilphCo2F, 7, kNpcs_SilphCo2F, 5, NULL, 0, NULL, 0, 0x2e },
    [0xd0] = { kWarps_SilphCo3F, 10, kNpcs_SilphCo3F, 3, NULL, 0, kItems_SilphCo3F, 1, 0x2e },
    [0xd1] = { kWarps_SilphCo4F, 7, kNpcs_SilphCo4F, 4, NULL, 0, kItems_SilphCo4F, 3, 0x2e },
    [0xd2] = { kWarps_SilphCo5F, 7, kNpcs_SilphCo5F, 8, NULL, 0, kItems_SilphCo5F, 3, 0x2e },
    [0xd3] = { kWarps_SilphCo6F, 5, kNpcs_SilphCo6F, 8, NULL, 0, kItems_SilphCo6F, 2, 0x2e },
    [0xd4] = { kWarps_SilphCo7F, 6, kNpcs_SilphCo7F, 9, NULL, 0, kItems_SilphCo7F, 2, 0x2e },
    [0xd5] = { kWarps_SilphCo8F, 7, kNpcs_SilphCo8F, 4, NULL, 0, NULL, 0, 0x2e },
    [0xd6] = { kWarps_PokemonMansion2F, 4, kNpcs_PokemonMansion2F, 3, NULL, 0, kItems_PokemonMansion2F, 1, 0x01 },
    [0xd7] = { kWarps_PokemonMansion3F, 3, kNpcs_PokemonMansion3F, 3, NULL, 0, kItems_PokemonMansion3F, 2, 0x01 },
    [0xd8] = { kWarps_PokemonMansionB1F, 1, kNpcs_PokemonMansionB1F, 3, NULL, 0, kItems_PokemonMansionB1F, 5, 0x01 },
    [0xd9] = { kWarps_SafariZoneEast, 5, NULL, 0, kSigns_SafariZoneEast, 3, kItems_SafariZoneEast, 4, 0x00 },
    [0xda] = { kWarps_SafariZoneNorth, 9, NULL, 0, kSigns_SafariZoneNorth, 5, kItems_SafariZoneNorth, 2, 0x00 },
    [0xdb] = { kWarps_SafariZoneWest, 8, NULL, 0, kSigns_SafariZoneWest, 4, kItems_SafariZoneWest, 4, 0x00 },
    [0xdc] = { kWarps_SafariZoneCenter, 9, NULL, 0, kSigns_SafariZoneCenter, 2, kItems_SafariZoneCenter, 1, 0x00 },
    [0xdd] = { kWarps_SafariZoneCenterRestHouse, 2, kNpcs_SafariZoneCenterRestHouse, 2, NULL, 0, NULL, 0, 0x0a },
    [0xde] = { kWarps_SafariZoneSecretHouse, 2, kNpcs_SafariZoneSecretHouse, 1, NULL, 0, NULL, 0, 0x17 },
    [0xdf] = { kWarps_SafariZoneWestRestHouse, 2, kNpcs_SafariZoneWestRestHouse, 3, NULL, 0, NULL, 0, 0x0a },
    [0xe0] = { kWarps_SafariZoneEastRestHouse, 2, kNpcs_SafariZoneEastRestHouse, 3, NULL, 0, NULL, 0, 0x0a },
    [0xe1] = { kWarps_SafariZoneNorthRestHouse, 2, kNpcs_SafariZoneNorthRestHouse, 3, NULL, 0, NULL, 0, 0x0a },
    [0xe2] = { kWarps_CeruleanCave2F, 6, NULL, 0, NULL, 0, kItems_CeruleanCave2F, 3, 0x7d },
    [0xe3] = { kWarps_CeruleanCaveB1F, 1, kNpcs_CeruleanCaveB1F, 1, NULL, 0, kItems_CeruleanCaveB1F, 2, 0x7d },
    [0xe4] = { kWarps_CeruleanCave1F, 9, NULL, 0, NULL, 0, kItems_CeruleanCave1F, 3, 0x7d },
    [0xe5] = { kWarps_NameRatersHouse, 2, kNpcs_NameRatersHouse, 1, NULL, 0, NULL, 0, 0x0a },
    [0xe6] = { kWarps_CeruleanBadgeHouse, 3, kNpcs_CeruleanBadgeHouse, 1, NULL, 0, NULL, 0, 0x0c },
    [0xe7] = { kWarps_Route16Gate1F, 9, kNpcs_Route16Gate1F, 2, NULL, 0, NULL, 0, 0x0a },
    [0xe8] = { kWarps_RockTunnelB1F, 4, kNpcs_RockTunnelB1F, 8, NULL, 0, NULL, 0, 0x03 },
    [0xe9] = { kWarps_SilphCo9F, 5, kNpcs_SilphCo9F, 4, NULL, 0, NULL, 0, 0x2e },
    [0xea] = { kWarps_SilphCo10F, 6, kNpcs_SilphCo10F, 3, NULL, 0, kItems_SilphCo10F, 3, 0x2e },
    [0xeb] = { kWarps_SilphCo11F, 4, kNpcs_SilphCo11F, 5, NULL, 0, NULL, 0, 0x0d },
    [0xec] = { kWarps_SilphCoElevator, 2, NULL, 0, kSigns_SilphCoElevator, 1, NULL, 0, 0x0f },
    [0xed] = { kWarps_SilphCo2F, 7, kNpcs_SilphCo2F, 5, NULL, 0, NULL, 0, 0x2e },
    [0xee] = { kWarps_SilphCo2F, 7, kNpcs_SilphCo2F, 5, NULL, 0, NULL, 0, 0x2e },
    [0xef] = { NULL, 0, kNpcs_TradeCenter, 1, NULL, 0, NULL, 0, 0x0e },
    [0xf0] = { NULL, 0, kNpcs_Colosseum, 1, NULL, 0, NULL, 0, 0x0e },
    [0xf1] = { kWarps_SilphCo2F, 7, kNpcs_SilphCo2F, 5, NULL, 0, NULL, 0, 0x2e },
    [0xf2] = { kWarps_SilphCo2F, 7, kNpcs_SilphCo2F, 5, NULL, 0, NULL, 0, 0x2e },
    [0xf3] = { kWarps_SilphCo2F, 7, kNpcs_SilphCo2F, 5, NULL, 0, NULL, 0, 0x2e },
    [0xf4] = { kWarps_SilphCo2F, 7, kNpcs_SilphCo2F, 5, NULL, 0, NULL, 0, 0x2e },
    [0xf5] = { kWarps_LoreleisRoom, 4, kNpcs_LoreleisRoom, 1, NULL, 0, NULL, 0, 0x03 },
    [0xf6] = { kWarps_BrunosRoom, 4, kNpcs_BrunosRoom, 1, NULL, 0, NULL, 0, 0x03 },
    [0xf7] = { kWarps_AgathasRoom, 4, kNpcs_AgathasRoom, 1, NULL, 0, NULL, 0, 0x00 },
};
