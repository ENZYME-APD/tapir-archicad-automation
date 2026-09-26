# Tapir Add-On Installer

A small cross-platform (Windows/macOS) installer for the Tapir Archicad Add-On.
It detects the installed Archicad versions (on Windows primarily via the
registry, on macOS by scanning `/Applications`), downloads the matching
`TapirAddOn_AC<version>` asset from the
[latest GitHub release](https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest),
and places it into the `Add-Ons` folder of each Archicad installation
(`<Archicad folder>/Add-Ons/Tapir/`), so Archicad loads it automatically on the
next start.

End users should download the prebuilt executables from the release page:
`TapirInstaller_Win.exe` (Windows) or `TapirInstaller_Mac.zip` (macOS, signed
and notarized). They are built by the release workflow
([.github/workflows/archicad_addon.yml](../../.github/workflows/archicad_addon.yml))
with PyInstaller from [tapir_installer.py](tapir_installer.py).

## Running from source

The installer is a single Python 3 script using only the standard library
(tkinter for the GUI):

```bash
python3 tapir_installer.py
```

Command line options (any of `--console`, `--dryRun` or `--addOnsFolder`
switches to console mode):

| Option | Description |
| --- | --- |
| `--console` | Run in console mode without the GUI. Installs for all detected Archicad versions. |
| `--versions 28,29` | Only install for the given Archicad versions. |
| `--addOnsFolder <path>` | Install into an explicit `Add-Ons` folder instead of the detected ones (requires `--versions` with exactly one version). |
| `--uninstall` | Remove the installed Tapir Add-On (deletes the `Add-Ons/Tapir` subfolder). |
| `--dryRun` | Detect and download only; never modifies the Add-Ons folders. |
| `--mockRoot <path>` | Detect installations under this folder instead of the real system locations (for testing). |

## Building the executables locally

```bash
pip install pyinstaller

# Windows
pyinstaller --noconfirm --onefile --windowed --uac-admin --name TapirInstaller --icon Resources/TapirInstaller.ico tapir_installer.py

# macOS (onedir .app; onefile binaries cannot be stapled after notarization)
pyinstaller --noconfirm --windowed --name TapirInstaller --icon Resources/TapirInstaller.icns --osx-bundle-identifier com.enzyme-apd.tapir-installer tapir_installer.py
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
