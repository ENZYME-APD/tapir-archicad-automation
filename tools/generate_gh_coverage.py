"""Generate the Grasshopper coverage checklist of the Tapir Add-On's commands.

Lists every JSON command the Tapir Add-On registers (AddOnMain.cpp, read like
tools/generate_addon_docs.py does) and marks which of them the Grasshopper
plugin calls, and how much of each command's input its components cover.

A command counts as used when a class of the plugin names it (a string literal
equal to the command name, e.g. CommandName => "CreateWalls") and sends it to
the Add-On: ToAddOn / SendAddOnCommand in the class or one of its base classes.
A name sent with ToArchicad is an official Archicad command of the same name
(e.g. CreateViewMapFolder) and does not count.

Parameter coverage of a used command: its parameters are the property names of
its input schema at every depth (common definitions behind $ref, such as
ElementId, are not counted). A parameter counts as covered when its name is a
string literal (or part of a dotted one, like "linkToSettings.newCreationMode")
or a member of an anonymous object (new { zoneElementId }) in a class that
sends the command, in its base classes, in the classes derived from an
abstract one, or in a helper class they call; or a [JsonProperty] name of a
type they use. A component that also parses a JSON input
(JObject.Parse, e.g. AdditionalSettings) reaches the parameters it has no input
for through that JSON. This is a static check: it shows which parameters the
code names, not that each one works.

Status of a command: done (used on main), merged (used on main but not in the
latest release), in progress (an open pull request adds a component naming
it), claimed (an open issue names it and Grasshopper), open, not applicable or
skipped (both with a reason, from the rules file).

Standard library only. Usage:

    python tools/generate_gh_coverage.py \
        --sources grasshopper-plugin/TapirGrasshopperPlugin \
        [--release-sources <the plugin at the latest release tag>] \
        [--github-data github.json] \
        [--rules tools/gh_coverage_rules.json] \
        --out-dir out

github.json is the file tools/fetch_api_coverage_github_data.py writes.

Writes out/gh-coverage.json, out/gh-coverage.md, out/gh-coverage-summary.md
and, for the README, out/gh-coverage-badge.json (a shields.io endpoint badge)
and out/gh-coverage.svg / out/gh-coverage-dark.svg (the summary as an image).
"""

import argparse
import collections
import datetime
import html
import json
import pathlib
import re
import sys

import generate_addon_docs
from generate_api_coverage import (BAR_STATUSES, REPO_URL, STATUS_MARK, STATUS_ORDER, SVG_THEMES, coverage, link,
                                   percent, svg_bar)

PLUGIN_PREFIX = 'grasshopper-plugin/'
CLASS_RE = re.compile (r'\b(?:class|struct)\s+([A-Za-z_][A-Za-z0-9_]*)(?:\s*<[^>{]*>)?\s*(?::\s*([^{]+?))?\s*(?:where\b[^{]*)?\{')
LITERAL_RE = re.compile (r'@?"((?:[^"\\]|\\.)*)"')
JSON_PROPERTY_RE = re.compile (r'JsonProperty\s*\(\s*"([^"]+)"')
IDENT_RE = re.compile (r'\b[A-Za-z_][A-Za-z0-9_]*\b')
ADDON_SEND_RE = re.compile (r'\b(?:ToAddOn|_toAddOn|toAddOn|SendAddOnCommand)\b')
ARCHICAD_SEND_RE = re.compile (r'\b(?:ToArchicad|_toArchicad|toArchicad)\b')
JSON_INPUT_RE = re.compile (r'\b(?:JObject|JToken|JArray)\.Parse\s*\(')
# Parameter coverage of a used command.
PARAMS_ORDER = ['all', 'json', 'partial']
PARAMS_MARK = {'all': '🟢', 'json': '🟡', 'partial': '🟠'}
PARAMS_LABEL = {
    'all': 'every parameter has an input',
    'json': 'the rest through a JSON input',
    'partial': 'some parameters missing',
}


# ------------------------------------------------------------------ C# code --

def strip_comments (text):
    """The text without // and /* */ comments; string literals are kept."""
    out, i, n = [], 0, len (text)
    while i < n:
        c = text[i]
        if c == '"' or (c == '@' and text[i + 1:i + 2] == '"'):
            verbatim = c == '@'
            j = i + (2 if verbatim else 1)
            while j < n:
                if text[j] == '\\' and not verbatim:
                    j += 2
                    continue
                if text[j] == '"':
                    if verbatim and text[j + 1:j + 2] == '"':
                        j += 2
                        continue
                    break
                j += 1
            out.append (text[i:j + 1])
            i = j + 1
        elif text.startswith ('//', i):
            j = text.find ('\n', i)
            i = n if j < 0 else j
        elif text.startswith ('/*', i):
            j = text.find ('*/', i + 2)
            i = n if j < 0 else j + 2
        else:
            out.append (c)
            i += 1
    return ''.join (out)


def block_end (text, start):
    """The index after the brace that closes the one at start; braces in strings are skipped."""
    depth, i, n = 0, start, len (text)
    while i < n:
        c = text[i]
        if c == '"':
            j = i + 1
            while j < n and text[j] != '"':
                j += 2 if text[j] == '\\' else 1
            i = j + 1
            continue
        if c == '{':
            depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1
    return n


def read_plugin (root):
    """Every class of the plugin: name -> {base, abstract, text, path}; text is the class's
    own body, without the bodies of the classes nested in it."""
    classes = {}
    if root is None:
        return classes
    for path in sorted (pathlib.Path (root).rglob ('*.cs')):
        if {'bin', 'obj'} & set (path.parts):
            continue
        text = strip_comments (path.read_text (encoding='utf-8-sig', errors='replace'))
        spans = []
        for m in CLASS_RE.finditer (text):
            bases = [b.strip () for b in (m.group (2) or '').split (',') if b.strip ()]
            base = re.sub (r'<.*', '', bases[0]).strip () if bases else None
            abstract = 'abstract' in text[text.rfind ('\n', 0, m.start ()) + 1:m.start ()]
            spans.append ((m.group (1), base, abstract, m.end () - 1, block_end (text, m.end () - 1)))
        for name, base, abstract, start, end in spans:
            inner = sorted ((s, e) for _, _, _, s, e in spans if start < s and e <= end)
            own, position = [], start
            for s, e in inner:
                if s >= position:
                    own.append (text[position:s])
                    position = e
            own.append (text[position:end])
            classes.setdefault (name, {'base': base, 'abstract': abstract, 'text': ''.join (own),
                                       'path': path.relative_to (root).as_posix ()})
    return classes


def chain (classes, name):
    """The class and its base classes of the plugin, nearest first."""
    result, seen = [], set ()
    while name in classes and name not in seen:
        seen.add (name)
        result.append (name)
        name = classes[name]['base']
    return result


def statement_at (text, position):
    """The statement around position: from the previous ; { or } to the next ;."""
    start = max (text.rfind (c, 0, position) for c in ';{}') + 1
    end = text.find (';', position)
    return text[start:end if end >= 0 else len (text)]


def sent_with (statement):
    """'addon', 'archicad' or None: where the call in the statement sends to."""
    addon = ADDON_SEND_RE.search (statement)
    archicad = ARCHICAD_SEND_RE.search (statement)
    if addon and (not archicad or addon.start () < archicad.start ()):
        return 'addon'
    return 'archicad' if archicad else None


def sends_to_addon (classes, owner, text, position):
    """Whether the command name literal at position of owner's text is sent to the Add-On.

    A name given as CommandName is sent where CommandName is used, in the class
    or in its base classes (the nearest one that uses it); any other literal
    counts only as an argument of a call that sends to the Add-On, so a
    component's display name or a resource name equal to a command does not.
    """
    if re.search (r'\bCommandName\s*=>\s*$', text[max (0, position - 80):position]):
        for name in chain (classes, owner):
            body = classes[name]['text']
            found = [sent_with (statement_at (body, m.start ()))
                     for m in re.finditer (r'\bCommandName\b(?!\s*=>)(?!\s*\{)', body)]
            found = [f for f in found if f]
            if found:
                return 'addon' in found
        return False
    return sent_with (statement_at (text, position)) == 'addon'


def type_keys (classes):
    """For each class: its [JsonProperty] names and the plugin classes it names."""
    result = {}
    for name, info in classes.items ():
        result[name] = (set (JSON_PROPERTY_RE.findall (info['text'])),
                        set (IDENT_RE.findall (info['text'])) & classes.keys ())
    return result


def anonymous_members (text):
    """The member names of the anonymous objects in text: new { a, b = x, c.D } -> a, b, D."""
    names = set ()
    for body in re.findall (r'\bnew\s*\{([^{}]*)\}', text):
        for member in body.split (','):
            member = member.strip ()
            m = re.match (r'([A-Za-z_][A-Za-z0-9_]*)\s*=(?!=)', member)
            if m:
                names.add (m.group (1))
            elif member:
                last = IDENT_RE.findall (member)
                if last:
                    names.add (last[-1])
    return names


def literal_keys (text):
    """The keys text names: its string literals, their dotted parts and its anonymous object members."""
    keys = set ()
    for literal in LITERAL_RE.findall (text):
        keys.add (literal)
        keys.update (literal.split ('.'))
    return keys | anonymous_members (text)


def keys_of (classes, types, owner):
    """The JSON keys a sending class names: literals of its chain and the JSON
    properties of the types it uses, transitively; and whether it parses JSON."""
    names = chain (classes, owner)
    keys, uses_json = set (), False
    pending = []
    for name in names:
        text = classes[name]['text']
        uses_json = uses_json or bool (JSON_INPUT_RE.search (text))
        keys |= literal_keys (text)
        pending += list (types[name][1])
    # The helper classes it calls (not components) fill in keys too, e.g. a
    # shared reader of the placement inputs; only directly, so the general
    # helpers they call in turn do not count.
    for name in set (pending) - set (names):
        if not any ('CommandName' in classes[c]['text'] for c in chain (classes, name)):
            keys |= literal_keys (classes[name]['text'])
    seen = set (names)
    while pending:
        name = pending.pop ()
        if name in seen:
            continue
        seen.add (name)
        keys |= types[name][0]
        pending += list (types[name][1])
    return keys, uses_json


def used_commands (classes, commands):
    """command -> sorted names of the classes that send it to the Add-On."""
    used = collections.defaultdict (set)
    for name, info in classes.items ():
        text = info['text']
        for m in LITERAL_RE.finditer (text):
            if m.group (1) in commands and sends_to_addon (classes, name, text, m.start ()):
                used[m.group (1)].add (name)
    return {command: sorted (owners) for command, owners in used.items ()}


def components_of (classes, owners):
    """The components behind the owners: an abstract owner stands for the
    classes derived from it that are not abstract themselves."""
    result = set ()
    for owner in owners:
        if not classes[owner]['abstract']:
            result.add (owner)
            continue
        result.update (name for name, info in classes.items ()
                       if not info['abstract'] and owner in chain (classes, name)[1:])
    return sorted (result) or sorted (owners)


# -------------------------------------------------------------- the Add-On --

def schema_parameters (node, path=''):
    """The property names of an input schema at every depth, not following $ref."""
    found = {}
    if isinstance (node, dict):
        for key, value in (node.get ('properties') or {}).items ():
            name = f'{path}.{key}' if path else key
            found.setdefault (key, name)
            found.update ({k: v for k, v in schema_parameters (value, name).items () if k not in found})
        for key in ('items', 'additionalProperties'):
            if isinstance (node.get (key), dict):
                found.update ({k: v for k, v in schema_parameters (node[key], path).items () if k not in found})
        for key in ('oneOf', 'anyOf', 'allOf'):
            for sub in node.get (key) or []:
                found.update ({k: v for k, v in schema_parameters (sub, path).items () if k not in found})
    return found


def read_commands ():
    sources = generate_addon_docs.Sources (generate_addon_docs.GetDocsArchicadVersion ())
    interpreter = generate_addon_docs.Interpreter (sources)
    commands = {}
    for group, items in generate_addon_docs.CollectCommandGroups (sources, interpreter):
        for item in items:
            schema = json.loads (item['inputScheme']) if item['inputScheme'] else None
            commands[item['name']] = {
                'group': group, 'version': item['version'], 'description': item['description'],
                'parameters': schema_parameters (schema) if schema else {},
            }
    return commands


# -------------------------------------------------------------------- build --

def build (args):
    commands = read_commands ()
    classes = read_plugin (args.sources)
    types = type_keys (classes)
    used = used_commands (classes, commands)
    release_used = None
    if args.release_sources:
        release_used = used_commands (read_plugin (args.release_sources), commands)

    rules = json.loads (args.rules.read_text (encoding='utf-8')) if args.rules else {}
    github = json.loads (args.github_data.read_text (encoding='utf-8')) if args.github_data else {}
    pr_text = [(pr['number'], pr.get ('title', ''),
                '\n'.join (f.get ('addedText', '') for f in pr.get ('addedFiles', []) if f.get ('path', '').startswith (PLUGIN_PREFIX)))
               for pr in github.get ('openPullRequests', [])]
    issue_text = [(i['number'], i.get ('title', ''), i.get ('title', '') + '\n' + (i.get ('body') or ''))
                  for i in github.get ('openIssues', [])]

    def rule_for (name, group):
        for section in ('skip', 'notApplicable'):
            data = rules.get (section, {})
            reason = data.get ('items', {}).get (name) or data.get ('groups', {}).get (group)
            if not reason:
                reason = next ((r for p, r in data.get ('patterns', {}).items () if re.search (p, name)), None)
            if reason:
                return ('skipped' if section == 'skip' else 'not applicable'), reason
        return None, None

    items = []
    for name, command in commands.items ():
        status, reason = rule_for (name, command['group'])
        owners = used.get (name, [])
        prs = [{'number': n, 'title': t} for n, t, text in pr_text if f'"{name}"' in text]
        issues = [{'number': n, 'title': t} for n, t, text in issue_text
                  if re.search (r'\b' + re.escape (name) + r'\b', text) and re.search (r'grasshopper', text, re.I)]
        if owners:
            released = None if release_used is None else bool (release_used.get (name))
            status = 'merged' if released is False else 'done'
            reason = None
        elif not status:
            status = 'in progress' if prs else 'claimed' if issues else 'open'

        parameters = command['parameters']
        components = components_of (classes, owners) if owners else []
        covered, missing, uses_json = [], [], False
        if owners:
            keys = set ()
            for owner in set (owners) | set (components):
                owner_keys, owner_json = keys_of (classes, types, owner)
                keys |= owner_keys
                uses_json = uses_json or owner_json
            for key, path in sorted (parameters.items (), key=lambda p: p[1]):
                (covered if key in keys else missing).append (path)
        params = None
        if owners:
            params = 'all' if not missing else 'json' if uses_json else 'partial'
        items.append ({
            'id': name, 'group': command['group'], 'version': command['version'], 'brief': command['description'],
            'status': status, 'reason': reason or '',
            'components': [{'name': o, 'path': classes[o]['path']} for o in components],
            'parameters': params, 'parameterCount': len (parameters),
            'coveredParameters': covered, 'missingParameters': missing,
            'pullRequests': prs, 'issues': issues,
        })
    return items, github.get ('generatedAt', '')


# ------------------------------------------------------------------- output --

def write_svg (path, counts, params, now, theme):
    """The summary as an image for the README, which cannot run scripts or read data."""
    width, pad = 760, 24
    used, applicable = coverage (counts)
    text = f'font-family="-apple-system, BlinkMacSystemFont, \'Segoe UI\', Helvetica, Arial, sans-serif" fill="{theme["fg"]}"'
    out = [
        '<defs>',
        f'<pattern id="claimed" width="6" height="6" patternUnits="userSpaceOnUse" patternTransform="rotate(45)">'
        f'<rect width="6" height="6" fill="{theme["track"]}"/><rect width="3" height="6" fill="{theme["claimed"]}"/></pattern>',
        '</defs>',
        f'<text x="{pad}" y="34" font-size="15" font-weight="600" {text}>Tapir commands in Grasshopper</text>',
        f'<text x="{pad}" y="84" font-size="44" font-weight="600" {text}>{percent (used, applicable)} %</text>',
        f'<text x="168" y="64" font-size="15" {text}><tspan font-weight="600">{used} of {applicable}</tspan> Tapir commands have a Grasshopper component</text>',
        f'<text x="168" y="85" font-size="13" {text} style="fill:{theme["muted"]}">'
        f'{counts["in progress"]} more in open pull requests, {counts["claimed"]} claimed by open issues, {counts["open"]} open</text>',
    ]
    out += svg_bar (pad, 104, width - 2 * pad, 14, counts, theme)
    legend = [('done', 'used on main'), ('merged', 'not yet released'), ('in progress', 'in a pull request'),
              ('claimed', 'claimed in an issue'), ('open', 'open')]
    widths = [15 + 7 * len (f'{label} {counts[status]}') for status, label in legend]
    gap = (width - 2 * pad - sum (widths)) / (len (legend) - 1)
    x = pad
    for (status, label), entry_width in zip (legend, widths):
        fill = 'url(#claimed)' if status == 'claimed' else theme['track' if status == 'open' else status]
        out.append (f'<rect x="{x:.1f}" y="134" width="10" height="10" rx="2" fill="{fill}" stroke="{theme["line"]}"/>')
        out.append (f'<text x="{x + 15:.1f}" y="143" font-size="12" {text} style="fill:{theme["muted"]}">{html.escape (label)} '
                    f'<tspan style="fill:{theme["fg"]}">{counts[status]}</tspan></text>')
        x += entry_width + gap
    # Parameter coverage of the used commands: all / through JSON / partial.
    y = 176
    total = sum (params.values ())
    out.append (f'<text x="{pad}" y="{y + 11}" font-size="13" {text}>Parameters</text>')
    out.append (f'<clipPath id="params"><rect x="180" y="{y}" width="400" height="12" rx="6"/></clipPath>')
    out.append (f'<rect x="180" y="{y}" width="400" height="12" rx="6" fill="{theme["track"]}"/>')
    position = 180
    for key, fill in (('all', theme['done']), ('json', theme['merged']), ('partial', theme['in progress'])):
        part = 400 * params[key] / max (total, 1)
        if part > 0:
            out.append (f'<rect x="{position:.1f}" y="{y}" width="{part:.1f}" height="12" fill="{fill}" clip-path="url(#params)"/>')
        position += part
    out.append (f'<text x="{width - pad}" y="{y + 11}" font-size="13" text-anchor="end" {text}>'
                f'<tspan font-weight="600">{percent (params["all"], total)} %</tspan>'
                f'<tspan style="fill:{theme["muted"]}"> complete</tspan></text>')
    y += 22
    out.append (f'<text x="180" y="{y + 10}" font-size="12" {text} style="fill:{theme["muted"]}">'
                f'{params["all"]} with every parameter, {params["json"]} with the rest through a JSON input, '
                f'{params["partial"]} with some missing</text>')
    y += 24
    out.append (f'<text x="{pad}" y="{y + 16}" font-size="12" {text} style="fill:{theme["muted"]}">'
                f'Not applicable and skipped commands are left out. Updated {html.escape (now)}.</text>')
    height = y + 32
    path.write_text (
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}" '
        f'role="img" aria-label="Tapir commands in Grasshopper: {used} of {applicable} ({percent (used, applicable)} %)">\n'
        f'<rect x="0.5" y="0.5" width="{width - 1}" height="{height - 1}" rx="8" fill="{theme["bg"]}" stroke="{theme["line"]}"/>\n'
        + '\n'.join (out) + '\n</svg>\n', encoding='utf-8')


def write_outputs (items, generated_at, out_dir):
    out_dir.mkdir (parents=True, exist_ok=True)
    now = generated_at or datetime.datetime.now (datetime.timezone.utc).strftime ('%Y-%m-%d %H:%M UTC')
    counts = collections.Counter (i['status'] for i in items)
    params = collections.Counter (i['parameters'] for i in items if i['parameters'])
    covered, applicable = coverage (counts)
    (out_dir / 'gh-coverage.json').write_text (json.dumps (
        {'generatedAt': now, 'counts': counts, 'parameters': params, 'items': items}, indent=1, ensure_ascii=False), encoding='utf-8')

    groups = collections.defaultdict (list)
    for item in items:
        groups[item['group']].append (item)

    used_total = sum (params.values ())
    summary = [
        f'**{covered} of {applicable}** applicable Tapir commands have a Grasshopper component '
        f'({percent (covered, applicable)} %): '
        + ', '.join (f'{STATUS_MARK[s]} {counts[s]} {s}' for s in STATUS_ORDER if counts[s]) + '.',
        '',
        f'Parameters of the {used_total} used commands: '
        + ', '.join (f'{PARAMS_MARK[p]} {params[p]} {PARAMS_LABEL[p]}' for p in PARAMS_ORDER) + '.',
        '',
        '| Command group | Used | Applicable | ' + ' | '.join (STATUS_MARK[s] for s in STATUS_ORDER) + ' | '
        + ' | '.join (PARAMS_MARK[p] for p in PARAMS_ORDER) + ' |',
        '|---|---|---|' + '---|' * (len (STATUS_ORDER) + len (PARAMS_ORDER)),
    ]
    for group, group_items in groups.items ():
        c = collections.Counter (i['status'] for i in group_items)
        p = collections.Counter (i['parameters'] for i in group_items if i['parameters'])
        group_used, group_applicable = coverage (c)
        summary.append (f'| {group} | {percent (group_used, group_applicable)} % | {group_applicable} | '
                        + ' | '.join (str (c[s] or '') for s in STATUS_ORDER) + ' | '
                        + ' | '.join (str (p[k] or '') for k in PARAMS_ORDER) + ' |')
    summary += ['', f'Updated {now}.']
    (out_dir / 'gh-coverage-summary.md').write_text ('\n'.join (summary) + '\n', encoding='utf-8')

    # For the README: https://img.shields.io/endpoint?url=<raw URL of this file>
    (out_dir / 'gh-coverage-badge.json').write_text (json.dumps ({
        'schemaVersion': 1, 'label': 'Grasshopper coverage',
        'message': f'{percent (covered, applicable)} % ({covered}/{applicable})', 'color': '1c5cab',
    }), encoding='utf-8')
    for name, theme in (('gh-coverage.svg', 'light'), ('gh-coverage-dark.svg', 'dark')):
        write_svg (out_dir / name, counts, params, now, SVG_THEMES[theme])

    lines = [
        '# Grasshopper coverage of the Tapir commands',
        '',
        'Every JSON command of the Tapir Add-On, whether a Grasshopper component calls it, and how much of its input '
        'the components cover. Generated by `tools/generate_gh_coverage.py`; do not edit by hand.',
        '',
    ] + summary + [
        '',
        '| | Status |',
        '|---|---|',
        '| ✅ | a component calls it on `main` |',
        '| 🔀 | on `main`, not yet in a release |',
        '| 🚧 | an open pull request adds a component for it |',
        '| 🙋 | an open issue asks for a component for it |',
        '| ⬜ | open |',
        '| ⛔ | a maintainer decided not to make a component for it (reason given) |',
        '| ➖ | not useful as a Grasshopper component (reason given) |',
        '| 🟢 | every parameter of the command has an input |',
        '| 🟡 | the parameters without an input can be given through a JSON input |',
        '| 🟠 | some parameters cannot be given (listed) |',
        '',
        'A parameter counts as covered when the component\'s code names it (a static check of the C# sources, '
        'not a test of each option).',
        '',
    ]
    for group, group_items in groups.items ():
        lines += [f'## {group}', '']
        for item in group_items:
            text = f'- {STATUS_MARK[item["status"]]} `{item["id"]}`'
            if item['parameters']:
                text += (f' {PARAMS_MARK[item["parameters"]]} {len (item["coveredParameters"])}/{item["parameterCount"]} parameters')
            if item['components']:
                text += ' – ' + ', '.join (f'`{c["name"]}`' for c in item['components'])
            refs = [link ('pr', p) for p in item['pullRequests']] + [link ('issue', i) for i in item['issues']]
            if refs:
                text += ' — ' + ', '.join (refs)
            if item['reason']:
                text += f' *({item["reason"]})*'
            if item['missingParameters']:
                text += '<br><sub>no input: ' + ', '.join (f'`{p}`' for p in item['missingParameters']) + '</sub>'
            lines.append (text)
        lines.append ('')
    (out_dir / 'gh-coverage.md').write_text ('\n'.join (lines), encoding='utf-8')
    return counts, params


def main ():
    parser = argparse.ArgumentParser (description=__doc__.split ('\n')[0])
    parser.add_argument ('--sources', required=True, type=pathlib.Path, help='grasshopper-plugin/TapirGrasshopperPlugin')
    parser.add_argument ('--release-sources', type=pathlib.Path)
    parser.add_argument ('--github-data', type=pathlib.Path)
    parser.add_argument ('--rules', type=pathlib.Path)
    parser.add_argument ('--out-dir', required=True, type=pathlib.Path)
    args = parser.parse_args ()
    try:
        items, generated_at = build (args)
    except generate_addon_docs.GeneratorError as e:
        print ('error: ' + str (e), file=sys.stderr)
        return 2
    counts, params = write_outputs (items, generated_at, args.out_dir)
    print (f'{len (items)} commands: ' + ', '.join (f'{counts[s]} {s}' for s in STATUS_ORDER if counts[s])
           + '; parameters: ' + ', '.join (f'{params[p]} {p}' for p in PARAMS_ORDER))
    return 0


if __name__ == '__main__':
    sys.exit (main ())
