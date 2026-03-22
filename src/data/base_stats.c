/* base_stats.c -- Generated from pokered-master/data/pokemon/base_stats/<all>.asm */
#include "base_stats.h"

const base_stats_t gBaseStats[152] = {
    [0] = {0}, /* unused */
    [1] = { .dex_id=1, .hp=45, .atk=49, .def=49, .spd=45, .spc=65, .type1=0x16, .type2=0x03, .catch_rate=45, .base_exp=64, .start_moves={33, 45, 0, 0}, .growth_rate=3 },  /* bulbasaur */
    [2] = { .dex_id=2, .hp=60, .atk=62, .def=63, .spd=60, .spc=80, .type1=0x16, .type2=0x03, .catch_rate=45, .base_exp=141, .start_moves={33, 45, 73, 0}, .growth_rate=3 },  /* ivysaur */
    [3] = { .dex_id=3, .hp=80, .atk=82, .def=83, .spd=80, .spc=100, .type1=0x16, .type2=0x03, .catch_rate=45, .base_exp=208, .start_moves={33, 45, 73, 22}, .growth_rate=3 },  /* venusaur */
    [4] = { .dex_id=4, .hp=39, .atk=52, .def=43, .spd=65, .spc=50, .type1=0x14, .type2=0x14, .catch_rate=45, .base_exp=65, .start_moves={10, 45, 0, 0}, .growth_rate=3 },  /* charmander */
    [5] = { .dex_id=5, .hp=58, .atk=64, .def=58, .spd=80, .spc=65, .type1=0x14, .type2=0x14, .catch_rate=45, .base_exp=142, .start_moves={10, 45, 52, 0}, .growth_rate=3 },  /* charmeleon */
    [6] = { .dex_id=6, .hp=78, .atk=84, .def=78, .spd=100, .spc=85, .type1=0x14, .type2=0x02, .catch_rate=45, .base_exp=209, .start_moves={10, 45, 52, 43}, .growth_rate=3 },  /* charizard */
    [7] = { .dex_id=7, .hp=44, .atk=48, .def=65, .spd=43, .spc=50, .type1=0x15, .type2=0x15, .catch_rate=45, .base_exp=66, .start_moves={33, 39, 0, 0}, .growth_rate=3 },  /* squirtle */
    [8] = { .dex_id=8, .hp=59, .atk=63, .def=80, .spd=58, .spc=65, .type1=0x15, .type2=0x15, .catch_rate=45, .base_exp=143, .start_moves={33, 39, 145, 0}, .growth_rate=3 },  /* wartortle */
    [9] = { .dex_id=9, .hp=79, .atk=83, .def=100, .spd=78, .spc=85, .type1=0x15, .type2=0x15, .catch_rate=45, .base_exp=210, .start_moves={33, 39, 145, 55}, .growth_rate=3 },  /* blastoise */
    [10] = { .dex_id=10, .hp=45, .atk=30, .def=35, .spd=45, .spc=20, .type1=0x07, .type2=0x07, .catch_rate=255, .base_exp=53, .start_moves={33, 81, 0, 0}, .growth_rate=0 },  /* caterpie */
    [11] = { .dex_id=11, .hp=50, .atk=20, .def=55, .spd=30, .spc=25, .type1=0x07, .type2=0x07, .catch_rate=120, .base_exp=72, .start_moves={106, 0, 0, 0}, .growth_rate=0 },  /* metapod */
    [12] = { .dex_id=12, .hp=60, .atk=45, .def=50, .spd=70, .spc=80, .type1=0x07, .type2=0x02, .catch_rate=45, .base_exp=160, .start_moves={93, 0, 0, 0}, .growth_rate=0 },  /* butterfree */
    [13] = { .dex_id=13, .hp=40, .atk=35, .def=30, .spd=50, .spc=20, .type1=0x07, .type2=0x03, .catch_rate=255, .base_exp=52, .start_moves={40, 81, 0, 0}, .growth_rate=0 },  /* weedle */
    [14] = { .dex_id=14, .hp=45, .atk=25, .def=50, .spd=35, .spc=25, .type1=0x07, .type2=0x03, .catch_rate=120, .base_exp=71, .start_moves={106, 0, 0, 0}, .growth_rate=0 },  /* kakuna */
    [15] = { .dex_id=15, .hp=65, .atk=80, .def=40, .spd=75, .spc=45, .type1=0x07, .type2=0x03, .catch_rate=45, .base_exp=159, .start_moves={31, 0, 0, 0}, .growth_rate=0 },  /* beedrill */
    [16] = { .dex_id=16, .hp=40, .atk=45, .def=40, .spd=56, .spc=35, .type1=0x00, .type2=0x02, .catch_rate=255, .base_exp=55, .start_moves={16, 0, 0, 0}, .growth_rate=3 },  /* pidgey */
    [17] = { .dex_id=17, .hp=63, .atk=60, .def=55, .spd=71, .spc=50, .type1=0x00, .type2=0x02, .catch_rate=120, .base_exp=113, .start_moves={16, 28, 0, 0}, .growth_rate=3 },  /* pidgeotto */
    [18] = { .dex_id=18, .hp=83, .atk=80, .def=75, .spd=91, .spc=70, .type1=0x00, .type2=0x02, .catch_rate=45, .base_exp=172, .start_moves={16, 28, 98, 0}, .growth_rate=3 },  /* pidgeot */
    [19] = { .dex_id=19, .hp=30, .atk=56, .def=35, .spd=72, .spc=25, .type1=0x00, .type2=0x00, .catch_rate=255, .base_exp=57, .start_moves={33, 39, 0, 0}, .growth_rate=0 },  /* rattata */
    [20] = { .dex_id=20, .hp=55, .atk=81, .def=60, .spd=97, .spc=50, .type1=0x00, .type2=0x00, .catch_rate=90, .base_exp=116, .start_moves={33, 39, 98, 0}, .growth_rate=0 },  /* raticate */
    [21] = { .dex_id=21, .hp=40, .atk=60, .def=30, .spd=70, .spc=31, .type1=0x00, .type2=0x02, .catch_rate=255, .base_exp=58, .start_moves={64, 45, 0, 0}, .growth_rate=0 },  /* spearow */
    [22] = { .dex_id=22, .hp=65, .atk=90, .def=65, .spd=100, .spc=61, .type1=0x00, .type2=0x02, .catch_rate=90, .base_exp=162, .start_moves={64, 45, 43, 0}, .growth_rate=0 },  /* fearow */
    [23] = { .dex_id=23, .hp=35, .atk=60, .def=44, .spd=55, .spc=40, .type1=0x03, .type2=0x03, .catch_rate=255, .base_exp=62, .start_moves={35, 43, 0, 0}, .growth_rate=0 },  /* ekans */
    [24] = { .dex_id=24, .hp=60, .atk=85, .def=69, .spd=80, .spc=65, .type1=0x03, .type2=0x03, .catch_rate=90, .base_exp=147, .start_moves={35, 43, 40, 0}, .growth_rate=0 },  /* arbok */
    [25] = { .dex_id=25, .hp=35, .atk=55, .def=30, .spd=90, .spc=50, .type1=0x17, .type2=0x17, .catch_rate=190, .base_exp=82, .start_moves={84, 45, 0, 0}, .growth_rate=0 },  /* pikachu */
    [26] = { .dex_id=26, .hp=60, .atk=90, .def=55, .spd=100, .spc=90, .type1=0x17, .type2=0x17, .catch_rate=75, .base_exp=122, .start_moves={84, 45, 86, 0}, .growth_rate=0 },  /* raichu */
    [27] = { .dex_id=27, .hp=50, .atk=75, .def=85, .spd=40, .spc=30, .type1=0x04, .type2=0x04, .catch_rate=255, .base_exp=93, .start_moves={10, 0, 0, 0}, .growth_rate=0 },  /* sandshrew */
    [28] = { .dex_id=28, .hp=75, .atk=100, .def=110, .spd=65, .spc=55, .type1=0x04, .type2=0x04, .catch_rate=90, .base_exp=163, .start_moves={10, 28, 0, 0}, .growth_rate=0 },  /* sandslash */
    [29] = { .dex_id=29, .hp=55, .atk=47, .def=52, .spd=41, .spc=40, .type1=0x03, .type2=0x03, .catch_rate=235, .base_exp=59, .start_moves={45, 33, 0, 0}, .growth_rate=3 },  /* nidoranf */
    [30] = { .dex_id=30, .hp=70, .atk=62, .def=67, .spd=56, .spc=55, .type1=0x03, .type2=0x03, .catch_rate=120, .base_exp=117, .start_moves={45, 33, 10, 0}, .growth_rate=3 },  /* nidorina */
    [31] = { .dex_id=31, .hp=90, .atk=82, .def=87, .spd=76, .spc=75, .type1=0x03, .type2=0x04, .catch_rate=45, .base_exp=194, .start_moves={33, 10, 39, 34}, .growth_rate=3 },  /* nidoqueen */
    [32] = { .dex_id=32, .hp=46, .atk=57, .def=40, .spd=50, .spc=40, .type1=0x03, .type2=0x03, .catch_rate=235, .base_exp=60, .start_moves={43, 33, 0, 0}, .growth_rate=3 },  /* nidoranm */
    [33] = { .dex_id=33, .hp=61, .atk=72, .def=57, .spd=65, .spc=55, .type1=0x03, .type2=0x03, .catch_rate=120, .base_exp=118, .start_moves={43, 33, 30, 0}, .growth_rate=3 },  /* nidorino */
    [34] = { .dex_id=34, .hp=81, .atk=92, .def=77, .spd=85, .spc=75, .type1=0x03, .type2=0x04, .catch_rate=45, .base_exp=195, .start_moves={33, 30, 40, 37}, .growth_rate=3 },  /* nidoking */
    [35] = { .dex_id=35, .hp=70, .atk=45, .def=48, .spd=35, .spc=60, .type1=0x00, .type2=0x00, .catch_rate=150, .base_exp=68, .start_moves={1, 45, 0, 0}, .growth_rate=4 },  /* clefairy */
    [36] = { .dex_id=36, .hp=95, .atk=70, .def=73, .spd=60, .spc=85, .type1=0x00, .type2=0x00, .catch_rate=25, .base_exp=129, .start_moves={47, 3, 107, 118}, .growth_rate=4 },  /* clefable */
    [37] = { .dex_id=37, .hp=38, .atk=41, .def=40, .spd=65, .spc=65, .type1=0x14, .type2=0x14, .catch_rate=190, .base_exp=63, .start_moves={52, 39, 0, 0}, .growth_rate=0 },  /* vulpix */
    [38] = { .dex_id=38, .hp=73, .atk=76, .def=75, .spd=100, .spc=100, .type1=0x14, .type2=0x14, .catch_rate=75, .base_exp=178, .start_moves={52, 39, 98, 46}, .growth_rate=0 },  /* ninetales */
    [39] = { .dex_id=39, .hp=115, .atk=45, .def=20, .spd=20, .spc=25, .type1=0x00, .type2=0x00, .catch_rate=170, .base_exp=76, .start_moves={47, 0, 0, 0}, .growth_rate=4 },  /* jigglypuff */
    [40] = { .dex_id=40, .hp=140, .atk=70, .def=45, .spd=45, .spc=50, .type1=0x00, .type2=0x00, .catch_rate=50, .base_exp=109, .start_moves={47, 50, 111, 3}, .growth_rate=4 },  /* wigglytuff */
    [41] = { .dex_id=41, .hp=40, .atk=45, .def=35, .spd=55, .spc=40, .type1=0x03, .type2=0x02, .catch_rate=255, .base_exp=54, .start_moves={141, 0, 0, 0}, .growth_rate=0 },  /* zubat */
    [42] = { .dex_id=42, .hp=75, .atk=80, .def=70, .spd=90, .spc=75, .type1=0x03, .type2=0x02, .catch_rate=90, .base_exp=171, .start_moves={141, 103, 44, 0}, .growth_rate=0 },  /* golbat */
    [43] = { .dex_id=43, .hp=45, .atk=50, .def=55, .spd=30, .spc=75, .type1=0x16, .type2=0x03, .catch_rate=255, .base_exp=78, .start_moves={71, 0, 0, 0}, .growth_rate=3 },  /* oddish */
    [44] = { .dex_id=44, .hp=60, .atk=65, .def=70, .spd=40, .spc=85, .type1=0x16, .type2=0x03, .catch_rate=120, .base_exp=132, .start_moves={71, 77, 78, 0}, .growth_rate=3 },  /* gloom */
    [45] = { .dex_id=45, .hp=75, .atk=80, .def=85, .spd=50, .spc=100, .type1=0x16, .type2=0x03, .catch_rate=45, .base_exp=184, .start_moves={78, 79, 51, 80}, .growth_rate=3 },  /* vileplume */
    [46] = { .dex_id=46, .hp=35, .atk=70, .def=55, .spd=25, .spc=55, .type1=0x07, .type2=0x16, .catch_rate=190, .base_exp=70, .start_moves={10, 0, 0, 0}, .growth_rate=0 },  /* paras */
    [47] = { .dex_id=47, .hp=60, .atk=95, .def=80, .spd=30, .spc=80, .type1=0x07, .type2=0x16, .catch_rate=75, .base_exp=128, .start_moves={10, 78, 141, 0}, .growth_rate=0 },  /* parasect */
    [48] = { .dex_id=48, .hp=60, .atk=55, .def=50, .spd=45, .spc=40, .type1=0x07, .type2=0x03, .catch_rate=190, .base_exp=75, .start_moves={33, 50, 0, 0}, .growth_rate=0 },  /* venonat */
    [49] = { .dex_id=49, .hp=70, .atk=65, .def=60, .spd=90, .spc=90, .type1=0x07, .type2=0x03, .catch_rate=75, .base_exp=138, .start_moves={33, 50, 77, 141}, .growth_rate=0 },  /* venomoth */
    [50] = { .dex_id=50, .hp=10, .atk=55, .def=25, .spd=95, .spc=45, .type1=0x04, .type2=0x04, .catch_rate=255, .base_exp=81, .start_moves={10, 0, 0, 0}, .growth_rate=0 },  /* diglett */
    [51] = { .dex_id=51, .hp=35, .atk=80, .def=50, .spd=120, .spc=70, .type1=0x04, .type2=0x04, .catch_rate=50, .base_exp=153, .start_moves={10, 45, 91, 0}, .growth_rate=0 },  /* dugtrio */
    [52] = { .dex_id=52, .hp=40, .atk=45, .def=35, .spd=90, .spc=40, .type1=0x00, .type2=0x00, .catch_rate=255, .base_exp=69, .start_moves={10, 45, 0, 0}, .growth_rate=0 },  /* meowth */
    [53] = { .dex_id=53, .hp=65, .atk=70, .def=60, .spd=115, .spc=65, .type1=0x00, .type2=0x00, .catch_rate=90, .base_exp=148, .start_moves={10, 45, 44, 103}, .growth_rate=0 },  /* persian */
    [54] = { .dex_id=54, .hp=50, .atk=52, .def=48, .spd=55, .spc=50, .type1=0x15, .type2=0x15, .catch_rate=190, .base_exp=80, .start_moves={10, 0, 0, 0}, .growth_rate=0 },  /* psyduck */
    [55] = { .dex_id=55, .hp=80, .atk=82, .def=78, .spd=85, .spc=80, .type1=0x15, .type2=0x15, .catch_rate=75, .base_exp=174, .start_moves={10, 39, 50, 0}, .growth_rate=0 },  /* golduck */
    [56] = { .dex_id=56, .hp=40, .atk=80, .def=35, .spd=70, .spc=35, .type1=0x01, .type2=0x01, .catch_rate=190, .base_exp=74, .start_moves={10, 43, 0, 0}, .growth_rate=0 },  /* mankey */
    [57] = { .dex_id=57, .hp=65, .atk=105, .def=60, .spd=95, .spc=60, .type1=0x01, .type2=0x01, .catch_rate=75, .base_exp=149, .start_moves={10, 43, 2, 154}, .growth_rate=0 },  /* primeape */
    [58] = { .dex_id=58, .hp=55, .atk=70, .def=45, .spd=60, .spc=50, .type1=0x14, .type2=0x14, .catch_rate=190, .base_exp=91, .start_moves={44, 46, 0, 0}, .growth_rate=5 },  /* growlithe */
    [59] = { .dex_id=59, .hp=90, .atk=110, .def=80, .spd=95, .spc=80, .type1=0x14, .type2=0x14, .catch_rate=75, .base_exp=213, .start_moves={46, 52, 43, 36}, .growth_rate=5 },  /* arcanine */
    [60] = { .dex_id=60, .hp=40, .atk=50, .def=40, .spd=90, .spc=40, .type1=0x15, .type2=0x15, .catch_rate=255, .base_exp=77, .start_moves={145, 0, 0, 0}, .growth_rate=3 },  /* poliwag */
    [61] = { .dex_id=61, .hp=65, .atk=65, .def=65, .spd=90, .spc=50, .type1=0x15, .type2=0x15, .catch_rate=120, .base_exp=131, .start_moves={145, 95, 55, 0}, .growth_rate=3 },  /* poliwhirl */
    [62] = { .dex_id=62, .hp=90, .atk=85, .def=95, .spd=70, .spc=70, .type1=0x15, .type2=0x01, .catch_rate=45, .base_exp=185, .start_moves={95, 55, 3, 34}, .growth_rate=3 },  /* poliwrath */
    [63] = { .dex_id=63, .hp=25, .atk=20, .def=15, .spd=90, .spc=105, .type1=0x18, .type2=0x18, .catch_rate=200, .base_exp=73, .start_moves={100, 0, 0, 0}, .growth_rate=3 },  /* abra */
    [64] = { .dex_id=64, .hp=40, .atk=35, .def=30, .spd=105, .spc=120, .type1=0x18, .type2=0x18, .catch_rate=100, .base_exp=145, .start_moves={100, 93, 50, 0}, .growth_rate=3 },  /* kadabra */
    [65] = { .dex_id=65, .hp=55, .atk=50, .def=45, .spd=120, .spc=135, .type1=0x18, .type2=0x18, .catch_rate=50, .base_exp=186, .start_moves={100, 93, 50, 0}, .growth_rate=3 },  /* alakazam */
    [66] = { .dex_id=66, .hp=70, .atk=80, .def=50, .spd=35, .spc=35, .type1=0x01, .type2=0x01, .catch_rate=180, .base_exp=88, .start_moves={2, 0, 0, 0}, .growth_rate=3 },  /* machop */
    [67] = { .dex_id=67, .hp=80, .atk=100, .def=70, .spd=45, .spc=50, .type1=0x01, .type2=0x01, .catch_rate=90, .base_exp=146, .start_moves={2, 67, 43, 0}, .growth_rate=3 },  /* machoke */
    [68] = { .dex_id=68, .hp=90, .atk=130, .def=80, .spd=55, .spc=65, .type1=0x01, .type2=0x01, .catch_rate=45, .base_exp=193, .start_moves={2, 67, 43, 0}, .growth_rate=3 },  /* machamp */
    [69] = { .dex_id=69, .hp=50, .atk=75, .def=35, .spd=40, .spc=70, .type1=0x16, .type2=0x03, .catch_rate=255, .base_exp=84, .start_moves={22, 74, 0, 0}, .growth_rate=3 },  /* bellsprout */
    [70] = { .dex_id=70, .hp=65, .atk=90, .def=50, .spd=55, .spc=85, .type1=0x16, .type2=0x03, .catch_rate=120, .base_exp=151, .start_moves={22, 74, 35, 0}, .growth_rate=3 },  /* weepinbell */
    [71] = { .dex_id=71, .hp=80, .atk=105, .def=65, .spd=70, .spc=100, .type1=0x16, .type2=0x03, .catch_rate=45, .base_exp=191, .start_moves={79, 78, 51, 75}, .growth_rate=3 },  /* victreebel */
    [72] = { .dex_id=72, .hp=40, .atk=40, .def=35, .spd=70, .spc=100, .type1=0x15, .type2=0x03, .catch_rate=190, .base_exp=105, .start_moves={51, 0, 0, 0}, .growth_rate=5 },  /* tentacool */
    [73] = { .dex_id=73, .hp=80, .atk=70, .def=65, .spd=100, .spc=120, .type1=0x15, .type2=0x03, .catch_rate=60, .base_exp=205, .start_moves={51, 48, 35, 0}, .growth_rate=5 },  /* tentacruel */
    [74] = { .dex_id=74, .hp=40, .atk=80, .def=100, .spd=20, .spc=30, .type1=0x05, .type2=0x04, .catch_rate=255, .base_exp=86, .start_moves={33, 0, 0, 0}, .growth_rate=3 },  /* geodude */
    [75] = { .dex_id=75, .hp=55, .atk=95, .def=115, .spd=35, .spc=45, .type1=0x05, .type2=0x04, .catch_rate=120, .base_exp=134, .start_moves={33, 111, 0, 0}, .growth_rate=3 },  /* graveler */
    [76] = { .dex_id=76, .hp=80, .atk=110, .def=130, .spd=45, .spc=55, .type1=0x05, .type2=0x04, .catch_rate=45, .base_exp=177, .start_moves={33, 111, 0, 0}, .growth_rate=3 },  /* golem */
    [77] = { .dex_id=77, .hp=50, .atk=85, .def=55, .spd=90, .spc=65, .type1=0x14, .type2=0x14, .catch_rate=190, .base_exp=152, .start_moves={52, 0, 0, 0}, .growth_rate=0 },  /* ponyta */
    [78] = { .dex_id=78, .hp=65, .atk=100, .def=70, .spd=105, .spc=80, .type1=0x14, .type2=0x14, .catch_rate=60, .base_exp=192, .start_moves={52, 39, 23, 45}, .growth_rate=0 },  /* rapidash */
    [79] = { .dex_id=79, .hp=90, .atk=65, .def=65, .spd=15, .spc=40, .type1=0x15, .type2=0x18, .catch_rate=190, .base_exp=99, .start_moves={93, 0, 0, 0}, .growth_rate=0 },  /* slowpoke */
    [80] = { .dex_id=80, .hp=95, .atk=75, .def=110, .spd=30, .spc=80, .type1=0x15, .type2=0x18, .catch_rate=75, .base_exp=164, .start_moves={93, 50, 29, 0}, .growth_rate=0 },  /* slowbro */
    [81] = { .dex_id=81, .hp=25, .atk=35, .def=70, .spd=45, .spc=95, .type1=0x17, .type2=0x17, .catch_rate=190, .base_exp=89, .start_moves={33, 0, 0, 0}, .growth_rate=0 },  /* magnemite */
    [82] = { .dex_id=82, .hp=50, .atk=60, .def=95, .spd=70, .spc=120, .type1=0x17, .type2=0x17, .catch_rate=60, .base_exp=161, .start_moves={33, 49, 84, 0}, .growth_rate=0 },  /* magneton */
    [83] = { .dex_id=83, .hp=52, .atk=65, .def=55, .spd=60, .spc=58, .type1=0x00, .type2=0x02, .catch_rate=45, .base_exp=94, .start_moves={64, 28, 0, 0}, .growth_rate=0 },  /* farfetchd */
    [84] = { .dex_id=84, .hp=35, .atk=85, .def=45, .spd=75, .spc=35, .type1=0x00, .type2=0x02, .catch_rate=190, .base_exp=96, .start_moves={64, 0, 0, 0}, .growth_rate=0 },  /* doduo */
    [85] = { .dex_id=85, .hp=60, .atk=110, .def=70, .spd=100, .spc=60, .type1=0x00, .type2=0x02, .catch_rate=45, .base_exp=158, .start_moves={64, 45, 31, 0}, .growth_rate=0 },  /* dodrio */
    [86] = { .dex_id=86, .hp=65, .atk=45, .def=55, .spd=45, .spc=70, .type1=0x15, .type2=0x15, .catch_rate=190, .base_exp=100, .start_moves={29, 0, 0, 0}, .growth_rate=0 },  /* seel */
    [87] = { .dex_id=87, .hp=90, .atk=70, .def=80, .spd=70, .spc=95, .type1=0x15, .type2=0x19, .catch_rate=75, .base_exp=176, .start_moves={29, 45, 62, 0}, .growth_rate=0 },  /* dewgong */
    [88] = { .dex_id=88, .hp=80, .atk=80, .def=50, .spd=25, .spc=40, .type1=0x03, .type2=0x03, .catch_rate=190, .base_exp=90, .start_moves={1, 50, 0, 0}, .growth_rate=0 },  /* grimer */
    [89] = { .dex_id=89, .hp=105, .atk=105, .def=75, .spd=50, .spc=65, .type1=0x03, .type2=0x03, .catch_rate=75, .base_exp=157, .start_moves={1, 50, 139, 0}, .growth_rate=0 },  /* muk */
    [90] = { .dex_id=90, .hp=30, .atk=65, .def=100, .spd=40, .spc=45, .type1=0x15, .type2=0x15, .catch_rate=190, .base_exp=97, .start_moves={33, 110, 0, 0}, .growth_rate=5 },  /* shellder */
    [91] = { .dex_id=91, .hp=50, .atk=95, .def=180, .spd=70, .spc=85, .type1=0x15, .type2=0x19, .catch_rate=60, .base_exp=203, .start_moves={110, 48, 128, 62}, .growth_rate=5 },  /* cloyster */
    [92] = { .dex_id=92, .hp=30, .atk=35, .def=30, .spd=80, .spc=100, .type1=0x08, .type2=0x03, .catch_rate=190, .base_exp=95, .start_moves={122, 109, 101, 0}, .growth_rate=3 },  /* gastly */
    [93] = { .dex_id=93, .hp=45, .atk=50, .def=45, .spd=95, .spc=115, .type1=0x08, .type2=0x03, .catch_rate=90, .base_exp=126, .start_moves={122, 109, 101, 0}, .growth_rate=3 },  /* haunter */
    [94] = { .dex_id=94, .hp=60, .atk=65, .def=60, .spd=110, .spc=130, .type1=0x08, .type2=0x03, .catch_rate=45, .base_exp=190, .start_moves={122, 109, 101, 0}, .growth_rate=3 },  /* gengar */
    [95] = { .dex_id=95, .hp=35, .atk=45, .def=160, .spd=70, .spc=30, .type1=0x05, .type2=0x04, .catch_rate=45, .base_exp=108, .start_moves={33, 103, 0, 0}, .growth_rate=0 },  /* onix */
    [96] = { .dex_id=96, .hp=60, .atk=48, .def=45, .spd=42, .spc=90, .type1=0x18, .type2=0x18, .catch_rate=190, .base_exp=102, .start_moves={1, 95, 0, 0}, .growth_rate=0 },  /* drowzee */
    [97] = { .dex_id=97, .hp=85, .atk=73, .def=70, .spd=67, .spc=115, .type1=0x18, .type2=0x18, .catch_rate=75, .base_exp=165, .start_moves={1, 95, 50, 93}, .growth_rate=0 },  /* hypno */
    [98] = { .dex_id=98, .hp=30, .atk=105, .def=90, .spd=50, .spc=25, .type1=0x15, .type2=0x15, .catch_rate=225, .base_exp=115, .start_moves={145, 43, 0, 0}, .growth_rate=0 },  /* krabby */
    [99] = { .dex_id=99, .hp=55, .atk=130, .def=115, .spd=75, .spc=50, .type1=0x15, .type2=0x15, .catch_rate=60, .base_exp=206, .start_moves={145, 43, 11, 0}, .growth_rate=0 },  /* kingler */
    [100] = { .dex_id=100, .hp=40, .atk=30, .def=50, .spd=100, .spc=55, .type1=0x17, .type2=0x17, .catch_rate=190, .base_exp=103, .start_moves={33, 103, 0, 0}, .growth_rate=0 },  /* voltorb */
    [101] = { .dex_id=101, .hp=60, .atk=50, .def=70, .spd=140, .spc=80, .type1=0x17, .type2=0x17, .catch_rate=60, .base_exp=150, .start_moves={33, 103, 49, 0}, .growth_rate=0 },  /* electrode */
    [102] = { .dex_id=102, .hp=60, .atk=40, .def=80, .spd=40, .spc=60, .type1=0x16, .type2=0x18, .catch_rate=90, .base_exp=98, .start_moves={140, 95, 0, 0}, .growth_rate=5 },  /* exeggcute */
    [103] = { .dex_id=103, .hp=95, .atk=95, .def=85, .spd=55, .spc=125, .type1=0x16, .type2=0x18, .catch_rate=45, .base_exp=212, .start_moves={140, 95, 0, 0}, .growth_rate=5 },  /* exeggutor */
    [104] = { .dex_id=104, .hp=50, .atk=50, .def=95, .spd=35, .spc=40, .type1=0x04, .type2=0x04, .catch_rate=190, .base_exp=87, .start_moves={125, 45, 0, 0}, .growth_rate=0 },  /* cubone */
    [105] = { .dex_id=105, .hp=60, .atk=80, .def=110, .spd=45, .spc=50, .type1=0x04, .type2=0x04, .catch_rate=75, .base_exp=124, .start_moves={125, 45, 43, 116}, .growth_rate=0 },  /* marowak */
    [106] = { .dex_id=106, .hp=50, .atk=120, .def=53, .spd=87, .spc=35, .type1=0x01, .type2=0x01, .catch_rate=45, .base_exp=139, .start_moves={24, 96, 0, 0}, .growth_rate=0 },  /* hitmonlee */
    [107] = { .dex_id=107, .hp=50, .atk=105, .def=79, .spd=76, .spc=35, .type1=0x01, .type2=0x01, .catch_rate=45, .base_exp=140, .start_moves={4, 97, 0, 0}, .growth_rate=0 },  /* hitmonchan */
    [108] = { .dex_id=108, .hp=90, .atk=55, .def=75, .spd=30, .spc=60, .type1=0x00, .type2=0x00, .catch_rate=45, .base_exp=127, .start_moves={35, 48, 0, 0}, .growth_rate=0 },  /* lickitung */
    [109] = { .dex_id=109, .hp=40, .atk=65, .def=95, .spd=35, .spc=60, .type1=0x03, .type2=0x03, .catch_rate=190, .base_exp=114, .start_moves={33, 123, 0, 0}, .growth_rate=0 },  /* koffing */
    [110] = { .dex_id=110, .hp=65, .atk=90, .def=120, .spd=60, .spc=85, .type1=0x03, .type2=0x03, .catch_rate=60, .base_exp=173, .start_moves={33, 123, 124, 0}, .growth_rate=0 },  /* weezing */
    [111] = { .dex_id=111, .hp=80, .atk=85, .def=95, .spd=25, .spc=30, .type1=0x04, .type2=0x05, .catch_rate=120, .base_exp=135, .start_moves={30, 0, 0, 0}, .growth_rate=5 },  /* rhyhorn */
    [112] = { .dex_id=112, .hp=105, .atk=130, .def=120, .spd=40, .spc=45, .type1=0x04, .type2=0x05, .catch_rate=60, .base_exp=204, .start_moves={30, 23, 39, 31}, .growth_rate=5 },  /* rhydon */
    [113] = { .dex_id=113, .hp=250, .atk=5, .def=5, .spd=50, .spc=105, .type1=0x00, .type2=0x00, .catch_rate=30, .base_exp=255, .start_moves={1, 3, 0, 0}, .growth_rate=4 },  /* chansey */
    [114] = { .dex_id=114, .hp=65, .atk=55, .def=115, .spd=60, .spc=100, .type1=0x16, .type2=0x16, .catch_rate=45, .base_exp=166, .start_moves={132, 20, 0, 0}, .growth_rate=0 },  /* tangela */
    [115] = { .dex_id=115, .hp=105, .atk=95, .def=80, .spd=90, .spc=40, .type1=0x00, .type2=0x00, .catch_rate=45, .base_exp=175, .start_moves={4, 99, 0, 0}, .growth_rate=0 },  /* kangaskhan */
    [116] = { .dex_id=116, .hp=30, .atk=40, .def=70, .spd=60, .spc=70, .type1=0x15, .type2=0x15, .catch_rate=225, .base_exp=83, .start_moves={145, 0, 0, 0}, .growth_rate=0 },  /* horsea */
    [117] = { .dex_id=117, .hp=55, .atk=65, .def=95, .spd=85, .spc=95, .type1=0x15, .type2=0x15, .catch_rate=75, .base_exp=155, .start_moves={145, 108, 0, 0}, .growth_rate=0 },  /* seadra */
    [118] = { .dex_id=118, .hp=45, .atk=67, .def=60, .spd=63, .spc=50, .type1=0x15, .type2=0x15, .catch_rate=225, .base_exp=111, .start_moves={64, 39, 0, 0}, .growth_rate=0 },  /* goldeen */
    [119] = { .dex_id=119, .hp=80, .atk=92, .def=65, .spd=68, .spc=80, .type1=0x15, .type2=0x15, .catch_rate=60, .base_exp=170, .start_moves={64, 39, 48, 0}, .growth_rate=0 },  /* seaking */
    [120] = { .dex_id=120, .hp=30, .atk=45, .def=55, .spd=85, .spc=70, .type1=0x15, .type2=0x15, .catch_rate=225, .base_exp=106, .start_moves={33, 0, 0, 0}, .growth_rate=5 },  /* staryu */
    [121] = { .dex_id=121, .hp=60, .atk=75, .def=85, .spd=115, .spc=100, .type1=0x15, .type2=0x18, .catch_rate=60, .base_exp=207, .start_moves={33, 55, 106, 0}, .growth_rate=5 },  /* starmie */
    [122] = { .dex_id=122, .hp=40, .atk=45, .def=65, .spd=90, .spc=100, .type1=0x18, .type2=0x18, .catch_rate=45, .base_exp=136, .start_moves={93, 112, 0, 0}, .growth_rate=0 },  /* mrmime */
    [123] = { .dex_id=123, .hp=70, .atk=110, .def=80, .spd=105, .spc=55, .type1=0x07, .type2=0x02, .catch_rate=45, .base_exp=187, .start_moves={98, 0, 0, 0}, .growth_rate=0 },  /* scyther */
    [124] = { .dex_id=124, .hp=65, .atk=50, .def=35, .spd=95, .spc=95, .type1=0x19, .type2=0x18, .catch_rate=45, .base_exp=137, .start_moves={1, 142, 0, 0}, .growth_rate=0 },  /* jynx */
    [125] = { .dex_id=125, .hp=65, .atk=83, .def=57, .spd=105, .spc=85, .type1=0x17, .type2=0x17, .catch_rate=45, .base_exp=156, .start_moves={98, 43, 0, 0}, .growth_rate=0 },  /* electabuzz */
    [126] = { .dex_id=126, .hp=65, .atk=95, .def=57, .spd=93, .spc=85, .type1=0x14, .type2=0x14, .catch_rate=45, .base_exp=167, .start_moves={52, 0, 0, 0}, .growth_rate=0 },  /* magmar */
    [127] = { .dex_id=127, .hp=65, .atk=125, .def=100, .spd=85, .spc=55, .type1=0x07, .type2=0x07, .catch_rate=45, .base_exp=200, .start_moves={11, 0, 0, 0}, .growth_rate=5 },  /* pinsir */
    [128] = { .dex_id=128, .hp=75, .atk=100, .def=95, .spd=110, .spc=70, .type1=0x00, .type2=0x00, .catch_rate=45, .base_exp=211, .start_moves={33, 0, 0, 0}, .growth_rate=5 },  /* tauros */
    [129] = { .dex_id=129, .hp=20, .atk=10, .def=55, .spd=80, .spc=20, .type1=0x15, .type2=0x15, .catch_rate=255, .base_exp=20, .start_moves={150, 0, 0, 0}, .growth_rate=5 },  /* magikarp */
    [130] = { .dex_id=130, .hp=95, .atk=125, .def=79, .spd=81, .spc=100, .type1=0x15, .type2=0x02, .catch_rate=45, .base_exp=214, .start_moves={44, 82, 43, 56}, .growth_rate=5 },  /* gyarados */
    [131] = { .dex_id=131, .hp=130, .atk=85, .def=80, .spd=60, .spc=95, .type1=0x15, .type2=0x19, .catch_rate=45, .base_exp=219, .start_moves={55, 45, 0, 0}, .growth_rate=5 },  /* lapras */
    [132] = { .dex_id=132, .hp=48, .atk=48, .def=48, .spd=48, .spc=48, .type1=0x00, .type2=0x00, .catch_rate=35, .base_exp=61, .start_moves={144, 0, 0, 0}, .growth_rate=0 },  /* ditto */
    [133] = { .dex_id=133, .hp=55, .atk=55, .def=50, .spd=55, .spc=65, .type1=0x00, .type2=0x00, .catch_rate=45, .base_exp=92, .start_moves={33, 28, 0, 0}, .growth_rate=0 },  /* eevee */
    [134] = { .dex_id=134, .hp=130, .atk=65, .def=60, .spd=65, .spc=110, .type1=0x15, .type2=0x15, .catch_rate=45, .base_exp=196, .start_moves={33, 28, 98, 55}, .growth_rate=0 },  /* vaporeon */
    [135] = { .dex_id=135, .hp=65, .atk=65, .def=60, .spd=130, .spc=110, .type1=0x17, .type2=0x17, .catch_rate=45, .base_exp=197, .start_moves={33, 28, 98, 84}, .growth_rate=0 },  /* jolteon */
    [136] = { .dex_id=136, .hp=65, .atk=130, .def=60, .spd=65, .spc=110, .type1=0x14, .type2=0x14, .catch_rate=45, .base_exp=198, .start_moves={33, 28, 98, 52}, .growth_rate=0 },  /* flareon */
    [137] = { .dex_id=137, .hp=65, .atk=60, .def=70, .spd=40, .spc=75, .type1=0x00, .type2=0x00, .catch_rate=45, .base_exp=130, .start_moves={33, 159, 160, 0}, .growth_rate=0 },  /* porygon */
    [138] = { .dex_id=138, .hp=35, .atk=40, .def=100, .spd=35, .spc=90, .type1=0x05, .type2=0x15, .catch_rate=45, .base_exp=120, .start_moves={55, 110, 0, 0}, .growth_rate=0 },  /* omanyte */
    [139] = { .dex_id=139, .hp=70, .atk=60, .def=125, .spd=55, .spc=115, .type1=0x05, .type2=0x15, .catch_rate=45, .base_exp=199, .start_moves={55, 110, 30, 0}, .growth_rate=0 },  /* omastar */
    [140] = { .dex_id=140, .hp=30, .atk=80, .def=90, .spd=55, .spc=45, .type1=0x05, .type2=0x15, .catch_rate=45, .base_exp=119, .start_moves={10, 106, 0, 0}, .growth_rate=0 },  /* kabuto */
    [141] = { .dex_id=141, .hp=60, .atk=115, .def=105, .spd=80, .spc=70, .type1=0x05, .type2=0x15, .catch_rate=45, .base_exp=201, .start_moves={10, 106, 71, 0}, .growth_rate=0 },  /* kabutops */
    [142] = { .dex_id=142, .hp=80, .atk=105, .def=65, .spd=130, .spc=60, .type1=0x05, .type2=0x02, .catch_rate=45, .base_exp=202, .start_moves={17, 97, 0, 0}, .growth_rate=5 },  /* aerodactyl */
    [143] = { .dex_id=143, .hp=160, .atk=110, .def=65, .spd=30, .spc=65, .type1=0x00, .type2=0x00, .catch_rate=25, .base_exp=154, .start_moves={29, 133, 156, 0}, .growth_rate=5 },  /* snorlax */
    [144] = { .dex_id=144, .hp=90, .atk=85, .def=100, .spd=85, .spc=125, .type1=0x19, .type2=0x02, .catch_rate=3, .base_exp=215, .start_moves={64, 58, 0, 0}, .growth_rate=5 },  /* articuno */
    [145] = { .dex_id=145, .hp=90, .atk=90, .def=85, .spd=100, .spc=125, .type1=0x17, .type2=0x02, .catch_rate=3, .base_exp=216, .start_moves={84, 65, 0, 0}, .growth_rate=5 },  /* zapdos */
    [146] = { .dex_id=146, .hp=90, .atk=100, .def=90, .spd=90, .spc=125, .type1=0x14, .type2=0x02, .catch_rate=3, .base_exp=217, .start_moves={64, 83, 0, 0}, .growth_rate=5 },  /* moltres */
    [147] = { .dex_id=147, .hp=41, .atk=64, .def=45, .spd=50, .spc=50, .type1=0x1A, .type2=0x1A, .catch_rate=45, .base_exp=67, .start_moves={35, 43, 0, 0}, .growth_rate=5 },  /* dratini */
    [148] = { .dex_id=148, .hp=61, .atk=84, .def=65, .spd=70, .spc=70, .type1=0x1A, .type2=0x1A, .catch_rate=45, .base_exp=144, .start_moves={35, 43, 86, 0}, .growth_rate=5 },  /* dragonair */
    [149] = { .dex_id=149, .hp=91, .atk=134, .def=95, .spd=80, .spc=100, .type1=0x1A, .type2=0x02, .catch_rate=45, .base_exp=218, .start_moves={35, 43, 86, 97}, .growth_rate=5 },  /* dragonite */
    [150] = { .dex_id=150, .hp=106, .atk=110, .def=90, .spd=130, .spc=154, .type1=0x18, .type2=0x18, .catch_rate=3, .base_exp=220, .start_moves={93, 50, 129, 94}, .growth_rate=5 },  /* mewtwo */
};

const uint8_t gSpeciesToDex[256] = {
    /* 0x00 */   0, 112, 115,  32,  35,  21, 100,  34,  80,   2, 103, 108, 102,  88,  94,  29,
    /* 0x10 */  31, 104, 111, 131,  59, 151, 130,  90,  72,  92, 123, 120,   9, 127, 114,   0,
    /* 0x20 */   0,  58,  95,  22,  16,  79,  64,  75, 113,  67, 122, 106, 107,  24,  47,  54,
    /* 0x30 */  96,  76,   0, 126,   0, 125,  82, 109,   0,  56,  86,  50, 128,   0,   0,   0,
    /* 0x40 */  83,  48, 149,   0,   0,   0,  84,  60, 124, 146, 144, 145, 132,  52,  98,   0,
    /* 0x50 */   0,   0,  37,  38,  25,  26,   0,   0, 147, 148, 140, 141, 116, 117,   0,   0,
    /* 0x60 */  27,  28, 138, 139,  39,  40, 133, 136, 135, 134,  66,  41,  23,  46,  61,  62,
    /* 0x70 */  13,  14,  15,   0,  85,  57,  51,  49,  87,   0,   0,  10,  11,  12,  68,   0,
    /* 0x80 */  55,  97,  42, 150, 143, 129,   0,   0,  89,   0,  99,  91,   0, 101,  36, 110,
    /* 0x90 */  53, 105,   0,  93,  63,  65,  17,  18, 121,   1,   3,  73,   0, 118, 119,   0,
    /* 0xA0 */   0,   0,   0,  77,  78,  19,  20,  33,  30,  74, 137, 142,   0,  81,   0,   0,
    /* 0xB0 */   4,   7,   5,   8,   6,   0,   0,   0,   0,  43,  44,  45,  69,  70,  71,   0,
    /* 0xC0 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 0xD0 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 0xE0 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    /* 0xF0 */   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
};
