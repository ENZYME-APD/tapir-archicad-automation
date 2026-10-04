# Tapir Add-On Installer

A small cross-platform (Windows/macOS) installer for the Tapir Archicad Add-On.
It detects the installed Archicad versions (on Windows primarily via the
registry, on macOS by scanning `/Applications`), downloads the matching
`TapirAddOn_AC<version>` asset from the
[latest GitHub release](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest),
and places it into the Add-Ons folder of each Archicad installation
(`<Archicad folder>/<Add-Ons folder>/Tapir/`), so Archicad loads it
automatically on the next start.

The name of the Add-Ons folder is localized in the language versions of
Archicad (e.g. `Extensions` in FRA, `Dodatki` in POL). The installer first
looks for a folder with one of the localized names published by Graphisoft
(`LOCALIZED_ADDONS_FOLDER_NAMES` in the script). If there is none, it picks the
folder that contains the `XReadCfg.txt` file (directly or up to two levels
below it), as recommended by the Graphisoft multi-language add-on guide.
On macOS the Add-Ons folder is next to the `Archicad NN.app` bundle in the
Archicad folder, so the installer does not look inside the bundle.

End users should download the prebuilt executables from the release page:
`TapirInstaller_Win.exe` (Windows) or `TapirInstaller_Mac.zip` (macOS, signed
and notarized). They are built by the release workflow
([.github/workflows/archicad_addon.yml](../../.github/workflows/archicad_addon.yml))
with PyInstaller from [tapir_installer.py](tapir_installer.py).

## Running from source

The installer is a single Python 3 script using only the standard library
(tkinter for the GUI). The optional `truststore` / `certifi` packages are used
for HTTPS certificate verification when installed (the released executables
bundle them; without them a PyInstaller build on macOS fails with
`CERTIFICATE_VERIFY_FAILED`):

```bash
python3 tapir_installer.py
```

Command line options (any of `--console`, `--dryRun`, `--uninstall`,
`--versions` or `--addOnsFolder` switches to console mode, except in update
mode, which shows the GUI unless `--console` is given). Unknown options are
ignored, so that a newer Add-On can pass new options to an older installer.
Options are never abbreviated, so this also holds for a new option that starts
like an existing one:

| Option | Description |
| --- | --- |
| `--console` | Run in console mode without the GUI. Installs for all detected Archicad versions. |
| `--versions 28,29` | Only install for the given Archicad versions. |
| `--addOnsFolder <path>` | Install into an explicit `Add-Ons` folder instead of the detected ones (requires `--versions` with exactly one version). |
| `--uninstall` | Remove the installed Tapir Add-On (deletes the `Tapir` subfolder of the Add-Ons folder). |
| `--dryRun` | Detect and download only; never modifies the Add-Ons folders. In update mode it never contacts Archicad either. |
| `--mockRoot <path>` | Detect installations under this folder instead of the real system locations (for testing). |
| `--addOnFile <path>` | Update mode: replace exactly this Tapir Add-On (the `.apx` file on Windows, the `.bundle` folder on macOS) with the latest release, instead of installing into `<Add-Ons folder>/Tapir`. Requires `--versions` with exactly one version; not allowed with `--addOnsFolder`, `--uninstall` or `--mockRoot`. |
| `--archicadPort <port>` | Update mode: once the download succeeded, quit the Archicad whose Tapir JSON API listens on this port, replace the add-on, then start Archicad again (also when replacing failed). |
| `--archicadPid <pid>` | Update mode, with `--archicadPort`: once the port has closed, also wait (up to 60 s) until this Archicad process has exited before replacing the add-on and starting Archicad again. On Windows the installer also lets this process bring its save prompt to the front. |
| `--noRestart` | Update mode: do not start Archicad again after the update (for testing). |

### Update mode

The Add-On's auto-update ("Check for Updates...") downloads the installer from
the latest release and starts it in update mode, so the update gets the
installer's protections: the Windows administrator (UAC) prompt, the macOS
administrator password fallback and visible progress and errors:

```bash
TapirInstaller --addOnFile <loaded add-on> --versions 29 --archicadPort 19723 --archicadPid <Archicad process id>
```

On Windows the Add-On starts the installer with the administrator (UAC)
prompt. If the user declines the prompt, or a policy does not allow
administrator rights, the Add-On checks whether the user can replace the
Add-On: it creates and deletes a probe file `.TapirWriteCheck_<process id>` in
the Add-On's folder, and the `.apx` must not be read-only. If so, it starts the
installer with the user's rights and without a prompt (with the `RunAsInvoker`
compatibility layer in `__COMPAT_LAYER`, set in Archicad's environment only
while it starts the installer), as the legacy update script updates such an
Add-On; an administrator who clicks No then gets the update too. Otherwise a
declined prompt ends the update with "The update was cancelled." and the note
that the Add-On is in a folder that only an administrator can change, and any
other failure with "Failed to start the Tapir Installer." and the Windows error
code; both point to the release page for a manual update.

The macOS installer is a universal2 app, so it runs on Intel Macs and on Apple
silicon. The release workflow builds it with the python.org universal2 Python
and checks every binary in the app with `lipo`. On an Intel Mac the Add-On
does not start a downloaded installer that has no Intel code (Archicad running
under Rosetta counts as Apple silicon): it says so and points to the release
page for a manual update.

The installer downloads the add-on first and checks it (its size, and that it
is a DLL or a zip, not a page from a proxy), so a failed download leaves
Archicad running. It then quits Archicad through the Tapir JSON API (on
Windows it first lets Archicad come to the front, so that Archicad's save
prompt is not hidden behind the installer window) and waits for it to exit.
The user may answer the save prompt any time later, so the installer keeps
waiting while Archicad still runs: while the quit command gets no answer, or
while its port accepts connections, even if Archicad does not answer them
during the prompt. After about 5 s it asks the user to answer the prompt in
Archicad and offers Cancel, which stops waiting within about 2 s: nothing is
replaced and Archicad, which still runs, is not started again. If Archicad
refuses to quit (the quit command fails), the update fails at once.
In console mode (for automated runs) the installer gives up after 120 s
instead. Once the port has closed, Archicad has quit: the installer waits up to
60 s for its process to end, replaces the add-on in place and starts Archicad
again, also when the replace failed and also when the process is still there
(for example behind a crash report dialog; on Windows the replace retries for
up to 60 s while the add-on file is locked). On macOS it starts a new instance
(`open -n`), as plain `open` would only activate the instance that is still
exiting. On macOS the bundle is replaced, not merged, and keeps its name: the
old bundle is moved aside to `<bundle>.old` and only deleted once the new one
is in place, and moved back if that fails. An old bundle that cannot be deleted
is reported as a warning: Tapir is updated, and the old bundle is left to be
deleted by hand.
Once Archicad was quit, the update can only be started again from Archicad.
When the installer runs as administrator on Windows, it does not see the
network drives the user mapped to drive letters; for an add-on on such a drive
it uses the network folder of the persistent mapping from the user's registry.
Released Add-Ons that predate this download
[update_addon_and_restart_archicad.py](../Tools/update_addon_and_restart_archicad.py)
instead. It hands off to the installer only when the installer of the release
it updates to has update mode, which it detects from the installer source at
that release tag (keep the `--addOnFile` option and the path of
`tapir_installer.py`); otherwise it updates the add-on in place as before.

Only one update runs for an Archicad at a time. With `--archicadPort` the
installer locks the file `TapirUpdate_<port>.lock` in the temporary folder
before it contacts Archicad, and holds the lock until Archicad was started
again. The legacy update script uses the same lock, so the two exclude each
other too. The locked byte is the first one of the file (on Windows a locked
byte cannot be read through any other file handle, not even one of the same
process). Right after it, the update that holds the lock writes its state, 8
bytes: `quitting` from just before it asks Archicad to quit until that wait
ends, however it ends, and `updating` otherwise (also from the moment it
takes the lock, which overwrites the state of an update that was killed). A
second update for the same Archicad (the update started again in Archicad, or
the legacy script) touches nothing and reads that state, never the locked
byte, to pick its text:

- only for exactly `quitting`: "Tapir is being updated already, and that
  update waits for Archicad to quit. Quit Archicad to finish it; Archicad is
  then started again."
- otherwise, for example while the other update downloads or starts Archicad
  again, or when the state is missing or cannot be read: "Tapir is being
  updated already. Wait for that update to finish: it closes Archicad itself
  and starts it again." Quitting Archicad before the other update asks it to
  quit would make that update fail.

The legacy script shows the same texts. A stopped installer exits with code 1
in console mode; its GUI shows the text and does not offer to update again.
The legacy script holds the lock while it starts the installer, so a held lock
is waited for up to 3 s first. Dry runs take no lock, and if the lock file
cannot be used, the update goes on without it and writes no state. Failing to
write or read the state never stops an update.

[test_update_mode.py](test_update_mode.py) tests update mode offline against a
mock Archicad (`python test_update_mode.py`).

## Building the executables locally

```bash
pip install pyinstaller truststore certifi

# Windows
pyinstaller --noconfirm --onefile --windowed --uac-admin --name TapirInstaller --icon Resources/TapirInstaller.ico tapir_installer.py

# macOS (onedir .app; onefile binaries cannot be stapled after notarization).
# universal2 needs a universal2 Python, such as the python.org installer;
# check the result with: lipo -archs dist/TapirInstaller.app/Contents/MacOS/TapirInstaller
python3 -m PyInstaller --noconfirm --windowed --target-arch universal2 --name TapirInstaller --icon Resources/TapirInstaller.icns --osx-bundle-identifier com.enzyme-apd.tapir-installer tapir_installer.py
```

On macOS the release pipeline signs and notarizes `dist/TapirInstaller.app`
with [Tools/code_sign_and_notarize.sh](../Tools/code_sign_and_notarize.sh),
using [installer.entitlements](installer.entitlements) (the add-on's
`addon.entitlements` must not be used here: its `get-task-allow` entitlement is
rejected by the notary service on application executables).

## Resources

`Resources/TapirInstaller.ico` and `Resources/TapirInstaller.icns` were
generated once from
[branding/logo/png/tapir_logo_black_512.png](../../branding/logo/png/tapir_logo_black_512.png)
with Pillow:

```python
from PIL import Image
img = Image.open ('tapir_logo_black_512.png').convert ('RGBA')
img.save ('TapirInstaller.ico', format = 'ICO', sizes = [(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
img.save ('TapirInstaller.icns', format = 'ICNS')
```
