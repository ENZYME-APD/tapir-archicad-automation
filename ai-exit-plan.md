# Maintaining Tapir when the Claude subscription ends in January

Measured on 2026-11-23 against `main` (`17ac834`).

**What this document is.** The build, release and test pipelines do not need the Claude subscription that
currently pays for them; the *change volume* and the *code review* do. The subscription ends in January.
This is a proposal for what to do about that, in date order, addressed to the maintainers, who are the only
people who can decide the items in section 6. Everything here is a proposal, not a record of a decision.

## 1. What the AI actually carries

| | 90 days |
| --- | --- |
| PRs opened by `github-actions[bot]` | 1,029 of 1,507 (**68%**) |
| bot PRs merged | ~90% (26 of the last 29) |
| what they contain | behaviour changes to existing commands, new commands, checks, documentation |
| code review of every PR | the Claude review bot |
| issue triage, Discord answers, PR stewardship | the same subscription |
| authentication | `secrets.CLAUDE_CODE_OAUTH_TOKEN` (subscription); `ANTHROPIC_API_KEY` already wired as a fallback |

This is not "AI assists". The AI is the maintainer of record for change volume, and the second reviewer
for every human PR.

## 2. Why the repository fits an AI so well

The unit of work is a command, and a command costs about ten artifacts in eight to eleven places: the C++
implementation, the command list, the JSON schema, the generated header, the command reference page, the
Python client, the Grasshopper component, its node registration, the marketplace entry with its image, and
a `Test/checks/` file. That ritual is tedious for a person and free for an agent, so the repository grew a
shape that only an agent wants to maintain. Removing the AI without changing the ritual does not produce a
smaller job; it produces an unstaffed one.

## 3. What does *not* depend on the subscription

Builds (six Archicad versions, Windows and macOS), releases, the schema generator, the command reference,
the checks suite and the site are deterministic. Without a token the repository still compiles and ships
releases. What stops is change velocity and review.

## 4. Plan, by when

### Now — the auth swap (about a day)
- `CLAUDE_CODE_OAUTH_TOKEN` → `ANTHROPIC_API_KEY`, which the workflows already support.
- Scope the agent to **review and triage only**; turn off autonomous feature PRs first, since they are
  the expensive part (1,029 PRs a quarter) and the part that adds surface faster than anyone verifies it.
- Set a monthly token budget and an alarm.

### December, first half — remove the ritual (the real fix)
Three moves, in this order, each shippable on its own:
1. per-command schemas from C++ literals to data, compiled into a header (pilot: the four Keynotes
   commands, built on all six versions);
2. Grasshopper components generated from ~10 lines of mapping data instead of 95 lines of C# each;
3. generated output (75k lines of command reference, 11k of Python client) out of git.

End state: **a new command is two places instead of ten** — small enough for a volunteer, and small
enough that any agent, or none, can do it.

### December, second half — make review a check, not a subscription
- Already shipped: `tools/check_contracts.py` (#3022), the version-sync and schema-expander tests (#3023),
  and a release that no longer fails because one hosted runner was never picked up (#3025).
- Already there: `check_examples.py`, `check_marketplace.py`, `check_tool_settings.py`.
- Still missing: a generated-file drift gate — regenerate, diff, fail on a diff nobody committed. Written
  once already and withdrawn; it is about forty lines, and its only real constraint is that generating the
  command reference takes ~15 minutes, so it belongs on a schedule rather than on every PR.
Everything the review bot caught in practice was mechanical: a rule table copied into a test, a dead note
path, a CI claim the diff did not support. Mechanical review can be encoded. Taste cannot — but taste can
be written down.

### December, last two weeks — record the judgement, and the people
- `CLAUDE.md` (684 lines) is the operational spec: publish it also as `AGENTS.md` so that any tool or
  person reads the same instructions, and vendor-neutral.
- Write down the decisions that are policy rather than code: deprecation, which commands are supported
  per Archicad version, and what "best effort" means for the rest.
- Two people who have **each cut a release themselves**, following the documented path, before January.
- One release run with the AI workflows disabled. A drill, in December, not a discovery in January.

### January — the sampling audit
1,029 merged bot PRs means a large amount of code no human has read. Reviewing all of it is impossible.
Risk-ordered sampling is not: behaviour changes first, documentation last, with the gates above making
each sample cheap to check. This is the same work as the checks, read in the other direction.

## 5. What will genuinely be lost

- **Throughput**, by a large factor. That is survivable and it is the point of the refactor: fewer, better
  places to change.
- **Instant review.** Replaceable only partly; the checks cover correctness, a person must cover judgement.
- **Nothing in knowledge**, provided the two items above happen. No knowledge exists only inside the model.

## 6. What the maintainers need to decide

1. Whether to pay for the API after January, and at what monthly ceiling.
2. Whether an agent may keep opening feature PRs — `github-actions[bot]` opened 68% of the last 90 days of
   PRs — or only review and triage.
3. Whether this document stays in the repository, becomes an issue, or is deleted once decided.

## 7. The first three things, if only three happen

1. Swap the auth and scope the agent down (a day, stops the cliff being a surprise).
2. Ship the Keynotes schema pilot (proves the pattern that makes a command small).
3. Two humans cut a release, unaided, in December.
