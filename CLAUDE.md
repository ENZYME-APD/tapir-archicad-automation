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
  Full command list: <https://enzyme-apd.github.io/tapir-archicad-automation/archicad-addon>.
- **Tapir Grasshopper Plugin** — in [grasshopper-plugin/](grasshopper-plugin/). A
  Grasshopper (Rhino) plugin that exposes the above commands as visual components so
  non-programmers can use them. Work in progress.

The Add-On uses the **Archicad API (DevKit)**. The API interface headers live under
[archicad-addon/Build/DevKits/](archicad-addon/Build/DevKits/) — e.g.
`archicad-addon/Build/DevKits/AC29/Support` for Archicad 29, with one folder per
supported version.

**Supported Archicad versions:** [archicad-addon/devkits.json](archicad-addon/devkits.json)
is the one list of them, each with its DevKit release. The build checks, the release,
the weekly release's asset count, the API coverage, the bots' DevKit download,
`build_all_win.bat` and the docs generator all read it through
[tools/devkits.py](tools/devkits.py). A new Archicad version needs no person:
`.github/workflows/archicad_devkit_update.yml` adds it there when Graphisoft publishes
its DevKit, and the bot pipeline builds, fixes and merges it (see CI). Never change a
listed version's release on a whim: add-ons built with a later DevKit build may need a
later Archicad update.

The Add-On code is based on Tibor Lorantfy's original
[archicad-additional-json-commands](https://github.com/tlorantfy/archicad-additional-json-commands).

The current version is defined in
[archicad-addon/Sources/AddOnVersion.hpp](archicad-addon/Sources/AddOnVersion.hpp)
and [tools/package_info.json](tools/package_info.json) — keep these in sync.
It is bumped automatically after each release by the weekly release
workflow, so it is not repeated here.

## Repository layout

```
archicad-addon/          C++ Archicad Add-On
  devkits.json           The supported Archicad versions and their DevKit releases
  Sources/               Command implementations (*.cpp/*.hpp), one file per command group
  Build/                 CMake build output + DevKits/ (downloaded Archicad API SDKs)
  Tools/                 Build/packaging scripts (CMake helpers, resource compiler, signing),
                         legacy auto-update script (update_addon_and_restart_archicad.py)
  Examples/              Python usage examples (one .py per feature) + aclib/ helper
  Test/                  test_examples.py runs Examples against TestProject.pla
  Installer/             Cross-platform end-user installer (tapir_installer.py, PyInstaller-packaged in CI)
  README.md
grasshopper-plugin/      C# Grasshopper plugin (.NET, produces .gha), packaged via Yak
  TapirGrasshopperPlugin/  Components/, Types/, Helps/, Resources/,
                         ToolSettings/ (API field -> Archicad GUI label tables per tool)
builtin-scripts/         Bundled automation scripts
branding/                Logos, diagrams
docs/                    Generated docs (archicad-addon command reference),
                         api-coverage/ page of the Archicad API coverage checklist
tools/                   Version bump scripts (update_version.py), package_info.json,
                         Add-On docs generator (generate_addon_docs.py),
                         command scaffolder (new_command.py),
                         Grasshopper GUI label table checker (check_tool_settings.py),
                         API coverage generator (generate_api_coverage.py,
                         fetch_api_coverage_github_data.py, api_coverage_rules.json)
                         Archicad version list reader and updater (devkits.py)
  discord-issue-bot/     Discord -> GitHub issue bot (run by .github/workflows/discord_issue_bot.yml)
sandbox/                 Experiments / scratch
.github/workflows/       CI: build checks + release pipelines for both components
```

## Archicad Add-On architecture

- **Entry point:** [archicad-addon/Sources/AddOnMain.cpp](archicad-addon/Sources/AddOnMain.cpp).
  `Initialize()` registers every command, grouped into `CommandGroup`s (Application,
  Project, Element, Attribute, Property, Classification, Navigator, Issue, Revision,
  Design Options, IFC, Library, Teamwork, Favorites, Solid Element Operation, etc.).
  Each `RegisterCommand<T>(group, version, description)` call also records the version
  the command was introduced and a human description used for the generated docs.
- **Commands** are grouped by domain into paired `*.cpp`/`*.hpp` files in `Sources/`
  (e.g. `ElementCommands`, `ElementCreationCommands`, `AttributeCommands`,
  `NavigatorCommands`, `PropertyCommands`, `IFCCommands`). All commands live in the
  `TapirCommand` namespace and are invoked via the Archicad JSON interface.
- **CommandBase** ([Sources/CommandBase.hpp](archicad-addon/Sources/CommandBase.hpp))
  is the base class. It defines the namespace, execution policy, and schema hooks, and
  provides many shared helpers — GUID <-> ObjectState conversion, coordinate/polygon
  helpers, story/floor-index resolution (`GetStories`, `ResolveFloorIndexAndOffset`),
  attribute lookup, and `Create*Response`/`Create*ExecutionResult` builders. Reuse these
  helpers when adding commands; match the existing style.
- **Schemas:** input/response JSON schemas are defined per command
  (`GetInputParametersSchema` / `GetResponseSchema`) plus shared definitions in
  [Sources/SchemaDefinitions.cpp](archicad-addon/Sources/SchemaDefinitions.cpp).
- **UI:** the Add-On adds a menu + palette ([TapirPalette](archicad-addon/Sources/TapirPalette.cpp)),
  an About dialog, and an auto-update/version check ([VersionChecker](archicad-addon/Sources/VersionChecker.cpp)).
- **Auto-update:** the update is done by the Tapir Installer (see Installation). The
  Add-On downloads the installer of the latest release and starts it in update mode
  (`--addOnFile <own add-on> --versions <AC version> --archicadPort <port> --archicadPid <pid>`);
  the installer quits Archicad, replaces the add-on and starts Archicad again.
  Already released Add-Ons instead download
  [Tools/update_addon_and_restart_archicad.py](archicad-addon/Tools/update_addon_and_restart_archicad.py)
  from `main` and run it with uv (`--port`, `--downloadUrl`, `--addOnLocation`), so
  never move or rename that script, keep it standard-library only and keep its options.
  It hands off to the installer of the release it updates to when that installer has
  update mode. It detects update mode from `archicad-addon/Installer/tapir_installer.py`
  at the release tag on raw.githubusercontent.com (the release workflow builds the
  installer from the tagged commit), by the quoted `'--addOnFile'` option in the
  installer's argument parser. So neither the `'--addOnFile'` option name nor that
  file's path may change without updating the script on `main`. It updates
  the add-on itself (download and file signature check first, write check, quit,
  replace, restart) for releases without update mode, when that check cannot be made,
  on platforms without an installer, when the installer cannot be downloaded or
  started, and when the administrator prompt was declined but the add-on is writable
  without it. It is not a development tool. The script and the installer's update
  mode run one update per Archicad by locking the first byte of `TapirUpdate_<port>.lock`
  in the temp folder, so keep that name in both. After that byte both the script and
  the installer's update mode write `quitting` while they wait for Archicad to quit
  and `updating` otherwise; a second update, by either of them, tells the user to
  quit Archicad only when it reads exactly `quitting`, because quitting while the
  first update still downloads makes it fail. Keep the offset, the states and the
  texts the same in both (test_update_mode.py checks they match).

### Adding or changing a command

[tools/new_command.py](tools/new_command.py) writes the repetitive part of steps 1–4 for new
commands from a short spec (format in its docstring): the class, schemas, an `Execute` skeleton,
the registration and a starter example. The Archicad API calls in `Execute` and any
`Test/ExpectedOutputs/` are still written by hand.

1. Add/modify the command class in the appropriate `Sources/*Commands.{cpp,hpp}`
   (or create a new group file and include it in `AddOnMain.cpp`).
2. Implement input/response schemas; reuse `SchemaDefinitions` and `CommandBase` helpers.
3. Register it in `AddOnMain.cpp` `Initialize()` with the version it is introduced in
   and a clear description.
4. Add a Python example under [archicad-addon/Examples/](archicad-addon/Examples/) and,
   if it should be tested, expected output under `Test/ExpectedOutputs/`.
5. Docs at <https://enzyme-apd.github.io/...> are generated from the registered
   descriptions and schemas by `python tools/generate_addon_docs.py` (no Archicad
   needed; same output as the `GenerateDocumentation` developer command). The weekly
   release regenerates them automatically; run it yourself to update them earlier.
6. Bump the version consistently (see Versioning below) when releasing.

## Building the Add-On

Requires **CMake ≥ 3.17**, **Python** (for DevKit download + resource compilation),
and Visual Studio (Windows) / Xcode (macOS). The Archicad API DevKits are downloaded
automatically by the build scripts.

Windows, all supported versions (downloads DevKits, then builds every version in `devkits.json`):

```bat
cd archicad-addon\Tools
build_all_win.bat
```

Single version manually (example, AC29):

```bat
cd archicad-addon
python Tools\download_and_unzip.py <devkit-url> Build\DevKits\AC29
cmake -B Build/AC29 -G "Visual Studio 18 2026" -A x64 -T v143 -DAC_VERSION=29 -DAC_API_DEVKIT_DIR="Build\DevKits\AC29\Support" .
cmake --build Build/AC29 --config RelWithDebInfo
```

Notes:
- `AC_VERSION` selects the target Archicad major version; `AC_API_DEVKIT_DIR` must
  point at the matching DevKit's `Support` folder.
- Toolset: `v142` before AC29, `v143` from AC29 (see `build_all_win.bat`).
- Build config produces `TapirAddOn_AC<version>_<Win|Mac>` (`.apx` on Windows,
  `.bundle`/`.zip` on macOS).

## Testing the Add-On

Tests are Python-based and run the example scripts against a running Archicad with
the built Add-On loaded, comparing to golden outputs:

```bash
cd archicad-addon/Test
python test_examples.py
```

- Requires the official `archicad` Python package (`ACConnection`) and a running
  Archicad instance; `test_examples.py` opens `TestProject.pla`, runs each script in
  [Examples/](archicad-addon/Examples/), and diffs against `ExpectedOutputs/`.
- Example scripts talk to the Add-On via `ExecuteAddOnCommand(AddOnCommandId('TapirCommand', <CommandName>), params)`.
- `aclib/` under `Examples/` is a small shared helper used by the example scripts.
- Pull requests and local builds are tested on real Archicad with
  [tools/community-test-runner/](tools/community-test-runner/) (JSON checks in
  `archicad-addon/Test/checks/`). Use its `--brief` output (`prs`, `test … --brief --no-post`,
  `trial … --brief`, `check --brief`) rather than reading reports or its source; its skill is
  `tools/community-test-runner/claude-skill/tapir-pr-test/SKILL.md`.

## Grasshopper plugin

- C# / .NET project ([grasshopper-plugin/TapirGrasshopperPlugin/](grasshopper-plugin/TapirGrasshopperPlugin/)),
  multi-targeting `net7.0-windows;net7.0;net48` (Rhino 6/7/8), output extension `.gha`.
- Components are organized by domain under `Components/` (Elements, Attributes,
  Properties, Classifications, Navigator, Issues, Teamwork, Favorites, Project, etc.),
  mirroring the Add-On command groups. Uses `Newtonsoft.Json` for the JSON interface.
- Build with `dotnet build` / Visual Studio using
  [TapirGrasshopperPlugin.sln](grasshopper-plugin/TapirGrasshopperPlugin.sln).
- Packaged for the Rhino Package Manager (Yak) as **tapir**; see
  [grasshopper-plugin/YakPackage/](grasshopper-plugin/YakPackage/) and
  `create_package_win.bat`.

### Naming from the Archicad GUI

Every Grasshopper component uses the names a user sees in Archicad, not the API names.
- An input, output or JSON key that stands for a setting of an Archicad tool is named after
  the English label of that setting's control in the tool's settings dialog, e.g. the wall's
  `begThickness` gets the label of its thickness control. This applies to new and changed inputs
  and outputs; existing ones are renamed when their tool component (below) replaces them.
- The labels, with the dialog panel (Group) and the panel's subheading (Subgroup), are kept per
  tool in [grasshopper-plugin/TapirGrasshopperPlugin/ToolSettings/](grasshopper-plugin/TapirGrasshopperPlugin/ToolSettings/)
  (`<Tool>.json`, e.g. `Wall.json`), checked by `python tools/check_tool_settings.py`. Take the
  labels from there; code only uses that copy.
- Never invent a label. Add a label to the table only when it is found verbatim on Graphisoft's help
  page for that settings dialog (help.graphisoft.com, the Archicad version the table names: its text
  or its dialog screenshots) or in a DevKit comment that names the control, and name the page and
  screenshot in `labelSource`; otherwise leave it `null`, and the API name is used. Capitalization
  follows the dialog (Title Case).
- Labels are unique per tool, case-insensitively, because the JSON is translated back to API
  names by label. A field belongs to exactly one Group.

### Archicad tool components (target architecture)

Elements are handled by **one component per Archicad tool** (Walls, Slabs, Columns, …) that
mirrors the tool's settings dialog. It is introduced tool by tool, starting with Walls; new work on
an element tool follows these rules:
- **Name and place:** the plural tool name ("Walls"), in the group Graphisoft puts the tool in:
  `GroupNames.DesignTools`, `DocumentTools` or `ViewpointTools` ("Design Tools", …; to be added
  to `GroupNames.cs` with the first tool component).
- **Three buttons, one set of JSON objects:** *Refresh* reads (`GetDetailsOfElements`), *Execute*
  writes, *Create* makes new elements (the tool's `Create<Type>` command, starting from the tool
  defaults or the `FavoriteName` input). Writing and creating only happen on a button press; the
  component ignores `AutoExecute`, only *Refresh* may run automatically. Clear the request flags
  inside `SolveInstance`, where they are used, not after `ExpireSolution (true)`.
- **One JSON input and output per dialog panel:** the Group name without spaces + `_JSON`
  (`GeometryAndPositioning_JSON`, `FloorPlanAndSection_JSON`, `Model_JSON`,
  `ClassificationAndProperties_JSON`), keys are the GUI labels from the table. Only the keys given are
  written (partial updates), arrays are replaced, not merged. With N elements each JSON input holds
  0 (unchanged), 1 (all) or N items; anything else is an error.
- **Plain inputs/outputs** for what is not on a dialog panel: `ElementGuid`, `ElementID`,
  `LayerIndex`, `DrawOrder`; and **every input that defines the geometry** (drawn in Archicad), e.g.
  `StartPoint`/`EndPoint` for walls, which override the JSON on *Create*.
- **Execute and Create accept the same keys:** on the add-on side they share one apply-function
  (walls: `ApplyWallDetails`, used by `CreateWalls` and `ModifyWalls`; `SetDetailsOfElements`'s
  wall case only takes coordinates, height, offset and the thicknesses of trapezoid walls).
- **Delete is never part of a tool component**; it stays in the separate `DeleteElements`
  component.
- A `<Tool>_API` twin with the raw API names may exist for testing.
- The old per-command components stay until the tool component reaches parity, then get
  `Exposure` `GH_Exposure.hidden` for one release and are removed in the next. Keep a component's
  GUID when moving it. Archicad attributes (building materials, surfaces, composites, …) will follow
  the same pattern later.

## Versioning

Version lives in multiple places that must stay in sync:
- [archicad-addon/Sources/AddOnVersion.hpp](archicad-addon/Sources/AddOnVersion.hpp) (`ADDON_VERSION`)
- Grasshopper `.csproj` `<Version>`
- [tools/package_info.json](tools/package_info.json)

Use the helper to bump: [tools/update_version.py](tools/update_version.py)
(or `tools/update_version.bat`). Commands also carry the version they were introduced
in, as the second argument to `RegisterCommand<>` in `AddOnMain.cpp`: use the version
`main` currently carries (in `tools/package_info.json`).

`main` always carries the next patch version (1.6.1 after 1.6.0). The weekly release
ships it as a patch release, except that the first release of a calendar month is a
minor release (1.6.1 ships as 1.7.0), and a release after a merged PR labeled
`major-release` is a major one (1.6.1 ships as 2.0.0). For those two the release
workflow rewrites the version files and the commands registered with the skipped
version, so new commands keep using the version `main` carries. Label a PR
`major-release` only for a significant change, not for a new command or
Grasshopper component.

## CI

GitHub Actions in [.github/workflows/](.github/workflows/):
- `archicad_addon_build_check.yml`, `grasshopper_plugin_build_check.yml`,
  `installer_build_check.yml` — PR build checks for each component, also run
  on pushes to `main` and by `workflow_dispatch` (which the bot PR steward
  uses); a newer push to a pull request cancels its superseded runs. The installer
  check also runs `Installer/test_update_mode.py` and syntax-checks the legacy
  auto-update script.
- `archicad_addon.yml`, `grasshopper_plugin.yml` — release/publish pipelines.
  `archicad_addon.yml` also builds the Tapir Installer executables
  (`TapirInstaller_Win.exe`, `TapirInstaller_Mac.zip`); `weekly_release.yml`
  expects two assets per version in `devkits.json` plus the two installers
  (`EXPECTED_ASSETS`). The Mac installer is
  universal2 (Intel and Apple silicon): both workflows build it with the
  python.org Python pinned in their `env` (keep the two the same) and check it
  with `lipo`. The Windows installer is code signed by SignPath (certificate by
  SignPath Foundation): on a release tag `build_installer_win` waits for a manual
  approval in SignPath, then `release_installer_win` attaches the signed file to
  the draft (split so SignPath never holds a write token); the weekly release
  waits for both. A failed or unapproved signing never blocks a release: the
  unsigned installer is attached instead, and the release notes say so (the
  SignPath footer only appears on signed releases). SignPath requires the exe's ProductName `Tapir` and
  ProductVersion = the release version, which `Installer/write_version_file.py`
  writes; the installer build tools are pinned in
  `Installer/requirements-build.txt`, used by both workflows. The README's
  "Code signing policy" section is required by the SignPath Foundation.
- `weekly_release.yml` — tags and releases `main` every Monday when it changed
  (see Versioning for which version it ships); first regenerates
  `docs/archicad-addon` with `tools/generate_addon_docs.py` and pushes it to `main`
  when it changed (never hand-edit those docs).
- `claude_issue_triage.yml` — the issue bot. Every newly opened issue (`fix` job) and
  every new human comment on an issue (`reply` job), from anyone, runs Claude with
  write access: it answers, closes verified already-fixed issues, or opens a
  ready-for-review PR on a `claude/issue-<n>-…` branch with `Fixes #<n>`, and asks
  the issue's author (tagged) to verify it in Archicad. An issue too large for
  one PR gets a split plan (`<!-- claude-split-plan: parts=N -->`, at most six
  parts) and one PR per part, opened one run at a time (`next-part` job) on
  `claude/issue-<n>-part<k>-…` branches with `Part <k> of <N> of #<n>`. No label
  is needed; applying `claude-fix` re-runs `fix` on an existing issue, and a
  `workflow_dispatch` with an `issue_number` runs it on that issue. Its header
  documents the security bounds and the repository rulesets they rely on.
- `claude_pr_review.yml` — the PR review bot. Reviews on open/push and re-reviews on
  any human PR comment, without re-posting open findings, and resolves the threads it
  opened once the change or the discussion settles them. On the issue bot's PRs it
  also records its verdict as the `claude-review` commit status (set by a job
  without a model, from a nonce-tagged marker in the review).
- The Claude jobs of `claude_issue_triage.yml` (`fix`, `reply`),
  `claude_bot_pr_steward.yml` (`fix`) and `claude_pr_review.yml` get the DevKit
  headers of every version in `devkits.json`, and the newest DevKit's Examples, in
  `archicad-addon/Build/DevKits/` before Claude starts, from
  [.github/actions/download-devkits](.github/actions/download-devkits/action.yml).
  The steward and the review load that action from a checkout of the workflow's own
  commit, never from a pull request's branch; they read the pull request's
  `devkits.json` (data only), so a PR adding a version has its headers.
- `archicad_devkit_update.yml` — daily: when Graphisoft has published the DevKit of
  an Archicad version newer than the newest in `devkits.json` (its first public
  build, x.3000 or later, with Windows and macOS zips), `tools/devkits.py update` adds
  it there and to the README's download table, and this opens a bot PR on a
  `claude/devkit-…` branch and starts the steward. That PR touches no release
  critical path, so the steward builds it, fixes what the new DevKit breaks, and
  `claude_auto_merge.yml` merges it like any bot PR. `DEVKIT_UPDATE=false` turns it
  off.
- `claude_bot_pr_steward.yml` — drives the issue bot's PRs to mergeable without a
  maintainer (their `GITHUB_TOKEN` pushes start no runs): an hourly sweep, also
  started by the issue bot and the review, dispatches the build checks and the
  review on each new head, links the built add-on in a comment asking the
  issue's author to test it in Archicad, and starts a `fix` run on a failed build,
  a blocking review or a conflict with `main` (which it merges in), at most
  `BOT_PR_MAX_FIX_ROUNDS` (default 6) times, on at most `BOT_PR_SWEEP_BATCH`
  (default 15) PRs per sweep. Then
  it labels the PR `needs-maintainer` and stops; removing the label hands the PR
  back with its fix round and review counts reset. `BOT_PR_STEWARD=false` turns it
  off; `needs-maintainer`, `do-not-merge` or draft takes a PR out of its hands.
- Model tiers of these three Claude workflows, from repository variables
  (aliases, so each tier follows new models): `CLAUDE_LIGHT_MODEL` (default
  `sonnet`) for issue replies and PR re-reviews, `CLAUDE_MODEL` (default `opus`)
  for issue fixes, first reviews and steward fix rounds, and
  `CLAUDE_ESCALATION_MODEL` (default `best`) only for the last steward fix round
  and one retry of an issue fix that did work but left no comment or PR.
- The subscription's usage limit (#977): at most `CLAUDE_MAX_RUNS` (default 8)
  Claude runs of these workflows call the model at a time, oldest first; a run
  the API refuses on its first turn fails in its "Stop on a refused run" step
  with the error in its job summary; the hourly `retry` job of
  `claude_issue_triage.yml` starts refused issue fixes again (`CLAUDE_RETRY_BATCH`
  per hour, `CLAUDE_RETRY=false` turns it off), and the steward neither counts
  failed review or fix runs as rounds nor starts one within 50 minutes of a
  failure, nor a review within 50 minutes of one that recorded no verdict on
  the same head (`BOT_PR_MAX_EMPTY_REVIEWS`, default 8, such reviews hand the
  PR over). A review posts its summary from `.review-context/summary.md`, and
  a step after it posts that file when the review's own `gh pr comment` did
  not, so a refused command no longer loses the verdict.
- `claude_auto_merge.yml` — merges green, non-draft PRs by write-access human
  authors that touch no release-critical path. It also merges the issue bot's
  PRs (`claude/` branches) with no human interaction once every build check is
  green and `claude-review` has been a success for `BOT_AUTO_MERGE_DELAY_HOURS`
  (default 72), with no open thread by a person, no changes requested and no
  `do-not-merge`/`needs-maintainer` label; `BOT_AUTO_MERGE=false` turns that off.
  This is a deliberate maintainer decision to minimize human maintenance.
- `discord_issue_bot.yml` — files GitHub issues from bug reports and feature
  requests in the project's Discord channels, then dispatches
  `claude_issue_triage.yml` for each (issues created with `GITHUB_TOKEN` trigger
  no workflow on their own; see
  [tools/discord-issue-bot/README.md](tools/discord-issue-bot/README.md)).
- `api_coverage.yml` — keeps the Archicad API coverage checklist live: on a push
  to `main`, a release, pull request and issue events, hourly and by hand, it runs
  `tools/generate_api_coverage.py` over the DevKits in `devkits.json` (C functions with
  their older names and AC25/26 command ids, and the C++ `ACAPI::`/`IFCAPI::` API)
  and marks each item used on `main`, merged but not released, in an open PR,
  claimed by an open issue (the "Archicad API request" issue form), open, skipped
  or not applicable. It publishes `api-coverage.json`/`.md` to the `api-coverage`
  branch (never to `main`), read by `docs/api-coverage/index.html`, with the
  README's badge (`api-coverage-badge.json`, a shields.io endpoint) and summary
  image (`api-coverage.svg`, `api-coverage-dark.svg`), so the README never needs
  an edit when the coverage changes; it also writes the
  summary into the issue numbered in the `API_COVERAGE_ISSUE` repository variable
  when set; `API_COVERAGE=false` turns it off. It uses `pull_request_target`, so it
  must never check out or run pull request code. Maintainers mark items skipped,
  with a reason, in the `skip` section of `tools/api_coverage_rules.json`; the
  `notApplicable` section lists what cannot be a JSON command.

## Installation (end users)

- **Add-On (recommended):** download and run the Tapir Installer
  (`TapirInstaller_Win.exe` / `TapirInstaller_Mac.zip`) from
  [Releases](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest);
  it detects the installed Archicad versions and installs the matching Add-On
  into their `Add-Ons` folders (source: [archicad-addon/Installer/](archicad-addon/Installer/)).
  The Add-On's auto-update runs the same installer in update mode (`--addOnFile`),
  which replaces exactly the running add-on.
- **Add-On (manual):** download the matching `TapirAddOn_AC<version>_<Win|Mac>` file from
  [Releases](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest),
  then in Archicad: *Options > Add-On Manager > Edit List of Available Add-Ons > Add*,
  browse to the file, OK.
- **Grasshopper plugin:** install the **tapir** package from the Rhino Package Manager.

## Conventions & tips for changes

- Match the surrounding C++ style (Graphisoft/GS naming: `GS::`, `API_`, spaces before
  parentheses in calls, etc.). Prefer the existing `CommandBase` helpers over reimplementing.
- Keep command input/response schemas, the C++ implementation, the registered
  description in `AddOnMain.cpp`, and a Python example consistent when adding features.
- Some commands are version-gated (e.g. Design Options "Available from Archicad 29");
  guard version-specific API usage with the appropriate `ServerMainVers_*` / `AC_VERSION`
  checks as done elsewhere.
- Cross-platform: code must compile on both Windows and macOS; resources are compiled
  via `Tools/CompileResources.py`.

## Documentation & links

- Add-On JSON command reference: <https://enzyme-apd.github.io/tapir-archicad-automation/archicad-addon>
- Archicad API coverage checklist: <https://enzyme-apd.github.io/tapir-archicad-automation/api-coverage/>
  (data on the `api-coverage` branch, generated by `.github/workflows/api_coverage.yml`)
- Developer docs (wiki): <https://github.com/ENZYME-APD/tapir-archicad-automation/wiki>
- Add-On development guide: <https://github.com/ENZYME-APD/tapir-archicad-automation/wiki/Archicad-Add-On-Development>
- Roadmap: <https://github.com/orgs/ENZYME-APD/projects/4/views/1>
- Community: Discord (badge/link in [README.md](README.md))
