---
name: monet
description: Persistent project memory workflow for Codex. Use when the user mentions monet, mnemo, persistent memory, memory tools, or when Codex should rely on the monet MCP server for recall, tree-aware exploration, code-aware reads/searches, and durable capture of project decisions or gotchas.
---

# monet

Use the `monet` MCP server as the primary memory layer for project understanding and durable knowledge capture.

## Start of turn

- Call `memory_recall("<what the user just said>")` at the start of the turn when monet is part of the requested workflow.
- If `memory_recall` or another monet tool returns a connection or MCP error, tell the user monet is unavailable and stop any memory-dependent workflow instead of silently falling back.

## Memory-first exploration

- Use `memory_search(query)` to see what the tree already knows.
- Use `memory_explore(question)` for "how does this work?" exploration.
- Use `memory_plan(task)` for "what should I change?" planning context.
- Use `memory_read(file)`, `memory_grep(pattern, intent)`, and `memory_glob(pattern)` when learning the codebase and you want tree context or stale-anchor warnings.
- Use raw file or shell reads only for narrow mechanical checks when no understanding is required.

## Code changes

- Prefer `memory_write(path, content)` and `memory_edit(path, old, new)` for edits that should update the tree automatically.
- If a change is too large or awkward for direct monet editing, make the code change normally and then capture the reason with `memory_claim(...)`.

## Storing knowledge

Store the why, not the obvious what.

- Use `memory_claim(content, domain)` for decisions, architecture intent, conventions, fragile areas, active tasks, dependency constraints, and failed attempts.
- Use domains such as `architecture`, `decisions`, `patterns`, `issues`, `dependencies`, `tasks`, and `history`.
- Use `memory_link(source, target, rel)` when two facts should stay connected.

## Maintaining knowledge

- Use `memory_update(old_address, new_content, reason=...)` when a node is stale or wrong.
- Use `memory_reinforce(address)` when a node is still valid.
- Do not leave known-wrong memory in place.

## Session management

- Use `memory_status()` to check active-set pressure and drift signals.
- Use `memory_compress(summary, addresses=[...])` when pressure is high or a cluster should collapse into a summary.
- Use `memory_session_compress(summary)` when wrapping a substantial work cycle.
- Use `memory_checkpoint(label, done=[...], remaining=[...])` if the work will likely continue later.

## Useful tools

- `memory_soul()` for a high-level project memory document
- `memory_diff()` for what changed since the last root
- `memory_verify()` for anchor drift
- `memory_help("quick")` or `memory_help("claude-code")` for the full guide

## Working style

- Trust monet for recall and durable context, but still verify code behavior when the task depends on current implementation details.
- Keep claims concise and specific enough to help a future session act correctly.
- Prefer tree-aware exploration before broad manual searching.
