# CLAUDE.md

Guidance for Claude Code (and other AI assistants) working in this repository.

## Project overview
This repository contains the **Tapir Archicad automation package** — an open-source
project (by ENZYME-APD) that extends Graphisoft Archicad with additional JSON
automation commands and tooling. Upstream: <https://github.com/ENZYME-APD/tapir-archicad-automation>.

It consists of two main components:

- **Tapir Archicad Add-On** — in [archicad-addon/](archicad-addon/). A C++ Archicad
  Add-On that registers many new JSON commands on top of the official commands
  provided by Graphisoft. Ready to use; see installation instructions below.
  Full command list: <https://archicad-api.com/archicad-addon/>.
- **Tapir Grasshopper Plugin** — in [grasshopper-plugin/](grasshopper-plugin/). A
  Grasshopper (Rhino) plugin that exposes the above commands as visual components so
  non-programmers can use them. Work in progress.

Read these files of [maintaining/](maintaining/), which hold the rest of this file word for word, when the work needs them:
- [maintaining/addon.md](maintaining/addon.md): before using the DevKit headers or `devkits.json`, or when you need the add-on's structure, a one-version build or a test in real Archicad.
- [maintaining/ci.md](maintaining/ci.md): before changing a workflow under `.github/` or a bot, or when a check or a bot run fails.
- [maintaining/release.md](maintaining/release.md): before bumping a version, labelling a PR `major-release` or changing a release workflow.
- [maintaining/grasshopper.md](maintaining/grasshopper.md): before changing anything under `grasshopper-plugin/`.
- [maintaining/marketplace.md](maintaining/marketplace.md): before adding or changing a script in `marketplace/` or `builtin-scripts/`.
- [maintaining/auto-update.md](maintaining/auto-update.md): before changing the installer, the add-on's auto-update or `update_addon_and_restart_archicad.py`.
- [maintaining/website.md](maintaining/website.md): before changing a page under `docs/` or the limits page, or for the project's links.
- [maintaining/runbook.md](maintaining/runbook.md): to release, merge or turn the bots off by hand, with no bot running.

## Repository layout
```
archicad-addon/          C++ Archicad Add-On
  devkits.json           The supported Archicad versions and their DevKit releases
  Sources/               Command implementations (*.cpp/*.hpp), one file per command group
  Build/                 CMake build output + DevKits/ (downloaded Archicad API SDKs)
  Tools/                 Build/packaging scripts (CMake helpers, resource compiler, signing),
                         legacy auto-update script (update_addon_and_restart_archicad.py)
  Examples/              One example per command: <GroupFolder>/<CommandName>.py (+ an aclib.py
                         shim per folder) and the aclib/ helper; shown in the command docs
  Workflows/             Scripts combining several commands into a task or tool
  Test/                  test_examples.py runs Scripts/ (regression tests, goldens in
                         ExpectedOutputs/) and smoke-tests Examples against TestProject.pla;
                         Scripts/manual/ are tests run by hand
  Installer/             Cross-platform end-user installer (tapir_installer.py, PyInstaller-packaged in CI)
  README.md
grasshopper-plugin/      C# Grasshopper plugin (.NET, produces .gha), packaged via Yak
  TapirGrasshopperPlugin/  Components/, Types/, Helps/, Resources/,
                         ToolSettings/ (API field -> Archicad GUI label tables per tool)
builtin-scripts/         Bundled automation scripts
marketplace/             Shared scripts, one <slug>/ folder each (format in its README.md)
branding/                Logos, diagrams, WEBSITE_STYLE.md (the website's style rules)
docs/                    Generated docs (archicad-addon command reference),
tools/                   Version bump scripts (update_version.py), package_info.json,
sandbox/                 Experiments / scratch
.github/workflows/       CI: build checks + release pipelines for both components
```

## Schemas
- **Schemas:** input/response JSON schemas are defined per command
  (`GetInputParametersSchema` / `GetResponseSchema`) plus shared named definitions in
  [Sources/RFIX/Images/CommonSchemaDefinitions.json](archicad-addon/Sources/RFIX/Images/CommonSchemaDefinitions.json)
  (loaded by [Sources/SchemaDefinitions.cpp](archicad-addon/Sources/SchemaDefinitions.cpp)), which a
  schema references with `{"$ref": "#/<Name>"}`. **Shared shapes are named definitions, never inline
  copies**, so they can't drift apart:
  - An object shape that more than one command uses, or that describes an element, attribute,
    view or other Archicad item (its details, settings, a geometry, a status), is a definition in
    `CommonSchemaDefinitions.json`, and every command that uses it refers to it with `$ref`.
  - Adding or changing a field of a shared shape changes its definition, so every command that
    uses it gets the field. Never add the field to one inline copy.
  - A change that touches an inline schema whose shape already has a definition (or a second
    command's inline copy of the same shape) replaces it with the `$ref` in the same change. A new
    command's own object shape becomes a definition as soon as a second command needs it.
  - Only parameters that are specific to one command (its own flags, a one-off option object)
    stay inline. Name definitions in PascalCase after what they describe (`WallDetails`,
    `Hole2D`), and keep their descriptions as complete as the inline text was.
  - Moving a schema into a definition must not change what the command accepts or returns. Check
    it with `python tools/generate_addon_docs.py` and then `python tools/expand_schemas.py --out`
    before and after the change, and `python tools/expand_schemas.py --compare before.json after.json`:
    the expanded schemas must be the same (the generator writes the `$ref`s as they are; only
    `expand_schemas.py` and the docs page in the browser expand them).

### Adding or changing a command
[tools/new_command.py](tools/new_command.py) writes the repetitive part of steps 1–5 for new
commands from a short spec (format in its docstring): the class, schemas, an `Execute` skeleton,
the registration, a starter example and a starter check. The Archicad API calls in `Execute`,
the check's real parameters and expectations, and any `Test/ExpectedOutputs/` are still written
by hand.

1. Add/modify the command class in the appropriate `Sources/*Commands.{cpp,hpp}`
   (or create a new group file and include it in `AddOnMain.cpp`).
2. Implement input/response schemas; reuse the shared definitions (`$ref`, see *Schemas* above,
   and add one for a shape a second command uses) and the `CommandBase` helpers.
3. Register it in `AddOnMain.cpp` `Initialize()` with the version it is introduced in
   and a clear description.
4. Add its example as `archicad-addon/Examples/<GroupFolder>/<CommandName>.py`: a module
   docstring (shown as the example's description in the docs' *Example* popdown, which loads the
   file from `main`) and a short script calling the command through `aclib`; the convention and
   the `<GroupFolder>` rule (group name without "Commands" and parentheses, PascalCase, e.g.
   `CustomData`, `MEP`) are in [archicad-addon/Examples/README.md](archicad-addon/Examples/README.md),
   and `python tools/check_examples.py` checks them. A script that tests behaviour goes to
   `archicad-addon/Test/Scripts/` (with its golden under `Test/ExpectedOutputs/` if
   `test_examples.py` should compare it), a multi-command task to `archicad-addon/Workflows/`.
5. Add or update a **check** for every command you add or whose behaviour you change:
   `archicad-addon/Test/checks/<snake_case command>.json`, the commands the
   [community test runner](tools/community-test-runner/) sends to an empty, untitled Archicad
   project and the answers they must give (format: its README, "Writing checks"). It is how a
   pull request gets run in real Archicad before it is merged: testers' runners test what is
   labelled `needs-test`, which the bot PR steward sets on a bot pull request once its builds are
   green and it has a check (see CI). Make the elements it needs in its own steps, and make the
   expectations catch the bug or prove the feature, not only that the command answers.

## Building the Add-On
Requires **CMake ≥ 3.17**, **Python** (for DevKit download + resource compilation),
and Visual Studio (Windows) / Xcode (macOS). The Archicad API DevKits are downloaded
automatically by the build scripts.

Windows, all supported versions (downloads DevKits, then builds every version in `devkits.json`):

```bat
cd archicad-addon\Tools
build_all_win.bat
```

## Testing the Add-On
Tests are Python-based and run scripts against a running Archicad with the built Add-On
loaded, comparing to golden outputs:

```bash
cd archicad-addon/Test
python test_examples.py
```

- Requires the official `archicad` Python package (`ACConnection`) and a running
  Archicad instance; `test_examples.py` reopens `TestProject.pla` before each script, runs
  every script in [Test/Scripts/](archicad-addon/Test/Scripts/) (diffed against
  `ExpectedOutputs/<name>.py.output`) and every example in [Examples/](archicad-addon/Examples/)
  as a smoke test (`ExpectedOutputs/Examples/<GroupFolder>/<name>.py.output`, recorded on the first
  run; `EXAMPLES_NOT_SMOKE_TESTED` lists the examples that cannot run unattended). Names given on
  the command line run only those; `record_expected_output.py <name>.py` records one golden.
  `Test/Scripts/manual/` is not run.

## Conventions & tips for changes
- Match the surrounding C++ style (Graphisoft/GS naming: `GS::`, `API_`, spaces before
  parentheses in calls, etc.). Prefer the existing `CommandBase` helpers over reimplementing.
- Some commands are version-gated (e.g. Design Options "Available from Archicad 29");
  guard version-specific API usage with the appropriate `ServerMainVers_*` / `AC_VERSION`
  checks as done elsewhere.
- Cross-platform: code must compile on both Windows and macOS; resources are compiled
  via `Tools/CompileResources.py`.
