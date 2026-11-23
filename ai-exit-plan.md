# Maintaining Tapir when the Claude subscription ends

Measured on 2026-11-23 against `main` (`17ac834`).

**What this document is.** The build, release and test pipelines do not need the Claude subscription that
currently pays for them. The *change volume*, the *checks worth running* and the *code review* do. That
subscription ends in January. This is a proposal for what to do about it, in date order, addressed to the
maintainers, who are the only people who can decide the items in section 7. Everything here is a proposal,
not a record of a decision.

## 1. What the AI actually carries

| | last 90 days |
| --- | --- |
| PRs opened by `github-actions[bot]` | 1,029 of 1,507 (**68%**) |
| of those, merged | ~90% (26 of the last 29) |
| what they contain | behaviour changes to existing commands, new commands, checks, documentation |
| code review of every PR | the Claude review bot |
| issue triage, Discord answers, PR stewardship (labels, `needs-test`, build dispatch) | the same subscription |
| authentication | `secrets.CLAUDE_CODE_OAUTH_TOKEN`, a subscription token, in all four agent workflows |

The four workflows that would stop are `claude_pr_review.yml`, `claude_issue_triage.yml`,
`claude_bot_pr_steward.yml` and `discord_issue_bot.yml`. The review bot is, in practice, the second reviewer
on every human PR as well.

## 2. What the repository already does without it

More than the file count suggests, and this matters for the argument:

- **`tools/new_command.py`** writes the repetitive part of a new command from a short spec: the class, the
  input/response schemas, an `Execute` skeleton, the `RegisterCommand` call in `AddOnMain.cpp`, a starter
  example and a starter check.
- **`python-client/generate.py`** writes the Python client from the command reference, and
  `python-client/generate.py --check` (plus `tests/test_client.py`) keeps the committed file honest.
- **`tools/generate_addon_docs.py`** writes the command reference, and `archicad_addon_build_check.yml`
  regenerates it on every PR, then runs `check_examples.py`, `check_marketplace.py` and
  `generate_catalogue.py --self-test`.

So adding a command by hand is two or three places, not ten: the `Execute` body, the check's real
parameters and expectations (and any `Test/ExpectedOutputs/` golden), and a Grasshopper component `.cs` if
the command is exposed there. The ritual is largely scripted already.

## 3. What therefore actually depends on the AI

Not the typing. The judgement:

- **the Archicad API behaviour inside `Execute`.** Most of the 1,029 bot PRs change the behaviour of an
  existing command — `CreateStairs: accept a per-item layerIndex`, `ModifySections: move the section line
  and depth`, `CloseProject fails when Archicad leaves the project open` — not add surface area;
- **checks whose expectations catch the bug**, rather than prove that the command answers. The community
  test runner runs them against real Archicad, so a weak expectation is invisible until a user hits it;
- **review.**

No generator writes any of those three, and a volunteer needs months of Archicad API time to learn them.
That is the dependency, and it is why more tooling alone will not remove it.

## 4. What the subscription buys, and what replacing it costs

A flat subscription becomes a per-token bill. Review-only, on one model, over this repository's PR volume,
is a small fraction of what the current volume costs — the expensive part is 1,029 PRs a quarter of
autonomous feature work, which is also the part least reviewed by people.

## 5. Plan, by date

### Now — the auth swap (a day, four workflows)
All four agent workflows authenticate with `secrets.CLAUDE_CODE_OAUTH_TOKEN`, but not the same way, so
there is no single edit:

- `claude_pr_review.yml:345` and `claude_issue_triage.yml` pass the action input `claude_code_oauth_token`;
  both already carry a commented-out `anthropic_api_key:` line (`:14` and `:120`), so there the change is
  uncomment that and replace the OAuth line;
- `claude_bot_pr_steward.yml:769` passes the same input and has no API-key line, so one has to be added;
- `discord_issue_bot.yml` does not use the action input at all: it exports `CLAUDE_CODE_OAUTH_TOKEN` as an
  environment variable for the CLI (`:127`) and gates on a `HAVE_CLAUDE_TOKEN` boolean (`:107`), so both
  the variable and the condition move to `ANTHROPIC_API_KEY`.

Then add the secret and rotate the OAuth token out.

- Scope the agent to **review and triage**; stop autonomous feature PRs first, since they are the
  expensive part and the part no human reads.
- Set a monthly token ceiling and an alert on it.

### December, first half — the one gate still missing
Regenerating the docs without diffing them means a PR can leave `docs/archicad-addon` stale until the
weekly release fixes it. A drift gate is "regenerate, `git diff --exit-code`", and it should run on a
schedule rather than on every PR because generating the reference takes ~15 minutes. The client already
has its equivalent (`--check`), and two more gates are written and green but **not merged**: the schema
contract check (#3022) and the version/schema-expander tests (#3023).

### December, second half — record the judgement, and the people
- `CLAUDE.md` (683 lines) is the operational spec: publish it also as `AGENTS.md`, so that any tool or
  person reads the same instructions and the file stops being vendor-shaped.
- Write down the decisions that are policy rather than code: deprecation, which commands are supported per
  Archicad version, and what "best effort" means for the rest.
- Two people who have **each cut a release unaided**, following the documented path, before January.
- One release run with the AI workflows disabled. A drill in December, not a discovery in January.

### January — the sampling audit
1,029 merged bot PRs means a large amount of code and Archicad behaviour that no human has read. Reviewing
all of it is impossible; risk-ordered sampling is not. Behaviour changes first, documentation last, and the
gates above make each sample cheap to check. This is the checks work read in the other direction.

## 6. What will genuinely be lost

- **Throughput**, by a large factor, and mostly in exactly the third of the work that is API judgement.
  Survivable; nothing about it is irreversible.
- **Instant review.** Partly replaceable by gates, which cover correctness but not taste.
- **Nothing in knowledge**, provided section 5's last two items happen. No knowledge exists only inside
  the model; it exists in `CLAUDE.md`, in the checks, and in people who have not yet been asked to learn
  it.

## 7. What the maintainers need to decide

1. Whether to pay for the API after January, and at what monthly ceiling.
2. Whether an agent may keep opening feature PRs (`github-actions[bot]` opened 68% of the last 90 days of
   PRs) or only review and triage.
3. Whether this document stays in the repository, becomes an issue, or is deleted once decided.

## 8. The first three things, if only three happen

1. Swap the auth and scope the agent down — a day, and it turns an abrupt end into a budget decision.
2. Two people cut a release unaided, in December, while the AI is still available to compare against.
3. Land the two open gates (#3022, #3023) and the docs drift check, so that correctness review does not
   leave with the subscription.
