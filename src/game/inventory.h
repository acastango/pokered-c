#pragma once
#include <stdint.h>

/* inventory.h -- Bag item management.
 *
 * wBagItems layout: [item_id, qty] pairs, up to BAG_ITEM_CAPACITY slots,
 * terminated by 0xFF.  wNumBagItems = number of occupied slots.
 * Max qty per slot = 99.  Key items have qty 1 and cannot be tossed.
 *
 * Item IDs (from pokered item_constants.asm):
 */
#define ITEM_NONE        0x00
#define ITEM_MASTER_BALL 0x01
#define ITEM_ULTRA_BALL  0x02
#define ITEM_GREAT_BALL  0x03
#define ITEM_POKE_BALL   0x04
#define ITEM_TOWN_MAP    0x05
#define ITEM_BICYCLE     0x06
#define ITEM_POKEDEX     0x09
#define ITEM_MOON_STONE  0x0A
#define ITEM_ANTIDOTE    0x0B
#define ITEM_BURN_HEAL   0x0C
#define ITEM_ICE_HEAL    0x0D
#define ITEM_AWAKENING   0x0E
#define ITEM_PARLYZ_HEAL 0x0F
#define ITEM_FULL_RESTORE 0x10
#define ITEM_MAX_POTION  0x11
#define ITEM_HYPER_POTION 0x12
#define ITEM_SUPER_POTION 0x13
#define ITEM_POTION      0x14
#define ITEM_ESCAPE_ROPE 0x1D
#define ITEM_REPEL       0x1E
#define ITEM_FIRE_STONE  0x20
#define ITEM_THUNDER_STONE 0x21
#define ITEM_WATER_STONE 0x22
#define ITEM_RARE_CANDY   0x28
#define ITEM_DOME_FOSSIL  0x29
#define ITEM_HELIX_FOSSIL 0x2A
#define ITEM_POKE_DOLL   0x33
#define ITEM_FULL_HEAL   0x34
#define ITEM_REVIVE      0x35
#define ITEM_MAX_REVIVE  0x36
#define ITEM_SUPER_REPEL 0x38
#define ITEM_MAX_REPEL   0x39
#define ITEM_NUGGET      0x31
#define ITEM_SS_TICKET   0x3F
#define ITEM_OAKS_PARCEL 0x46
#define ITEM_POKE_FLUTE  0x49
#define HM01             0xC4
#define TM01             0xC9
#define TM_THUNDER_WAVE  0xF5

/* Add qty of item_id to bag.  Returns 0 on success, -1 if bag full. */
int Inventory_Add(uint8_t item_id, uint8_t qty);

/* Remove qty of item_id from bag.  Returns 0 on success, -1 if not found. */
int Inventory_Remove(uint8_t item_id, uint8_t qty);

/* Return quantity of item_id in bag (0 if not present). */
int Inventory_GetQty(uint8_t item_id);

/* Return 1 if item_id is a key item (cannot toss, qty not shown). */
int Inventory_IsKeyItem(uint8_t item_id);

/* Return pokéred-encoded name for item_id (ITEM_NAME_LENGTH bytes, CHAR_TERM padded). */
const uint8_t *Inventory_GetName(uint8_t item_id);

/* Decode item name to ASCII into buf (null-terminated, at most buf_size-1 chars).
 * Used for pickup/receive messages. */
void Inventory_DecodeASCII(uint8_t item_id, char *buf, int buf_size);
