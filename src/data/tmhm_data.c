/* tmhm_data.c — TM/HM to move ID mapping table.
 * Mirrors TechnicalMachines in data/moves/tmhm_moves.asm.
 * Indices 0-49 = TM01-TM50; 50-54 = HM01-HM05. */
#include "tmhm_data.h"

const uint8_t gTMHMMoves[NUM_TM_HM] = {
    /* TM01 */ 0x05, /* MEGA_PUNCH    */
    /* TM02 */ 0x0D, /* RAZOR_WIND    */
    /* TM03 */ 0x0E, /* SWORDS_DANCE  */
    /* TM04 */ 0x12, /* WHIRLWIND     */
    /* TM05 */ 0x19, /* MEGA_KICK     */
    /* TM06 */ 0x5C, /* TOXIC         */
    /* TM07 */ 0x20, /* HORN_DRILL    */
    /* TM08 */ 0x22, /* BODY_SLAM     */
    /* TM09 */ 0x24, /* TAKE_DOWN     */
    /* TM10 */ 0x26, /* DOUBLE_EDGE   */
    /* TM11 */ 0x3D, /* BUBBLEBEAM    */
    /* TM12 */ 0x37, /* WATER_GUN     */
    /* TM13 */ 0x3A, /* ICE_BEAM      */
    /* TM14 */ 0x3B, /* BLIZZARD      */
    /* TM15 */ 0x3F, /* HYPER_BEAM    */
    /* TM16 */ 0x06, /* PAY_DAY       */
    /* TM17 */ 0x42, /* SUBMISSION    */
    /* TM18 */ 0x44, /* COUNTER       */
    /* TM19 */ 0x45, /* SEISMIC_TOSS  */
    /* TM20 */ 0x63, /* RAGE          */
    /* TM21 */ 0x48, /* MEGA_DRAIN    */
    /* TM22 */ 0x4C, /* SOLARBEAM     */
    /* TM23 */ 0x52, /* DRAGON_RAGE   */
    /* TM24 */ 0x55, /* THUNDERBOLT   */
    /* TM25 */ 0x57, /* THUNDER       */
    /* TM26 */ 0x59, /* EARTHQUAKE    */
    /* TM27 */ 0x5A, /* FISSURE       */
    /* TM28 */ 0x5B, /* DIG           */
    /* TM29 */ 0x5E, /* PSYCHIC       */
    /* TM30 */ 0x64, /* TELEPORT      */
    /* TM31 */ 0x66, /* MIMIC         */
    /* TM32 */ 0x68, /* DOUBLE_TEAM   */
    /* TM33 */ 0x73, /* REFLECT       */
    /* TM34 */ 0x75, /* BIDE          */
    /* TM35 */ 0x76, /* METRONOME     */
    /* TM36 */ 0x78, /* SELFDESTRUCT  */
    /* TM37 */ 0x79, /* EGG_BOMB      */
    /* TM38 */ 0x7E, /* FIRE_BLAST    */
    /* TM39 */ 0x81, /* SWIFT         */
    /* TM40 */ 0x82, /* SKULL_BASH    */
    /* TM41 */ 0x87, /* SOFTBOILED    */
    /* TM42 */ 0x8A, /* DREAM_EATER   */
    /* TM43 */ 0x8F, /* SKY_ATTACK    */
    /* TM44 */ 0x9C, /* REST          */
    /* TM45 */ 0x56, /* THUNDER_WAVE  */
    /* TM46 */ 0x95, /* PSYWAVE       */
    /* TM47 */ 0x99, /* EXPLOSION     */
    /* TM48 */ 0x9D, /* ROCK_SLIDE    */
    /* TM49 */ 0xA1, /* TRI_ATTACK    */
    /* TM50 */ 0xA4, /* SUBSTITUTE    */
    /* HM01 */ 0x0F, /* CUT           */
    /* HM02 */ 0x13, /* FLY           */
    /* HM03 */ 0x39, /* SURF          */
    /* HM04 */ 0x46, /* STRENGTH      */
    /* HM05 */ 0x94, /* FLASH         */
};
