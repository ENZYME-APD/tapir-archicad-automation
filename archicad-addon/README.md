# Tapir Additional JSON Commands Archicad Add-On

This is the source code of an Archicad Add-On that registers several additional JSON commands in Archicad. The code is based on the amazing work of Tibor Lorantfy on [archicad-additional-json-commands](https://github.com/tlorantfy/archicad-additional-json-commands).

## How to use?

### Download

Download the latest version here:
- [Archicad 25 (Windows)](https://github.com/ENZYME-APD/archicad-api-dev/releases/latest/download/TapirAddOn_AC25_Win.apx)
- [Archicad 26 (Windows)](https://github.com/ENZYME-APD/archicad-api-dev/releases/latest/download/TapirAddOn_AC26_Win.apx)
- [Archicad 25 (Mac)](https://github.com/ENZYME-APD/archicad-api-dev/releases/latest/download/TapirAddOn_AC25_Mac.dmg)
- [Archicad 26 (Mac)](https://github.com/ENZYME-APD/archicad-api-dev/releases/latest/download/TapirAddOn_AC26_Mac.dmg)

### Install

Once you downloaded the Add-On files you have to install it in Archicad. Follow these steps to install the Add-On.

1. Place the downloaded file somewhere on your computer.
2. In Archicad run the command "Options > Add-On Manager".
3. Open the "Edit List of Available Add-Ons" tabpage, and click on the "Add" button.
4. Browse the downloaded Add-On file.
5. Click "OK" in the Add-On Manager.

**Important note for Mac users:** The Add-On is not properly signed at the moment, so once you try to load the Add-On a warning will appear on the screen. You can ignore this warning, but after doing the process above go to Security Settings, allow the Add-On file to run and restart Archicad.

### Documentation
Here you can find the documentation of commands registered by the Add-On:

https://enzyme-apd.github.io/archicad-api-dev/archicad-addon

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
