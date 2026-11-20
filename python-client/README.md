# Tapir Python client

One Python method per [Tapir command](https://archicad-api.com/archicad-addon/), so a script does not
have to write the `API.ExecuteAddOnCommand` JSON by hand. Standard library only (Python 3.8 or later).

```python
from tapir import Client, TapirError

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
  the method raises `TapirError` (with `code`, `message` and `command`). The `executionResults` of
  commands that work on several items are returned as they are: check each item yourself.
- **Port and host:** `Client (host = 'http://127.0.0.1', port = 19723, timeout = None)`. The first
  Archicad listens on 19723, the next ones on the following ports.

To use it, put the `python-client` folder on `PYTHONPATH` (or copy its `tapir` folder next to your
script). It is not published on PyPI.

## Keeping it up to date

`tapir/commands.py` is generated from the command reference (`docs/archicad-addon/command_definitions.js`
and `common_schema_definitions.js`, which the weekly release regenerates). Never edit it by hand; after
the command reference changed, run:

```
python python-client/generate.py
python -m unittest discover python-client/tests
```

The tests need no Archicad: they check that the committed `tapir/commands.py` is what `generate.py`
writes, that every command has its method, and the client's requests and errors against a fake server.
`python python-client/generate.py --check` only reports whether `tapir/commands.py` is out of date.
