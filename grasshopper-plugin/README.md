# Tapir [Grasshopper](https://www.grasshopper3d.com/) plugin.

Contains the source code for Grasshopper® components that use the [Archicad JSON API](https://archicadapi.graphisoft.com/JSONInterfaceDocumentation/#Introduction).

## View plugin changelogs [here](CHANGELOG.md)

## Pre-requisites
1. [IronPython](https://ironpython.net/) is required to build the plugin (2.7+)

## How to build?
1. Execute the ```build_module.py``` file *(Check docstrings for more information)*
2. A new folder named ```bin``` will be created with the compiled plugin ```tapir.ghpy``` inside.


**MacOS:** *IronPython 2.x*
```
mono /Users/username/Library/Python/IronPython.2.7.9/net45/ipy.exe /tapir/grasshopper-plugin/build_module.py
```

**MacOS:** *IronPython 3.x*
```
ipy /tapir/grasshopper-plugin/build_module.py
```
## Download compiled plugin
[tAPIr GH Plugin](https://github.com/ENZYME-APD/tapir-archicad-automation/raw/first-nodes/grasshopper-plugin/Plugin%20compiled/tapir_gh.ghpy)
