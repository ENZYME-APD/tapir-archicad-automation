# tapir_py

Contains the source code for IronPython friendly adaptation of the official [archicad](https://pypi.org/project/archicad/) package.

## Installation

```
pip install tapir_py
```

## Usage

```Python
# Sample code to check connection
# with open ARCHICAD project.

import tapir_py

result = tapir_py.is_alive()
print(result)
```

```Python
# Sample code to get all elements
# from open ARCHICAD project, and
# print the GUID.

from tapir_py import core

# Create an archicad command object.
archicad = core.Command.create()

# Print GUID of elements
elements = archicad.GetAllElements()
for element in elements:
    print(element.guid)
```