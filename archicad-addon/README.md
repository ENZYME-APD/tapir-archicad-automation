# Tapir Additional JSON Commands Archicad Add-On

This is the source code of an Archicad Add-On that registers several additional JSON commands in Archicad. The code is based on the amazing work of Tibor Lorantfy on [archicad-additional-json-commands](https://github.com/tlorantfy/archicad-additional-json-commands).

## How to build?

### Prerequisites

You should install some prerequisites to build the Add-On:
- [Visual Studio 2019 or later](https://visualstudio.microsoft.com/downloads).
- [Archicad API Development Kit](https://archicadapi.graphisoft.com) for the target version.
- [CMake](https://cmake.org) for generating the project file (3.16+).
- [Python](https://www.python.org) for resource compilation (2.7+ or 3.8+).

### Generate the project and build

Generate and build the project with cmake.

For example on Windows run these commands to build for Archicad 26:
```
cmake -B Build/AC26 -G "Visual Studio 17 2022" -A "x64" -T "v142" -DAC_API_DEVKIT_DIR="C:\Program Files\GRAPHISOFT\API Development Kit 26.3000" .
cmake --build Build/AC26 --config Debug
```
