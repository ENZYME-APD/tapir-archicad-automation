# Writes the Windows version resource of the Tapir Installer, for
# "pyinstaller --version-file", and prints the version it wrote.
#
# SignPath signs TapirInstaller_Win.exe only when its ProductName is "Tapir"
# and its ProductVersion equals the "version" parameter of the signing request
# (the SignPath Foundation requires these metadata restrictions on every
# signed file; see the artifact configuration in the release workflow). The
# version is the one in tools/package_info.json, which at a release tag is the
# release version.
#
# Used by archicad_addon.yml, installer_build_check.yml and the local build in
# README.md - keep the three the same.
#
# Usage: python write_version_file.py <output file>

import json
import os
import re
import sys

REPO_ROOT = os.path.abspath (os.path.join (os.path.dirname (os.path.abspath (__file__)), '..', '..'))
PRODUCT_NAME = 'Tapir'

TEMPLATE = '''VSVersionInfo(
  ffi=FixedFileInfo(
    filevers={numbers},
    prodvers={numbers},
    mask=0x3f,
    flags=0x0,
    OS=0x40004,
    fileType=0x1,
    subtype=0x0,
    date=(0, 0)
  ),
  kids=[
    StringFileInfo([
      StringTable('040904B0', [
        StringStruct('FileDescription', 'Tapir Installer'),
        StringStruct('FileVersion', {version!r}),
        StringStruct('InternalName', 'TapirInstaller'),
        StringStruct('OriginalFilename', 'TapirInstaller_Win.exe'),
        StringStruct('ProductName', {productName!r}),
        StringStruct('ProductVersion', {version!r})
      ])
    ]),
    VarFileInfo([VarStruct('Translation', [1033, 1200])])
  ]
)
'''


def Main ():
    if len (sys.argv) != 2:
        sys.exit ('Usage: python write_version_file.py <output file>')
    with open (os.path.join (REPO_ROOT, 'tools', 'package_info.json'), encoding = 'utf-8') as file:
        version = json.load (file)['version']
    if not re.fullmatch (r'\d+\.\d+\.\d+', version):
        sys.exit ('Unexpected version {!r} in tools/package_info.json'.format (version))
    # The fixed-size version numbers have four parts; the strings carry the
    # exact package_info version, which is what SignPath compares.
    numbers = tuple (int (part) for part in version.split ('.')) + (0,)
    outputPath = os.path.abspath (sys.argv[1])
    os.makedirs (os.path.dirname (outputPath), exist_ok = True)
    with open (outputPath, 'w', encoding = 'utf-8') as file:
        file.write (TEMPLATE.format (numbers = numbers, version = version, productName = PRODUCT_NAME))
    print (version)


if __name__ == '__main__':
    Main ()
