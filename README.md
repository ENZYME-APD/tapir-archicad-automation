# archicad-api

## Description
Archicad Python API - Grasshopper Plugin
The easiest way to use the JSON/Python API from Archicad without knowing how to code.
This is set of nodes for a Grasshopper that handles functions of ARCHICAD Python API
and helps with creation BIMx extensions (.bimxx)

## Installation
Download mannualy api.zip file 
  * Copy ARCHICAD_API folder to Grasshopper's UserObjects folder (GH Menu: File/Special Folders/User Object Folder)
  * Copy api2.py to Rhino's scripts Folder **[How to locate scripts Folder](https://wiki.mcneel.com/rhino/macroscriptsetup)**

## How to use it 
Goto [archicad-api.com](https://www.archicad-api.com/) to get more info on plugin usage

## ARCHICAD Addon
For some functions one must install an Addon developed by [Kovacsv](https://github.com/ENZYME-APD/archicad-api-dev/commits?author=kovacsv) which is based on [TLorantfy](https://github.com/tlorantfy/archicad-additional-json-commands) work. (Thank You guys!)

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

Currently three experimental  nodes use this Addon:
 * Pick Connection Port
 * Get Project Info
 * GetSelectedElements (NEW!!)
 and they can be found in 'Addon Commands' group on ARCHICAD_API Ribbon

