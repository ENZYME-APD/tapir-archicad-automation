# Where the repository is going and why

The decisions taken in #3030, as they stand. Each one is a direction, not
work already done; the pull requests that carry them out are named.

## 1. Fewer, cheaper bot runs

Every process that can be a script is a script. A bot run is for judgement
only: Archicad API behaviour, check expectations and review. The plan is for
the issue bot to run only on the `claude-fix` label, reviews only on a push,
one Claude run at a time, and the light model by default. This needs
`.github/` changes, done in a separate pull request from a fork (PR 2 of
#3030).

## 2. Schemas move out of the C++

Command schemas move into `archicad-addon/Sources/Schemas/<Command>.json`,
bundled into a `FILE` resource the way `CommonSchemaDefinitions.json` already
is (`RFIX/AddOnFix.grc`, loaded by `SchemaDefinitions.cpp`), and looked up by
command name in `CommandBase`. The 62 schemas built by code
(`BuildLibraryPartBasedSchema` and the like) stay in the C++ for now. It
starts with the seven Keynote commands, in a separate pull request that also
changes `tools/` (PR 3 of #3030).

## 3. Grasshopper components are generated

Only for commands that no Archicad tool component will cover, and keeping
each component's GUID and parameter order: saved definitions find components
by GUID and their parameters by position.

The count so far (`python sandbox/count_gh_passthrough.py`): **83 of 830
components are pass-through by the strict rules.** This is a floor, not the
real share. 204 components fail only rule 4, because a base class sends the
command, and 460 fail rule 8 for any loop, LINQ call or ternary, so many of
them may still be generatable. The decision waits on a second count that
treats base-class senders and one-to-one loops as pass-through.

## 4. Generated output stays committed

`docs/archicad-addon`, `python-client/tapir_archicad/commands.py` and the
site's copies for AI assistants stay committed until a maintainer moves
GitHub Pages to deploy from Actions. Until then gates keep them correct:
#3021, and a scheduled docs drift check (regenerate, then
`git diff --exit-code`) in PR 2 of #3030.

## 5. Icons

Icons are built from SVG only for the ~280 of 753 PNGs that have a source in
`IconSource/`. The others stay committed.

## 6. The full build matrix stays

Every add-on pull request keeps building every Archicad version on both
platforms. #2968 failed only on Windows AC25 and AC26 and passed on 27–30,
and bot pull requests are merged automatically once every build is green.

## 7. People: December

In December, two people each cut a release unaided with
[runbook.md](runbook.md), and one release runs with the bots off.

## 8. People: January

In January, people read a risk-ordered sample of merged bot pull requests:
behaviour changes first, documentation last.

## 9. For the maintainers to decide

- The Claude plan after January, and its usage ceiling.
- Whether bots may still open feature pull requests, or only review and
  triage.
