![Tapir](branding/logo/png/tapir_bar_2000_600.png?raw=true)

# tapir-archicad-automation

[![Archicad Add-On Build](https://github.com/ENZYME-APD/tapir-archicad-automation/actions/workflows/archicad_addon.yml/badge.svg)](https://github.com/ENZYME-APD/tapir-archicad-automation/actions/workflows/archicad_addon.yml)
[![Grasshopper Plugin Build](https://github.com/ENZYME-APD/tapir-archicad-automation/actions/workflows/grasshopper_plugin.yml/badge.svg)](https://github.com/ENZYME-APD/tapir-archicad-automation/actions/workflows/grasshopper_plugin.yml)

This repository contains the Archicad Python API Grasshopper Plugin, the easiest way to use the JSON/Python API from Archicad without knowing how to code. The repository contains the following components:
- [grasshopper-plugin](grasshopper-plugin): This is set of nodes for a Grasshopper that handles functions of ARCHICAD Python API and helps with creation BIMx extensions (.bimxx).
- [archicad-addon](archicad-addon): The source code of the Tapir Additional JSON Commands Archicad Add-On. This Add-On registers some additional JSON commands on the Archicad JSON API.
- [python-package](python-package): Contains the source code for IronPython friendly adaptation of the official [archicad](https://pypi.org/project/archicad/) package.
- [sandbox](sandbox): Several useful information and code examples that we would like to keep for later reference.

## Roadmap

[The roadmap is publicly available here.](https://github.com/orgs/ENZYME-APD/projects/4)

## Grasshopper Plugin

### Installation
* Download manualy the [api.zip](https://github.com/ENZYME-APD/tapir-archicad-automation/raw/main/grasshopper-plugin/api.zip) file, and extract it on your computer.
* Copy ARCHICAD_API folder to Grasshopper's UserObjects folder (GH Menu: File/Special Folders/User Object Folder).
* Copy api2.py to Rhino's scripts folder. See [how to locate scripts Folder](https://wiki.mcneel.com/rhino/macroscriptsetup) for details.

### How to use it?
Goto [archicad-api.com](https://www.archicad-api.com/) to get more info on plugin usage

## ARCHICAD Addon
For some functions one must install an Addon developed by [kovacsv](https://github.com/kovacsv) which is based on [tlorantfy](https://github.com/tlorantfy/archicad-additional-json-commands)'s work. (Thank You guys!)

### Installation

Download the latest version here:
- [Archicad 25 (Windows)](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC25_Win.apx)
- [Archicad 26 (Windows)](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC26_Win.apx)
- [Archicad 27 (Windows)](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC27_Win.apx)
- [Archicad 25 (Mac)](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC25_Mac.dmg)
- [Archicad 26 (Mac)](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC26_Mac.dmg)
- [Archicad 27 (Mac)](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC27_Mac.dmg)

Once you downloaded the Add-On files you have to install it in Archicad. Follow these steps to install the Add-On.

1. Place the downloaded file somewhere on your computer.
2. In Archicad run the command "Options > Add-On Manager".
3. Open the "Edit List of Available Add-Ons" tabpage, and click on the "Add" button.
4. Browse the downloaded Add-On file.
5. Click "OK" in the Add-On Manager.

Currently three experimental  nodes use this Addon:
 * Pick Connection Port
 * Get Project Info
 * GetSelectedElements (NEW!!)
 and they can be found in 'Addon Commands' group on ARCHICAD_API Ribbon
 
 ### Documentation
 
 You can find the documentation of the implemented commands here: https://enzyme-apd.github.io/tapir-archicad-automation/archicad-addon.
 
