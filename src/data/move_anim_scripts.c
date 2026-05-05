#include "move_anim_scripts.h"

static const uint8_t sMoveAnimScript_000[] = {  }; /* PoundAnim */
static const uint8_t sMoveAnimScript_001[] = { 0x08, 0x01, 0x03, 0xFF }; /* KarateChopAnim */
static const uint8_t sMoveAnimScript_002[] = { 0x05, 0x02, 0x01, 0x05, 0x02, 0x01, 0xFF }; /* DoubleSlapAnim */
static const uint8_t sMoveAnimScript_003[] = { 0x04, 0x03, 0x02, 0x04, 0x03, 0x02, 0xFF }; /* CometPunchAnim */
static const uint8_t sMoveAnimScript_004[] = { 0x46, 0x04, 0x04, 0xFF }; /* MegaPunchAnim */
static const uint8_t sMoveAnimScript_005[] = { 0x08, 0x00, 0x01, 0x04, 0x05, 0x52, 0xFF }; /* PayDayAnim */
static const uint8_t sMoveAnimScript_006[] = { 0x06, 0x06, 0x02, 0x46, 0xFF, 0x11, 0xFF }; /* FirePunchAnim */
static const uint8_t sMoveAnimScript_007[] = { 0x06, 0x07, 0x02, 0x10, 0xFF, 0x2F, 0xFF }; /* IcePunchAnim */
static const uint8_t sMoveAnimScript_008[] = { 0x06, 0x08, 0x02, 0xFD, 0xFF, 0x46, 0xFF, 0x2B, 0xFC, 0xFF, 0xFF }; /* ThunderPunchAnim */
static const uint8_t sMoveAnimScript_009[] = { 0x06, 0x09, 0x0F, 0xFF }; /* ScratchAnim */
static const uint8_t sMoveAnimScript_010[] = { 0x08, 0x0A, 0x2A, 0xFF }; /* VicegripAnim */
static const uint8_t sMoveAnimScript_011[] = { 0x06, 0x0B, 0x2A, 0xFF }; /* GuillotineAnim */
static const uint8_t sMoveAnimScript_012[] = { 0x04, 0x0C, 0x16, 0xFF }; /* RazorWindAnim */
static const uint8_t sMoveAnimScript_013[] = { 0x46, 0x0D, 0x18, 0x46, 0x0D, 0x18, 0x46, 0x0D, 0x18, 0xFF }; /* SwordsDanceAnim */
static const uint8_t sMoveAnimScript_014[] = { 0xFE, 0x0E, 0x04, 0xFF, 0x16, 0xFF }; /* CutAnim */
static const uint8_t sMoveAnimScript_015[] = { 0x46, 0x0F, 0x10, 0x06, 0xFF, 0x02, 0xFF }; /* GustAnim */
static const uint8_t sMoveAnimScript_016[] = { 0x46, 0x10, 0x04, 0xFF }; /* WingAttackAnim */
static const uint8_t sMoveAnimScript_017[] = { 0x46, 0x11, 0x10, 0xDB, 0xFF, 0xFF }; /* WhirlwindAnim */
static const uint8_t sMoveAnimScript_018[] = { 0x46, 0x12, 0x04, 0xDD, 0xFF, 0xFF }; /* FlyAnim */
static const uint8_t sMoveAnimScript_019[] = { 0x04, 0x13, 0x23, 0x04, 0x13, 0x23, 0xFF }; /* BindAnim */
static const uint8_t sMoveAnimScript_020[] = { 0x06, 0x14, 0x02, 0xFF }; /* SlamAnim */
static const uint8_t sMoveAnimScript_021[] = { 0x01, 0x15, 0x16, 0x08, 0xFF, 0x01, 0xFF }; /* VineWhipAnim */
static const uint8_t sMoveAnimScript_022[] = { 0x48, 0x16, 0x05, 0xFF }; /* StompAnim */
static const uint8_t sMoveAnimScript_023[] = { 0x08, 0x17, 0x01, 0x08, 0x17, 0x01, 0xFF }; /* DoubleKickAnim */
static const uint8_t sMoveAnimScript_024[] = { 0x46, 0x18, 0x04, 0xFF }; /* MegaKickAnim */
static const uint8_t sMoveAnimScript_025[] = { 0x46, 0x19, 0x04, 0xFF }; /* JumpKickAnim */
static const uint8_t sMoveAnimScript_026[] = { 0xFE, 0x1A, 0x46, 0xFF, 0x04, 0xFF }; /* RollingKickAnim */
static const uint8_t sMoveAnimScript_027[] = { 0x46, 0x1B, 0x28, 0xFF }; /* SandAttackAnim */
static const uint8_t sMoveAnimScript_028[] = { 0x46, 0x1C, 0x05, 0xFF }; /* HeadbuttAnim */
static const uint8_t sMoveAnimScript_029[] = { 0x06, 0x1D, 0x45, 0x46, 0xFF, 0x05, 0xFF }; /* HornAttackAnim */
static const uint8_t sMoveAnimScript_030[] = { 0x02, 0x1E, 0x46, 0x02, 0xFF, 0x46, 0xFF }; /* FuryAttackAnim */
static const uint8_t sMoveAnimScript_031[] = { 0x42, 0x1F, 0x05, 0x42, 0xFF, 0x05, 0x42, 0xFF, 0x05, 0x42, 0xFF, 0x05, 0x42, 0xFF, 0x05, 0xFF }; /* HornDrillAnim */
static const uint8_t sMoveAnimScript_032[] = { 0xF2, 0x48, 0xF1, 0xFF, 0xFF }; /* TackleAnim */
static const uint8_t sMoveAnimScript_033[] = { 0xF2, 0x48, 0xFE, 0xFF, 0xFE, 0xFF, 0xF1, 0xFF, 0xFF }; /* BodySlamAnim */
static const uint8_t sMoveAnimScript_034[] = { 0x04, 0x22, 0x23, 0x04, 0x22, 0x23, 0x04, 0x22, 0x23, 0xFF }; /* WrapAnim */
static const uint8_t sMoveAnimScript_035[] = { 0xF2, 0x48, 0xFE, 0x23, 0xF1, 0xFF, 0xFF }; /* TakeDownAnim */
static const uint8_t sMoveAnimScript_036[] = { 0x46, 0x24, 0x04, 0xFF }; /* ThrashAnim */
static const uint8_t sMoveAnimScript_037[] = { 0xF0, 0x48, 0x06, 0xFF, 0x2D, 0xFC, 0xFF, 0xF2, 0xFF, 0xFE, 0x25, 0xF1, 0xFF, 0xFF }; /* DoubleEdgeAnim */
static const uint8_t sMoveAnimScript_038[] = { 0xF2, 0x84, 0xE1, 0xFF, 0xF1, 0x84, 0xE1, 0xFF, 0xF2, 0x84, 0xE1, 0xFF, 0xF1, 0x84, 0xFF }; /* TailWhipAnim */
static const uint8_t sMoveAnimScript_039[] = { 0x06, 0x27, 0x00, 0xFF }; /* PoisonStingAnim */
static const uint8_t sMoveAnimScript_040[] = { 0x05, 0x28, 0x01, 0x05, 0x28, 0x01, 0xFF }; /* TwineedleAnim */
static const uint8_t sMoveAnimScript_041[] = { 0x03, 0x29, 0x01, 0xFF }; /* PinMissileAnim */
static const uint8_t sMoveAnimScript_042[] = { 0xFD, 0x48, 0xFE, 0x2A, 0xFE, 0x2A, 0xFC, 0xFF, 0xFF }; /* LeerAnim */
static const uint8_t sMoveAnimScript_043[] = { 0x08, 0x2B, 0x02, 0xFF }; /* BiteAnim */
static const uint8_t sMoveAnimScript_044[] = { 0x46, 0x2C, 0x12, 0xFF }; /* GrowlAnim */
static const uint8_t sMoveAnimScript_045[] = { 0x46, 0x2D, 0x15, 0x46, 0x2D, 0x15, 0x46, 0x2D, 0x15, 0xFF }; /* RoarAnim */
static const uint8_t sMoveAnimScript_046[] = { 0x46, 0x2E, 0x12, 0x50, 0xFF, 0x40, 0x50, 0xFF, 0x40, 0xFF }; /* SingAnim */
static const uint8_t sMoveAnimScript_047[] = { 0x06, 0x2F, 0x31, 0xFF }; /* SupersonicAnim */
static const uint8_t sMoveAnimScript_048[] = { 0x46, 0x2D, 0x15, 0x46, 0x2D, 0x15, 0x46, 0x0F, 0x10, 0x46, 0xFF, 0x05, 0xFF }; /* SonicBoomAnim */
static const uint8_t sMoveAnimScript_049[] = { 0xFD, 0x48, 0xFE, 0x2A, 0xFE, 0x2A, 0xFC, 0xFF, 0xFF }; /* DisableAnim */
static const uint8_t sMoveAnimScript_050[] = { 0x46, 0x32, 0x13, 0x46, 0x32, 0x14, 0xFF }; /* AcidAnim */
static const uint8_t sMoveAnimScript_051[] = { 0x46, 0x33, 0x11, 0xFF }; /* EmberAnim */
static const uint8_t sMoveAnimScript_052[] = { 0x46, 0x34, 0x1F, 0x46, 0x34, 0x0C, 0x46, 0x34, 0x0D, 0xFF }; /* FlamethrowerAnim */
static const uint8_t sMoveAnimScript_053[] = { 0xF0, 0xFF, 0xFA, 0x38, 0xFC, 0xFF, 0xFF }; /* MistAnim */
static const uint8_t sMoveAnimScript_054[] = { 0x06, 0x36, 0x2C, 0xFF }; /* WaterGunAnim */
static const uint8_t sMoveAnimScript_055[] = { 0x06, 0x37, 0x1A, 0x06, 0x37, 0x1A, 0xFF }; /* HydroPumpAnim */
static const uint8_t sMoveAnimScript_056[] = { 0xFA, 0x38, 0x06, 0x37, 0x1A, 0xFF }; /* SurfAnim */
static const uint8_t sMoveAnimScript_057[] = { 0x03, 0x39, 0x2E, 0x10, 0xFF, 0x2F, 0xFF }; /* IceBeamAnim */
static const uint8_t sMoveAnimScript_058[] = { 0x04, 0x3A, 0x38, 0x04, 0x37, 0x38, 0xFF }; /* BlizzardAnim */
static const uint8_t sMoveAnimScript_059[] = { 0x03, 0x3B, 0x2E, 0xF8, 0xFF, 0xFF }; /* PsyBeamAnim */
static const uint8_t sMoveAnimScript_060[] = { 0x12, 0x3C, 0x35, 0xFF }; /* BubbleBeamAnim */
static const uint8_t sMoveAnimScript_061[] = { 0x03, 0x3D, 0x2E, 0xE1, 0xFF, 0xE1, 0xFF, 0xFF }; /* AuroraBeamAnim */
static const uint8_t sMoveAnimScript_062[] = { 0xFD, 0x48, 0xE2, 0xFF, 0x02, 0x3E, 0x2E, 0xFE, 0xFF, 0xFE, 0xFF, 0x46, 0x04, 0x04, 0xFC, 0xFF, 0xFF }; /* HyperBeamAnim */
static const uint8_t sMoveAnimScript_063[] = { 0x08, 0x3F, 0x01, 0xFF }; /* PeckAnim */
static const uint8_t sMoveAnimScript_064[] = { 0x46, 0x40, 0x04, 0xFF }; /* DrillPeckAnim */
static const uint8_t sMoveAnimScript_065[] = { 0xF4, 0x41, 0x06, 0xFF, 0x01, 0xDD, 0xFF, 0xFF }; /* SubmissionAnim */
static const uint8_t sMoveAnimScript_066[] = { 0xF4, 0x42, 0x46, 0xFF, 0x04, 0xDD, 0xFF, 0xFF }; /* LowKickAnim */
static const uint8_t sMoveAnimScript_067[] = { 0xF4, 0x43, 0x46, 0xFF, 0x04, 0xDD, 0xFF, 0xFF }; /* CounterAnim */
static const uint8_t sMoveAnimScript_068[] = { 0xDE, 0xFF, 0x41, 0x8B, 0x4E, 0xDF, 0xFF, 0xF4, 0xFF, 0x42, 0x44, 0x4F, 0xE1, 0xFF, 0xE1, 0xFF, 0xDD, 0xFF, 0x41, 0x44, 0x50, 0xDC, 0xFF, 0xFB, 0xFF, 0xFF }; /* SeismicTossAnim */
static const uint8_t sMoveAnimScript_069[] = { 0xF2, 0x48, 0xF1, 0xFF, 0x46, 0x06, 0x04, 0xFF }; /* StrengthAnim */
static const uint8_t sMoveAnimScript_070[] = { 0xF0, 0x46, 0x06, 0xFF, 0x21, 0x06, 0xFF, 0x22, 0xFC, 0xFF, 0xFF }; /* AbsorbAnim */
static const uint8_t sMoveAnimScript_071[] = { 0xF0, 0x47, 0xFE, 0xFF, 0x06, 0xFF, 0x21, 0x06, 0xFF, 0x22, 0xFE, 0xFF, 0xFC, 0xFF, 0xFF }; /* MegaDrainAnim */
static const uint8_t sMoveAnimScript_072[] = { 0x46, 0x48, 0x1B, 0x55, 0x4D, 0x1C, 0xFF }; /* LeechSeedAnim */
static const uint8_t sMoveAnimScript_073[] = { 0xF0, 0x49, 0xE2, 0xFF, 0xFC, 0xFF, 0xFF }; /* GrowthAnim */
static const uint8_t sMoveAnimScript_074[] = { 0xE7, 0x4A, 0x41, 0x80, 0x44, 0x01, 0x0C, 0x16, 0xFF }; /* RazorLeafAnim */
static const uint8_t sMoveAnimScript_075[] = { 0x06, 0x4B, 0x2E, 0x06, 0xFF, 0x01, 0xFF }; /* SolarBeamAnim */
static const uint8_t sMoveAnimScript_076[] = { 0x06, 0x4C, 0x36, 0xFF }; /* PoisonPowderAnim */
static const uint8_t sMoveAnimScript_077[] = { 0x06, 0x4D, 0x36, 0xFF }; /* StunSporeAnim */
static const uint8_t sMoveAnimScript_078[] = { 0x06, 0x4E, 0x36, 0xFF }; /* SleepPowderAnim */
static const uint8_t sMoveAnimScript_079[] = { 0xF0, 0x4F, 0xE6, 0xFF, 0xFC, 0xFF, 0xFF }; /* PetalDanceAnim */
static const uint8_t sMoveAnimScript_080[] = { 0x08, 0x50, 0x37, 0xFF }; /* StringShotAnim */
static const uint8_t sMoveAnimScript_081[] = { 0x46, 0x51, 0x1F, 0x46, 0xFF, 0x0C, 0x46, 0xFF, 0x0D, 0x46, 0xFF, 0x0E, 0xFF }; /* DragonRageAnim */
static const uint8_t sMoveAnimScript_082[] = { 0x46, 0x52, 0x0C, 0x46, 0xFF, 0x0D, 0x46, 0xFF, 0x0E, 0xFF }; /* FireSpinAnim */
static const uint8_t sMoveAnimScript_083[] = { 0x42, 0x53, 0x29, 0xFF }; /* ThunderShockAnim */
static const uint8_t sMoveAnimScript_084[] = { 0x41, 0x54, 0x29, 0x41, 0x54, 0x29, 0xFF }; /* ThunderBoltAnim */
static const uint8_t sMoveAnimScript_085[] = { 0x42, 0x55, 0x29, 0x02, 0xFF, 0x23, 0x04, 0xFF, 0x23, 0xFF }; /* ThunderWaveAnim */
static const uint8_t sMoveAnimScript_086[] = { 0xFD, 0x56, 0xFE, 0xFF, 0x46, 0xFF, 0x2B, 0xFE, 0xFF, 0x42, 0x54, 0x29, 0xFC, 0xFF, 0xFF }; /* ThunderAnim */
static const uint8_t sMoveAnimScript_087[] = { 0x04, 0x57, 0x30, 0xFF }; /* RockThrowAnim */
static const uint8_t sMoveAnimScript_088[] = { 0xFB, 0x58, 0xFB, 0x58, 0xFF }; /* EarthquakeAnim */
static const uint8_t sMoveAnimScript_089[] = { 0xFE, 0x59, 0xFB, 0xFF, 0xFE, 0x59, 0xFB, 0xFF, 0xFF }; /* FissureAnim */
static const uint8_t sMoveAnimScript_090[] = { 0x46, 0x5A, 0x04, 0xF7, 0xFF, 0xFF }; /* DigAnim */
static const uint8_t sMoveAnimScript_091[] = { 0xFA, 0x38, 0x46, 0x5B, 0x14, 0xFF }; /* ToxicAnim */
static const uint8_t sMoveAnimScript_092[] = { 0xF8, 0x5C, 0xFF }; /* ConfusionAnim */
static const uint8_t sMoveAnimScript_093[] = { 0xF8, 0x5D, 0xD8, 0xFF, 0xFF }; /* PsychicAnim */
static const uint8_t sMoveAnimScript_094[] = { 0xF8, 0x5E, 0xFF }; /* HypnosisAnim */
static const uint8_t sMoveAnimScript_095[] = { 0xF0, 0x5F, 0x46, 0xFF, 0x43, 0xFE, 0xFF, 0xFC, 0xFF, 0xFF }; /* MeditateAnim */
static const uint8_t sMoveAnimScript_096[] = { 0xF0, 0x60, 0xFC, 0xFF, 0xFF }; /* AgilityAnim */
static const uint8_t sMoveAnimScript_097[] = { 0xF4, 0x61, 0x46, 0xFF, 0x04, 0xDD, 0xFF, 0xFF }; /* QuickAttackAnim */
static const uint8_t sMoveAnimScript_098[] = { 0x06, 0x62, 0x01, 0xFF }; /* RageAnim */
static const uint8_t sMoveAnimScript_099[] = { 0xEE, 0x63, 0xED, 0xFF, 0xFF }; /* TeleportAnim */
static const uint8_t sMoveAnimScript_100[] = { 0xF8, 0x5C, 0xD8, 0xFF, 0xFF }; /* NightShadeAnim */
static const uint8_t sMoveAnimScript_101[] = { 0x46, 0x65, 0x21, 0x46, 0x65, 0x22, 0xFF }; /* MimicAnim */
static const uint8_t sMoveAnimScript_102[] = { 0x46, 0x66, 0x12, 0xFF }; /* ScreechAnim */
static const uint8_t sMoveAnimScript_103[] = { 0xFD, 0xFF, 0xE1, 0xFF, 0xE1, 0xFF, 0xFE, 0xFF, 0xFE, 0xFF, 0xFC, 0xFF, 0xDA, 0x67, 0xDD, 0xFF, 0x46, 0x6F, 0x33, 0xFF }; /* DoubleTeamAnim */
static const uint8_t sMoveAnimScript_104[] = { 0xF3, 0x68, 0xF0, 0xFF, 0xE2, 0xFF, 0xFC, 0xFF, 0xFF }; /* RecoverAnim */
static const uint8_t sMoveAnimScript_105[] = { 0xF0, 0x69, 0x46, 0xFF, 0x43, 0xFE, 0xFF, 0xFC, 0xFF, 0xFF }; /* HardenAnim */
static const uint8_t sMoveAnimScript_106[] = { 0xF0, 0x6A, 0xE2, 0xFF, 0xEA, 0xFF, 0xFC, 0xFF, 0xFF }; /* MinimizeAnim */
static const uint8_t sMoveAnimScript_107[] = { 0x46, 0x6B, 0x28, 0x04, 0xFF, 0x0A, 0xF9, 0xFF, 0xE1, 0xFF, 0xE1, 0xFF, 0xFD, 0xFF, 0xE1, 0xFF, 0xE1, 0xFF, 0xE1, 0xFF, 0xE1, 0xFF, 0xE1, 0xFF, 0xE1, 0xFF, 0xF9, 0xFF, 0xE1, 0xFF, 0xFC, 0xFF, 0xFF }; /* SmokeScreenAnim */
static const uint8_t sMoveAnimScript_108[] = { 0xFD, 0x6C, 0x46, 0xFF, 0x3E, 0xFC, 0xFF, 0xFF }; /* ConfuseRayAnim */
static const uint8_t sMoveAnimScript_109[] = { 0xF0, 0x6E, 0xF6, 0xFF, 0x06, 0xFF, 0x51, 0xFC, 0xFF, 0xDD, 0xFF, 0xFF }; /* WithdrawAnim */
static const uint8_t sMoveAnimScript_110[] = { 0xF0, 0x6E, 0x06, 0xFF, 0x43, 0xFE, 0xFF, 0xFC, 0xFF, 0xFF }; /* DefenseCurlAnim */
static const uint8_t sMoveAnimScript_111[] = { 0x46, 0x6F, 0x33, 0x46, 0x6F, 0x33, 0xFF }; /* BarrierAnim */
static const uint8_t sMoveAnimScript_112[] = { 0xF0, 0xFF, 0x46, 0x70, 0x33, 0x46, 0x70, 0x33, 0xFC, 0xFF, 0xFF }; /* LightScreenAnim */
static const uint8_t sMoveAnimScript_113[] = { 0xF9, 0xFF, 0xFA, 0x38, 0xFC, 0xFF, 0xFF }; /* HazeAnim */
static const uint8_t sMoveAnimScript_114[] = { 0xFD, 0xFF, 0x46, 0x72, 0x33, 0x46, 0x72, 0x33, 0xFC, 0xFF, 0xFF }; /* ReflectAnim */
static const uint8_t sMoveAnimScript_115[] = { 0xE2, 0x73, 0xFF }; /* FocusEnergyAnim */
static const uint8_t sMoveAnimScript_116[] = { 0x46, 0x74, 0x04, 0xFF }; /* BideAnim */
static const uint8_t sMoveAnimScript_117[] = { 0xF2, 0x84, 0xE1, 0xFF, 0xF1, 0x84, 0xE1, 0xFF, 0xF2, 0x84, 0xE1, 0xFF, 0xF1, 0x84, 0xFF }; /* MetronomeAnim */
static const uint8_t sMoveAnimScript_118[] = { 0x08, 0x76, 0x01, 0xFF }; /* MirrorMoveAnim */
static const uint8_t sMoveAnimScript_119[] = { 0x43, 0x77, 0x34, 0xFF }; /* SelfdestructAnim */
static const uint8_t sMoveAnimScript_120[] = { 0x44, 0x78, 0x41, 0x44, 0x78, 0x42, 0xFF }; /* EggBombAnim */
static const uint8_t sMoveAnimScript_121[] = { 0x46, 0x7B, 0x14, 0xFF }; /* LickAnim */
static const uint8_t sMoveAnimScript_122[] = { 0xF9, 0x48, 0x46, 0x7A, 0x19, 0xFC, 0xFF, 0xFF }; /* SmogAnim */
static const uint8_t sMoveAnimScript_123[] = { 0x46, 0x7B, 0x13, 0x46, 0x7B, 0x14, 0xFF }; /* SludgeAnim */
static const uint8_t sMoveAnimScript_124[] = { 0x08, 0x7C, 0x02, 0xFF }; /* BoneClubAnim */
static const uint8_t sMoveAnimScript_125[] = { 0x46, 0x7D, 0x1F, 0x46, 0xFF, 0x20, 0x46, 0xFF, 0x20, 0x46, 0xFF, 0x0C, 0x46, 0xFF, 0x0D, 0xFF }; /* FireBlastAnim */
static const uint8_t sMoveAnimScript_126[] = { 0xF6, 0x48, 0x06, 0x37, 0x1A, 0x08, 0xFF, 0x02, 0xF7, 0xFF, 0xFF }; /* WaterfallAnim */
static const uint8_t sMoveAnimScript_127[] = { 0x08, 0x7F, 0x2A, 0x06, 0x83, 0x23, 0x06, 0x83, 0x23, 0xFF }; /* ClampAnim */
static const uint8_t sMoveAnimScript_128[] = { 0x43, 0x80, 0x3F, 0xFF }; /* SwiftAnim */
static const uint8_t sMoveAnimScript_129[] = { 0x46, 0x81, 0x05, 0xFF }; /* SkullBashAnim */
static const uint8_t sMoveAnimScript_130[] = { 0x44, 0x82, 0x04, 0xFF }; /* SpikeCannonAnim */
static const uint8_t sMoveAnimScript_131[] = { 0x06, 0x83, 0x23, 0x06, 0x83, 0x23, 0x06, 0x83, 0x23, 0xFF }; /* ConstrictAnim */
static const uint8_t sMoveAnimScript_132[] = { 0x08, 0x84, 0x25, 0x08, 0x84, 0x25, 0xFF }; /* AmnesiaAnim */
static const uint8_t sMoveAnimScript_133[] = { 0x08, 0x85, 0x01, 0xFF }; /* KinesisAnim */
static const uint8_t sMoveAnimScript_134[] = { 0xE5, 0x48, 0x08, 0x86, 0x4C, 0xF0, 0xFF, 0xE2, 0xFF, 0xFC, 0xFF, 0xDD, 0xFF, 0xFF }; /* SoftboiledAnim */
static const uint8_t sMoveAnimScript_135[] = { 0x46, 0x87, 0x04, 0xFF }; /* HiJumpKickAnim */
static const uint8_t sMoveAnimScript_136[] = { 0xFD, 0x48, 0xFE, 0x88, 0xFE, 0xFF, 0xFC, 0xFF, 0xFF }; /* GlareAnim */
static const uint8_t sMoveAnimScript_137[] = { 0xF8, 0x89, 0xFD, 0x89, 0x08, 0x89, 0x02, 0xFC, 0xFF, 0xFF }; /* DreamEaterAnim */
static const uint8_t sMoveAnimScript_138[] = { 0x46, 0x8A, 0x19, 0xFF }; /* PoisonGasAnim */
static const uint8_t sMoveAnimScript_139[] = { 0x43, 0x8B, 0x41, 0x05, 0xFF, 0x55, 0xFF }; /* BarrageAnim */
static const uint8_t sMoveAnimScript_140[] = { 0x08, 0x8C, 0x02, 0xFE, 0xFF, 0x06, 0xFF, 0x21, 0x06, 0xFF, 0x22, 0xFE, 0xFF, 0xFF }; /* LeechLifeAnim */
static const uint8_t sMoveAnimScript_141[] = { 0x06, 0x8D, 0x12, 0xFF }; /* LovelyKissAnim */
static const uint8_t sMoveAnimScript_142[] = { 0xEE, 0x8E, 0xED, 0xFF, 0x46, 0x87, 0x04, 0xDD, 0xFF, 0xFF }; /* SkyAttackAnim */
static const uint8_t sMoveAnimScript_143[] = { 0x46, 0x8F, 0x21, 0x44, 0x8F, 0x22, 0x08, 0xFF, 0x47, 0xE8, 0xFF, 0xFF }; /* TransformAnim */
static const uint8_t sMoveAnimScript_144[] = { 0x16, 0x90, 0x35, 0xFF }; /* BubbleAnim */
static const uint8_t sMoveAnimScript_145[] = { 0x06, 0x91, 0x17, 0x06, 0x91, 0x17, 0x06, 0x91, 0x17, 0x06, 0x02, 0x02, 0xFF }; /* DizzyPunchAnim */
static const uint8_t sMoveAnimScript_146[] = { 0x06, 0x92, 0x36, 0xFF }; /* SporeAnim */
static const uint8_t sMoveAnimScript_147[] = { 0xF0, 0x48, 0xFE, 0x88, 0xFE, 0xFF, 0xFC, 0xFF, 0xFF }; /* FlashAnim */
static const uint8_t sMoveAnimScript_148[] = { 0x06, 0x2F, 0x31, 0xD8, 0x5C, 0xFF }; /* PsywaveAnim */
static const uint8_t sMoveAnimScript_149[] = { 0xEB, 0x95, 0xFF }; /* SplashAnim */
static const uint8_t sMoveAnimScript_150[] = { 0xE9, 0x96, 0xFF }; /* AcidArmorAnim */
static const uint8_t sMoveAnimScript_151[] = { 0x46, 0x97, 0x05, 0x06, 0xFF, 0x2A, 0xFF }; /* CrabHammerAnim */
static const uint8_t sMoveAnimScript_152[] = { 0x43, 0x98, 0x34, 0xFF }; /* ExplosionAnim */
static const uint8_t sMoveAnimScript_153[] = { 0x04, 0x99, 0x0F, 0xFF }; /* FurySwipesAnim */
static const uint8_t sMoveAnimScript_154[] = { 0x06, 0x9A, 0x02, 0xFF }; /* BonemerangAnim */
static const uint8_t sMoveAnimScript_155[] = { 0x10, 0x9B, 0x3A, 0x10, 0x9B, 0x3A, 0xFF }; /* RestAnim */
static const uint8_t sMoveAnimScript_156[] = { 0x04, 0x9C, 0x1D, 0x03, 0x9C, 0x1E, 0x46, 0x9D, 0x04, 0xFF }; /* RockSlideAnim */
static const uint8_t sMoveAnimScript_157[] = { 0x06, 0x9D, 0x02, 0xFF }; /* HyperFangAnim */
static const uint8_t sMoveAnimScript_158[] = { 0xF0, 0x9E, 0x46, 0xFF, 0x43, 0xFE, 0xFF, 0xFC, 0xFF, 0xFF }; /* SharpenAnim */
static const uint8_t sMoveAnimScript_159[] = { 0xFE, 0x9F, 0x46, 0xFF, 0x21, 0x46, 0xFF, 0x22, 0xFE, 0xFF, 0xFF }; /* ConversionAnim */
static const uint8_t sMoveAnimScript_160[] = { 0xFE, 0xA0, 0x46, 0xFF, 0x4D, 0xFE, 0xFF, 0xFF }; /* TriAttackAnim */
static const uint8_t sMoveAnimScript_161[] = { 0xFD, 0x48, 0x46, 0xA1, 0x04, 0xFC, 0xFF, 0xFF }; /* SuperFangAnim */
static const uint8_t sMoveAnimScript_162[] = { 0x06, 0xA2, 0x0F, 0xFF }; /* SlashAnim */
static const uint8_t sMoveAnimScript_163[] = { 0xF4, 0xA3, 0x08, 0xFF, 0x47, 0xD9, 0xFF, 0xFF }; /* SubstituteAnim */
static const uint8_t sMoveAnimScript_164[] = { 0x08, 0x00, 0x01, 0xFF }; /* StruggleAnim */
static const uint8_t sMoveAnimScript_165[] = { 0xDC, 0xFF, 0xFF }; /* ShowPicAnim */
static const uint8_t sMoveAnimScript_166[] = { 0xDD, 0xFF, 0xFF }; /* EnemyFlashAnim */
static const uint8_t sMoveAnimScript_167[] = { 0xF5, 0xFF, 0xFF }; /* PlayerFlashAnim */
static const uint8_t sMoveAnimScript_168[] = { 0xE4, 0xFF, 0xFF }; /* EnemyHUDShakeAnim */
static const uint8_t sMoveAnimScript_169[] = { 0x86, 0xFF, 0x48, 0xFF }; /* TradeBallDropAnim */
static const uint8_t sMoveAnimScript_170[] = { 0x84, 0xFF, 0x49, 0xFF }; /* TradeBallAppear1Anim */
static const uint8_t sMoveAnimScript_171[] = { 0x86, 0xFF, 0x4A, 0xFF }; /* TradeBallAppear2Anim */
static const uint8_t sMoveAnimScript_172[] = { 0x86, 0xFF, 0x4B, 0xFF }; /* TradeBallPoofAnim */
static const uint8_t sMoveAnimScript_173[] = { 0xF0, 0xFF, 0xE2, 0xFF, 0xFC, 0xFF, 0xFF }; /* XStatItemAnim */
static const uint8_t sMoveAnimScript_174[] = { 0xF0, 0xFF, 0xE2, 0xFF, 0xFC, 0xFF, 0xFF }; /* XStatItemAnim */
static const uint8_t sMoveAnimScript_175[] = { 0xF0, 0xFF, 0x46, 0xFF, 0x43, 0xFC, 0xFF, 0xFF }; /* ShrinkingSquareAnim */
static const uint8_t sMoveAnimScript_176[] = { 0xF0, 0xFF, 0x46, 0xFF, 0x43, 0xFC, 0xFF, 0xFF }; /* ShrinkingSquareAnim */
static const uint8_t sMoveAnimScript_177[] = { 0xF9, 0xFF, 0xE2, 0xFF, 0xFC, 0xFF, 0xFF }; /* XStatItemBlackAnim */
static const uint8_t sMoveAnimScript_178[] = { 0xF9, 0xFF, 0xE2, 0xFF, 0xFC, 0xFF, 0xFF }; /* XStatItemBlackAnim */
static const uint8_t sMoveAnimScript_179[] = { 0xF9, 0xFF, 0x46, 0xFF, 0x43, 0xFC, 0xFF, 0xFF }; /* ShrinkingSquareBlackAnim */
static const uint8_t sMoveAnimScript_180[] = { 0xF9, 0xFF, 0x46, 0xFF, 0x43, 0xFC, 0xFF, 0xFF }; /* ShrinkingSquareBlackAnim */
static const uint8_t sMoveAnimScript_181[] = { 0xF0, 0xFF, 0xEC, 0xFF, 0xFC, 0xFF, 0xFF }; /* UnusedAnim */
static const uint8_t sMoveAnimScript_182[] = { 0xF0, 0xFF, 0xEC, 0xFF, 0xFC, 0xFF, 0xFF }; /* UnusedAnim */
static const uint8_t sMoveAnimScript_183[] = { 0x04, 0x13, 0x24, 0x04, 0x13, 0x24, 0xFF }; /* ParalyzeAnim */
static const uint8_t sMoveAnimScript_184[] = { 0x04, 0x13, 0x24, 0x04, 0x13, 0x24, 0xFF }; /* ParalyzeAnim */
static const uint8_t sMoveAnimScript_185[] = { 0x08, 0x13, 0x27, 0x08, 0x13, 0x27, 0xFF }; /* PoisonAnim */
static const uint8_t sMoveAnimScript_186[] = { 0x08, 0x13, 0x27, 0x08, 0x13, 0x27, 0xFF }; /* PoisonAnim */
static const uint8_t sMoveAnimScript_187[] = { 0x10, 0x9B, 0x3A, 0x10, 0x9B, 0x3A, 0xFF }; /* SleepPlayerAnim */
static const uint8_t sMoveAnimScript_188[] = { 0x10, 0x9B, 0x3B, 0x10, 0x9B, 0x3B, 0xFF }; /* SleepEnemyAnim */
static const uint8_t sMoveAnimScript_189[] = { 0x08, 0x84, 0x25, 0x08, 0x84, 0x25, 0xFF }; /* ConfusedPlayerAnim */
static const uint8_t sMoveAnimScript_190[] = { 0x08, 0x84, 0x26, 0x08, 0x84, 0x26, 0xFF }; /* ConfusedEnemyAnim */
static const uint8_t sMoveAnimScript_191[] = { 0xF6, 0x5A, 0xFF }; /* SlideDownAnim */
static const uint8_t sMoveAnimScript_192[] = { 0x03, 0xFF, 0x06, 0xFF }; /* BallTossAnim */
static const uint8_t sMoveAnimScript_193[] = { 0x04, 0xFF, 0x09, 0xFF }; /* BallShakeAnim */
static const uint8_t sMoveAnimScript_194[] = { 0x04, 0xFF, 0x0A, 0xFF }; /* BallPoofAnim */
static const uint8_t sMoveAnimScript_195[] = { 0x03, 0xFF, 0x0B, 0xFF }; /* BallBlockAnim */
static const uint8_t sMoveAnimScript_196[] = { 0x03, 0xFF, 0x07, 0xFF }; /* GreatTossAnim */
static const uint8_t sMoveAnimScript_197[] = { 0x02, 0xFF, 0x08, 0xFF }; /* UltraTossAnim */
static const uint8_t sMoveAnimScript_198[] = { 0xFB, 0xFF, 0xFF }; /* ShakeScreenAnim */
static const uint8_t sMoveAnimScript_199[] = { 0xDF, 0xFF, 0xFF }; /* HidePicAnim */
static const uint8_t sMoveAnimScript_200[] = { 0x03, 0x8B, 0x53, 0xFF }; /* ThrowRockAnim */
static const uint8_t sMoveAnimScript_201[] = { 0x03, 0x8B, 0x54, 0xFF }; /* ThrowBaitAnim */
static const uint8_t sMoveAnimScript_202[] = { 0xD8, 0xFF, 0xFF }; /* ZigZagScreenAnim */

const uint8_t *const gMoveAnimAttackAnimationPointers[203] = {
    sMoveAnimScript_000,
    sMoveAnimScript_001,
    sMoveAnimScript_002,
    sMoveAnimScript_003,
    sMoveAnimScript_004,
    sMoveAnimScript_005,
    sMoveAnimScript_006,
    sMoveAnimScript_007,
    sMoveAnimScript_008,
    sMoveAnimScript_009,
    sMoveAnimScript_010,
    sMoveAnimScript_011,
    sMoveAnimScript_012,
    sMoveAnimScript_013,
    sMoveAnimScript_014,
    sMoveAnimScript_015,
    sMoveAnimScript_016,
    sMoveAnimScript_017,
    sMoveAnimScript_018,
    sMoveAnimScript_019,
    sMoveAnimScript_020,
    sMoveAnimScript_021,
    sMoveAnimScript_022,
    sMoveAnimScript_023,
    sMoveAnimScript_024,
    sMoveAnimScript_025,
    sMoveAnimScript_026,
    sMoveAnimScript_027,
    sMoveAnimScript_028,
    sMoveAnimScript_029,
    sMoveAnimScript_030,
    sMoveAnimScript_031,
    sMoveAnimScript_032,
    sMoveAnimScript_033,
    sMoveAnimScript_034,
    sMoveAnimScript_035,
    sMoveAnimScript_036,
    sMoveAnimScript_037,
    sMoveAnimScript_038,
    sMoveAnimScript_039,
    sMoveAnimScript_040,
    sMoveAnimScript_041,
    sMoveAnimScript_042,
    sMoveAnimScript_043,
    sMoveAnimScript_044,
    sMoveAnimScript_045,
    sMoveAnimScript_046,
    sMoveAnimScript_047,
    sMoveAnimScript_048,
    sMoveAnimScript_049,
    sMoveAnimScript_050,
    sMoveAnimScript_051,
    sMoveAnimScript_052,
    sMoveAnimScript_053,
    sMoveAnimScript_054,
    sMoveAnimScript_055,
    sMoveAnimScript_056,
    sMoveAnimScript_057,
    sMoveAnimScript_058,
    sMoveAnimScript_059,
    sMoveAnimScript_060,
    sMoveAnimScript_061,
    sMoveAnimScript_062,
    sMoveAnimScript_063,
    sMoveAnimScript_064,
    sMoveAnimScript_065,
    sMoveAnimScript_066,
    sMoveAnimScript_067,
    sMoveAnimScript_068,
    sMoveAnimScript_069,
    sMoveAnimScript_070,
    sMoveAnimScript_071,
    sMoveAnimScript_072,
    sMoveAnimScript_073,
    sMoveAnimScript_074,
    sMoveAnimScript_075,
    sMoveAnimScript_076,
    sMoveAnimScript_077,
    sMoveAnimScript_078,
    sMoveAnimScript_079,
    sMoveAnimScript_080,
    sMoveAnimScript_081,
    sMoveAnimScript_082,
    sMoveAnimScript_083,
    sMoveAnimScript_084,
    sMoveAnimScript_085,
    sMoveAnimScript_086,
    sMoveAnimScript_087,
    sMoveAnimScript_088,
    sMoveAnimScript_089,
    sMoveAnimScript_090,
    sMoveAnimScript_091,
    sMoveAnimScript_092,
    sMoveAnimScript_093,
    sMoveAnimScript_094,
    sMoveAnimScript_095,
    sMoveAnimScript_096,
    sMoveAnimScript_097,
    sMoveAnimScript_098,
    sMoveAnimScript_099,
    sMoveAnimScript_100,
    sMoveAnimScript_101,
    sMoveAnimScript_102,
    sMoveAnimScript_103,
    sMoveAnimScript_104,
    sMoveAnimScript_105,
    sMoveAnimScript_106,
    sMoveAnimScript_107,
    sMoveAnimScript_108,
    sMoveAnimScript_109,
    sMoveAnimScript_110,
    sMoveAnimScript_111,
    sMoveAnimScript_112,
    sMoveAnimScript_113,
    sMoveAnimScript_114,
    sMoveAnimScript_115,
    sMoveAnimScript_116,
    sMoveAnimScript_117,
    sMoveAnimScript_118,
    sMoveAnimScript_119,
    sMoveAnimScript_120,
    sMoveAnimScript_121,
    sMoveAnimScript_122,
    sMoveAnimScript_123,
    sMoveAnimScript_124,
    sMoveAnimScript_125,
    sMoveAnimScript_126,
    sMoveAnimScript_127,
    sMoveAnimScript_128,
    sMoveAnimScript_129,
    sMoveAnimScript_130,
    sMoveAnimScript_131,
    sMoveAnimScript_132,
    sMoveAnimScript_133,
    sMoveAnimScript_134,
    sMoveAnimScript_135,
    sMoveAnimScript_136,
    sMoveAnimScript_137,
    sMoveAnimScript_138,
    sMoveAnimScript_139,
    sMoveAnimScript_140,
    sMoveAnimScript_141,
    sMoveAnimScript_142,
    sMoveAnimScript_143,
    sMoveAnimScript_144,
    sMoveAnimScript_145,
    sMoveAnimScript_146,
    sMoveAnimScript_147,
    sMoveAnimScript_148,
    sMoveAnimScript_149,
    sMoveAnimScript_150,
    sMoveAnimScript_151,
    sMoveAnimScript_152,
    sMoveAnimScript_153,
    sMoveAnimScript_154,
    sMoveAnimScript_155,
    sMoveAnimScript_156,
    sMoveAnimScript_157,
    sMoveAnimScript_158,
    sMoveAnimScript_159,
    sMoveAnimScript_160,
    sMoveAnimScript_161,
    sMoveAnimScript_162,
    sMoveAnimScript_163,
    sMoveAnimScript_164,
    sMoveAnimScript_165,
    sMoveAnimScript_166,
    sMoveAnimScript_167,
    sMoveAnimScript_168,
    sMoveAnimScript_169,
    sMoveAnimScript_170,
    sMoveAnimScript_171,
    sMoveAnimScript_172,
    sMoveAnimScript_173,
    sMoveAnimScript_174,
    sMoveAnimScript_175,
    sMoveAnimScript_176,
    sMoveAnimScript_177,
    sMoveAnimScript_178,
    sMoveAnimScript_179,
    sMoveAnimScript_180,
    sMoveAnimScript_181,
    sMoveAnimScript_182,
    sMoveAnimScript_183,
    sMoveAnimScript_184,
    sMoveAnimScript_185,
    sMoveAnimScript_186,
    sMoveAnimScript_187,
    sMoveAnimScript_188,
    sMoveAnimScript_189,
    sMoveAnimScript_190,
    sMoveAnimScript_191,
    sMoveAnimScript_192,
    sMoveAnimScript_193,
    sMoveAnimScript_194,
    sMoveAnimScript_195,
    sMoveAnimScript_196,
    sMoveAnimScript_197,
    sMoveAnimScript_198,
    sMoveAnimScript_199,
    sMoveAnimScript_200,
    sMoveAnimScript_201,
    sMoveAnimScript_202,
};

const move_anim_special_effect_ptr_t gMoveAnimSpecialEffectPointers[39] = {
    { 0xFE, "AnimationFlashScreen" },
    { 0xFD, "AnimationDarkScreenPalette" },
    { 0xFC, "AnimationResetScreenPalette" },
    { 0xFB, "AnimationShakeScreen" },
    { 0xFA, "AnimationWaterDropletsEverywhere" },
    { 0xF9, "AnimationDarkenMonPalette" },
    { 0xF8, "AnimationFlashScreenLong" },
    { 0xF7, "AnimationSlideMonUp" },
    { 0xF6, "AnimationSlideMonDown" },
    { 0xF5, "AnimationFlashMonPic" },
    { 0xF4, "AnimationSlideMonOff" },
    { 0xF3, "AnimationBlinkMon" },
    { 0xF2, "AnimationMoveMonHorizontally" },
    { 0xF1, "AnimationResetMonPosition" },
    { 0xF0, "AnimationLightScreenPalette" },
    { 0xEF, "AnimationHideMonPic" },
    { 0xEE, "AnimationSquishMonPic" },
    { 0xED, "AnimationShootBallsUpward" },
    { 0xEC, "AnimationShootManyBallsUpward" },
    { 0xEB, "AnimationBoundUpAndDown" },
    { 0xEA, "AnimationMinimizeMon" },
    { 0xE9, "AnimationSlideMonDownAndHide" },
    { 0xE8, "AnimationTransformMon" },
    { 0xE7, "AnimationLeavesFalling" },
    { 0xE6, "AnimationPetalsFalling" },
    { 0xE5, "AnimationSlideMonHalfOff" },
    { 0xE4, "AnimationShakeEnemyHUD" },
    { 0xE3, "AnimationShakeEnemyHUD" },
    { 0xE2, "AnimationSpiralBallsInward" },
    { 0xE1, "AnimationDelay10" },
    { 0xE0, "AnimationFlashEnemyMonPic" },
    { 0xDF, "AnimationHideEnemyMonPic" },
    { 0xDE, "AnimationBlinkEnemyMon" },
    { 0xDD, "AnimationShowMonPic" },
    { 0xDC, "AnimationShowEnemyMonPic" },
    { 0xDB, "AnimationSlideEnemyMonOff" },
    { 0xDA, "AnimationShakeBackAndForth" },
    { 0xD9, "AnimationSubstitute" },
    { 0xD8, "AnimationWavyScreen" },
};

const uint8_t gMoveAnimTilesetTileCounts[3] = { 79, 79, 64 };
const char *const gMoveAnimTilesetAsmLabels[3] = { "MoveAnimationTiles0", "MoveAnimationTiles1", "MoveAnimationTiles2" };
