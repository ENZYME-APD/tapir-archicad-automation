# Examples

One short Python script per Tapir command, in one folder per command group:
`Examples/<GroupFolder>/<CommandName>.py`, e.g. `Application/GetAddOnVersion.py` or
`MEP/GetMEPPorts.py`. The [command reference](https://archicad-api.com/archicad-addon/)
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

### Errors, timeout and finding Archicad

- When no Archicad answers (not running, no project open, wrong port, or no answer in time),
  `aclib` raises `TapirConnectionError`; when Archicad answers but the Tapir Add-On is not loaded,
  `TapirNotInstalledError`. Left uncaught, both print only what to do, without a traceback.
- A command that fails prints `Error:` and returns `None` (or the response holding the error),
  and the script carries on. **For new scripts set `aclib.raiseOnError = True`** at the top: a
  failed command then raises `TapirCommandError` (with `.command`, `.code`, `.message`), so the
  script stops at the first failure instead of working on with a wrong result.
  `RunTapirCommand` and `RunCommand` also take `raiseOnError = True/False` per call. An
  `executionResults` list with a failed item is not a failed command; check its items.
- `aclib.timeout` (seconds, default `None`: no limit, as some commands run long) and a per-call
  `timeout = ...`; a timeout raises `TapirConnectionError`.
- `aclib.FindArchicadPorts ()` lists the running Archicads, e.g.
  `[{'port': 19723, 'tapirVersion': '1.8.1'}, {'port': 19724, 'tapirVersion': None}]`
  (`None`: that Archicad has no Tapir). Set `aclib.port` to talk to one of them.
- `aclib.RequireTapir ('1.8.0')` stops a script with "needs Tapir 1.8.0, you have …" when the
  installed Add-On is older, and returns the installed version.

```python
import aclib

aclib.raiseOnError = True
aclib.RequireTapir ('1.8.0')
aclib.PrintJson (aclib.RunTapirCommand ('GetProjectInfo'))
```

`Test/test_aclib.py` tests these without Archicad: `python archicad-addon/Test/test_aclib.py`.

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
