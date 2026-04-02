#!/usr/bin/env node
/* gen_event_constants.js — Generate src/data/event_constants.h from
 * pokered-master/constants/event_constants.asm.
 *
 * Usage: node tools/gen_event_constants.js
 * Run from the repo root. */

const fs = require('fs');
const path = require('path');

const ASM_IN  = path.join(__dirname, '..', 'pokered-master', 'constants', 'event_constants.asm');
const HDR_OUT = path.join(__dirname, '..', 'src', 'data', 'event_constants.h');

const src   = fs.readFileSync(ASM_IN, 'utf8');
const lines = src.split('\n');

let counter = 0;
const defines = [];
let maxVal = 0;

for (const raw of lines) {
    const line = raw.trim();
    if (line === 'const_def') { counter = 0; continue; }

    let m;
    m = line.match(/^const_next\s+\$([0-9A-Fa-f]+)/);
    if (m) { counter = parseInt(m[1], 16); continue; }
    m = line.match(/^const_next\s+(\d+)/);
    if (m) { counter = parseInt(m[1]); continue; }
    m = line.match(/^const_skip\s+(\d+)/);
    if (m) { counter += parseInt(m[1]); continue; }
    if (line === 'const_skip') { counter += 1; continue; }
    m = line.match(/^const\s+(EVENT_\w+)/);
    if (m) {
        defines.push([counter, m[1]]);
        if (counter > maxVal) maxVal = counter;
        counter++;
    }
}

let out = `#pragma once
/* event_constants.h — Generated from pokered-master/constants/event_constants.asm.
 * DO NOT EDIT — regenerate via: node tools/gen_event_constants.js */

`;
for (const [v, n] of defines) {
    out += `#define ${n} ${v}u\n`;
}
out += `\n/* Total named flags: ${defines.length}, max value: ${maxVal} (0x${maxVal.toString(16).toUpperCase()}) */\n`;
out += `/* NUM_EVENTS = 0xA00 = 2560 (original pokered allocation) */\n`;

fs.writeFileSync(HDR_OUT, out);
console.log(`Written ${defines.length} defines to ${HDR_OUT}`);
console.log(`Max flag value: ${maxVal} (0x${maxVal.toString(16).toUpperCase()}), requires ${Math.ceil((maxVal+1)/8)} bytes`);
