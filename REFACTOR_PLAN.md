# Refactoring Tapir: what the size actually is, and what to do about it

A proposal for discussion, not a decision. Everything below is measured on `main` at `0bb1e0f`
(4,333 files, ~778k counted lines, excluding binaries); the numbers are what the argument rests on,
so they are easy to re-check with the commands in the last section.

## 1. The repo is not fat. It is tall.

There is almost no duplication to delete: the whole tree has **two** byte-identical file pairs. The size
comes from one property — *the unit of this repository is a command, and every command has to appear in
8–11 places by hand*.

There are **804 commands in the published reference** (831 in the sources). `ModifyKeynoteItems` alone
touches seven areas: the C++ (schema + implementation), an example, a check, a Grasshopper component,
the Python client, the command reference, plus an icon. `CreateColumns` touches sixteen check files.

| area | files | lines | nature |
| --- | --- | --- | --- |
| `archicad-addon/Sources` | 178 | 247,942 | product logic — **~69k of it is JSON schema literals inside C++** |
| `grasshopper-plugin/…/Components` | 868 | 114,663 | one class per command, median 95 non-blank lines, **~53% scaffolding** |
| `docs/archicad-addon` | 11 | 75,493 | **generated, committed** |
| `archicad-addon/Test/checks` | 380 | 69,250 | hand-written tests — the real suite, keep |
| `marketplace` + `builtin-scripts` | 128 | 33,321 | user-facing scripts, keep |
| `tools` | 64 | 16,590 | tooling, keep |
| `archicad-addon/Examples` | 855 | 15,037 | one script per command, documentation content |
| `archicad-addon/Test/Scripts` | 170 | 12,703 | golden scripts |
| `python-client` | 9 | 12,061 | **generated** `commands.py` is 11,425 of it |
| GH icons | 753 PNG + 289 SVG | — | derived from the SVG sources |
| everything else | ~150 | ~30,000 | site pages, branding, installer |

## 2. Three things are already half-migrated, and finishing them is the whole job

**Schemas are half data.** Shared definitions already live in a data file —
`Sources/RFIX/Images/CommonSchemaDefinitions.json` (1.3 MB), loaded at runtime. But the **per-command**
schemas — 729 `GetInputParametersSchema` and 826 response-schema implementations — are JSON written as
raw string literals inside 74 `.cpp` files. A third of the C++ is JSON that happens to be spelled in C++.

**Grasshopper components are data spelled as code.** 868 hand-written C# classes, one per command, with
~53% of their non-blank lines being boilerplate (`using`, class header, `In*`/`Out*` calls with a
description). The proof that this is data wearing a costume: `tools/generate_gh_docs.py` is a 700-line
**regex parser over the C#** whose only job is to read those inputs back out, and it documents that a
name built at run time "is left out rather than guessed".

**Generated output is committed.** The command reference (75k lines) and the Python client (11.4k) are
produced by `tools/generate_addon_docs.py` and `python-client/generate.py`, and both are checked in.
Every release rewrites 75k lines that no one reviews, which is why a drift gate is worth having.

## 3. The moves, in the order that pays

### Move 1 — finish the schema migration (enabler; no line count won, everything else unlocked)

Extract each command's input and response schema from the C++ to `schemas/<Command>.json`, and have
CMake compile them into a generated header so the add-on still hands them to Archicad at registration
with no runtime file loading. The shared definitions already work this way; this makes per-command
schemas match them.

*It is scriptable and semantically safe*: the literals are already valid JSON, so extraction is
mechanical, and the pilot can be one family (the four `Keynotes` commands) built on all six Archicad
versions before batching the other ~800.

Payoff: a new command becomes **schema + implementation** instead of ten places; docs, client,
Grasshopper, checks skeletons and LLM files all read one input. Effort L, risk M, and it is the
prerequisite for Move 2.

### Move 2 — generate the Grasshopper components (−~95k lines, 12% of the repo)

Today each component is a 95-line class encoding three things: the command name, a per-parameter
mapping from JSON to Grasshopper inputs (type, order, label), and boilerplate. Move 1 makes the first
mechanical; the second is a real decision that should be **~10 lines of JSON per command**
(`components/<Command>.json`), and the third should not be written by hand at all.

A generator plus a small runtime (the `ArchicadExecutorComponent` base already exists) turns 114k lines
of C# into ~8k lines of mapping data + ~2k of generator. And `generate_gh_docs.py` stops regex-parsing
C# — it reads the same mapping data.

Migratable command by command, with a golden comparison of the 868 components' metadata (name, group,
inputs, outputs) before and after. Effort L, risk M–H.

### Move 3 — stop committing generated output (−87k lines, 11%)

`docs/archicad-addon` and `python-client/commands.py` become build artifacts: generated in the Pages
workflow and in the release, published to a branch the way `api-coverage` already is, with a `--check`
in CI for the pull requests that touch schemas. Effort S–M, risk M (the site must consume the artifact),
and it removes 75k lines from every release diff.

### Move 4 — shrink the per-command ritual and the icons (−1,000 files)

753 PNGs derived from 289 SVGs should be built, not committed (or replaced by the SVG at runtime). The
`Test/checks` skeleton and the `Examples` stub can start from the schema the way `tools/new_command.py`
already scaffolds the rest.

### Move 5 — thin the build matrix on pull requests (CI cost, not files)

`devkits.json` lists six Archicad versions (25–30), and every pull request builds all six on Windows and
macOS: **12 builds**, which is the largest thing a pull request triggers. Compile-time API differences
between versions are real, so the matrix is not waste — but not every pull request can break them. Two
builds per OS (the oldest and the newest supported version) catch the common case on the PR, and the full
matrix runs nightly and in the release. Effort S, risk L–M: the "nightly finds it instead" window is the
cost, so it should stay a per-PR matrix for changes under `archicad-addon/Sources` and shrink only for
documentation, test and tooling changes — which is most pull requests.

### What I would *not* do

- **Do not split into separate repositories yet.** The schema is the shared dependency; splitting first
  gives five repos with the same replication and a versioning problem between them. Split after Move 1,
  if at all.
- **Do not touch `Test/checks`.** 380 files, 69k lines, and it is the only part of the repo that
  actually checks the product.
- **Do not shrink the marketplace and built-in scripts.** They are user-facing product, not overhead.
- **Do not touch the per-version `RFIX` resources.** 1.4 MB in total; not the problem.

## 4. Where it lands

| | now | after |
| --- | --- | --- |
| files | 4,333 | ~2,600 |
| lines | ~778k | ~590k |
| C++ that is JSON | ~69k | 0 |
| Grasshopper C# | 114,663 | ~10k (data + generator) |
| generated output in git | 87k | 0 |
| places to touch for a new command | 8–11 | **2** (schema, implementation) |
| CI per pull request | 12 builds (6 Archicad versions × 2 OS) | 2 on PR, full matrix nightly |

The line counts are the smaller half of the payoff. The real one is that adding a command stops being a
ten-file ritual that only a person with the whole convention in their head can do correctly — which is
also why a new command is so easy to get partially right: nothing in the repository says which of the
ten places is the source and which are copies.

## 5. Start here

Move 1 is the enabler and Move 3 is the cheapest. The first pull request I would open is the **Keynotes
pilot**: four commands, schemas extracted to JSON, compiled into a header, built on all six versions,
with the docs generator reading the JSON instead of the C++ — a small, reviewable slice that proves the
whole pattern before 800 commands follow it.

## 6. How to re-check the numbers

```bash
git ls-files | wc -l                                             # 4,333 files
wc -l archicad-addon/Sources/*.cpp | tail -1                     # 201,380 lines of C++
git ls-files grasshopper-plugin/TapirGrasshopperPlugin/Components | wc -l    # 868 components
grep -c "GetInputParametersSchema" archicad-addon/Sources/*.cpp | awk -F: '{s+=$2} END {print s}'   # 729
ls grasshopper-plugin/TapirGrasshopperPlugin/Resources/*.png | wc -l        # 753 icons
```

The schema share of the C++ (~34%) is the count of lines in those files that are JSON member
declarations (`"$ref"`, `"type"`, `"description"`, `"properties"`, braces and brackets) — an
approximation, and the only number here that is one.
