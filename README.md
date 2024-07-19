# tapir-archicad-automation

[![Archicad Add-On Build](https://github.com/ENZYME-APD/tapir-archicad-automation/actions/workflows/archicad_addon.yml/badge.svg)](https://github.com/ENZYME-APD/tapir-archicad-automation/actions/workflows/archicad_addon.yml)
[![Grasshopper Plugin Build](https://github.com/ENZYME-APD/tapir-archicad-automation/actions/workflows/grasshopper_plugin.yml/badge.svg)](https://github.com/ENZYME-APD/tapir-archicad-automation/actions/workflows/grasshopper_plugin.yml)

This repository contains the Tapir Archicad automation package. It consists of several components:
- [Tapir Archicad Add-On](archicad-addon): An Add-On that registers several new JSON commands on top of the official commands provided by Graphisoft. You can see the list of new commands [here](https://enzyme-apd.github.io/tapir-archicad-automation/archicad-addon). This is ready to use, see the installation instructions below.
- [Tapir Python Package](python-package): A Grasshopper friendly Python wrapper for the Archicad JSON commands. This is heavily work in progress at the moment.
- [Tapir Grasshopper Plugin](grasshopper-plugin): A Grasshopper plugin to help using the above components even for non-programmers. This is heavily work in progress at the moment.

## Architecture

The diagram below explains the components and their dependencies.

![Tapir](branding/diagrams/TapirArchitecture.png?raw=true)

## Roadmap

[The roadmap is publicly available here.](https://github.com/orgs/ENZYME-APD/projects/4)

## ARCHICAD Addon
For some functions one must install an Addon developed by [kovacsv](https://github.com/kovacsv) which is based on [tlorantfy](https://github.com/tlorantfy/archicad-additional-json-commands)'s work. (Thank You guys!)

### Installation

Download the latest version here:
- [Archicad 25 (Windows)](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC25_Win.apx)
- [Archicad 26 (Windows)](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC26_Win.apx)
- [Archicad 27 (Windows)](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC27_Win.apx)
- [Archicad 28 (Windows)](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC28_Win.apx)
- [Archicad 25 (Mac)](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC25_Mac.zip)
- [Archicad 26 (Mac)](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC26_Mac.zip)
- [Archicad 27 (Mac)](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC27_Mac.zip)
- [Archicad 28 (Mac)](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC28_Mac.zip)

Once you downloaded the Add-On files you have to install it in Archicad. Follow these steps to install the Add-On.

1. Place the downloaded file somewhere on your computer.
2. In Archicad run the command "Options > Add-On Manager".
3. Open the "Edit List of Available Add-Ons" tabpage, and click on the "Add" button.
4. Browse the downloaded Add-On file.
5. Click "OK" in the Add-On Manager.
 
### Documentation
 
You can find the documentation of the implemented commands here: https://enzyme-apd.github.io/tapir-archicad-automation/archicad-addon.

