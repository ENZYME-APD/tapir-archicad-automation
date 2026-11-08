"""Write catalogue.json, the list of marketplace scripts the website's Scripts page shows.

docs/scripts/scripts.js reads it from the site-data branch (written there by
.github/workflows/site_data.yml, never on main); its format is described at the top of that file:

    {"version": 1,
     "scripts": [{"slug", "name", "summary", "category", "tags", "author", "license", "entry",
                  "changes", "screenshots",
                  "versions": [{"version", "archicadVersions", "minTapirVersion", "changelog",
                                "commit", "url", "sha256"}, ...]}, ...]}

Each script is a marketplace/<slug>/ folder of the checked-out commit (see marketplace/README.md):
  - the listing fields are those of its listing.json;
  - changes: the "changes" of the entry file's # /// tapir header ("model", "files", "read-only");
  - screenshots: the PNG and JPG files in its screenshots/ folder, by name, at most 5;
  - per version, commit: the newest commit of the folder at which that version was the newest one
    in listing.json, so the page reads the README, screenshots and code at it; url: the entry file
    on raw.githubusercontent.com at that commit; sha256: the SHA-256 of the entry file there, which
    the page checks before it shows the code. A version that was never the newest in a commit (it
    was added below a newer one) gets no commit, url or sha256.
A folder that tools/check_marketplace.py refuses, or that has no commit yet, is left out with a
warning, so one wrong listing never takes the others off the site. Scripts are sorted by slug, and
the file has no date in it, so it only changes when a listing does. It needs the full history of
the checkout (git log of each folder) and only the standard library.

    python tools/generate_catalogue.py --repo ENZYME-APD/tapir-archicad-automation --out out/catalogue.json
    python tools/generate_catalogue.py --self-test    # builds a sample repository and checks the output
"""

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile

sys.path.insert (0, os.path.dirname (os.path.abspath (__file__)))
import check_marketplace  # noqa: E402

ROOT = os.path.dirname (os.path.dirname (os.path.abspath (__file__)))
DEFAULT_REPO = 'ENZYME-APD/tapir-archicad-automation'
SCREENSHOT_EXTENSIONS = ('.png', '.jpg', '.jpeg')


def warn (text):
    print (f'warning: {text}', file=sys.stderr)


def git (root, *args, binary=False):
    """The output of a git command in root; None when it fails (a file missing at a commit)."""
    result = subprocess.run (['git', '-C', root, *args], capture_output=True)
    if result.returncode != 0:
        return None
    return result.stdout if binary else result.stdout.decode ('utf-8')


def listing_at (root, commit, folder):
    text = git (root, 'show', f'{commit}:{folder}/listing.json')
    if text is None:
        return None
    try:
        listing = json.loads (text)
    except ValueError:
        return None
    return listing if isinstance (listing, dict) else None


def newest_version (listing):
    versions = listing.get ('versions') if isinstance (listing, dict) else None
    if not isinstance (versions, list) or not versions or not isinstance (versions[0], dict):
        return None
    version = versions[0].get ('version')
    return version if isinstance (version, str) else None


def version_commits (root, folder):
    """{version: (commit, entry)}: the newest commit of the folder at which each version was the newest."""
    log = git (root, 'log', '--format=%H', '--', folder)
    found = {}
    for commit in (log or '').split ():
        listing = listing_at (root, commit, folder)
        version = newest_version (listing)
        if version is not None and version not in found and isinstance (listing.get ('entry'), str):
            found[version] = (commit, listing['entry'])
    return found


def catalogue_entry (root, path, repo):
    """The catalogue entry of one marketplace/<slug>/ folder, or None when it is left out."""
    slug = os.path.basename (path)
    folder = f'marketplace/{slug}'
    errors = check_marketplace.check_folder (path)
    if errors:
        warn (f'{folder} is left out, it does not follow marketplace/README.md: {"; ".join (errors)}')
        return None
    with open (os.path.join (path, 'listing.json'), encoding='utf-8') as file:
        listing = json.load (file)
    commits = version_commits (root, folder)
    if newest_version (listing) not in commits:
        warn (f'{folder} is left out, its newest version has no commit yet')
        return None

    versions = []
    for version in listing['versions']:
        entry = dict (version)
        if version['version'] in commits:
            commit, entry_file = commits[version['version']]
            code = git (root, 'show', f'{commit}:{folder}/{entry_file}', binary=True)
            if code is not None:
                entry['commit'] = commit
                entry['url'] = f'https://raw.githubusercontent.com/{repo}/{commit}/{folder}/{entry_file}'
                entry['sha256'] = hashlib.sha256 (code).hexdigest ()
        versions.append (entry)

    with open (os.path.join (path, listing['entry']), encoding='utf-8') as file:
        header, _ = check_marketplace.read_tapir_header (file.read ())
    shots_dir = os.path.join (path, 'screenshots')
    screenshots = sorted (name for name in os.listdir (shots_dir)
                          if name.lower ().endswith (SCREENSHOT_EXTENSIONS)) if os.path.isdir (shots_dir) else []

    script = {key: listing[key] for key in ('slug', 'name', 'summary', 'category', 'tags', 'author', 'license', 'entry')}
    script['changes'] = header['changes']
    if screenshots:
        script['screenshots'] = screenshots[:check_marketplace.MAX_SCREENSHOTS]
    script['versions'] = versions
    return script


def build_catalogue (root, repo):
    scripts = []
    for path in check_marketplace.script_folders (root):
        script = catalogue_entry (root, path, repo)
        if script is not None:
            scripts.append (script)
    return {'version': 1, 'scripts': scripts}


def write_json (path, data):
    directory = os.path.dirname (path)
    if directory:
        os.makedirs (directory, exist_ok=True)
    with open (path, 'w', encoding='utf-8', newline='\n') as file:
        json.dump (data, file, ensure_ascii=False, indent=2)
        file.write ('\n')


SAMPLE_SCRIPT = '''# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# category = "zones-areas"
# archicad-versions = [28, 29]
# min-tapir-version = "{tapir}"
# changes = "read-only"
# ///

print ("{text}")
'''


def self_test ():
    """Builds a sample repository with a good, a wrong and an uncommitted listing, and checks the catalogue."""
    work = tempfile.mkdtemp (prefix='tapir-catalogue-')
    try:
        def run (*args):
            subprocess.run (['git', '-C', work, *args], check=True, capture_output=True)

        def write (relative, text):
            path = os.path.join (work, relative)
            os.makedirs (os.path.dirname (path), exist_ok=True)
            with open (path, 'w', encoding='utf-8', newline='\n') as file:
                file.write (text)

        def listing (slug, versions, entry='area_check.py'):
            return json.dumps ({'slug': slug, 'name': 'Area check', 'summary': 'Checks the zone areas.',
                                'category': 'zones-areas', 'tags': ['zones'], 'author': {'name': 'Jane'},
                                'license': 'MIT', 'entry': entry, 'versions': versions}, indent=4)

        def version (number, tapir='1.8.0'):
            return {'version': number, 'archicadVersions': [28, 29], 'minTapirVersion': tapir, 'changelog': f'Version {number}.'}

        run ('init', '-q')
        run ('config', 'user.email', 'test@example.com')
        run ('config', 'user.name', 'Test')
        write ('marketplace/area-check/listing.json', listing ('area-check', [version ('1.0.0')]))
        write ('marketplace/area-check/area_check.py', SAMPLE_SCRIPT.format (tapir='1.8.0', text='one'))
        write ('marketplace/area-check/README.md', 'Checks the zone areas.\n')
        write ('marketplace/area-check/screenshots/window.png', 'png')
        write ('marketplace/wrong-slug/listing.json', listing ('another-slug', [version ('1.0.0')]))
        write ('marketplace/wrong-slug/area_check.py', SAMPLE_SCRIPT.format (tapir='1.8.0', text='x'))
        write ('marketplace/wrong-slug/README.md', 'x\n')
        run ('add', '-A')
        run ('commit', '-q', '-m', 'First version')
        first = git (work, 'rev-parse', 'HEAD').strip ()

        write ('marketplace/area-check/listing.json', listing ('area-check', [version ('1.1.0', '1.8.1'), version ('1.0.0')]))
        write ('marketplace/area-check/area_check.py', SAMPLE_SCRIPT.format (tapir='1.8.1', text='two'))
        run ('add', '-A')
        run ('commit', '-q', '-m', 'Second version')
        second = git (work, 'rev-parse', 'HEAD').strip ()
        write ('marketplace/area-check/README.md', 'Checks the zone areas, in more words.\n')
        run ('add', '-A')
        run ('commit', '-q', '-m', 'README only')
        third = git (work, 'rev-parse', 'HEAD').strip ()

        write ('marketplace/not-committed/listing.json', listing ('not-committed', [version ('1.0.0')]))
        write ('marketplace/not-committed/area_check.py', SAMPLE_SCRIPT.format (tapir='1.8.0', text='y'))
        write ('marketplace/not-committed/README.md', 'y\n')

        catalogue = build_catalogue (work, 'owner/repo')
        assert catalogue['version'] == 1, catalogue
        assert [script['slug'] for script in catalogue['scripts']] == ['area-check'], catalogue
        script = catalogue['scripts'][0]
        assert script['changes'] == 'read-only', script
        assert script['screenshots'] == ['window.png'], script
        newest, oldest = script['versions']
        assert newest['version'] == '1.1.0' and newest['commit'] == third, newest
        assert oldest['version'] == '1.0.0' and oldest['commit'] == first, oldest
        code = git (work, 'show', f'{second}:marketplace/area-check/area_check.py', binary=True)
        assert newest['sha256'] == hashlib.sha256 (code).hexdigest (), newest
        assert newest['url'] == f'https://raw.githubusercontent.com/owner/repo/{third}/marketplace/area-check/area_check.py', newest
        assert check_marketplace.check_folder (os.path.join (work, 'marketplace', 'wrong-slug')), 'the wrong slug passed'
    finally:
        shutil.rmtree (work, ignore_errors=True)
    print ('The catalogue self-test passed.')
    return 0


def main ():
    parser = argparse.ArgumentParser (description='Write catalogue.json from the marketplace/<slug>/ folders.')
    parser.add_argument ('--root', default=ROOT, help='the repository checkout (default: this one)')
    parser.add_argument ('--repo', default=DEFAULT_REPO, help=f'owner/name for the raw.githubusercontent.com addresses (default: {DEFAULT_REPO})')
    parser.add_argument ('--out', default='catalogue.json', help='where to write it (default: catalogue.json)')
    parser.add_argument ('--self-test', action='store_true', help='build a sample repository and check the output')
    args = parser.parse_args ()
    if args.self_test:
        return self_test ()
    catalogue = build_catalogue (args.root, args.repo)
    write_json (args.out, catalogue)
    print (f'{args.out}: {len (catalogue["scripts"])} scripts')
    return 0


if __name__ == '__main__':
    sys.exit (main ())
