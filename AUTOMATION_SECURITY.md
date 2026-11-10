# Automation security rulebook

This repository manages itself: bots answer issues, write fixes, review, test and merge them,
and a weekly workflow ships `main` as signed binaries. That only stays safe if the rules below
hold. They apply to every person and every bot that changes a workflow, a bot prompt, the
installer or `tools/`. Read them before you do, and tell the maintainers when a rule has to bend.

Only a person merges a change to this file (it is in `PROTECTED` of `claude_auto_merge.yml`).

## The threat in one paragraph

Anyone on the internet can open an issue, comment, send a Discord message, open a pull request
or post a test result. The bots turn that text into code. A pull request the issue bot opens
reaches testers' machines once its builds are green, and reaches `main`, and the next signed
release, after the waits in `claude_auto_merge.yml`. So **text from outside is the attack**, and
the automation's job is to keep it from becoming code that runs with our credentials.

## What we protect, most important first

1. The signed release (Windows installer signing, `main`, tags) and the users who install it.
2. Testers' machines (the community test runner installs bot builds).
3. Secrets: `CLAUDE_CODE_OAUTH_TOKEN`, `DISCORD_BOT_TOKEN`, signing credentials, `GITHUB_TOKEN`.
4. The subscription's budget (any account can start model runs) and the Discord server.
5. Visitors of the website (`docs/` is served as written).

## The rules

1. **Outside text is data.** A prompt tells the model what to do with an issue; nothing the
   issue says changes what it may do. A decision that matters (merge, label, allow) is made by
   plain code (shell, jq) that fails closed, never by a model reading what a model wrote.
2. **Outside text never goes into a script through `${{ }}`.** Put it in `env:` and read the
   variable. Applies to titles, bodies, comments, branch names, commit messages, labels.
   *Checked: `tools/check_workflow_security.py`, rule 2.*
3. **A workflow a stranger can trigger** (`issues`, `issue_comment`, `pull_request_target`,
   `workflow_run`, review comments) **never checks out or runs a pull request's code.** It checks
   out `main`, the workflow's own commit or the base. *Checked, rule 3.*
4. **Least privilege.** Every job declares its `permissions`. Write scopes sit only on jobs that
   run no model. No job gets the `workflows` permission, and no model job holds a secret it does
   not need. *Checked, rule 4 (existing gaps are in `tools/workflow_security_baseline.json`).*
5. **Third-party actions are pinned to a full commit SHA**, because a moved tag runs in our
   workflows with our credentials. First-party `actions/*` and local actions may use tags.
   *Checked, rule 5 (existing gaps in the baseline; the list only gets shorter).*
6. **A person merges anything that can change the rules or the release.** `PROTECTED` in
   `claude_auto_merge.yml` lists those paths: `.github/`, `tools/`, the installer and its
   tools, `devkits.json`, build definitions, and the files that steer the bots (`CLAUDE.md`,
   `AGENTS.md`, `SKILL.md`, `.claude/`, this file). *Checked, rule 6.* A change to the list is
   itself a change under `.github/`.
7. **A wait is shortened only by a trusted identity, never by an artifact anyone can create.**
   A commit status set from a comment, a label, the issue text, the size of a diff or a "bug fix"
   title is not trust: a stranger can produce all of them. A person with write access is. (This
   is why the fast lane of `claude_auto_merge.yml` reads who tested, not just that a test passed.)
8. **Every automation has an off switch and a cap.** Switches: `BOT_AUTO_MERGE`,
   `BOT_PR_STEWARD`, `DEVKIT_UPDATE`, `SITE_DATA`, `CLAUDE_RETRY`, `CLAUDE_SPLIT_NEXT_PART`,
   `API_COVERAGE` (repository variables, `false` stops it). Caps: `CLAUDE_MAX_RUNS`, the Discord
   bot's `MAX_ISSUES_PER_DAY`, the steward's fix rounds. A new automation that acts without a
   person gets both.
9. **Outside services get the smallest key.** A token for another service (the `/triage`
   Worker's GitHub token, a Discord token) is scoped to one repository and one permission,
   given an expiry, and kept in that service's secret store, never in a file or a prompt.
10. **No secret, no write token and no model in the same job as untrusted code.** If a job must
    run code from a branch, it has a read-only token and no secrets.

## Settings that cannot be in code (a maintainer confirms them)

Rules 3 to 6 only hold if the repository itself enforces them. The workflow headers say these
are load-bearing; this list is the one place to check them:

- [ ] A ruleset blocks direct pushes to `main`, and force pushes and deletions on all branches.
- [ ] Tag creation is restricted (the weekly release ships what is tagged).
- [ ] Default workflow permissions are read-only; Actions may not approve pull requests.
- [ ] Workflows from outside contributors need approval before they run.
- [ ] Secret scanning and push protection are on; the signing secrets live in an environment
      that needs approval (the SignPath approval already works this way).
- [ ] Two-factor authentication is required for everyone with write access.

## Risks the maintainers accept (decisions, not oversights)

From the workflow headers, kept here so a reviewer knows what is already decided:

- Any issue or comment starts a model run with write access (`claude_issue_triage.yml`).
- Bot pull requests merge without a person after `BOT_AUTO_MERGE_DELAY_HOURS` of a clean review;
  nothing but that review and the wait stands between a crafted issue and the next release.
- Bot builds reach testers' Archicad through `needs-test` before a person looks (#1138).
- `CLAUDE_CODE_OAUTH_TOKEN` is in the environment of model runs and could be copied out by an
  injected command; a run's cost is open to anyone who can open an issue.
- Anyone's `tapir-test:` comment sets an `archicad-test/*` status (it can block a merge, and by
  rule 7 it cannot speed one up).

## Before you add or change automation

- [ ] Which trigger starts it, and can a stranger cause it? If yes, rules 2, 3, 8 apply.
- [ ] What does the token and each secret in the job allow? Is that the minimum?
- [ ] Does any outside text reach a shell, a prompt or a file path? Through `env:` only?
- [ ] Is the decision plain code that fails closed? Is there a test for the refusing cases?
- [ ] Does it shorten a wait or widen what merges itself? Then rule 7: whose identity backs it?
- [ ] Off switch, cap, and a line in `CLAUDE.md` CI section.
- [ ] `python tools/check_workflow_security.py` passes.

## If something goes wrong

1. Stop the automation: set the off switches of rule 8 to `false`, close or draft the pull
   requests involved, add `do-not-merge`.
2. Revoke what may have leaked: `CLAUDE_CODE_OAUTH_TOKEN` (run `claude setup-token` again),
   the Discord bot token (reset it in the developer portal), any scoped token of rule 9.
3. Find what merged since: `git log` on `main`, the weekly release's compare link, the files
   under `PROTECTED`. Revert it with a pull request a person merges.
4. If a release went out, mark it as bad, publish a fixed one, and say so in the release notes.
