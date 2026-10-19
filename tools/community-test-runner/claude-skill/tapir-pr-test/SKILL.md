---
name: tapir-pr-test
description: Use when asked to test Tapir pull requests (or a local Tapir build) in a real Archicad, check what is waiting for a test, or write a check for a Tapir command. Drives tools/community-test-runner with its one-line output, so testing many pull requests costs few tokens.
---

# Testing Tapir pull requests with the community test runner

The runner (`tools/community-test-runner/tapir_test_runner.py` in a Tapir checkout) installs a pull
request's CI build into Archicad through Archicad's add-on list, runs the JSON checks, quits
Archicad and puts the tester's own Tapir back. You only read its output. Never open Archicad
yourself for this, never read the runner's source to use it, and never print full reports.

## Rules that keep it cheap

- Always pass `--brief`: one line per pull request and Archicad version, plus at most five
  failure lines. Without it a report is a markdown table of every expectation.
- Run `prs` once to choose, not per pull request.
- Don't post results unless the user asked: add `--no-post`. Posting comments in the user's name
  is their decision.
- Exit codes: 0 all passed or skipped, 1 a check failed, 2 the runner stopped (Archicad would not
  quit, a dialog, no answer). On 2, report the line and stop; don't retry in a loop.

## Commands (run from the checkout root; prefix `python tools/community-test-runner/tapir_test_runner.py`)

| Goal | Command |
|---|---|
| Is it set up? | `prs` (fails with "run setup first" if not; setup asks for a token, so the user runs `setup` themselves) |
| What can be tested | `prs` → `#869 own \| bf11c1e \| labels - \| checks 2 \| builds AC25,…,30 \| last AC30 PASS 12/12 … \| title` |
| Test chosen pull requests | `test 869 870 --ac 30 --brief --no-post` |
| Test everything open | `test --all --brief --no-post` (a PR without checks of its own runs the general checks from main; `--every-check` adds them for the others) |
| Test again on the same commit | add `--retest` |
| Test a local build | `trial --ac 30 --build archicad-addon/Build/AC30/RelWithDebInfo/TapirAddOn_AC30_Win.apx --brief` |
| Archicad is already open with an empty untitled project | `check --brief` (this checkout's checks) or `check path/to/check.json --brief` |
| A test was interrupted | `restore` |

Archicad must be **closed** for `test` and `trial` (the line says "AC30 is open - skipped").
Ask the user before closing their Archicad; it may hold unsaved work.

Pull requests from forks are skipped unless the fork owner is in `trustedForks` in the
configuration (`setup` suggests the user's own name). Tell the user which were skipped rather than
editing that list yourself: it decides whose native code is installed on their computer.

## Reading a line

`#869 AC30 FAIL 10/12 build bf11c1e tapir 1.7.2` then `  x <check title> / <step id> / <what>: <actual> (want <expectation>)`.
`skipped: no finished AC30 build` means CI has not built that commit (bot pull requests get no
build today). A report noting "general check file(s)" means the PR had no checks of its own; offer to write one
for its new commands.

## Writing a check without reading other checks

1. With Archicad open on an empty, untitled project: `record --out archicad-addon/Test/checks/<name>.json --title "<what it shows>"`.
2. Send the commands to port 19800 instead of 19723 (the Tapir MCP, a script, Grasshopper).
3. Ctrl+C (or stop the background process). The file has one step per command, guids saved as
   `${g1}`…, and the answer's values as expectations. Keep only the expectations that prove the
   behaviour, then `check <file> --brief`.

Format, if you must edit by hand: `steps[]` of `{id, command, params, save{name: path},
expect[{path|expr, eq|ne|approx(+tol)|lt|gt|len|contains|exists, name}], creates:false}`,
`${name}` in params, top-level `title`, `minTapir`, `teardown`. Created elements are deleted
automatically.
