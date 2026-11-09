# Tapir palette scripts

The Tapir Palette in Archicad (*Tapir Palette* in the Tapir menu) runs Python scripts that drive
Archicad through the Tapir and official JSON commands. The scripts in this folder are the built-in
ones: every Tapir user has them in the palette. This page explains how such a script is written,
how to try your own, and how to share it.

## Where the palette gets its scripts

The palette reads `Documents/Tapir/config.json`, which it creates with the defaults when it does
not exist yet. Its list of scripts is built when Archicad starts and again when you choose
*Reload scripts...* at the bottom of the palette's script list, from:

- **`repositories`:** GitHub repositories. The default entry is this folder:

  ```json
  {
      "displayName": "Tapir",
      "repoOwner": "ENZYME-APD",
      "repoName": "tapir-archicad-automation",
      "relativeLoc": "builtin-scripts"
  }
  ```

  Every file below `relativeLoc` in the repository's default branch is downloaded into a temporary
  folder, keeping its path, so a script finds the files next to it. The scripts are listed as
  `<file name> (<displayName>)`, or `<file name> (<repoOwner>/<repoName>)` without a `displayName`.
- **`localFolders`:** folders on your computer or a network drive (`displayName`, `path`). The
  scripts run from where they are; nothing is copied.
- The `Documents/Tapir/custom-scripts` folder and the scripts added with the palette's
  *Add Script* button.

By default only the `.py` files directly in `relativeLoc` (or the local folder) are listed; files in
subfolders, like `utilities/` here, are downloaded but not listed. That is how helper modules and
[template/TEMPLATE.py](template/TEMPLATE.py) stay out of the list. A repository or local folder can
change this with patterns, which are regular expressions that must match the whole path:

| Key | Matched against | Effect |
| --- | --- | --- |
| `excludePattern` | The path in the repository, including `relativeLoc` (`builtin-scripts/Foo.py`); in a local folder, the path relative to the folder (`Foo.py`) | Every file that does not match is listed, `.py` or not, in subfolders too |
| `includePattern` | The same | Only the matching files are listed; ignored when `excludePattern` is set |
| `excludeFromDownloadPattern` (repositories only) | `<repoOwner>/<repoName>/<path in the repository>` | The matching files are not downloaded |

A repository entry can also have a `token` (a GitHub token for a private repository; it is sent to
GitHub and passed to the scripts as `--token`), and both kinds of entry a `uvProject` (the folder of
a `pyproject.toml`, relative to `relativeLoc` or the local folder) to run the scripts in that uv
project instead of as standalone scripts.

The tooltip of the script list and of the *Run Script* button shows the selected script's summary
and what it changes, both read from the script itself (see [Writing a script](#writing-a-script)).

## How a script runs

The palette runs a `.py` file with [uv](https://docs.astral.sh/uv/):

```
uv run --script <script>.py --port <port of this Archicad>
```

uv reads the script's dependencies and Python version from its inline metadata, makes an
environment for it and runs it. `aclib` reads `--port` (and `--host`) from the command line, so the
script talks to the Archicad whose palette started it, even when several are running. The palette
reports what the script prints and its exit code.

## Writing a script

Start from [template/TEMPLATE.py](template/TEMPLATE.py): copy it into this folder (or your own,
see [Testing your script](#testing-your-script)), rename it and change it. A built-in script has
these parts, in this order:

1. **The `# /// script` header** that uv reads:

   ```python
   # /// script
   # requires-python = ">=3.11"
   # dependencies = []
   # ///
   ```

   The built-in scripts use the standard library only (`dependencies = []`), so they start quickly
   and work offline; the window is made with `tkinter`.
2. **The `# /// tapir` header**, Tapir's own, which uv ignores:

   ```python
   # /// tapir
   # category = "element-data-properties"
   # archicad-versions = [25, 26, 27, 28, 29, 30]
   # min-tapir-version = "1.5.7"
   # changes = "read-only"
   # ///
   ```

   `changes` says what running the script can change: `model` (anything in the project), `files`
   (it only writes files) or `read-only` (nothing). The palette shows it as *Changes the model*,
   *Writes files* or *Read only*; when in doubt, write `model`. `category` is one of the
   [marketplace's categories](../marketplace/README.md#categories); the palette groups the scripts
   by it, and a script without one is listed under *Other*. A built-in script needs `changes` and
   `category` (the category of its marketplace copy, when it has one). A
   [marketplace](#sharing-your-script) script also needs `archicad-versions` (the
   Archicad versions it works in) and `min-tapir-version` (the newest of the versions the
   [command reference](https://archicad-api.com/archicad-addon/) shows
   for the Tapir commands it uses). [marketplace/README.md](../marketplace/README.md#the-scripts-headers)
   defines these keys.
3. **A docstring** with the script's name alone on its first line, a blank line, then one paragraph
   on what the script does. The palette shows that paragraph as the script's summary. Credits (`by
   <author>`, a link, a "Modified by" line) and more details go in later paragraphs.
4. **`SCRIPT_NAME`**, the name shown in the window's title and in its error messages.
5. **The window**, made with [utilities/ui.py](utilities/ui.py):
   - `ScriptWindow(title, description)`: a header with the name and the description, a `body`
     frame for your widgets, and a footer with a status line, a *Keep on top* switch and the
     buttons. `add_button(text, action, primary=False)` adds a button, `set_status(text, kind)`
     writes the status line (`kind` is `info`, `ok`, `warning` or `error`), `with busy(text):`
     disables the buttons while a long call runs, `guarded(action)` shows an error in the status
     line instead of closing the window (buttons are guarded already), `ask_ok_cancel(title,
     message)` asks before a change, and `run(on_start=..., on_close=..., can_close=...)` shows the
     window.
   - `make_tree(parent, columns)`: a list with a scrollbar; `columns` holds
     `(id, heading, width, anchor)` tuples. Place it with `tree.container.grid(...)`.
   - `require_archicad(SCRIPT_NAME)`: call it first; it stops with a clear message when no
     Archicad answers.
6. **The Archicad calls**, through [utilities/archicad.py](utilities/archicad.py). These raise
   `ArchicadError`, which the window shows in its status line:
   - `run_tapir(command, parameters)` runs a [Tapir command](https://archicad-api.com/archicad-addon/),
     `run_command(command, parameters)` an official one (`API.` is added when missing).
   - `get_details_of_elements(elements, fields)`, `get_elements(scope, element_types, filters)`
     (`scope` is `selection`, `story` or `project`), `get_story_names()`,
     `select_elements(element_ids)`, `item_errors(items)` and `batched(items)`.

   To save a table, [utilities/export.py](utilities/export.py) has `write_table(path, headers,
   rows)`, which writes an Excel workbook for an `.xlsx` path and a CSV file otherwise, and
   `FILE_TYPES` for the save dialog.
7. **`if __name__ == "__main__":`**, which calls `require_archicad(SCRIPT_NAME)` and opens the
   window.

The script imports `utilities` and `aclib` from the folder it is in, so it must stay next to the
`utilities/` and `aclib/` folders.

## Testing your script

Add a local folder to `localFolders` in `Documents/Tapir/config.json`, keeping the existing
entries:

```json
"localFolders": [
    {
        "displayName": "My scripts",
        "path": "C:\\Users\\me\\tapir-archicad-automation\\builtin-scripts"
    }
]
```

On macOS the path looks like `/Users/me/tapir-archicad-automation/builtin-scripts`. The easiest
folder is this one in your clone of the repository; any other folder needs copies of `utilities/`
and `aclib/` next to the script. Then choose *Reload scripts...* in the palette: your script appears
as `<file name> (My scripts)`. Edits to the file take effect the next time you run it.

## Sharing your script

Scripts made by Tapir users are shared in this repository's [script marketplace](../marketplace/README.md):
one folder per script, `marketplace/<slug>/`, with the script, a `listing.json` and a `README.md`.
They are not built-in scripts: nobody gets one without choosing to install it. The website's
[Share a script](https://archicad-api.com/scripts/share/) page explains it for users.

There are three ways to send a script:

- **On Discord:** post it in the `#share-a-script` channel of the
  [Tapir Discord server](https://discord.gg/NAnSennmpY), with a few words on what it does and which
  Archicad version you use. No GitHub account is needed.
- **From the Tapir palette:** the palette is getting a *Share* command that does the same, and
  shows you the whole file before anything is sent. Until your palette has it, use Discord.
- **As a pull request:** add the folder `marketplace/<slug>/` yourself, in the format that
  [marketplace/README.md](../marketplace/README.md#a-listing) describes.

A script sent on Discord or from the palette arrives as a GitHub issue labelled `script-share`.
Tapir's issue bot then opens a pull request that adds its folder and adapts the script for public
use, by the [rules in marketplace/README.md](../marketplace/README.md#adapting-a-shared-script-for-public-use):
risky code is removed or flagged, hard-coded names become settings, and the script works in every
Archicad language and in metric and imperial units. It keeps what the script does, lists every
change in plain words, and asks you to try the adapted version and confirm that it still does what
you meant. Every pull request is reviewed before it is merged. The script is then listed on the
website's [Scripts page](https://archicad-api.com/scripts/), with your name.

Every shared script is free and open source: it is published under the MIT licence unless you
choose another open-source licence. Share only a script you wrote, or one whose author agreed to
share it.

The scripts shared so far are on the [current marketplace](https://tapir-marketplace.vercel.app/)
until they move here.

### A built-in script

Every built-in script is in every Tapir user's palette, so this folder gets a new script only when a
maintainer wants it there. A script asked for in an issue goes to the marketplace, as
`marketplace/<slug>/`, unless a maintainer asks in the issue for a built-in one
([A script requested in an issue](../marketplace/README.md#a-script-requested-in-an-issue)).

To propose a script for every Tapir user's palette instead, put it into `builtin-scripts/` and open
a pull request here. Once it is merged into `main`, it appears in every Tapir user's palette the
next time they start Archicad or reload the scripts; no Tapir release is needed. Keep it to the
standard library and the helpers above, give it a `# /// tapir` header with `changes`, and check
that it works with the released Tapir Add-On.

## Using a marketplace script in the palette

The palette is getting a window to find and install marketplace scripts. Until your palette has
it, download the script into a folder on your computer and add that folder to `localFolders` in
`Documents/Tapir/config.json`, keeping the existing entries:

```json
"localFolders": [
    {
        "displayName": "Marketplace",
        "path": "C:\\Users\\me\\Documents\\Tapir scripts"
    }
]
```

On macOS the path looks like `/Users/me/Documents/Tapir scripts`. A marketplace folder holds no
copies of `utilities/` or `aclib/`, so a script that imports them needs copies of those folders next
to it. Then choose *Reload scripts...* in the palette: the script appears as
`<file name> (Marketplace)`. The website's [Get started](https://archicad-api.com/start/#marketplace)
page shows the same steps.
