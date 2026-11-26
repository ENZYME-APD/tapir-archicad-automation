## CI

GitHub Actions in [.github/workflows/](../.github/workflows/):
- `archicad_addon_build_check.yml`, `grasshopper_plugin_build_check.yml`,
  `installer_build_check.yml` — PR build checks for each component, also run
  on pushes to `main` and by `workflow_dispatch` (which the bot PR steward
  uses); a newer push to a pull request cancels its superseded runs. The add-on
  check also runs `tools/check_examples.py` (one example per command), and
  `tools/check_marketplace.py` with `tools/generate_catalogue.py --self-test`. The installer
  check also runs `Installer/test_update_mode.py` and syntax-checks the legacy
  auto-update script.
  A pull request (or the steward's dispatch on a branch) whose changed files are
  all in the skip list of [.github/actions/build-gate](../.github/actions/build-gate/action.yml)
  — `docs/**`, `archicad-addon/Test/**`, `archicad-addon/Examples/**`,
  `archicad-addon/Workflows/**`, `builtin-scripts/**`, `marketplace/**`,
  `branding/**`, `**/*.md` — skips the add-on and installer builds: a cheap
  `changes` job gates `build_win`, `build_mac` and `build_installer` by `if:`, so
  their checks still appear as skipped, which the steward, auto-merge and required
  checks count as passing (not `paths-ignore`, whose missing checks would hang
  them). `build_win` still runs when `archicad-addon/Test/checks/` changed, for the
  community test runner; pushes to `main` and the release workflows always build.
- The release workflows (`archicad_addon.yml`, `grasshopper_plugin.yml`, `weekly_release.yml`) are in [release.md](release.md).
- `claude_issue_triage.yml` — the issue bot. Every newly opened issue (`fix` job) and
  every new human comment on an issue (`reply` job), from anyone, runs Claude with
  write access: it answers, closes verified already-fixed issues, or opens a
  ready-for-review PR on a `claude/issue-<n>-…` branch with `Fixes #<n>`, and asks
  the issue's author (tagged) to verify it in Archicad. Its commits end with a
  `Co-authored-by:` trailer for the person who opened the issue (their GitHub
  no-reply address, worked out by a step without a model; none for issues a bot
  opened) and its PR body says `Reported by @<login>`, so the reporter is
  credited when the PR is merged. An issue too large for
  one PR gets a split plan (`<!-- claude-split-plan: parts=N -->`, at most six
  parts; never for a `marketplace/` script, which is always one PR) and one PR per
  part, opened one run at a time (`next-part` job) on
  `claude/issue-<n>-part<k>-…` branches with `Part <k> of <N> of #<n>`; a run
  that opened its part still starts the next one when it then fails, and one that
  opened none ends the chain. Merging a part restarts a stopped chain:
  `claude_split_next_part.yml` (`pull_request_target: closed`, and dispatched by
  `claude_auto_merge.yml`, whose `GITHUB_TOKEN` merges start no run; no model, no
  checkout; `CLAUDE_SPLIT_NEXT_PART=false` turns it off) dispatches the issue
  when it is open, has fewer part PRs than planned and no run queued or in
  progress (#2145). No label
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
  headers of every version in `devkits.json`, and the C/C++ sources of the newest
  DevKit's Examples, in `archicad-addon/Build/DevKits/` before Claude starts, from
  the cached [.github/actions/download-devkits](../.github/actions/download-devkits/action.yml)
  (which `api_coverage.yml` uses too).
  The steward and the review load that action from a checkout of the workflow's own
  commit, never from a pull request's branch; they read the pull request's
  `devkits.json` (data only), so a PR adding a version has its headers.
- `archicad_devkit_update.yml` — daily: when Graphisoft has published the DevKit of
  an Archicad version newer than the newest in `devkits.json` (its first public
  build, x.3000 or later, with Windows and macOS zips), `tools/devkits.py update` adds
  it there and to the README's download table, and this opens a bot PR on a
  `claude/devkit-<version>-<release>` branch and starts the steward. The steward
  builds it and fixes what the new DevKit breaks, and `claude_auto_merge.yml`
  merges it like any bot PR. `devkits.json` is a release critical path, so
  auto-merge lets a bot PR through with it only when the file is `main`'s with new
  versions appended (ascending, each `NN.NNNN` of its own version, nothing else
  changed) and the branch is named after the newest one. `DEVKIT_UPDATE=false`
  turns it off.
- `claude_bot_pr_steward.yml` — drives the issue bot's PRs to mergeable without a
  maintainer (their `GITHUB_TOKEN` pushes start no runs): an hourly sweep, also
  started by the issue bot and the review, dispatches the build checks and the
  review on each new head, links the built add-on in a comment asking the
  issue's author to test it in Archicad, labels it `needs-test` when it has a check
  file (for the community test runner, see below), and starts a `fix` run on a failed build,
  a blocking review or a conflict with `main`, at most
  `BOT_PR_MAX_FIX_ROUNDS` (default 6) times, on at most `BOT_PR_SWEEP_BATCH`
  (default 15) PRs per sweep. Then
  it labels the PR `needs-maintainer` and stops; removing the label hands the PR
  back with its fix round and review counts reset. `BOT_PR_STEWARD=false` turns it
  off; `needs-maintainer`, `do-not-merge` or draft takes a PR out of its hands.
  `main` is merged in by the model and pushed with `GITHUB_TOKEN`; there is no
  GitHub App for it (#1623). A step without a model checks the merge first and
  hands the PR over at once (`needs-maintainer`, asking a person to merge `main`
  in with "Update branch" or by hand and then remove the label) when `main`
  brings changes under `.github/`, which `GITHUB_TOKEN` may not push, or the
  merge conflicts under `.github/`, `tools/`, `archicad-addon/Tools/` or
  `archicad-addon/Installer/`, which never go to the model. A person's push
  starts the build checks itself, so the sweep counts runs queued on a head as
  started. A fix run that pushes nothing ends its comment with
  `<!-- steward-fix-outcome: <outcome> <nonce> -->`: `transient-ci` re-runs the
  failed build jobs and `review-disputed` starts a fresh review (each at most
  twice per PR), `waits-for:#N` parks the PR without a label (commit status
  `steward-fix` pending) until PR #N is merged; `merge-blocked`, `stuck` or no
  marker hands it over at once.
  The `needs-test` label is set by the sweep, which runs no model, only on the
  bot's own PRs (`claude/` branches of this repository, opened by the Actions bot),
  only once every build check of the head is green, and only when the PR adds or
  changes a file in `archicad-addon/Test/checks/`. It is never removed by the
  steward. That label is what testers' community test runners poll, and they
  install the PR's built add-on on their own machines: so a bot PR's native code,
  written by a model from public issue text, reaches testers' Archicads with no
  person looking at it first, also for every later head the bot pushes and the
  build check builds. The maintainers chose this (#1138) so bot PRs get tested
  before they are merged; a person takes a PR out of it by removing the label
  (the next green head sets it again unless the PR is also labelled
  `needs-maintainer` or `do-not-merge`, or is a draft).
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
  A bot PR that fixes a `release-failure` issue (the Actions bot's open issue with
  that label, whose body starts with `<!-- release-failure: <version> run=<id> -->`
  naming a weekly release run that failed while the issue was opened, for the version
  `main` still carries; the PR on the issue's `claude/issue-<n>-` branch with
  `Fixes #<n>`) skips the waiting time, and its merge starts `weekly_release.yml`;
  `claude_pr_review.yml` starts auto-merge when it records a passing verdict, so the
  merge follows the review at once.
- `delete_merged_branches.yml` — deletes the head branch of every merged pull
  request of this repository, however it was merged, when its tip is still the
  merged head and no open pull request uses it; run by hand (dry run by default)
  it sweeps the leftovers. `KEEP` in it lists the branches without a pull
  request that must stay (`api-coverage`, `site-data`, image branches).
- `discord_issue_bot.yml` — files GitHub issues from bug reports and feature
  requests in the project's Discord channels, then dispatches
  `claude_issue_triage.yml` for each (issues created with `GITHUB_TOKEN` trigger
  no workflow on their own; see
  [tools/discord-issue-bot/README.md](../tools/discord-issue-bot/README.md)). It is
  scheduled at :13 and :43, off the full and half hour that GitHub drops most.
  `/triage` in Discord starts it on demand through the Worker in
  [tools/discord-issue-bot/triage-command/](../tools/discord-issue-bot/triage-command/README.md).
- `api_coverage.yml` — keeps the Archicad API coverage checklist live: on a push
  to `main`, a release, issue events, hourly (which picks up pull request changes)
  and by hand, it runs
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
  when set; `API_COVERAGE=false` turns it off. It never runs on pull request
  events (its check would only keep pull requests from showing green), and it
  must never check out or run pull request code. Maintainers mark items skipped,
  with a reason, in the `skip` section of `tools/api_coverage_rules.json`; the
  `notApplicable` section lists what cannot be a JSON command. Its groups and
  patterns are broad, so the `applicable` section names, each with its verdict,
  the items they would rule out that a script or a Grasshopper definition could
  use (the audit in #2237); an item Tapir already calls counts as used whatever
  a rule says. Check an item there before calling it not applicable. The same run
  writes the Grasshopper coverage (`tools/generate_gh_coverage.py`): which Tapir
  commands a Grasshopper component calls (sent with `ToAddOn`; a same-named
  official command sent with `ToArchicad` does not count) and whether the
  components name every parameter of the command's input schema, cover the
  rest through a JSON input, or miss some. It publishes `gh-coverage.*` to the
  same branch, read by `docs/gh-coverage/index.html`; commands that need no
  component go in `tools/gh_coverage_rules.json`.
- `site_data.yml` — daily, by hand and on pushes to `main` that change
  `marketplace/`, `tools/check_marketplace.py` or `tools/generate_catalogue.py`:
  `tools/generate_community_data.py` counts,
  per month, the commits of `main`, the issues opened and pull requests merged, and
  the Discord messages and members of the project's server (only counts, never the
  text of a message), plus the totals and top contributors (commit authors and
  the people in `Co-authored-by` trailers, bots and AI assistants left out, each
  person once whatever names and addresses they used), and publishes
  `community.json` to the `site-data` branch (never to `main`), read by
  `docs/community/community.js` (the Momentum figures and the growth chart). The
  Discord bot token is `DISCORD_STATS_TOKEN`, else `DISCORD_BOT_TOKEN`; members per
  month need the bot's Server Members Intent, and without a token only the public
  invite's member count is read. Every run also writes `catalogue.json` (the
  marketplace scripts, read by `docs/scripts/scripts.js` and the site's live
  script count) to the same branch with `tools/generate_catalogue.py`, which needs
  the full history for each version's commit, URL and SHA-256; a push run writes
  only that and leaves `community.json` as it is. Daily and by hand it also writes
  `user-map.json` with `tools/generate_user_map.py`: the towns people name on Discord
  (the /where slash command of tools/discord-where/, answers to the location question in #welcome-and-rules, and "I'm from ..." anywhere),
  placed with OpenStreetMap's Nominatim and counted per town, never a name or a message,
  read by `docs/community/user-map.js` (the map); it needs the bot's Message Content
  Intent, and `tools/user_map_overrides.json` corrects a message it misreads. Its runs never cancel each
  other. It never runs on pull request events and never checks out pull request
  code; `SITE_DATA=false` turns it off.
