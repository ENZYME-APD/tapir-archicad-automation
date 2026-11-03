"""The Archicad versions Tapir is built for, from archicad-addon/devkits.json.

That file is the one list of supported Archicad versions and their DevKit
releases (GRAPHISOFT/archicad-api-devkit). Everything that needs the list
reads it through this script or, where Python is not at hand, with the same
checks in jq:

    python3 tools/devkits.py matrix WIN|MAC   GitHub Actions build matrix (JSON)
    python3 tools/devkits.py pairs            "25:25.3002 26:26.3000 ..."
    python3 tools/devkits.py lines WIN|MAC    "<version> <DevKit URL>" lines (build_all_win.bat)
    python3 tools/devkits.py newest           the newest Archicad version
    python3 tools/devkits.py update [--summary FILE] [--releases-json FILE]

`update` appends the Archicad versions Graphisoft has published a DevKit for
since, and rewrites the README's download table to match. It never changes
the release chosen for a version already listed. The release taken for a new
version is the one the list has always used: the lowest build from x.3000 up
(the first public release; the 2xxx builds come before it) that is not a
pre-release and has both the Windows and the macOS zip. It prints what changed
and exits with 0, or with 3 when nothing changed.
"""

import argparse
import json
import os
import re
import sys
import urllib.request

ROOT = os.path.dirname (os.path.dirname (os.path.abspath (__file__)))
DEVKITS_JSON = os.path.join (ROOT, 'archicad-addon', 'devkits.json')
README = os.path.join (ROOT, 'README.md')
DEVKIT_REPO = 'GRAPHISOFT/archicad-api-devkit'
DOWNLOAD_URL = 'https://github.com/' + DEVKIT_REPO + '/releases/download/{0}/API.Development.Kit.{1}.{0}.zip'
ADDON_URL = 'https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/TapirAddOn_AC{0}_{1}'
TABLE_START = '<!-- archicad-versions:start (written by tools/devkits.py update) -->'
TABLE_END = '<!-- archicad-versions:end -->'
NO_CHANGE = 3


class DevKitError (Exception):
    pass


def load (path=DEVKITS_JSON):
    """The validated list of {archicad, release}, oldest version first."""
    with open (path, encoding='utf-8') as f:
        devkits = json.load (f)['devkits']
    previous = 0
    for devkit in devkits:
        archicad, release = devkit.get ('archicad'), devkit.get ('release')
        if not isinstance (archicad, int) or not 20 <= archicad <= 99 or archicad <= previous:
            raise DevKitError (f'{path}: "archicad" must be a version from 20 to 99, in ascending order: {devkit}')
        if not isinstance (release, str) or not re.fullmatch (rf'{archicad}\.[0-9]{{4}}', release):
            raise DevKitError (f'{path}: "release" must be {archicad}.NNNN: {devkit}')
        previous = archicad
    if not devkits:
        raise DevKitError (f'{path}: no DevKits')
    return devkits


def save (devkits, path=DEVKITS_JSON):
    with open (path, encoding='utf-8') as f:
        data = json.load (f)
    data['devkits'] = devkits
    rows = ',\n'.join (f'        {{ "archicad": {d["archicad"]}, "release": "{d["release"]}" }}' for d in devkits)
    text = '{\n    "_comment": ' + json.dumps (data['_comment']) + ',\n    "devkits": [\n' + rows + '\n    ]\n}\n'
    with open (path, 'w', encoding='utf-8', newline='\n') as f:
        f.write (text)


def readme_table (devkits):
    lines = ['| Archicad version | Windows | macOS |', '| --- | --- | --- |']
    for devkit in devkits:
        version = devkit['archicad']
        lines.append (f'| Archicad {version} | [Download]({ADDON_URL.format (version, "Win.apx")}) '
                      f'| [Download]({ADDON_URL.format (version, "Mac.zip")}) |')
    return '\n'.join (lines)


def write_readme (devkits, path=README):
    with open (path, encoding='utf-8') as f:
        text = f.read ()
    start, end = text.find (TABLE_START), text.find (TABLE_END)
    if start == -1 or end < start:
        raise DevKitError (f'{path}: the {TABLE_START} ... {TABLE_END} markers are missing')
    new = text[:start + len (TABLE_START)] + '\n' + readme_table (devkits) + '\n' + text[end:]
    if new != text:
        with open (path, 'w', encoding='utf-8', newline='') as f:
            f.write (new)


def fetch_releases ():
    """Every release of the DevKit repository, from the GitHub API."""
    releases, page = [], 1
    headers = {'Accept': 'application/vnd.github+json', 'User-Agent': 'tapir-devkit-update'}
    if os.environ.get ('GITHUB_TOKEN'):
        headers['Authorization'] = 'Bearer ' + os.environ['GITHUB_TOKEN']
    while True:
        request = urllib.request.Request (f'https://api.github.com/repos/{DEVKIT_REPO}/releases?per_page=100&page={page}', headers=headers)
        with urllib.request.urlopen (request, timeout=60) as response:
            batch = json.load (response)
        releases += batch
        if len (batch) < 100:
            return releases
        page += 1


def first_public_releases (releases):
    """Archicad version -> the release a new version is added with."""
    found = {}
    for release in releases:
        tag = release.get ('tag_name', '')
        match = re.fullmatch (r'([0-9]{2})\.([0-9]{4})', tag)
        if not match or release.get ('draft') or release.get ('prerelease') or int (match.group (2)) < 3000:
            continue
        assets = {asset.get ('name') for asset in release.get ('assets', [])}
        if not all (f'API.Development.Kit.{platform}.{tag}.zip' in assets for platform in ('WIN', 'MAC')):
            continue
        version = int (match.group (1))
        if version not in found or int (match.group (2)) < int (found[version].split ('.')[1]):
            found[version] = tag
    return found


def update (args):
    devkits = load ()
    if args.releases_json:
        with open (args.releases_json, encoding='utf-8') as f:
            releases = json.load (f)
    else:
        releases = fetch_releases ()
    newest = devkits[-1]['archicad']
    added = [{'archicad': version, 'release': release}
             for version, release in sorted (first_public_releases (releases).items ()) if version > newest]
    if not added:
        print (f'No new Archicad version: the newest DevKit is still Archicad {newest}.')
        return NO_CHANGE
    devkits += added
    save (devkits)
    write_readme (devkits)
    summary = [f'- Archicad {d["archicad"]}: DevKit [{d["release"]}](https://github.com/{DEVKIT_REPO}/releases/tag/{d["release"]})'
               for d in added]
    print ('\n'.join (summary))
    if args.summary:
        with open (args.summary, 'w', encoding='utf-8') as f:
            f.write ('\n'.join (summary) + '\n')
    return 0


def main ():
    parser = argparse.ArgumentParser (description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers (dest='command', required=True)
    matrix = commands.add_parser ('matrix', help='GitHub Actions build matrix')
    matrix.add_argument ('platform', choices=['WIN', 'MAC'])
    commands.add_parser ('pairs', help='"<version>:<release>" pairs')
    lines = commands.add_parser ('lines', help='"<version> <DevKit URL>" lines')
    lines.add_argument ('platform', choices=['WIN', 'MAC'])
    commands.add_parser ('newest', help='the newest Archicad version')
    updater = commands.add_parser ('update', help='add newly published Archicad versions')
    updater.add_argument ('--summary', help='write the Markdown list of added versions here')
    updater.add_argument ('--releases-json', help='read the releases from this file instead of the GitHub API')
    args = parser.parse_args ()

    try:
        if args.command == 'update':
            return update (args)
        devkits = load ()
        if args.command == 'matrix':
            print (json.dumps ([{'acVersion': d['archicad'], 'devKitUrl': DOWNLOAD_URL.format (d['release'], args.platform)}
                                for d in devkits]))
        elif args.command == 'lines':
            print ('\n'.join (f'{d["archicad"]} {DOWNLOAD_URL.format (d["release"], args.platform)}' for d in devkits))
        elif args.command == 'pairs':
            print (' '.join (f'{d["archicad"]}:{d["release"]}' for d in devkits))
        else:
            print (devkits[-1]['archicad'])
        return 0
    except DevKitError as error:
        print (f'error: {error}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit (main ())
