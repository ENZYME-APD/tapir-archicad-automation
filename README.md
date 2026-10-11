# tapir-archicad-automation

[![Discord](https://img.shields.io/badge/Discord-join-blue?logo=discord&logoColor=fafafa)](https://discord.gg/NAnSennmpY)
[![Archicad Add-On Build Check](https://github.com/ENZYME-APD/tapir-archicad-automation/actions/workflows/archicad_addon_build_check.yml/badge.svg)](https://github.com/ENZYME-APD/tapir-archicad-automation/actions/workflows/archicad_addon_build_check.yml)
[![Grasshopper Plugin Build Check](https://github.com/ENZYME-APD/tapir-archicad-automation/actions/workflows/grasshopper_plugin_build_check.yml/badge.svg)](https://github.com/ENZYME-APD/tapir-archicad-automation/actions/workflows/grasshopper_plugin_build_check.yml)
[![Tapir Installer Build Check](https://github.com/ENZYME-APD/tapir-archicad-automation/actions/workflows/installer_build_check.yml/badge.svg)](https://github.com/ENZYME-APD/tapir-archicad-automation/actions/workflows/installer_build_check.yml)

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

#### Easy installation (recommended)

Download and run the Tapir Installer. It detects the installed Archicad versions, downloads the matching Add-On version and installs it, so Archicad loads it automatically on the next start.

| Windows | macOS |
| --- | --- |
| [Download Installer](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirInstaller_Win.exe) | [Download Installer](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirInstaller_Mac.zip) |

Notes:
- **Windows:** the installer asks for administrator rights (it copies files into the Archicad folder under Program Files). Since the executable is not code signed, Windows SmartScreen may warn about an unknown publisher: click "More info", then "Run anyway".
- **macOS:** unzip the downloaded file and open the app (it is signed and notarized).

#### Manual installation

Download the latest version here:

| Archicad version | Windows | macOS |
| --- | --- | --- |
| Archicad 25 | [Download](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC25_Win.apx) | [Download](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC25_Mac.zip) |
| Archicad 26 | [Download](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC26_Win.apx) | [Download](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC26_Mac.zip) |
| Archicad 27 | [Download](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC27_Win.apx) | [Download](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC27_Mac.zip) |
| Archicad 28 | [Download](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC28_Win.apx) | [Download](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC28_Mac.zip) |
| Archicad 29 | [Download](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC29_Win.apx) | [Download](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC29_Mac.zip) |

Once you downloaded the Add-On files you have to install it in Archicad. Follow these steps to install the Add-On.

1. Place the downloaded file somewhere on your computer.
2. In Archicad run the command "Options > Add-On Manager".
3. Open the "Edit List of Available Add-Ons" tabpage, and click on the "Add" button.
4. Browse the downloaded Add-On file.
5. Click "OK" in the Add-On Manager.

### Grasshopper Plugin

You can install the plugin from the Rhino Package Manager. The package is called "tapir".

## Code signing policy

Free code signing provided by [SignPath.io](https://about.signpath.io), certificate by [SignPath Foundation](https://signpath.org).

This covers the Windows installer, `TapirInstaller_Win.exe`. It is built from this repository's sources by GitHub Actions on GitHub-hosted runners, and every release is signed only after a manual approval. The Windows Add-On files (`.apx`) are not code signed. The macOS installer and macOS Add-Ons are signed and notarized with the project's own Apple Developer ID.

Team roles:
- Committers and reviewers: the maintainers with write access to this repository. Pull requests from people without write access, and the draft pull requests the project's automation opens from issues, are reviewed by a committer before they are merged; they are never merged automatically. Changes to the installer and to the build and release definitions are never merged automatically either, whoever proposes them.
- Approvers: [Tibor Lorántfy](https://github.com/tlorantfy)

### Privacy

The Tapir Installer contacts only GitHub:
- When its window opens, it asks `api.github.com` (or, if that fails, `github.com`) for the latest Tapir release.
- It downloads the Tapir Add-On from GitHub's release download servers when you click Install, or automatically when the Tapir Add-On's own update started it after you confirmed the update in Archicad.

These are plain HTTPS requests. Like any web request they show your IP address and the installer's name to GitHub, where the [GitHub General Privacy Statement](https://docs.github.com/en/site-policy/privacy-policies/github-general-privacy-statement) applies. The installer sends no personal data, has no telemetry and uploads nothing. During an update it also talks to the Archicad running on your own computer (127.0.0.1) to quit and restart it, which the Add-On announces before the update; this does not leave your computer. The installer writes only into the Add-Ons folders of the Archicad installations you select, or during an update next to the Tapir Add-On being updated, plus temporary files in your temp folder (an update lock file, `TapirUpdate_<port>.lock`, stays there). It can remove the Add-On again (Uninstall button, or `--uninstall`). Apart from this, this program will not transfer any information to other networked systems unless specifically requested by the user or the person installing or operating it.

## Documentation

- [Archicad JSON commands](https://enzyme-apd.github.io/tapir-archicad-automation/archicad-addon)
- [Tapir GitHub wiki](https://github.com/ENZYME-APD/tapir-archicad-automation/wiki)

