# Tapir community test runner

Tests open pull requests on real Archicad, on the tester's own machine, and posts the result on
the pull request. It needs Archicad, Python 3.10+ and a GitHub account. No Claude subscription:
the checks are plain JSON, run against the Tapir API. Claude Code can drive it too, cheaply
(see [For Claude Code](#for-claude-code)).

**New to this? Read [GUIDE.md](GUIDE.md)**: a step-by-step guide without the command line
(install Python, download, double-click `Start Tapir Test Runner.bat`).

## Quick start

```
python tapir_test_runner.py setup
python tapir_test_runner.py gui
```

`setup` asks a few questions and changes nothing in Program Files:

- finds every installed Archicad and its stock template (Archicad opens a template as a new,
  untitled project, which is where checks run), and shows which Tapir each one loads now;
- asks which versions to test;
- opens GitHub's page for a token (classic, only the `public_repo` scope), checks the token you
  paste and keeps it in Windows Credential Manager, so no environment variable is needed;
- suggests trusting your own fork, so your own pull requests are tested too;
- writes the configuration to `%LOCALAPPDATA%\TapirRunner\runner.json`;
- optionally opens the runner's window at every logon, and installs the Claude Code skill.

Run `setup` again at any time to change the answers. `setup --yes` asks nothing.

The window lists the open pull requests with their checks, builds and last results (gray: in a
fork you don't trust). **Test selected**, **Test everything due**, **Pause automatic testing**
(otherwise it tests what is labelled `needs-test` every 15 minutes), **Stop after this test**,
**Put my add-on back**, and whether to post results on GitHub. Double-click a row to open it.

The runner only works while the Archicad it tests is closed, and never touches an open one: close
Archicad and the next pass runs.

## What a test does

For each pull request and each Archicad version:

1. skips it if you already reported on its current commit, or if its branch is in a fork you
   don't trust;
2. fetches the pull request's check files (`archicad-addon/Test/checks/*.json` it adds or
   changes; with `--every-check` or **Run every check**, the whole folder) and the add-on the
   repository's **Archicad Add-On Build Check** built for that commit;
3. loads that build instead of your Tapir, starts Archicad with the template, runs the checks,
   asks Archicad to quit, and puts your Tapir back;
4. posts one comment whose first line can be read by a workflow:
   `tapir-test: AC28 GER PASS 12/12 build 1a2b3c4 tapir 1.7.1` (Archicad version and language),
   followed by a table.

**How your Tapir is left alone.** Archicad keeps the list of add-ons from *Add-On Manager* in the
registry (`HKCU\Software\GRAPHISOFT\Archicad\Archicad <version> …\Add-On Manager\Include` and
`…\Disable`). For a test the runner copies the build to `%LOCALAPPDATA%\TapirRunner\builds`,
adds it to that list, leaves every other Tapir out (including one in the `Add-Ons` folder), and
afterwards writes both lists back exactly as they were. The previous lists are saved to a file
first, so after a crash or a power cut the next run, or `restore`, puts them back. Configurations
with an `addonFile` (written by hand for the first version of the runner) still swap that file
instead.

**Archicad 25-28 with Tapir in the `Add-Ons` folder** (where the Tapir Installer puts it): these
versions do not load an add-on from the list while the same add-on is switched off at its usual
place; they say some add-ons could not be loaded and load neither (#1150). `setup` then sets
`addonFile` for that Archicad, and the runner swaps that file, which needs administrator rights
(run it as administrator). The add-on list worked this way on Archicad 30 (#1088); Archicad 29 is
not tested yet. A Tapir that is only in the add-on list is no problem on any version.

Ports follow the order in which Archicads start, so the runner asks each connection for its
Archicad version and finds the right port itself. It never kills Archicad. If Archicad does not
quit (a dialog, for instance), the runner stops and leaves the test build loaded; close Archicad
and use **Put my add-on back** (or `restore`). A dialog during the start (project recovery,
licence, add-ons that could not be loaded) stops it within seconds, naming the dialog, instead of
waiting out `startTimeout`.

## Command line

| | |
|---|---|
| `prs` | one line per open pull request: own branch or fork, commit, labels, checks, built versions, last results |
| `test` | what is labelled `needs-test`; `test 869 870` those; `test --all` every open non-draft one |
| | `--ac 30` (repeatable), `--every-check`, `--no-post`, `--retest`, `--brief` |
| `check [files]` | run checks against an Archicad that is open with an empty, untitled project (default: this checkout's `archicad-addon/Test/checks`); finds the port itself |
| `trial --ac 30 --build x.apx [files]` | the whole load, start, check, quit, restore cycle with a build of your own, no GitHub |
| `record --out x.json` | write a check from the commands you send through it (below) |
| `restore` | put your add-on back after a test that did not finish |
| `bench` | speed and token benchmark of the open Archicad (below); `bench-compare old.json new.json` compares two runs |
| `once`, `poll` | one pass, or a pass every `pollMinutes`, over what is labelled (for a scheduled task) |

`--config` is only needed for a configuration somewhere else than the default. `--brief` prints
the verdict line and at most five failures instead of the whole table. Exit codes: 0 passed or
skipped, 1 a check failed, 2 the runner stopped.

## Benchmark

`bench` (or **Benchmark open Archicad** in the window) measures how fast Tapir answers and how
many tokens its answers cost an agent, the same way every time, so runs on different builds,
Archicad versions or machines can be compared. Open Archicad with a new, empty project and do not
save it; it takes a few minutes (2.5 on Archicad 30 with an ordinary PC).

It seeds the project with columns, walls and slabs (60/30/10 %) up to each scale of the suite
(100, 1000 and 5000 elements), and at each scale times every case: whole-project reads, details
with and without a field filter, property values, bounding boxes, quantities and a move of every
element. Fixed reads (properties, layers, libraries, favorites) and pings run once; bursts of 1 to
16 parallel calls show throughput and whether anything fails under load. Then it deletes what it
made, also when stopped. For each case it records p50/p95 milliseconds, milliseconds and tokens
per element, the answer's size, its tokens as the indented JSON the Tapir MCP server returns, and
the tokens of the first page of 100 items that server hands out for a long list.

| | |
|---|---|
| `bench --quick` | 100 elements, 2 runs per case: about a minute, for a first look |
| `bench --scales 100,1000` | other scales than the suite's |
| `bench --read-only` | the reads on the open project as it is (up to each scale); creates and moves nothing |
| `bench --exact-tokens` | counts tokens with Anthropic's `count_tokens` (`pip install anthropic`, an API key) instead of estimating; it sends the measured answers to the API, so use it with `--read-only` only on projects you may share |
| `bench --brief` | only the headline, errors and what changed against the last run |
| `bench-compare old.json new.json` | every case whose p50 or tokens moved more than `--threshold` % (default 15; time changes under 5 ms never count); exit 1 when one got worse |

Results go to `benchmarks/` next to the runner's configuration as `<time>_AC<version>_tapir<version>.json`
with a Markdown report beside it, and each run is compared with the last run of the same Archicad,
suite version and scales. The cases are data in [benchmarks/standard.json](benchmarks/standard.json):
add cases freely, but raise its `version` when you change an existing one, since results are
matched by case id and scale. Estimated tokens count a GUID as 24 tokens and the rest as one per
3.4 characters; compare estimates with estimates.

## Writing checks

### By recording

With Archicad open on an empty, untitled project:

```
python tapir_test_runner.py record --out ../../archicad-addon/Test/checks/my_command.json --title "MyCommand keeps the height"
```

Send your commands to port **19800** instead of 19723 (a Python script, Grasshopper, the Tapir
MCP server); the runner passes each one on to Archicad and its answer back. Press Ctrl+C when
done. The file has one step per command; element guids an answer returns are saved as `${g1}`,
`${g2}`, … and used in the later steps; the answer's other values (up to twelve per step) are
expectations. Delete the ones that don't prove anything, then run it:

```
python tapir_test_runner.py check ../../archicad-addon/Test/checks/my_command.json
```

### By hand

A pull request adds a JSON file to `archicad-addon/Test/checks/`. Example:

```json
{
  "title": "DeleteElements with a nonexistent guid (#749)",
  "minTapir": "1.6.0",
  "steps": [
    {"id": "real wall", "command": "CreateWalls",
     "params": {"wallsData": [{"begCoordinate": {"x": 700, "y": 700}, "endCoordinate": {"x": 703, "y": 700},
                               "zCoordinate": 0, "height": 2.5, "thickness": 0.2}]},
     "save": {"w1": "elements[0].elementId.guid"}},
    {"id": "[real, bogus]", "command": "DeleteElements", "creates": false,
     "params": {"elements": [{"elementId": {"guid": "${w1}"}},
                             {"elementId": {"guid": "00000000-0000-0000-0000-00000000BEEF"}}]},
     "expect": [{"path": "executionResults[0].success", "eq": true},
                {"path": "executionResults[1].success", "eq": false}]}
  ]
}
```

- `command`: a Tapir command, or an official one with the `API.` prefix.
- `params` may use `${name}` for values saved by earlier steps.
- `save`: name → path into the answer (`a.b[0].c`, `elements[*].elementId.guid`).
- `expect`: `path` (or `expr`, arithmetic over saved values) with one of `eq`, `ne`,
  `approx` (+ `tol`), `lt`, `gt`, `len`, `contains`, `exists`; `name` labels the row.
- Every element a step creates is deleted at the end; set `"creates": false` on steps that
  don't create anything. `teardown` steps run last.
- Checks run on every language version of Archicad, each with its own template and defaults (stair
  rules, pens, layers, units). Send every value the result depends on, and prefer differences
  (one more riser adds the going) over absolute values (the run is 4.15 m), which a template can shift.
- `minTapir` skips the file on older add-ons. `requiresEmpty` changes the element types that
  must be absent.

`checks/` holds examples taken from the October 2026 verification of #747-#753.

## For Claude Code

`setup --install-claude-skill` (or the question in `setup`) copies
[claude-skill/tapir-pr-test/SKILL.md](claude-skill/tapir-pr-test/SKILL.md) to
`~/.claude/skills/`. With it, "test the open pull requests on AC30" makes Claude run
`prs` once and `test … --brief --no-post`, and read one line per pull request and version
(plus at most five failure lines) instead of reports, source or Archicad itself. The token
stays with the runner; Claude never sees it, and it posts only when you ask.

## Safety

The runner installs native code from a pull request. Hence:

- only pull requests whose branch is in this repository are tested, plus forks you list in
  `trustedForks` (`setup` suggests only your own); `allowForks: true` trusts every fork;
- only builds made by this repository's own build check are installed;
- checks are data. The runner itself is the only code that runs; a check can send Tapir
  commands to an untitled, empty project and nothing else. Expressions allow `+ - * /` only;
- the token is kept in Windows Credential Manager for your Windows user; `TAPIR_RUNNER_TOKEN`
  still overrides it;
- use a separate Windows account for the runner if you can.

## Verified so far (Windows)

- 2026-10-16: `check` on AC26 and AC28 (Tapir 1.7.0/1.7.1): all six example files run, clean up
  after themselves and report exactly the issues still open in the release. `trial` on AC26, the
  full cycle with the release build: install, start with a stock `.tpl` (opens as an untitled
  project), 57 checks, `QuitArchicad` (quits in seconds even with a changed untitled project, no
  save dialog), restore. About two minutes. The GitHub side of `once`: finding pull requests,
  reading their check files, downloading the build check's artifact for the head commit.
- 2026-10-18, AC30: `setup --yes` found AC28-AC30, their stock templates and the Tapir each loads
  (from Program Files, the `Add-Ons` folder or a dev folder). `trial` with the 1.7.0 release
  build through the add-on list: 61 checks in 1 min 42 s, the two issues still open in 1.7.0
  (#425, #749) reported, the add-on list identical before and after. A start that waits at a
  dialog (project recovery) and an interrupted run were put back by `restore`. `record` through
  port 19800 and replaying the file with `check`. The window opens and lists errors in its log.
  Archicad 30 on that machine crashed on every quit, with or without checks; the report shows it
  as a warning line and the add-on is still put back.

## Not verified yet

- `once`/`poll`/`test` posting end to end: no pull request carries the `needs-test` label yet,
  and bot pull requests get no build today; the build check has to be dispatched for them.
- macOS (`setup` and the add-on list are Windows only; `check` and `record` work anywhere).
