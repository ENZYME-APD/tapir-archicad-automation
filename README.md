# tapir-archicad-automation

[![Discord](https://img.shields.io/badge/Discord-join-blue?logo=discord&logoColor=fafafa)](https://discord.gg/NAnSennmpY)
[![Archicad Add-On Build Check](https://github.com/ENZYME-APD/tapir-archicad-automation/actions/workflows/archicad_addon_build_check.yml/badge.svg)](https://github.com/ENZYME-APD/tapir-archicad-automation/actions/workflows/archicad_addon_build_check.yml)
[![Grasshopper Plugin Build Check](https://github.com/ENZYME-APD/tapir-archicad-automation/actions/workflows/grasshopper_plugin_build_check.yml/badge.svg)](https://github.com/ENZYME-APD/tapir-archicad-automation/actions/workflows/grasshopper_plugin_build_check.yml)

This repository contains the Tapir Archicad automation package. It consists of several components:
- [Tapir Archicad Add-On](archicad-addon): An Add-On that registers several new JSON commands on top of the official commands provided by Graphisoft. You can see the list of new commands [here](https://enzyme-apd.github.io/tapir-archicad-automation/archicad-addon). This is ready to use, see the installation instructions below.
- [Tapir Grasshopper Plugin](grasshopper-plugin): A Grasshopper plugin to help using the above components even for non-programmers. This is work in progress at the moment.

## Overview

The diagram below explains the components and their dependencies.

![Tapir](branding/diagrams/TapirArchitecture.png?raw=true)

## Roadmap

[The Tapir Roadmap is available here.](https://github.com/orgs/ENZYME-APD/projects/4/views/1)

## Installation

### Archicad Add-On

Download the latest version here:

| Archicad version | Windows | macOS |
| --- | --- | --- |
| Archicad 25 | [Download](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC25_Win.apx) | [Download](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC25_Mac.zip) |
| Archicad 26 | [Download](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC26_Win.apx) | [Download](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC26_Mac.zip) |
| Archicad 27 | [Download](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC27_Win.apx) | [Download](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC27_Mac.zip) |
| Archicad 28 | [Download](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC28_Win.apx) | [Download](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC28_Mac.zip) |

Once you downloaded the Add-On files you have to install it in Archicad. Follow these steps to install the Add-On.

1. Place the downloaded file somewhere on your computer.
2. In Archicad run the command "Options > Add-On Manager".
3. Open the "Edit List of Available Add-Ons" tabpage, and click on the "Add" button.
4. Browse the downloaded Add-On file.
5. Click "OK" in the Add-On Manager.

### Grasshopper Plugin

You can install the plugin from the Rhino Package Manager. The package is called "tapir".

## Documentation

- [Archicad JSON commands](https://enzyme-apd.github.io/tapir-archicad-automation/archicad-addon)
- [Tapir GitHub wiki](https://github.com/ENZYME-APD/tapir-archicad-automation/wiki)

