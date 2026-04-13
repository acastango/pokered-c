## Oak's Lab / Overworld Textbox Vertical Offset Notes

### Current finding
- The standard overworld dialogue box in the C port currently matches the original ASM row layout.
- In `src/game/text.c`:
  - `BOX_ROW = 12`
  - `BOX_ROWS = 6`
  - `TEXT_ROW1 = 14`
  - `TEXT_ROW2 = 16`
- In `pokered-master/home/text.asm`:
  - `<LINE>` jumps to `hlcoord 1, 16`
  - paragraph clear returns to `hlcoord 1, 14`
  - scroll logic copies text from row `16` up toward `14`
- Conclusion: moving the general textbox up one tile in `text.c` would diverge from ASM.

### Why it can still *look* too low
- The port has more than one message-box layout:
  - `src/game/text.c` uses text rows `14/16` for normal dialogue.
  - `src/game/party_menu.c` uses message rows `13/15` for its local popup box.
- That means some UI in the port already trains the eye toward a slightly higher-looking two-line box, even though standard overworld dialogue is lower in the original.

### Concrete suspects before changing live code
1. Presentation mismatch, not data mismatch.
   - `Display_RenderScrolled()` is always used for the overworld, even while dialogue is open.
   - If the perceived issue only happens in specific map/script contexts, the problem may be the surrounding scene composition rather than the textbox rows themselves.

2. Wrong expectation imported from a non-dialog UI path.
   - Party-menu / submenu message boxes are drawn on `13/15`, which visually reads as "one tile higher" than overworld dialogue.
   - If someone normalized against that UI while judging Oak's Lab dialogue, the standard box will look low even when it matches ASM.

3. Another system may be redrawing around the box.
   - `Map_BuildScrollView()` is correctly skipped while `Text_IsOpen()` in the main overworld paths, but Oak's Lab has custom post-render logic for the YES/NO box.
   - If the report is specifically about Oak's Lab starter text rather than all dialogue, inspect custom Oak's Lab redraw flow before touching `text.c`.

### Safe next verification step
- Take a screenshot of a normal overworld NPC dialogue and an Oak's Lab starter dialogue.
- If both appear equally low, do not move `text.c`; the issue is likely expectation / surrounding composition.
- If only Oak's Lab looks low, inspect Oak's Lab-specific redraw and any custom box overlay interaction first.

### If a live fix becomes necessary
- Prefer a targeted Oak's Lab-specific fix over changing global textbox row constants.
- Do not change `BOX_ROW/TEXT_ROW1/TEXT_ROW2` without a stronger source-of-truth check, because the ASM currently supports the existing C rows.
