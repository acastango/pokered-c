#pragma once
#include <stdint.h>

/* anim.h — Animated background tile constants and API.
 *
 * Mirrors TILEANIM_* from constants/map_data_constants.asm and
 * UpdateMovingBgTiles from home/vcopy.asm.
 */

/* Animation type stored per tileset (tileset_data.h anim_type field). */
#define TILEANIM_NONE         0   /* no animated tiles */
#define TILEANIM_WATER        1   /* water tile $14 only */
#define TILEANIM_WATER_FLOWER 2   /* water $14 + flower $03 */

/* Called by Map_Load when a new tileset is loaded.
 * Resets hMovingBGTilesCounter1; leaves wMovingBGTilesCounter2 alone
 * (original behaviour — counter2 persists across map transitions). */
void Anim_SetTileset(uint8_t anim_type);

/* Called once per VBlank (60 Hz) from GameTick, before the 30 Hz overworld
 * gate.  Mirrors UpdateMovingBgTiles in home/vcopy.asm. */
void Anim_UpdateTiles(void);
