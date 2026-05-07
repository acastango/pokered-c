---
name: pokered-build
description: Build the pokered project (kills running instance, compiles with MinGW)
user-invocable: true
---

Kill any running game instance, then build:

1. Run `taskkill //F //IM pokered.exe 2>/dev/null || true` via Bash (ignore errors — game may not be running).
2. Run the build command via Bash:
   ```
   PATH=/c/msys64/mingw64/bin:/c/msys64/usr/bin:$PATH && /c/msys64/mingw64/bin/mingw32-make.exe -C /c/Users/Anthony/pokered/build
   ```
3. Report success or show the first compiler error if it fails.
