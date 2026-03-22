#pragma once
#include <stdint.h>

/* save.h — SRAM → file I/O replacement
 *
 * Original SRAM layout (bank 1):
 *   $598 bytes padding
 *   sPlayerName   (NAME_LENGTH)
 *   sMainData     (wMainDataEnd - wMainDataStart)
 *   sSpriteData   (wSpriteDataEnd - wSpriteDataStart)
 *   sPartyData    (wPartyDataEnd - wPartyDataStart)
 *   sCurBoxData   (wBoxDataEnd - wBoxDataStart)
 *   sTileAnimations (1 byte)
 *   sMainDataCheckSum (1 byte)
 *
 * On PC: stored as "pokered.sav" binary file with same layout.
 * Checksum: ~(sum of sMainData bytes) — 1 byte.
 */

#define SAVE_FILE "pokered.sav"

/* Returns 0 on success, -1 on missing/corrupt save */
int  Save_Load(void);

/* Returns 0 on success */
int  Save_Write(void);

/* Validate checksum only */
int  Save_ValidateChecksum(void);
