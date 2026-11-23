# Archicad Add-On: more details

The Add-On uses the **Archicad API (DevKit)**. The API interface headers live under
[archicad-addon/Build/DevKits/](../archicad-addon/Build/DevKits/) — e.g.
`archicad-addon/Build/DevKits/AC29/Support` for Archicad 29, with one folder per
supported version.

**Supported Archicad versions:** [archicad-addon/devkits.json](../archicad-addon/devkits.json)
is the one list of them, each with its DevKit release. The build checks, the release,
the weekly release's asset count, the API coverage, the bots' DevKit download,
`build_all_win.bat` and the docs generator all read it through
[tools/devkits.py](../tools/devkits.py). A new Archicad version needs no person:
`.github/workflows/archicad_devkit_update.yml` adds it there when Graphisoft publishes
its DevKit, and the bot pipeline builds, fixes and merges it (see CI). Never change a
listed version's release on a whim: add-ons built with a later DevKit build may need a
later Archicad update.

The Add-On code is based on Tibor Lorantfy's original
[archicad-additional-json-commands](https://github.com/tlorantfy/archicad-additional-json-commands).

## Repository layout: the rest of `docs/` and `tools/`

```
                         api-coverage/ page of the Archicad API coverage checklist,
                         gh-coverage/ page of the Grasshopper coverage checklist,
                         limits/ page of what the Archicad API cannot do (hand-written),
                         community/ and about/ pages (growth chart, momentum, origin story),
                         the copies for AI assistants (llms.txt, llms-full.txt, commands.json,
                         commands.md, <page>/index.md, sitemap.xml, robots.txt; generated,
                         never hand-edited); .nojekyll, so Pages serves the files as they are
                         Add-On docs generator (generate_addon_docs.py),
                         expanded-schema dump and compare (expand_schemas.py),
                         command scaffolder (new_command.py),
                         one-example-per-command checker (check_examples.py),
                         Grasshopper GUI label table checker (check_tool_settings.py),
                         API coverage generator (generate_api_coverage.py,
                         fetch_api_coverage_github_data.py, api_coverage_rules.json),
                         Grasshopper coverage generator (generate_gh_coverage.py,
                         gh_coverage_rules.json),
                         Archicad version list reader and updater (devkits.py),
                         Community page data (generate_community_data.py),
                         marketplace folder checker (check_marketplace.py) and
                         Scripts page catalogue (generate_catalogue.py),
                         site copies for AI assistants (generate_llm_files.py),
                         release notes summary (release_notes.py)
  discord-issue-bot/     Discord -> GitHub issue bot (run by .github/workflows/discord_issue_bot.yml)
    triage-command/      The Discord /triage slash command (Cloudflare Worker that starts that workflow)
```

## Archicad Add-On architecture

- **Entry point:** [archicad-addon/Sources/AddOnMain.cpp](../archicad-addon/Sources/AddOnMain.cpp).
  `Initialize()` registers every command, grouped into `CommandGroup`s (Application,
  Project, Element, Attribute, Property, Classification, Navigator, Issue, Revision,
  Design Options, IFC, Library, Teamwork, Favorites, Solid Element Operation, etc.).
  Each `RegisterCommand<T>(group, version, description)` call also records the version
  the command was introduced and a human description used for the generated docs.
- **Commands** are grouped by domain into paired `*.cpp`/`*.hpp` files in `Sources/`
  (e.g. `ElementCommands`, `ElementCreationCommands`, `AttributeCommands`,
  `NavigatorCommands`, `PropertyCommands`, `IFCCommands`). All commands live in the
  `TapirCommand` namespace and are invoked via the Archicad JSON interface.
- **CommandBase** ([Sources/CommandBase.hpp](../archicad-addon/Sources/CommandBase.hpp))
  is the base class. It defines the namespace, execution policy, and schema hooks, and
  provides many shared helpers — GUID <-> ObjectState conversion, coordinate/polygon
  helpers, story/floor-index resolution (`GetStories`, `ResolveFloorIndexAndOffset`),
  attribute lookup, and `Create*Response`/`Create*ExecutionResult` builders. Reuse these
  helpers when adding commands; match the existing style.
- **UI:** the Add-On adds a menu + palette ([TapirPalette](../archicad-addon/Sources/TapirPalette.cpp)),
  an About dialog, and an auto-update/version check ([VersionChecker](../archicad-addon/Sources/VersionChecker.cpp)).

## Adding or changing a command: steps 6 and 7

6. Docs at <https://archicad-api.com/...> are generated from the registered
   descriptions and schemas by `python tools/generate_addon_docs.py` (no Archicad
   needed; same output as the `GenerateDocumentation` developer command). The weekly
   release regenerates them automatically; run it yourself to update them earlier.
7. Bump the version consistently (see Versioning below) when releasing.

- Keep command input/response schemas, the C++ implementation, the registered
  description in `AddOnMain.cpp`, and a Python example consistent when adding features.

## Building one version

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

## Testing: the scripts' helper and real Archicad

- The scripts use `aclib/` under `Examples/` (`aclib.RunTapirCommand`); `Workflows/`,
  `Test/Scripts/` and each `Examples/<GroupFolder>/` have an `aclib.py` shim that loads it.
- Pull requests and local builds are tested on real Archicad with
  [tools/community-test-runner/](../tools/community-test-runner/) (JSON checks in
  `archicad-addon/Test/checks/`). Use its `--brief` output (`prs`, `test … --brief --no-post`,
  `trial … --brief`, `check --brief`) rather than reading reports or its source; its skill is
  `tools/community-test-runner/claude-skill/tapir-pr-test/SKILL.md`. Runners test what is
  labelled `needs-test`: a person sets it on a PR, and the bot PR steward sets it by itself on a
  bot PR with a check once its builds are green, so testers' runners install those bot builds
  without a person looking first (see `claude_bot_pr_steward.yml` under CI).
