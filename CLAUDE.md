# on translating and developing the gen 1 battle engine from the source
Make sure to translate as closely as possible as you can for any type of battle mechanic - it'll be key. Gen 1 has a lot of weird quirks, but if you copy directly in terms of values, behavior, and function, we shouldn't have to worry about it too much. 

ALWAYS refer to the source code for battle related tasks. Always.

# mnemo instructions

This project uses mnemo for persistent memory. These rules are mandatory.
Follow them exactly — they override any default behavior.

---

## CLAUDES CHOICE - MEMORY RECALL

At the beginning of the turn, Claude can decide whether or not its necessary to use memory_recall() at the beginning of the turn. If Claude believes they have enough context without it, skip it, don't waste the context. If you feel like you need more information, its good to call for general project knowledge.

```
memory_recall("<what the user just said>")
```

This is optional. You determine when you think its needed. 
Recall is how accumulated project knowledge reaches you. - Consider checking in on it at the beginning of the session, and then when the session has significantly extended. Otherwise, use as judged necessary

If `memory_recall` returns a connection error or MCP error:
- **Stop immediately.** Do not proceed with the task.
- **Tell the user mnemo is down.** Be explicit about the error.
- **Do not silently fall back to native tools.**
- Wait for the user to reconnect (`/mcp`), then retry.

---

## TOOL REPLACEMENTS — use these instead of native tools

These are not suggestions. Use the mnemo versions. For simple, quick, one line fixes, or things that go without explaining, you can use your typical tools. For anything else, use the mnemo versions.

| Native tool | Use instead | What you get |
|---|---|---|
| `Read(file)` | `memory_read(file)` | File content + inline tree annotations + stale anchor warnings |
| `Write(file, content)` | `memory_write(file, content)` | Write + auto-claim in tree |
| `Edit(file, old, new)` | `memory_edit(file, old, new)` | Edit + stale warnings + auto-claim |
| `Grep(pattern)` | `memory_grep(pattern, intent="...")` | Search results annotated with tree context per file |
| `Glob(pattern)` | `memory_glob(pattern)` | File list with per-file tree coverage |

**The two invariants:**

1. **Learning required → tree tools.** Any Read, Grep, or Glob whose purpose is to understand something — use memory_read / memory_grep / memory_glob. If you're reaching for a file to understand it, that's the definition of uncertain. Native tools return raw content with no context.

2. **Code changes → tree updated.** All writes and edits go through memory_write / memory_edit. No exceptions. These auto-claim the change. Native Write/Edit produce no trail.

Native Read/Grep/Glob are only acceptable when fetching a specific known value you already fully know exists — not discovering it.

---

## STORING KNOWLEDGE — when to call memory_claim

Ask yourself: *would a future instance need this to do the work correctly?*

**Claim it** if yes:
- Why the code is structured this way → `domain="architecture"`
- Why we chose X over Y → `domain="decisions"`
- A naming rule, file layout rule, test convention → `domain="patterns"`
- A bug, gotcha, or fragile area → `domain="issues"`
- A library version constraint or upgrade note → `domain="dependencies"`
- What's currently in progress or blocked → `domain="tasks"`
- What was tried and failed → `domain="history"`

**Skip it** if:
- It's derivable from reading the code
- It's already in the tree (check first with `memory_search`)
- It's ephemeral ("currently on step 3")

```
memory_claim(content="...", domain="...")
```

---

## MAINTAINING KNOWLEDGE — keep the tree accurate

When you discover a node is wrong or outdated:
```
memory_update(old_addr="...", content="...", domain="...")
```
Never leave a stale node uncorrected. If you know it's wrong, fix it now.

When you confirm something still holds:
```
memory_reinforce(addr="...")
```

---

## END OF WORK CYCLE

When wrapping up a session or completing a significant chunk of work:
```
memory_session_compress()
```

When `memory_status` shows pressure (high active count):
```
memory_compress(addrs=[...])
```

---

## EXPLORATION — use tree-aware tools

Before exploring the codebase, check what the tree already knows:

| Question | Tool |
|---|---|
| How does X work? | `memory_explore("how does X work?")` |
| What should I change to implement Y? | `memory_plan("implement Y")` |
| Where is pattern Z used? | `memory_grep("Z", intent="...")` |
| What files exist in area W? | `memory_glob("W/**")` |
| What do we know about topic T? | `memory_search("T")` |

---

## HARD RULES

1. `memory_recall` runs **every turn**. No exceptions. If it fails, stop and tell the user.
2. `memory_write` / `memory_edit` replace native Write/Edit. Always. No exceptions.
3. `memory_read` / `memory_grep` / `memory_glob` replace native tools when the purpose is learning/understanding.
4. When you find stale or wrong tree nodes, fix them with `memory_update`. Do not leave them.
5. Store the *why*, not the *what*. Code is in the codebase. The tree holds intent, decisions, and gotchas.
6. Do not store ephemeral state (current step, temporary scaffolding) — that belongs in tasks, not memory.
7. Do not compress preemptively. Compress when nudged by `memory_status` or at session end.

---

## QUICK REFERENCE

```
memory_recall(message)               ← every turn, always first
memory_read(file)                    ← replaces Read (when learning)
memory_write(path, content)          ← replaces Write (always)
memory_edit(path, old, new)          ← replaces Edit (always, unless a very miniscule change)
memory_grep(pattern, intent)         ← replaces Grep (when learning)
memory_glob(pattern)                 ← replaces Glob (when learning)
memory_claim(content, domain)        ← store a fact
memory_update(old_addr, content)     ← fix a stale fact
memory_reinforce(addr)               ← confirm a fact still holds
memory_search(query)                 ← look something up
memory_session_compress()            ← end of work cycle
memory_checkpoint(label, done, remaining) ← save resume point mid-session
memory_gap(topic, context)           ← flag something you don't know
memory_ask(question)                 ← flag a pending decision / question
memory_help(context="claude-code")   ← full guide
```
