## Versioning

Version lives in multiple places that must stay in sync:
- [archicad-addon/Sources/AddOnVersion.hpp](../archicad-addon/Sources/AddOnVersion.hpp) (`ADDON_VERSION`)
- Grasshopper `.csproj` `<Version>`
- [tools/package_info.json](../tools/package_info.json)

Use the helper to bump: [tools/update_version.py](../tools/update_version.py)
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

The current version is defined in
[archicad-addon/Sources/AddOnVersion.hpp](../archicad-addon/Sources/AddOnVersion.hpp)
and [tools/package_info.json](../tools/package_info.json) — keep these in sync.
It is bumped automatically after each release by the weekly release
workflow, so it is not repeated here.

## The release workflows

- `archicad_addon.yml`, `grasshopper_plugin.yml` — release/publish pipelines.
  `archicad_addon.yml` also builds the Tapir Installer executables
  (`TapirInstaller_Win.exe`, `TapirInstaller_Mac.zip`), and attaches them a second time
  under versioned names (`TapirInstaller_Win_<version>.exe`, `TapirInstaller_Mac_<version>.zip`,
  #1646); `weekly_release.yml` expects two assets per version in `devkits.json` plus these
  four installer files (`EXPECTED_ASSETS`). The add-on carries its version as a resource
  (Windows VERSIONINFO written by `Tools/CompileResources.py`, macOS
  `CFBundleShortVersionString`), which the installer shows. The Mac installer is
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
  `docs/archicad-addon` with `tools/generate_addon_docs.py`, then the site's copies
  for AI assistants with `tools/generate_llm_files.py` (`llms.txt`, `llms-full.txt`,
  `commands.json`/`.md`, an index with one file per command group under `docs/commands/`
  that holds every command's schemas, Grasshopper components and example, each page as
  `index.md`, `sitemap.xml`, `robots.txt`; a failure there only
  warns), and pushes them to `main` when they changed (never hand-edit those docs).
  Run both scripts yourself to update them earlier; `--check` reports stale files.
  A release that stops on a failed build job (not a runner that never started)
  opens one `release-failure` issue for its version, with the failed jobs and
  their error lines, and starts the issue bot on it (`request_fix` job;
  `RELEASE_FIX=false` turns it off; at most `RELEASE_FIX_MAX_ATTEMPTS`, default 2,
  issues per version reach the bot, then `needs-maintainer`; `RELEASE_FAILURE_NOTIFY`
  is appended as a cc). Its fix is merged without the waiting time (see
  `claude_auto_merge.yml`), and the merge starts the release again. An unpublished
  tag left on an older commit is deleted with its draft releases and the fixed
  `main` re-tagged under the same version, only when that version has no published
  release and no Grasshopper pipeline run on the tag (which may have pushed it to
  yak); otherwise the run stops and asks a person, as before. A dry run only reports
  that deletion.
  The release notes are the installer links, a short summary written by
  `tools/release_notes.py` (the new commands per group, the new Grasshopper components
  and marketplace scripts, who made the pull requests, the new contributors and the
  compare link) and the code signing line, not GitHub's list of every pull request,
  which is used only when the summary cannot be written.
