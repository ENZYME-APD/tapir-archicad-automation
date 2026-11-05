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
see below), rename it and change it. A built-in script has these parts, in this order:

1. **The `# /// script` header** that uv reads:

   ```python
   # /// script
   # requires-python = ">=3.11"
   # dependencies = []
   # ///
   ```

   The built-in scripts use the standard library only (`dependencies = []`), so they start quickly
   and work offline; the window is made with `tkinter`.
2. **A docstring** with the script's name on the first line, `by <author>` on the second (a link or
   a "Modified by" line may follow), then a few sentences on what the script does.
3. **`SCRIPT_NAME`**, the name shown in the window's title and in its error messages.
4. **The window**, made with [utilities/ui.py](utilities/ui.py):
   - `ScriptWindow(title, description)`: a header with the name and the description, a `body`
     frame for your widgets, and a footer with a status line, a *Keep on top* switch and the
     buttons. `add_button(text, action, primary=False)` adds a button, `set_status(text, kind)`
     writes the status line (`kind` is `info`, `ok`, `warning` or `error`), `with busy(text):`
     disables the buttons while a long call runs, `guarded(action)` shows an error in the status
     line instead of closing the window (buttons are guarded already), and
     `run(on_start=..., on_close=..., can_close=...)` shows the window.
   - `make_tree(parent, columns)`: a list with a scrollbar; `columns` holds
     `(id, heading, width, anchor)` tuples. Place it with `tree.container.grid(...)`.
   - `require_archicad(SCRIPT_NAME)`: call it first; it stops with a clear message when no
     Archicad answers.
5. **The Archicad calls**, through [utilities/archicad.py](utilities/archicad.py). These raise
   `ArchicadError`, which the window shows in its status line:
   - `run_tapir(command, parameters)` runs a [Tapir command](https://enzyme-apd.github.io/tapir-archicad-automation/archicad-addon),
     `run_command(command, parameters)` an official one (`API.` is added when missing).
   - `get_details_of_elements(elements, fields)`, `get_elements(scope, element_types, filters)`
     (`scope` is `selection`, `story` or `project`), `get_story_names()`,
     `select_elements(element_ids)`, `item_errors(items)` and `batched(items)`.
6. **`if __name__ == "__main__":`**, which calls `require_archicad(SCRIPT_NAME)` and opens the
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

### A pull request to this folder

Put the script into `builtin-scripts/` and open a pull request. Once it is merged into `main`, it
appears in every Tapir user's palette the next time they start Archicad or reload the scripts; no
Tapir release is needed. Keep it to the standard library and the helpers above, and check that it
works with the released Tapir Add-On.

### Your own GitHub repository

Put the script, with copies of `utilities/` and `aclib/`, into a public GitHub repository, in its
root or in a folder. Its users add an entry to `repositories` in their `config.json`, after the
Tapir one:

```json
{
    "displayName": "Jane's scripts",
    "repoOwner": "jane",
    "repoName": "my-tapir-scripts",
    "relativeLoc": "scripts"
}
```

Leave `relativeLoc` empty (`""`) when the scripts are in the root. Add `includePattern` or
`excludePattern` (see above) when other files should or should not be listed. The scripts are read
from the repository's default branch, and the palette's *Open in Browser* button links to the file on the
`main` branch, so name it `main`.

### The community list

[community.json](community.json) lists community script repositories. It is a list of entries
like this:

```json
{
    "displayName": "Jane's scripts",
    "repoOwner": "jane",
    "repoName": "my-tapir-scripts",
    "relativeLoc": "scripts",
    "description": "Scripts for renumbering doors and windows.",
    "author": "Jane Doe"
}
```

To list your repository, open a pull request that adds one line, your entry, to it. Each entry can
be pasted into `repositories` in `config.json` as it is (the palette ignores `description` and
`author`). The palette does not read `community.json` itself yet
([#1847](https://github.com/ENZYME-APD/tapir-archicad-automation/issues/1847)).
