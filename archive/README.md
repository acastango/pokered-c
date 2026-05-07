# Archive Policy

This folder is a non-destructive holding area for cleanup passes.

Rules:
- Nothing here is auto-deleted.
- Use dated subfolders under `archive/review-pending/` for each cleanup pass.
- Include a small `README.md` in each batch with:
  - what moved
  - why it moved
  - how to restore
- If a batch is approved later, files can be either:
  - moved back into active tree, or
  - moved into a long-term archive namespace.

Restore pattern:
`Move-Item -LiteralPath archive/review-pending/<batch>/<file> -Destination <target>`
