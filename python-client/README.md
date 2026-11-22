# Tapir Python client

One Python method per [Tapir command](https://archicad-api.com/archicad-addon/), so a script does not
have to write the `API.ExecuteAddOnCommand` JSON by hand. Standard library only (Python 3.8 or later).
It needs Archicad with the [Tapir Add-On](https://github.com/ENZYME-APD/tapir-archicad-automation)
running.

## Install

From this repository, any of:

- `pip install "git+https://github.com/ENZYME-APD/tapir-archicad-automation#subdirectory=python-client"`
  (the client of `main`; add `@<release tag>` after the repository URL for that release's client)
- put the `python-client` folder on `PYTHONPATH`
- copy the `tapir_archicad` folder next to your script

From PyPI, once it is published there: `pip install tapir-archicad`. Nothing publishes it yet: the
release workflow that publishes it with each Tapir release is
[#3001](https://github.com/ENZYME-APD/tapir-archicad-automation/pull/3001), and it stays off until a
maintainer has set up PyPI for it.

The package is `tapir-archicad` for pip and `tapir_archicad` in Python (the PyPI project `tapir` is an
unrelated one). It knows the commands of the Tapir version it comes from; a newer Tapir Add-On's
commands are always reachable with `run`.

## Use

```python
from tapir_archicad import Client, TapirError

tapir = Client ()                                    # Archicad on http://127.0.0.1:19723
print (tapir.get_add_on_version ()['version'])
walls = tapir.get_elements_by_type (element_type = 'Wall')['elements']
print (tapir.get_details_of_elements (elements = walls))
```

- **Names:** each command is a method in snake_case (`GetDetailsOfElements` -> `get_details_of_elements`),
  and its parameters are keyword arguments, also in snake_case (`elementType` -> `element_type`).
  `help (Client.get_details_of_elements)` shows the command's description and parameters.
  Arguments left at `None` are not sent. `tapir.run ('CommandName', {...})` runs a command by its own
  name and JSON parameters.
- **Errors:** when Archicad refuses a command, the response is an error, or Archicad does not answer,
  the method raises `TapirError` (with `code`, `message` and `command`). An answer that comes with an
  HTTP error status keeps Archicad's own error code and message too, so a failed command reads the
  same however Archicad answered it; an answer that is not Archicad's at all (a proxy's page, a
  firewall's) raises naming the status and what was answered. The `executionResults` of commands
  that work on several items are returned as they are: check each item yourself.
- **Port and host:** `Client (host = 'http://127.0.0.1', port = 19723, timeout = None)`. The first
  Archicad listens on 19723, the next ones on the following ports. Requests go straight to that
  address: the proxy environment variables (`http_proxy` and the like) are ignored, so a corporate
  proxy cannot stand between the script and Archicad.

## Keeping it up to date

`tapir_archicad/commands.py` is generated from the command reference
(`docs/archicad-addon/command_definitions.js` and `common_schema_definitions.js`, which the weekly
release regenerates). Never edit it by hand; after the command reference changed, run:

```
python python-client/generate.py
python -m unittest discover python-client/tests
```

The tests need no Archicad: they check that the committed `tapir_archicad/commands.py` is what
`generate.py` writes, that every command has its method, and the client's requests and errors against
a fake server. `python python-client/generate.py --check` only reports whether
`tapir_archicad/commands.py` is out of date.

## Version and release

The package's version has one place: `__version__` in `tapir_archicad/__init__.py`, which
`pyproject.toml` reads (`[tool.setuptools.dynamic]`). A checkout carries `0.0.0.dev0`. With
[#3001](https://github.com/ENZYME-APD/tapir-archicad-automation/pull/3001) merged and PyPI publishing
turned on, the weekly release sets it to the release's version before it builds the package from the
release tag, and publishes it to PyPI as `tapir-archicad`; until then no workflow sets it or publishes
the package. To build it yourself:

```
pip install build
python -m build python-client
```
