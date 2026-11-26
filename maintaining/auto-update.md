## Installation (end users)

- **Add-On (recommended):** download and run the Tapir Installer
  (`TapirInstaller_Win.exe` / `TapirInstaller_Mac.zip`) from
  [Releases](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest);
  it detects the installed Archicad versions and installs the matching Add-On
  into their `Add-Ons` folders (source: [archicad-addon/Installer/](../archicad-addon/Installer/)).
  The Add-On's auto-update runs the same installer in update mode (`--addOnFile`),
  which replaces exactly the running add-on.
- **Add-On (manual):** download the matching `TapirAddOn_AC<version>_<Win|Mac>` file from
  [Releases](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest),
  then in Archicad: *Options > Add-On Manager > Edit List of Available Add-Ons > Add*,
  browse to the file, OK.
- **Grasshopper plugin:** install the **tapir** package from the Rhino Package Manager.
- **Auto-update:** the update is done by the Tapir Installer (see Installation). The
  Add-On downloads the installer of the latest release and starts it in update mode
  (`--addOnFile <own add-on> --versions <AC version> --archicadPort <port> --archicadPid <pid>`);
  the installer quits Archicad, replaces the add-on and starts Archicad again.
  Already released Add-Ons instead download
  [Tools/update_addon_and_restart_archicad.py](../archicad-addon/Tools/update_addon_and_restart_archicad.py)
  from `main` and run it with uv (`--port`, `--downloadUrl`, `--addOnLocation`), so
  never move or rename that script, keep it standard-library only and keep its options.
  It hands off to the installer of the release it updates to when that installer has
  update mode. It detects update mode from `archicad-addon/Installer/tapir_installer.py`
  at the release tag on raw.githubusercontent.com (the release workflow builds the
  installer from the tagged commit), by the quoted `'--addOnFile'` option in the
  installer's argument parser. So neither the `'--addOnFile'` option name nor that
  file's path may change without updating the script on `main`. It updates
  the add-on itself (download and file signature check first, write check, quit,
  replace, restart) for releases without update mode, when that check cannot be made,
  on platforms without an installer, when the installer cannot be downloaded or
  started, and when the administrator prompt was declined but the add-on is writable
  without it. It is not a development tool. The script and the installer's update
  mode run one update per Archicad by locking the first byte of `TapirUpdate_<port>.lock`
  in the temp folder, so keep that name in both. After that byte both the script and
  the installer's update mode write `quitting` while they wait for Archicad to quit
  and `updating` otherwise; a second update, by either of them, tells the user to
  quit Archicad only when it reads exactly `quitting`, because quitting while the
  first update still downloads makes it fail. Keep the offset, the states and the
  texts the same in both (test_update_mode.py checks they match).
