# Examples

One short Python script per Tapir command, in one folder per command group:
`Examples/<GroupFolder>/<CommandName>.py`, e.g. `Application/GetAddOnVersion.py` or
`MEP/GetMEPPorts.py`. The [command reference](https://enzyme-apd.github.io/tapir-archicad-automation/archicad-addon)
shows each command's script in its *Example* popdown, loaded from this folder on `main`.

The folder is the group name as the command reference lists it, without the trailing
"Commands" and any part in parentheses, in PascalCase, all-caps words kept: "Custom Data Commands"
is `CustomData`, both "MEP Commands" and "MEP Commands (extended)" are `MEP`. The Developer Commands
have no examples.

## Running an example

Start Archicad with the Tapir Add-On, open a project, then:

```
python archicad-addon/Examples/Application/GetAddOnVersion.py
```

`aclib/` is the helper every example uses: `aclib.RunTapirCommand (name, parameters)` runs a Tapir
command and returns its response, `aclib.RunCommand ('API.<Command>', parameters)` runs an official
Archicad JSON command, and `aclib.PrintJson (value)` prints a response as indented JSON. Neither
prints by itself (only errors), so an example prints what it shows, usually with
`aclib.PrintJson (aclib.RunTapirCommand (...))`; pass `debug = True` to also see the command and its
parameters.
`--port <port>` talks to another Archicad than the one on 19723. The `aclib.py` in each group
folder only loads `../aclib/`, so the scripts run from any working directory.

## Writing an example

```python
"""Short sentence saying what this script does with <CommandName>.

Optional 1-4 lines: what the open project needs, what the script changes (and that it
restores it), "Requires Archicad NN or later." when the command needs it.
"""
import aclib

<code>
```

- The docstring is the example's description in the docs: plain and specific.
- Short top-level code, typically 5-30 lines; no argparse, no `input ()`, no GUI.
- Take ids from the project (e.g. `GetElementsByType`) instead of hard-coding GUIDs.
- Print what the example shows, with `aclib.PrintJson (...)` or `print (...)`; don't pass `debug`
  (`tools/check_examples.py` reports it).
- Change as little as possible and restore what was changed; delete only what the script created;
  write files only to a temporary folder.
- `tools/new_command.py` writes a starter example for a new command;
  `python tools/check_examples.py` checks that every command has one.

## What does not belong here

- Scripts that check behaviour (round trips, edge cases, regression scripts with a recorded
  output) go to [`../Test/Scripts/`](../Test/Scripts/).
- Scripts that combine several commands into a task or a tool go to [`../Workflows/`](../Workflows/).
