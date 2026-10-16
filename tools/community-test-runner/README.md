# Tapir community test runner

Tests open pull requests on real Archicad, on the tester's own machine, and posts the result on
the pull request. It needs Archicad, Python 3.10+ and a GitHub token. No Claude subscription:
the checks are plain JSON, run against the Tapir API.

## What it does

For every open pull request labelled `needs-test`, for each Archicad version you configured:

1. skips it if you already reported on its current commit, or if its branch is in a fork;
2. fetches the pull request's check files (`archicad-addon/Test/checks/*.json` it adds or
   changes) and the add-on the repository's **Archicad Add-On Build Check** built for that commit;
3. does nothing while that Archicad is running; otherwise swaps in the build, starts Archicad
   with your blank template, runs the checks, asks Archicad to quit, and puts your add-on back;
4. posts one comment whose first line can be read by a workflow:
   `tapir-test: AC28 PASS 12/12 build 1a2b3c4 tapir 1.7.1`, followed by a table.

Ports follow the order in which Archicads start, so the runner asks each connection for its Archicad
version and refuses a port that answers as another version. It never kills Archicad. If Archicad does not quit (a dialog, for instance), the runner stops and
leaves the test build in place; close Archicad and run it again to restore your add-on.

## Setup (once)

1. **Load Tapir from a folder you can write to.** The runner swaps the add-on file, and
   `Program Files` needs administrator rights. Put the release add-on into e.g.
   `C:\Users\me\TapirRunner\AC28\`, add it in Archicad under *Options > Add-On Manager >
   Edit List of Available Add-Ons*, and remove the copy from `Program Files` so only one Tapir loads.
2. **A blank template:** save an empty project as `.tpl`. Archicad opens a template as a new,
   untitled project, and the checks only run in an untitled project without walls, slabs,
   stairs, roofs or zones.
3. **A GitHub token** (classic, scope `public_repo`) in the environment variable
   `TAPIR_RUNNER_TOKEN`. It downloads builds and posts the comments in your name.
4. Copy `runner.example.json` to `runner.json` and fill in your paths and ports.
5. **When it runs** is up to you. It works through whatever is pending each time:
   - at logon: `schtasks /create /tn TapirTestRunner /sc onlogon /tr "pyw <path>\tapir_test_runner.py once --config <path>\runner.json"`
   - on demand: a shortcut to the same command;
   - continuously: `tapir_test_runner.py poll --config runner.json`.

## Writing checks

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
- `minTapir` skips the file on older add-ons. `requiresEmpty` changes the element types that
  must be absent.

Run checks against an Archicad you have open, before pushing them:

```
python tapir_test_runner.py check checks/*.json --port 19723
```

Try a build you downloaded by hand, with the whole install, start, check, quit and restore
cycle (the configured Archicad must be closed):

```
python tapir_test_runner.py trial --config runner.json --ac 26 --build TapirAddOn_AC26_Win.apx checks/*.json
```

`checks/` holds examples taken from the October 2026 verification of #747-#753.

## Safety

The runner installs native code from a pull request. Hence:

- only pull requests whose branch is in this repository are tested (`allowForks` is off);
- only builds made by this repository's own build check are installed;
- checks are data. The runner itself is the only code that runs; a check can send Tapir
  commands to an untitled, empty project and nothing else. Expressions allow `+ - * /` only;
- use a separate Windows account for the runner if you can.

## Verified so far (2026-10-16, Windows)

- `check` on AC26 and AC28 (Tapir 1.7.0/1.7.1): all six example files run, clean up after
  themselves and report exactly the issues still open in the release.
- `trial` on AC26, the full cycle with the release build: install, start with a stock `.tpl`
  (opens as an untitled project), 57 checks, `QuitArchicad` (quits in seconds even with a changed
  untitled project, no save dialog), restore. About two minutes.
- GitHub side of `once`: finding pull requests, reading their check files, downloading the
  build check's artifact for the head commit.

## Not verified yet

- `once`/`poll` end to end: no pull request carries the `needs-test` label yet, and bot pull
  requests get no build today; the build check has to be dispatched for them.
- macOS.
