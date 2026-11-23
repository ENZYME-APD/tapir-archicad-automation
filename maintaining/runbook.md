# Maintaining Tapir with no bot running

For a person with write access to the repository and no Claude at all. Every
workflow named here is in `.github/workflows/`, every script in `tools/`.
The background is in [ci.md](ci.md) and [release.md](release.md); the
wiki's [Release process](https://github.com/ENZYME-APD/tapir-archicad-automation/wiki/Release-process)
page is the older, manual version of what `weekly_release.yml` automates.

You need the GitHub CLI (`gh`), logged in with write access, and Python 3.

## 1. Cut a weekly release by hand

The release itself needs no bot: `weekly_release.yml` is plain shell. It runs
by itself every Monday at 06:00 UTC (`cron: '0 6 * * 1'`) and releases `main`
when anything other than the version files changed since the last tag. To
release by hand, run that same workflow; do not tag by hand (see the end of
this section).

1. **Check `main` is releasable.** The build checks of the last push to `main`
   (`archicad_addon_build_check.yml`, `grasshopper_plugin_build_check.yml`,
   `installer_build_check.yml`) are green on the Actions tab. Run
   `python tools/update_version.py` on an up-to-date `main`: it must change no
   file (the release stops otherwise, see the messages below).
2. **Try a dry run first** (optional): Actions > *Weekly Release* > *Run workflow*
   on `main` with `dry_run` ticked, or
   `gh workflow run weekly_release.yml --ref main -f dry_run=true`.
   The run's summary says which version it would release and why
   (`… commit(s) with changes since … - patch release … (main carries …)`).
   Nothing is tagged, built, published or bumped.
3. **Make sure an approver for SignPath is reachable** for the next 3 hours.
   The Windows installer is signed by SignPath (`build_installer_win` in
   `archicad_addon.yml`), and every release-signing request needs a manual
   approval **in SignPath, from SignPath's e-mail** to the approvers (there is
   no GitHub environment to approve). The job waits up to 3 hours
   (`wait-for-completion-timeout-in-seconds: 10800`).
4. **Start the release:** `gh workflow run weekly_release.yml --ref main`
   (it must be `main`; on any other ref the first step stops with an error).
   Its inputs:
   - `release_type`: `auto` (default; major after a merged PR labelled
     `major-release`, minor for the first release of a month, else patch),
     or force `patch`, `minor` or `major`.
   - `allow_early_release`: release although the last release is less than
     3 days old (`MIN_RELEASE_INTERVAL_SECONDS`).
   - `skip_docs_update`: release without regenerating `docs/archicad-addon`
     and the LLM files; only when `tools/generate_addon_docs.py` fails.
   - `dry_run`: see step 2.
5. **What it does, in order** (watch the run on the Actions tab):
   1. Checks the version and the tag (the plan step).
   2. For a minor or major release, sets that version in the version files
      and in the commands registered with the skipped version, and pushes
      `Set the version to … for the release` to `main`.
   3. Runs `tools/generate_addon_docs.py` and `tools/generate_llm_files.py`
      and pushes `Regenerate the Add-On docs and the LLM files` when they changed.
   4. Tags `main` with the version.
   5. Dispatches `archicad_addon.yml` on the tag and waits for it; it builds
      the add-on for every version in `archicad-addon/devkits.json` and the
      installers, and attaches them to a draft release.
   6. **Approve the SignPath request** when its e-mail arrives (step 3). If it
      is not approved in time, or signing fails, the release still goes out
      with the unsigned Windows installer, the run warns
      `The Windows installer of $VERSION is not code signed.` and the release
      notes say so. Deny a stale request in SignPath.
   7. Checks the draft has `EXPECTED_ASSETS` files (two per version in
      `devkits.json` plus four installer files), then dispatches
      `grasshopper_plugin.yml` on the tag, which pushes the plugin to the
      Rhino package manager (yak, secret `YAK_TOKEN`).
   8. Publishes the draft with the notes from `tools/release_notes.py`.
   9. Announces the release on Discord (secret `DISCORD_BOT_TOKEN`; never fails the run).
   10. Bumps the patch version with `tools/update_version.py` and pushes
       `Update version after release` to `main`.
6. **Do not use GitHub's *Re-run* buttons** on this workflow or on
   `archicad_addon.yml`: a re-run reuses the original commit, which misses the
   version and docs commits the release pushed (the header of
   `weekly_release.yml` says so). Start a new run of `weekly_release.yml` instead;
   it resumes a release whose tag still points at `main`'s head.
7. **Publishing by hand:** if you ever publish a draft yourself, the release
   page must carry the SignPath footer when the installer is signed (the
   SignPath Foundation terms; the text is in the "Publish the release" step of
   `weekly_release.yml`), and you must then bump the version yourself (step 5.10).

Why not push a tag by hand: `archicad_addon.yml` and `grasshopper_plugin.yml`
both also run on any pushed tag, so a hand-pushed tag starts them **in
parallel**, and the Grasshopper pipeline can push to yak before the add-on
draft is known to be complete, which `weekly_release.yml` avoids on purpose.

## 2. The messages `weekly_release.yml` can print, and what to do

Placeholders (`$VERSION`, …) are filled in by the run. A run that stops on a
failed build job also opens a `release-failure` issue (the `request_fix` job);
with no bot running, fix the issue yourself, or set `RELEASE_FIX` to `false`
(section 5) so the job opens none.

| Message | What to do |
|---|---|
| `The Weekly Release runs only on main (dispatched on '…'). To resume a failed release, dispatch the two pipeline workflows on the tag instead - this workflow always runs from main.` | Run it again with `--ref main`. |
| `Version '$VERSION' in tools/package_info.json is not MAJOR.MINOR.PATCH; fix it before releasing.` | Put a `1.2.3`-style version in `tools/package_info.json`, run `python tools/update_version.py`, commit to `main`, run again. |
| `The version files disagree with tools/package_info.json. Run tools/update_version.py and commit the result before releasing.` | Exactly that: run `python tools/update_version.py`, commit the changed files to `main`, run again. |
| `Tag $VERSION is unpublished on an older commit, but it cannot be shown that $VERSION reached neither GitHub nor the Rhino package manager (Grasshopper runs on the tag: …) - leaving it for a person.` | A warning; the run then stops with the "exists on another commit" error below. Follow that one. |
| `Tag $VERSION was never published and pointed at an older commit; deleted it and its draft release(s), so $VERSION is released from the current main.` | Nothing: the run carries on and releases the current `main`. |
| `Could not delete the stale tag $VERSION or its drafts - leaving it for a person.` | Delete the drafts for `$VERSION` on the Releases page and the tag (`git push origin :refs/tags/$VERSION`), after the check in the "exists on another commit" row, then run again. |
| `Tag $VERSION is already published but package_info.json still says $VERSION - the post-release version bump likely failed to push. Bump the version (tools/update_version.py), commit it to main, and the next run releases the new version.` | Set the next patch version in `tools/package_info.json`, run `python tools/update_version.py`, commit to `main`. |
| `Tag $VERSION exists on another commit but was never published - probably a failed release followed by new commits. First check with 'yak search tapir' whether $VERSION already reached the Rhino package manager …` | Run `yak search tapir`. If `$VERSION` is there, bump the version as in the row above. Only if it is not, delete the tag (`git push origin :refs/tags/$VERSION`) and run again; the run re-tags the current `main`. |
| `No changes since $LAST_TAG - nothing to release this week.` (run summary) | Nothing to do. |
| `$LAST_TAG was released …h ago, less than 3 day(s) - not releasing again yet. Dispatch this workflow with 'allow_early_release' to release anyway.` (run summary) | Wait, or run with `allow_early_release`. |
| `Unknown release type '$RELEASE_TYPE'.` | Run again with `auto`, `patch`, `minor` or `major`. |
| `A $RELEASE_TYPE release would be $RELEASE_VERSION, but that tag already exists. Set the next version in tools/package_info.json by hand, run tools/update_version.py and commit it to main.` | Exactly that, then run again. |
| `main moved while the version was set - re-run this workflow.` / `main moved while the docs were regenerated - re-run this workflow.` | Someone pushed to `main` meanwhile. Start a new run (not *Re-run*). |
| `tools/generate_llm_files.py failed; the LLM files were not updated.` | A warning; the release continues. Fix the script later and run it with `--check`. |
| (the "Regenerate the docs" step fails with no message of its own) | `tools/generate_addon_docs.py` failed. Fix it, or run again with `skip_docs_update`. |
| `The Add-On pipeline succeeded but the draft release for $VERSION has only $ASSETS of the expected $EXPECTED_ASSETS artifacts - not starting the Grasshopper pipeline. Delete the draft(s) for $VERSION on the releases page, re-run the 'Archicad Add-On Build and Release' workflow on tag $VERSION, then re-run this workflow.` | Exactly that: delete the drafts, `gh workflow run archicad_addon.yml --ref $VERSION`, wait for it, then start `weekly_release.yml` again. |
| `The dispatched run of $WORKFLOW on tag $VERSION never appeared.` | GitHub did not start the run. Start `weekly_release.yml` again. |
| `$WORKFLOW finished with conclusion '…', $FAILED_JOBS job(s) failed. The release is NOT published and the version is NOT bumped. Tag $VERSION stays in place - fix the build and re-run the pipelines on it.` | Read the failed job's log, fix the build in a pull request and merge it, then start `weekly_release.yml` again: the tag is now on an older commit, and the run deletes it and re-tags `main` when the version shipped nowhere. |
| `$WORKFLOW ended '…' and $OTHER unfinished job(s) were not ones GitHub failed to give a runner to: a cancelled release or a job that timed out. Not re-running it. …` | A job was cancelled or timed out (for example the SignPath wait). Start `weekly_release.yml` again; it resumes on the same tag. |
| `$WORKFLOW ended '…' but no job failed - a hosted runner was probably not acquired. Re-running the jobs that did not run (attempt N of 3).` | Nothing; the run retries by itself. |
| `$WORKFLOW still ended '…' after three re-runs, with no failed job: … re-run this workflow when GitHub has runner capacity.` | Start `weekly_release.yml` again later. |
| `No draft release found for tag $VERSION.` | The add-on pipeline made no draft. Run `gh workflow run archicad_addon.yml --ref $VERSION`, then start `weekly_release.yml` again. |
| `Draft release for $VERSION has only $ASSETS of the expected $EXPECTED_ASSETS artifacts - not publishing an incomplete release. …` | Delete every draft for `$VERSION` on the Releases page, run `archicad_addon.yml` on tag `$VERSION`, then start `weekly_release.yml` again. |
| `tools/release_notes.py failed; the release notes list every pull request instead.` | A warning; edit the release notes by hand if you want the summary. |
| `The Windows installer of $VERSION is not code signed.` | The SignPath request was not approved in time or failed. The release is out with the unsigned installer; deny the stale request in SignPath. |
| `DISCORD_BOT_TOKEN is not configured - skipping the Discord announcement.` / `Could not read channel … to look for an earlier announcement (…) - posting anyway.` | Warnings only. Announce on Discord by hand if wanted. |
| `Release $VERSION is published; only the version bump is missing. Push it manually: set the next version in tools/package_info.json, run tools/update_version.py, commit to main. …` | Exactly that. |
| `Could not start the issue bot on #$NUMBER; apply the claude-fix label to start it.` | With no bot running: ignore it and fix the `release-failure` issue yourself. |

## 3. Review and merge a pull request by hand

`claude_auto_merge.yml` merges by itself only what passes the checks below. To
merge by hand, check the same things on the pull request page:

1. It is open, not a draft, and its base is `main`.
2. **Every check is green or skipped**, and there is at least one. The checks
   come from `archicad_addon_build_check.yml` (`generate_docs`, `build_win`,
   `build_mac`), `installer_build_check.yml` (`build_installer`) and
   `grasshopper_plugin_build_check.yml` (`build`). A skipped build is fine: a
   pull request that changes only docs, tests, examples, scripts or `*.md`
   skips them on purpose (the skip list is in `.github/actions/build-gate/action.yml`).
3. **No release-critical path is changed.** Auto-merge never merges a change to
   these (from the `PROTECTED` pattern in `claude_auto_merge.yml`):
   `.github/`, `archicad-addon/Tools/`, `archicad-addon/Installer/`, `tools/`,
   `archicad-addon/devkits.json`, `CMakeLists.txt`, `*.cmake`, `*.csproj`.
   A pull request that touches them needs a careful look by a maintainer; they
   run with the repository's credentials or decide what ships.
4. GitHub says it can be merged (no conflict with `main`).
5. For a person's pull request: the author has write, maintain or admin access
   (anyone else's needs a maintainer's review first).
6. For a bot pull request (a `claude/` branch opened by the Actions bot):
   no `do-not-merge` or `needs-maintainer` label; the `claude-review` status is
   `success` (if the review bot is off, review it yourself instead); no
   `archicad-test/…` status (set by `archicad_test_status.yml` from the community
   test runners) is failing; no unresolved thread opened by a person and no
   standing "changes requested". Auto-merge also waits
   `BOT_AUTO_MERGE_DELAY_HOURS` (default 72) after the review passed.
7. Merge with a merge commit and delete the branch, as auto-merge does:
   `gh pr merge <number> --merge --delete-branch`.
   (`delete_merged_branches.yml` deletes left-over branches of merged pull requests too.)

For your own review: the rules a change must follow are in `CLAUDE.md` and the
files of `maintaining/`; a new or changed command needs its example
(`python tools/check_examples.py`) and its check under `archicad-addon/Test/checks/`,
and a pull request labelled `needs-test` is tried in real Archicad by the
community test runners (`tools/community-test-runner/`).

## 4. Add a command with `tools/new_command.py`

1. Write a spec file, for example `spec.json`; the format is in the docstring of
   `tools/new_command.py`:

   ```json
   [{
     "name": "GetSomething",
     "file": "ElementCommands",
     "group": "elementCommands",
     "description": "Returns something of the given elements.",
     "input": {"elements": ["#ElementIdArrayItem"]},
     "output": {"things": ["string"]}
   }]
   ```

   `name` is CamelCase, `file` the `Sources/<file>.cpp/.hpp` it goes into,
   `group` the `CommandGroup` variable in `AddOnMain.cpp`. Optional keys:
   `requires` (an Archicad version guard), `undoable`, `commonSchema`,
   `version` (defaults to `tools/package_info.json`), `example`, `check`.
   Use `#Name` for a shared schema definition from
   `archicad-addon/Sources/RFIX/Images/CommonSchemaDefinitions.json`.
2. Look first: `python tools/new_command.py spec.json --dry-run` (or `--schemas`
   for the expanded schemas only). Then write: `python tools/new_command.py spec.json`.
3. It writes the class in `Sources/<file>.hpp`, the constructor, `GetName`, both
   schemas and an `Execute` skeleton in `Sources/<file>.cpp`, the
   `RegisterCommand` call in `AddOnMain.cpp`, a starter example
   `archicad-addon/Examples/<GroupFolder>/<CommandName>.py` and a starter check
   `archicad-addon/Test/checks/<snake_case>.json`. It never touches an existing command.
4. **Written by hand:**
   - the Archicad API calls in `Execute` (marked `TODO`), checked against the
     DevKit headers of every version in `devkits.json` (`ServerMainVers_*` /
     `AC_VERSION` guards where they differ);
   - the check's title, its real parameters and expectations (see "Writing
     checks" in `tools/community-test-runner/README.md`);
   - the example's docstring and parameters, if the starter is not enough;
   - any golden output under `archicad-addon/Test/ExpectedOutputs/`.
5. Check: `python tools/check_examples.py`, and `python tools/generate_addon_docs.py`
   to see the schemas parse (then revert `docs/`; the weekly release regenerates it).
6. Build it (see "Building the Add-On" in `CLAUDE.md`) and run it in Archicad,
   or open the pull request and label it `needs-test` for the community test runners.

## 5. Turn each bot off

Set a repository variable: Settings > Secrets and variables > Actions >
Variables, or `gh variable set <NAME> --body false`. Delete the variable (or set
it to anything else) to turn the bot on again.

| Variable | Workflow | Turns off |
|---|---|---|
| `BOT_PR_STEWARD` | `claude_bot_pr_steward.yml` | the steward: its hourly sweep and its fix runs on bot pull requests |
| `BOT_AUTO_MERGE` | `claude_auto_merge.yml` | merging bot pull requests (a person's green pull request is still merged) |
| `CLAUDE_RETRY` | `claude_issue_triage.yml` | only the hourly `retry` job that restarts refused issue fixes |
| `CLAUDE_SPLIT_NEXT_PART` | `claude_split_next_part.yml` | starting the next part of a split issue |
| `RELEASE_FIX` | `weekly_release.yml` (`request_fix`), `claude_pr_review.yml`, `claude_auto_merge.yml` | the `release-failure` issue and its fix being merged without waiting |
| `DEVKIT_UPDATE` | `archicad_devkit_update.yml` | the bot pull request for a new Archicad DevKit |
| `API_COVERAGE` | `api_coverage.yml` | the API and Grasshopper coverage pages (no model; off only if wanted) |
| `SITE_DATA` | `site_data.yml` | the community and script catalogue data (no model; off only if wanted) |

**No off switch yet:**
- **The issue bot** (`fix` and `reply` jobs of `claude_issue_triage.yml`) has no
  variable: it runs on every new issue and every human comment on an issue.
- **The review bot** (`claude_pr_review.yml`) has no variable; `RELEASE_FIX`
  only stops one of its steps.
- **The Discord issue bot** (`discord_issue_bot.yml`) has no variable. Removing
  its secrets is not a clean switch either: it stays quiet only when
  `DISCORD_BOT_TOKEN`, `DISCORD_CHANNEL_IDS` and `CLAUDE_CODE_OAUTH_TOKEN` are
  all unset, and fails when only some are.

For these, disable the whole workflow with GitHub's own switch: Actions > the
workflow > ··· > *Disable workflow*, or
`gh workflow disable claude_issue_triage.yml` (and `claude_pr_review.yml`,
`discord_issue_bot.yml`); `gh workflow enable <file>` turns it on again. This
works for any workflow, including the ones in the table.
