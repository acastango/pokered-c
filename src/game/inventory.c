/* inventory.c -- Bag item management.
 *
 * Mirrors engine/items/inventory.asm: AddItemToInventory_ / RemoveItemFromInventory_
 *
 * wBagItems: pairs of [item_id, qty], up to BAG_ITEM_CAPACITY entries, terminated 0xFF.
 * wNumBagItems: count of occupied slots.
 */
#include "inventory.h"
#include "../platform/hardware.h"
#include "../data/item_data_gen.h"

#define MAX_QTY          99
#define ITEM_TERM        0xFF

int Inventory_Add(uint8_t item_id, uint8_t qty) {
    /* Search for existing slot */
    for (int i = 0; i < (int)wNumBagItems; i++) {
        if (wBagItems[i * 2] == item_id) {
            int new_qty = (int)wBagItems[i * 2 + 1] + qty;
            if (new_qty > MAX_QTY) new_qty = MAX_QTY;
            wBagItems[i * 2 + 1] = (uint8_t)new_qty;
            return 0;
        }
    }
    /* New slot */
    if (wNumBagItems >= BAG_ITEM_CAPACITY) return -1;
    int slot = wNumBagItems;
    wBagItems[slot * 2]     = item_id;
    wBagItems[slot * 2 + 1] = qty > MAX_QTY ? MAX_QTY : qty;
    wNumBagItems++;
    wBagItems[wNumBagItems * 2] = ITEM_TERM;
    return 0;
}

int Inventory_Remove(uint8_t item_id, uint8_t qty) {
    for (int i = 0; i < (int)wNumBagItems; i++) {
        if (wBagItems[i * 2] != item_id) continue;
        int new_qty = (int)wBagItems[i * 2 + 1] - qty;
        if (new_qty > 0) {
            wBagItems[i * 2 + 1] = (uint8_t)new_qty;
        } else {
            /* Compact: shift remaining slots down */
            for (int j = i; j < (int)wNumBagItems - 1; j++) {
                wBagItems[j * 2]     = wBagItems[(j + 1) * 2];
                wBagItems[j * 2 + 1] = wBagItems[(j + 1) * 2 + 1];
            }
            wNumBagItems--;
            wBagItems[wNumBagItems * 2] = ITEM_TERM;
        }
        return 0;
    }
    return -1;
}

int Inventory_GetQty(uint8_t item_id) {
    for (int i = 0; i < (int)wNumBagItems; i++)
        if (wBagItems[i * 2] == item_id)
            return wBagItems[i * 2 + 1];
    return 0;
}

void Inventory_DecodeASCII(uint8_t item_id, char *buf, int buf_size) {
    if (buf_size <= 0) return;
    if (item_id == 0 || item_id > NUM_ITEMS) { buf[0] = '\0'; return; }
    const uint8_t *src = kItemNames[item_id];
    int out = 0;
    for (int i = 0; i < ITEM_NAME_LENGTH && out < buf_size - 1; i++) {
        uint8_t c = src[i];
        if (c == 0x50) break;                         /* terminator */
        if (c >= 0x80 && c <= 0x99)                   { buf[out++] = 'A' + (c - 0x80); }
        else if (c >= 0xA0 && c <= 0xB9)              { buf[out++] = 'a' + (c - 0xA0); }
        else if (c == 0x7F)                            { buf[out++] = ' '; }
        else if (c >= 0xF6)                            { buf[out++] = '0' + (c - 0xF6); }
        else if (c == 0xE8)                            { buf[out++] = '.'; }
        else if (c == 0xE3)                            { buf[out++] = '-'; }
        else if (c == 0xBA)                            { buf[out++] = 'e'; } /* e-acute */
        else if ((c == 0xBB || c == 0xBC || c == 0xBD ||
                  c == 0xBE || c == 0xBF || c == 0xE4 || c == 0xE5)
                 && out < buf_size - 2) {
            /* Multi-char ligatures: 'd 'l 's 't 'v 'r 'm */
            static const char suf[] = "dlstvrm";
            static const uint8_t codes[] = {0xBB,0xBC,0xBD,0xBE,0xBF,0xE4,0xE5};
            buf[out++] = '\'';
            for (int k = 0; k < 7; k++) if (codes[k] == c) { buf[out++] = suf[k]; break; }
        }
        else { buf[out++] = '?'; }
    }
    buf[out] = '\0';
}

int Inventory_IsKeyItem(uint8_t item_id) {
    if (item_id == ITEM_NONE || item_id > NUM_ITEMS) return 0;
    int idx = item_id - 1;
    return (kKeyItemFlags[idx / 8] >> (idx % 8)) & 1;
}

const uint8_t *Inventory_GetName(uint8_t item_id) {
    /* IDs beyond NUM_ITEMS (TMs/HMs) fall back to NO_ITEM blank */
    if (item_id > NUM_ITEMS) item_id = 0;
    return kItemNames[item_id];
}
