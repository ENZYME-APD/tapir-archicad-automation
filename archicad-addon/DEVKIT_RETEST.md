# Retesting API items blocked by Archicad

Some Archicad API items are skipped in the API coverage
([tools/api_coverage_rules.json](../tools/api_coverage_rules.json), section `skip`) not because
Tapir does not need them, but because Archicad itself gets in the way: a bug, a crash, or a DevKit
that does not compile. A later DevKit may fix them, so each of them is listed in
[devkit_retest.json](devkit_retest.json) with the steps to retest it. When Graphisoft publishes a new
DevKit, a person with Archicad runs those steps.

## The file

`devkit_retest.json` has two keys:

- `entries`: one object per Archicad problem. Items that share one problem are one entry.
  - `item` (one id) or `items` (a list of ids): the ids exactly as in the `skip` section of the
    rules file.
  - `problem`: one sentence saying what goes wrong.
  - `since`: the first Archicad version (major number, e.g. `29`) the problem was found in.
  - `issue`: the GitHub issue number, or `null`.
  - `howToRetest`: the steps, in order, one string each: what to build, which Tapir command or
    script to run, with which input, and what result passes.
  - `unblocks`: what Tapir could do once the item works.
  - `stillBroken`: the Archicad versions (numbers) a retest found still broken; empty until then.
- `otherSkips`: every other skip of the rules file, with its kind: `duplicate` (another call or
  command already covers it) or `obsolete` (older Archicad versions only, replaced). These are never
  retested.

Every skip of the rules file is in exactly one of the two. Check the file by hand with:

```bash
python archicad-addon/Test/check_devkit_retest.py
```

It fails when an id is not a skip in the rules file, a required key is missing, an id is listed
twice, or a skip of the rules file is in neither `entries` nor `otherSkips`. So when you add a skip
to the rules file, add it here too: as an entry with its retest steps when Archicad blocks it,
otherwise in `otherSkips`. When you remove a skip, remove it here.

## Retesting with a new DevKit

1. Build the add-on with the new DevKit. A new Archicad version is added to
   [devkits.json](devkits.json) by the DevKit update pull request; build that branch (or download
   the add-on its build check produces) for the new version. For a code change the steps ask for,
   build locally as described in the repository's README / CLAUDE.md ("Building the Add-On").
2. Load it in that Archicad version (Options > Add-On Manager) and open a project you can lose, or
   `archicad-addon/Test/TestProject.pla`. Save first: some of these problems crash Archicad.
3. Run the `howToRetest` steps of each entry, through `aclib` (see `archicad-addon/Examples/`) or
   any JSON client.
4. Revert any temporary code change the steps asked for.

### When it passes

In a pull request by a person (the bots may not edit `tools/`):

- remove the item's skip from `tools/api_coverage_rules.json` and its entry here (remove only the
  passing ids of a grouped entry if the others still fail);
- build the command or use the item in the add-on, guarded with `ServerMainVers_*` / `AC_VERSION` so
  the older Archicad versions keep their workaround;
- close the issue named in `issue`, saying which Archicad version and DevKit fixed it.

### When it fails

Add the Archicad version to the entry's `stillBroken` list (keep it ascending) in a pull request, so
the next retest knows. Nothing else changes; the skip stays.

## How the DevKit update reads it

The DevKit update (`tools/devkits.py`, `.github/workflows/archicad_devkit_update.yml`) is meant to
list every entry in the body of the pull request that adds a new Archicad version: one checklist
line per entry with its `item`/`items`, `problem` and `issue`, and its `howToRetest` steps below
it. It is meant to fail its check, as `check_devkit_retest.py` does, when a skip of the rules file
is in neither `entries` nor `otherSkips`. Both are wired in by a maintainer.
