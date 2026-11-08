"""Check the script marketplace: every marketplace/<slug>/ folder against marketplace/README.md.

For each folder it checks:
  - listing.json: it parses and follows marketplace/listing.schema.json (written out here, so no
    package is needed), its slug is the folder's name, and its versions are newest first, each once;
  - the entry file: it exists and starts with the # /// script block, an empty line and the
    # /// tapir block, whose category, archicad-versions and min-tapir-version are those of the
    newest version in listing.json, and whose changes is model, files or read-only;
  - README.md exists;
  - the folder holds nothing else: no other files, no folders but screenshots/, which holds at
    most 5 PNG or JPG files of at most 1 MB each.

    python tools/check_marketplace.py            # every folder; exit code 1 when one is wrong
    python tools/check_marketplace.py --root .   # the repository to check (default: this one)

tools/generate_catalogue.py uses check_folder () and read_tapir_header () to leave out a listing
that is wrong, so the site never lists one this check would refuse.
"""

import argparse
import json
import os
import re
import sys
import tomllib

ROOT = os.path.dirname (os.path.dirname (os.path.abspath (__file__)))

SLUG_PATTERN = re.compile (r'^[a-z0-9]+(-[a-z0-9]+)*$')
TAG_PATTERN = SLUG_PATTERN
GITHUB_PATTERN = re.compile (r'^[A-Za-z0-9](?:[A-Za-z0-9-]{0,38})$')
ENTRY_PATTERN = re.compile (r'^[A-Za-z0-9_][A-Za-z0-9_-]*\.py$')
VERSION_PATTERN = re.compile (r'^[0-9]+\.[0-9]+\.[0-9]+$')
SCREENSHOT_PATTERN = re.compile (r'^[A-Za-z0-9_][A-Za-z0-9_.-]*\.(?:png|jpe?g)$', re.IGNORECASE)

CATEGORIES = {
    'documentation-layouts', 'element-data-properties', 'modeling-geometry', 'zones-areas',
    'import-export', 'quality-assurance', 'quantities-scheduling', 'libraries-objects',
    'navigator-views', 'classification-attributes', 'batch-automation', 'utilities',
}
CHANGES = {'model', 'files', 'read-only'}
LISTING_KEYS = {'slug', 'name', 'summary', 'category', 'tags', 'author', 'license', 'entry', 'versions'}
AUTHOR_KEYS = {'name', 'github', 'discord'}
VERSION_KEYS = {'version', 'archicadVersions', 'minTapirVersion', 'changelog'}
FOLDER_FILES = {'listing.json', 'README.md'}
MAX_SCREENSHOTS = 5
MAX_SCREENSHOT_BYTES = 1024 * 1024


def version_key (version):
    return tuple (int (part) for part in version.split ('.'))


def is_text (value, max_length=None):
    return isinstance (value, str) and value.strip () != '' and (max_length is None or len (value) <= max_length)


def check_listing (listing):
    """The errors of a parsed listing.json against listing.schema.json, as a list of texts."""
    if not isinstance (listing, dict):
        return ['listing.json: is not a JSON object']
    errors = []
    for key in sorted (LISTING_KEYS - set (listing)):
        errors.append (f'listing.json: "{key}" is missing')
    for key in sorted (set (listing) - LISTING_KEYS):
        errors.append (f'listing.json: "{key}" is not a listing field')

    if 'slug' in listing and not (isinstance (listing['slug'], str) and SLUG_PATTERN.match (listing['slug']) and len (listing['slug']) <= 60):
        errors.append ('listing.json: "slug" must be lowercase letters, digits and single hyphens, at most 60 characters')
    if 'name' in listing and not is_text (listing['name'], 60):
        errors.append ('listing.json: "name" must be a text of at most 60 characters')
    if 'summary' in listing and not is_text (listing['summary'], 160):
        errors.append ('listing.json: "summary" must be a text of at most 160 characters')
    if 'category' in listing and listing['category'] not in CATEGORIES:
        errors.append (f'listing.json: "category" must be one of {", ".join (sorted (CATEGORIES))}')
    if 'tags' in listing:
        tags = listing['tags']
        if not isinstance (tags, list) or len (tags) > 8 or len (set (map (str, tags))) != len (tags) or not all (
                isinstance (tag, str) and TAG_PATTERN.match (tag) and len (tag) <= 30 for tag in tags):
            errors.append ('listing.json: "tags" must be at most 8 different lowercase words or hyphenated words')
    if 'author' in listing:
        author = listing['author']
        if not isinstance (author, dict) or not is_text (author.get ('name')):
            errors.append ('listing.json: "author" needs a "name"')
        else:
            for key in sorted (set (author) - AUTHOR_KEYS):
                errors.append (f'listing.json: "author.{key}" is not an author field')
            if 'github' in author and not (isinstance (author['github'], str) and GITHUB_PATTERN.match (author['github'])):
                errors.append ('listing.json: "author.github" must be a GitHub login without @')
            if 'discord' in author and not is_text (author['discord']):
                errors.append ('listing.json: "author.discord" must be a text')
    if 'license' in listing and not is_text (listing['license']):
        errors.append ('listing.json: "license" must be an SPDX identifier, such as MIT')
    if 'entry' in listing and not (isinstance (listing['entry'], str) and ENTRY_PATTERN.match (listing['entry'])):
        errors.append ('listing.json: "entry" must be the name of a .py file in the folder')

    if 'versions' in listing:
        versions = listing['versions']
        if not isinstance (versions, list) or not versions:
            errors.append ('listing.json: "versions" must list at least one version')
        else:
            numbers = []
            for index, version in enumerate (versions):
                where = f'listing.json: versions[{index}]'
                if not isinstance (version, dict):
                    errors.append (f'{where} is not an object')
                    continue
                for key in sorted (VERSION_KEYS - set (version)):
                    errors.append (f'{where}: "{key}" is missing')
                for key in sorted (set (version) - VERSION_KEYS):
                    errors.append (f'{where}: "{key}" is not a version field')
                if 'version' in version:
                    if isinstance (version['version'], str) and VERSION_PATTERN.match (version['version']):
                        numbers.append (version['version'])
                    else:
                        errors.append (f'{where}: "version" must be MAJOR.MINOR.PATCH')
                if 'archicadVersions' in version:
                    archicad = version['archicadVersions']
                    if not isinstance (archicad, list) or not archicad or len (set (map (str, archicad))) != len (archicad) or not all (
                            isinstance (v, int) and not isinstance (v, bool) and v >= 25 for v in archicad):
                        errors.append (f'{where}: "archicadVersions" must list different Archicad versions, 25 or later')
                if 'minTapirVersion' in version and not (isinstance (version['minTapirVersion'], str) and VERSION_PATTERN.match (version['minTapirVersion'])):
                    errors.append (f'{where}: "minTapirVersion" must be MAJOR.MINOR.PATCH')
                if 'changelog' in version and not is_text (version['changelog']):
                    errors.append (f'{where}: "changelog" must say what changed')
            if len (numbers) == len (versions):
                keys = [version_key (number) for number in numbers]
                if len (set (keys)) != len (keys):
                    errors.append ('listing.json: a version is listed twice')
                elif keys != sorted (keys, reverse=True):
                    errors.append ('listing.json: "versions" must be listed newest first')
    return errors


def read_header_block (lines, start, name):
    """The lines inside the # /// <name> block that starts at lines[start], and the index after it."""
    if start >= len (lines) or lines[start].rstrip () != f'# /// {name}':
        return None, start
    for index in range (start + 1, len (lines)):
        line = lines[index].rstrip ()
        if line == '# ///':
            return lines[start + 1:index], index + 1
        if not (line == '#' or line.startswith ('# ')):
            break
    return None, start


def read_tapir_header (text):
    """The # /// tapir header of a marketplace script as a dict, and the errors that were found."""
    lines = text.lstrip ('﻿').splitlines ()
    script, after = read_header_block (lines, 0, 'script')
    if script is None:
        return None, ['the script must start with the # /// script block (see marketplace/README.md)']
    if after >= len (lines) or lines[after].strip () != '':
        return None, ['an empty line must follow the # /// script block, or uv reads both blocks as one']
    after += 1
    tapir, _ = read_header_block (lines, after, 'tapir')
    if tapir is None:
        return None, ['the # /// tapir block must follow the # /// script block and its empty line']
    try:
        header = tomllib.loads ('\n'.join (line[2:] if line.startswith ('# ') else '' for line in tapir))
    except tomllib.TOMLDecodeError as error:
        return None, [f'the # /// tapir block is not valid TOML: {error}']
    return header, []


def check_header (header, listing):
    """The errors of a script's # /// tapir header against its listing."""
    errors = []
    if header.get ('changes') not in CHANGES:
        errors.append ('# /// tapir: "changes" must be "model", "files" or "read-only"')
    if header.get ('category') != listing.get ('category'):
        errors.append ('# /// tapir: "category" must be the category of listing.json')
    versions = listing.get ('versions')
    newest = versions[0] if isinstance (versions, list) and versions and isinstance (versions[0], dict) else {}
    if header.get ('archicad-versions') != newest.get ('archicadVersions'):
        errors.append ('# /// tapir: "archicad-versions" must be the archicadVersions of the newest version in listing.json')
    if header.get ('min-tapir-version') != newest.get ('minTapirVersion'):
        errors.append ('# /// tapir: "min-tapir-version" must be the minTapirVersion of the newest version in listing.json')
    return errors


def check_folder (folder):
    """The errors of one marketplace/<slug>/ folder, each starting with the file it is about."""
    slug = os.path.basename (os.path.normpath (folder))
    errors = []
    listing_path = os.path.join (folder, 'listing.json')
    listing = None
    if not os.path.isfile (listing_path):
        errors.append ('listing.json is missing')
    else:
        try:
            with open (listing_path, encoding='utf-8') as file:
                listing = json.load (file)
        except (ValueError, UnicodeDecodeError) as error:
            errors.append (f'listing.json is not valid JSON: {error}')
    if listing is not None:
        errors.extend (check_listing (listing))
        if isinstance (listing, dict) and listing.get ('slug') != slug:
            errors.append (f'listing.json: "slug" must be the folder\'s name, "{slug}"')

    entry = listing.get ('entry') if isinstance (listing, dict) and isinstance (listing.get ('entry'), str) else None
    if entry is not None and ENTRY_PATTERN.match (entry):
        entry_path = os.path.join (folder, entry)
        if not os.path.isfile (entry_path):
            errors.append (f'{entry}: the entry file named in listing.json is missing')
        else:
            try:
                with open (entry_path, encoding='utf-8') as file:
                    header, header_errors = read_tapir_header (file.read ())
            except UnicodeDecodeError:
                header, header_errors = None, ['the script is not UTF-8 text']
            errors.extend (f'{entry}: {error}' for error in header_errors)
            if header is not None:
                errors.extend (f'{entry}: {error}' for error in check_header (header, listing))

    if not os.path.isfile (os.path.join (folder, 'README.md')):
        errors.append ('README.md is missing')

    allowed = FOLDER_FILES | ({entry} if entry else set ())
    for name in sorted (os.listdir (folder)):
        path = os.path.join (folder, name)
        if name == 'screenshots' and os.path.isdir (path):
            shots = sorted (os.listdir (path))
            if len (shots) > MAX_SCREENSHOTS:
                errors.append (f'screenshots/: at most {MAX_SCREENSHOTS} screenshots, not {len (shots)}')
            for shot in shots:
                shot_path = os.path.join (path, shot)
                if not os.path.isfile (shot_path) or not SCREENSHOT_PATTERN.match (shot):
                    errors.append (f'screenshots/{shot}: only PNG and JPG files belong in screenshots/')
                elif os.path.getsize (shot_path) > MAX_SCREENSHOT_BYTES:
                    errors.append (f'screenshots/{shot}: at most 1 MB')
        elif os.path.isdir (path):
            errors.append (f'{name}/: a marketplace folder holds no folders but screenshots/ (no copies of aclib or utilities)')
        elif name not in allowed:
            errors.append (f'{name}: a marketplace folder holds only listing.json, the script, README.md and screenshots/')
    return errors


def script_folders (root):
    """The marketplace/<slug>/ folders, by name."""
    marketplace = os.path.join (root, 'marketplace')
    if not os.path.isdir (marketplace):
        return []
    return [os.path.join (marketplace, name) for name in sorted (os.listdir (marketplace))
            if os.path.isdir (os.path.join (marketplace, name)) and not name.startswith ('.')]


def main ():
    parser = argparse.ArgumentParser (description='Check the marketplace/<slug>/ folders against marketplace/README.md.')
    parser.add_argument ('--root', default=ROOT, help='the repository to check (default: this one)')
    args = parser.parse_args ()

    folders = script_folders (args.root)
    failed = 0
    for folder in folders:
        errors = check_folder (folder)
        if errors:
            failed += 1
            name = os.path.relpath (folder, args.root).replace (os.sep, '/')
            for error in errors:
                print (f'{name}/{error}')
    if failed:
        print (f'{failed} of {len (folders)} marketplace scripts do not follow marketplace/README.md.')
        return 1
    print (f'All {len (folders)} marketplace scripts follow marketplace/README.md.')
    return 0


if __name__ == '__main__':
    sys.exit (main ())
