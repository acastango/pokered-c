# Tools Layout

This repo keeps `tools/` as the home for scripts and helper utilities.

Current structure:
- `tools/` root:
  - active/high-frequency scripts kept in place for compatibility
  - existing extract/generation utilities
- `tools/data/`:
  - one-off or legacy data extraction/transformation scripts migrated from repo root
- `tools/debug/`:
  - debug-only helpers
- `tools/dev/`:
  - development workflow helpers

Notes:
- This split is intentionally incremental to avoid breaking existing references.
- Future moves should prefer subfolders, but root scripts may remain when they are stable entrypoints.
