# Tapir script marketplace

Scripts shared by Tapir users, and scripts requested in issues, reviewed like any
other code in this repository.
Everything here is free and open source.

Marketplace scripts are not built-in scripts: they are never copied to
[`builtin-scripts/`](../builtin-scripts/), and nobody gets them without choosing
to install one. The website's `/scripts/` page lists them, and the Tapir
Palette will install them from there.

This page is the format of a listing and the rules a shared script is adapted
to before it is listed. It is read by script authors, by reviewers, and by the
issue bot, which turns a shared script into a pull request.

## A listing

Each script is one folder, named after its slug:

```
marketplace/<slug>/
  listing.json   name, summary, category, tags, author, licence, entry, versions
  <script>.py    the script, with its # /// script and # /// tapir headers
  README.md      what it does, its settings, its limits, "Original by <author>"
  screenshots/   optional: up to 5 PNG or JPG files, at most 1 MB each
```

The folder holds nothing else: no binaries, no archives, no copies of
`aclib` or `utilities`.

### `listing.json`

[`listing.schema.json`](listing.schema.json) is its JSON Schema.

| Field | Content |
| --- | --- |
| `slug` | The folder's name: lowercase letters, digits and hyphens, e.g. `door-numbering-by-zone`. |
| `name` | The name shown on the site and in the palette, at most 60 characters. |
| `summary` | What it does, at most 160 characters, in plain words. |
| `category` | One of the categories below. |
| `tags` | At most 8 lowercase words or hyphenated words, e.g. `doors`, `numbering`. |
| `author` | `name` (as the author wants to be credited), optional `github` (login without `@`), optional `discord` (user name). |
| `license` | The SPDX identifier of an OSI-approved licence. `MIT` unless the author chose another. |
| `entry` | The `.py` file in the folder that the palette runs. |
| `versions` | The released versions, newest first. Each has `version` (`MAJOR.MINOR.PATCH` of the script), `archicadVersions` (the Archicad versions it works in, e.g. `[28, 29, 30]`), `minTapirVersion` (the oldest Tapir Add-On with every command it uses) and `changelog` (what changed, in plain words). |

There is no price field: every listing is free.

Example:

```json
{
    "slug": "door-numbering-by-zone",
    "name": "Door numbering by zone",
    "summary": "Numbers the doors of each zone from the zone number, in the order you pick.",
    "category": "element-data-properties",
    "tags": ["doors", "zones", "numbering"],
    "author": { "name": "Jane Architect", "github": "janearchitect" },
    "license": "MIT",
    "entry": "door_numbering_by_zone.py",
    "versions": [
        {
            "version": "1.1.0",
            "archicadVersions": [28, 29, 30],
            "minTapirVersion": "1.8.1",
            "changelog": "Leaves doors without a zone alone instead of stopping."
        },
        {
            "version": "1.0.0",
            "archicadVersions": [28, 29],
            "minTapirVersion": "1.7.0",
            "changelog": "First version."
        }
    ]
}
```

### Categories

| `category` | For scripts that… |
| --- | --- |
| `documentation-layouts` | make or change layouts, drawings, publisher sets |
| `element-data-properties` | read or write element properties, IDs and parameters |
| `modeling-geometry` | create or change model elements |
| `zones-areas` | work on zones and areas |
| `import-export` | read or write files (Excel, CSV, IFC, …) |
| `quality-assurance` | check a model and report problems |
| `quantities-scheduling` | count, measure and list |
| `libraries-objects` | work on library parts and objects |
| `navigator-views` | work on views, view maps and the navigator |
| `classification-attributes` | work on classifications and attributes (layers, materials, …) |
| `batch-automation` | repeat a task over many projects, views or elements |
| `utilities` | anything else |

### The script's headers

The script starts with two comment blocks. The first is the standard
[inline script metadata](https://packaging.python.org/en/latest/specifications/inline-script-metadata/)
that `uv` reads; the second is Tapir's own, which `uv` ignores. Leave an empty line
between them: without it `uv` reads both blocks as one and refuses to run the script.

```python
# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# category = "element-data-properties"
# archicad-versions = [28, 29, 30]
# min-tapir-version = "1.8.1"
# changes = "model"
# ///
```

- `dependencies` lists every package from PyPI the script imports, so `uv`
  installs them. Standard library only is best.
- `category`, `archicad-versions` and `min-tapir-version` are the same as in the
  newest entry of `listing.json`. The Tapir palette uses them before it runs the
  script: it does not run a script whose `min-tapir-version` (written `N.N.N`) is
  newer than the installed Tapir and offers the update instead, and it warns,
  with **Run anyway**, when the running Archicad is not in `archicad-versions`.
- `changes` says what running the script can change:
  - `model`: elements, properties, attributes, views or anything else in the
    project. Shown as **Changes the model**.
  - `files`: it only writes files (exports, reports, logs). Shown as
    **Writes files**, with the files it writes named in its README.
  - `read-only`: it changes nothing and writes nothing. Shown as **Read only**.

  When in doubt, `model`.

### The README

Plain words, for architects:

1. What it does, in one or two sentences.
2. How to use it: what to select or open first, what the window asks for.
3. Its settings, with their defaults.
4. What it changes (or that it changes nothing), and which files it writes, where.
5. Its limits: element types, Archicad versions, Teamwork, units.
6. `Original by <author>`, and who adapted it if someone did; for a script
   requested in an issue, `Requested by @<login> in #<issue number>, written with
   the Tapir issue bot.` (see below).

### Screenshots

Up to 5 PNG or JPG files in `screenshots/`, at most 1 MB each, of the script
running in Archicad. Crop them to the script's window and the model it acts on;
no client names, project names or people in them.

## A script requested in an issue

A new script that someone asks for in an issue, and that the issue bot or a
contributor writes for them, goes here too, as `marketplace/<slug>/` in the
format above. It goes to [`builtin-scripts/`](../builtin-scripts/) only when a
maintainer asks for that in the issue, because every built-in script is in
every Tapir user's palette.

- The listing's `author` is the person who opened the issue: they specified
  the script and the issue bot wrote the code, as other contributors write
  code with AI help. `name` is their GitHub profile name when they have set
  one, else their login, and `github` is their login, e.g.
  `{ "name": "Parikshit Wadhwa", "github": "parikshit99" }`.
- Its README credits the request:
  `Requested by @<login> in #<issue number>, written with the Tapir issue bot.`
- An issue a bot opened (the Discord issue bot files them) has no GitHub
  author to name: the listing's `author` stays `{ "name": "Tapir" }`, and the
  README credits the Discord user who asked for it by name.
- Everything else (headers, README, the rules below) is as for a shared
  script; there is no author to ask, so the person who asked for it is asked
  to try it.

## The helpers `utilities` and `aclib`

A marketplace script may import the helpers of the built-in scripts,
[`builtin-scripts/utilities/`](../builtin-scripts/utilities/) and
[`builtin-scripts/aclib/`](../builtin-scripts/aclib/), like a built-in script
does. Its folder holds no copies of them; they are imported from the folder the
script runs in, so:

- Installed by hand (see
  [Using a marketplace script in the palette](../builtin-scripts/README.md#using-a-marketplace-script-in-the-palette)),
  copy `utilities/` and `aclib/` from `builtin-scripts/` next to the script.
- The palette's marketplace window, when it installs a script that imports
  them, puts copies of both folders, from `main`, next to it.

So changes to `utilities` and `aclib` stay backward compatible: a function or
argument a script uses is never removed or changed in meaning.

## Adapting a shared script for public use

A shared script was written for one office and one project. Before it is
listed it is adapted so that it is safe and works for others, **without changing
what it does**. Every change is listed in the pull request in plain words, and
the author is asked to try the adapted version and confirm it still does what
they meant.

### Security: removed, or flagged in the pull request

- Network calls other than to Archicad on the local machine (`127.0.0.1` or
  `localhost`, at the port the palette passes in).
- `subprocess`, `os.system`, `eval`, `exec`, and anything else that runs
  other programs or code built from text.
- Installing packages while running (`pip install`); packages belong in
  `dependencies`.
- Deleting or overwriting files, and writing anywhere other than the project's
  folder and `Documents/Tapir`.
- Passwords, tokens and keys.
- User names, `C:\Users\…` and `/Users/…` paths, and project, client or office
  names, in code and in comments.

A script that cannot do its job without one of these is not listed until a
maintainer has looked at it.

### Made to fit other offices

- Hard-coded property names, layer names, stories, paths and classification
  codes become settings at the top of the script, or fields of its window
  (`ShowScriptUI`, or the palette scripts' `ScriptWindow`).
- Built-in properties are found by their non-localized ID (`API.GetPropertyIds`
  with `{"type": "BuiltIn", "nonLocalizedName": "General_ElementID"}`), not by
  their name, which depends on the language of Archicad.
- Lengths and areas work in metric and imperial units.
- Paths work on Windows and on macOS (`pathlib`, no `\` in strings).

### Safe to run

- It says what it is about to change before changing it.
- It does nothing when nothing fits (no selection, no matching elements),
  and says so.
- It makes its changes in one Undo step where it can: a Tapir command changes
  all the elements it is given in one Undo step, so the script passes them all
  in one call instead of making one call per element.
- When a property, attribute or element type it needs is missing, it shows a
  clear message instead of crashing.

### Headers and README

- Every package it imports is in `# /// script` `dependencies`.
- `# /// tapir` has its category, Archicad versions, Tapir version and `changes`.
- The README is written as above and credits the author.

## Sharing a script

- From the Tapir Palette's Share button or in the Discord channel
  `#share-a-script`: the script arrives as an issue labelled `script-share`, and
  the issue bot opens a pull request adding its folder here. No GitHub account
  is needed.
- By a pull request adding `marketplace/<slug>/` yourself.

Either way the pull request is reviewed before it is merged, and the script is
published under its licence (MIT unless you chose another).

`python tools/check_marketplace.py` checks every folder against this page: the
listing, the two headers, the README and that the folder holds nothing else.
The add-on build check runs it on every pull request. Once a folder is on
`main` and passes, `tools/generate_catalogue.py` adds it to the list the
website's Scripts page shows (`catalogue.json` on the `site-data` branch,
written by `.github/workflows/site_data.yml` within minutes of the merge).
