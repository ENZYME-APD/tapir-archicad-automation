# Contributing to Tapir

Tapir is the JSON command layer for Graphisoft Archicad: a C++ Add-On that registers a few hundred
commands on Archicad's own JSON interface, a Grasshopper plugin, a Python client, an installer, and
the scripts and docs around them. Everything here is in this repository, and contributions of every
size are welcome — a fixed typo in the command reference, a check for a command nobody tested, a
marketplace script, or a new command.

If you are unsure where something belongs, open an issue first: the
[issue templates](.github/ISSUE_TEMPLATE) ask for what a maintainer needs to answer.

## What helps most

- **A check for a command that has none.** `archicad-addon/Test/checks/` holds one JSON file per
  command: the requests the community test runner sends to an empty Archicad project and the answers
  they must give. Every command whose behaviour a pull request changes needs one, and about a third
  of the commands still have none.
- **A tested build on your Archicad.** The runner in
  [tools/community-test-runner/](tools/community-test-runner/) tests a pull request's build on a real
  Archicad and posts the result; it needs nothing but Archicad and a GitHub account.
- **A marketplace script.** [marketplace/README.md](marketplace/README.md) has the format; a merged
  script appears on the Scripts page.
- **Docs.** The command reference is generated, but the pages under `docs/` are written by hand and
  follow [branding/WEBSITE_STYLE.md](branding/WEBSITE_STYLE.md).

## Building the Add-On

Requires **CMake ≥ 3.17**, **Python 3**, and Visual Studio (Windows) or Xcode (macOS). The Archicad
API DevKits are downloaded by the build scripts and are not in the repository.

The supported Archicad versions are the ones in
[archicad-addon/devkits.json](archicad-addon/devkits.json); everything builds against every one of
them, so a change that uses a newer API must be guarded with the `ServerMainVers_*` checks used
elsewhere in `archicad-addon/Sources/`.

Windows, every supported version:

```bat
cd archicad-addon\Tools
build_all_win.bat
```

macOS, one version (the flags are the ones
[archicad_addon_build_check.yml](.github/workflows/archicad_addon_build_check.yml) uses, which is the
authority when they change):

```bash
python3 archicad-addon/Tools/download_and_unzip.py <devkit-url> archicad-addon/Build/DevKits/AC30
cmake -B archicad-addon/Build -G "Xcode" \
  -DAC_VERSION=30 -DAC_API_DEVKIT_DIR="archicad-addon/Build/DevKits/AC30/Support" archicad-addon
cmake --build archicad-addon/Build --config RelWithDebInfo
```

The build writes `TapirAddOn_AC<version>_Win.apx` (Windows) or `TapirAddOn_AC<version>_Mac.bundle`
(macOS). To use it, close Archicad and copy it into that Archicad's Add-Ons folder, or select it in
the Add-On Manager.

The Grasshopper plugin builds with the solution in
[grasshopper-plugin/](grasshopper-plugin/) (`TapirGrasshopperPlugin.sln`, Visual Studio on Windows).

## Running the tests

**Without Archicad** — these need nothing but Python and run in seconds:

```bash
python3 -m unittest discover python-client/tests
python3 -m unittest discover -s tools/community-test-runner/tests
python3 -m unittest discover -s archicad-addon/Installer -p "test_*.py"

# the issue bot's, which needs its dependencies first
pip install -r tools/discord-issue-bot/requirements.txt
cd tools/discord-issue-bot && python3 -m unittest test_discord_issue_bot
```

**With Archicad** — the scripts under `archicad-addon/Test/` talk to a running Archicad with your
build loaded and compare against golden outputs:

```bash
cd archicad-addon/Test
python test_examples.py            # every script and example against TestProject.pla
python test_examples.py create_and_flip_walls  # only the ones named
python record_expected_output.py <name>.py   # record a golden output
```

The JSON checks in `archicad-addon/Test/checks/` run through the community test runner rather than
this script; see its [README](tools/community-test-runner/README.md).

## Adding or changing a command

`python3 tools/new_command.py` writes the repetitive part — the class, the schemas, an `Execute`
skeleton, the registration, a starter example and a starter check — from a short spec, whose format
is in its docstring. What remains is the Archicad API calls, the check's real parameters and
expectations, and any golden output.

Then, in the same pull request:

1. The command class in the right `archicad-addon/Sources/*Commands.{cpp,hpp}`, with its input and
   response schemas reusing the shared `$ref` definitions.
2. Its registration in `AddOnMain.cpp`, with the version it appears in and a clear description.
3. Its example under `archicad-addon/Examples/<Group>/<CommandName>.py` (`python3 tools/check_examples.py`
   checks it).
4. A **check** in `archicad-addon/Test/checks/<snake_case name>.json` that makes the elements it
   needs and whose expectations would catch the bug or prove the feature.
5. The regenerated documentation (below).

[CLAUDE.md](CLAUDE.md) has the long version of each step, the schema conventions and the architecture
tour; it is the repository's developer guide, written for the project's automated assistant.

## Generated files

Never edit these by hand; run the generator instead, and commit its output in the same pull request
whenever the command reference changes:

```bash
python3 tools/generate_addon_docs.py          # docs/archicad-addon/*.js  (add --check to test)
python3 tools/generate_llm_files.py           # docs/llms*.txt, docs/commands.* , sitemap.xml
python3 python-client/generate.py             # python-client/tapir_archicad/commands.py
python3 tools/generate_gh_docs.py             # Grasshopper component docs
```

`tools/generate_addon_docs.py` reads the C++ sources themselves, so a schema it cannot evaluate fails
the build rather than documenting something guessed — if that happens, the fix is a schema written
the way the other commands write theirs, or a change to the generator.

## Style

- C++ follows the Graphisoft/GS naming and spacing of the surrounding code (`GS::`, `API_`, a space
  before the parenthesis of a call). The Add-On builds with `/W4 /WX` and
  `-Wall -Wextra -Werror`, so warnings are errors.
- Python in this repository uses the same spacing (`def Name (arg)`) and no dependencies outside the
  standard library for the client and the tools.
- Keep the schema, the C++ implementation, the description in `AddOnMain.cpp`, the example and the
  check consistent — they are read by different readers and by the generators.
- Code must build on Windows and macOS; resources compile through `Tools/CompileResources.py`.

## Pull requests

- Explain what changes for a user of the command, not only what changed in the code, and say which
  Archicad versions you built and tested. A check plus a tested build is what gets a pull request
  merged quickly.
- The checks a pull request has to pass are the build checks for the Add-On, the Grasshopper plugin
  and the installer, and the documentation and example checks. The
  [build gate](.github/actions/build-gate) skips the expensive builds for changes that cannot affect
  them (docs, tests, examples, marketplace scripts).
- A committer reviews every pull request. Two kinds merge without a person: one whose author has
  write access, once every check has passed and the diff touches none of the release critical paths;
  and the issue bot's own, once its builds passed and its review has been clean for three days. The
  release critical paths — the installer, the build and release definitions, the workflows — always
  wait for a person. The labels `do-not-merge` and `needs-maintainer`, a request for changes, or a
  draft stop the automation.
- Testers pick up pull requests labelled `needs-test`. The bot PR steward sets it on the bot's own
  pull requests; on yours, ask a maintainer or a triager to add it, since changing labels needs
  triage access.

Security problems do not go in an issue: see [SECURITY.md](SECURITY.md).

## Conduct and questions

Be decent to people; the project runs on a shared Discord
([invite in the README](README.md)) and on issues from users who depend on this in their daily work.
"Works for me on Archicad 26" is a useful data point, not a dismissal of someone else's report.
