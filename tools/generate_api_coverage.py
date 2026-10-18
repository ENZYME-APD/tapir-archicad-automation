"""Generate the Archicad API coverage checklist of the Tapir Add-On.

Lists every item of the Archicad API in the given DevKits and marks which of
them archicad-addon/Sources uses:

- C functions: every exported ACAPI_* function of every DevKit, one row per
  function, with the Archicad versions that have it. A function renamed in
  Archicad 27 (for example ACAPI_Element_GetCategoryValue, now
  ACAPI_Category_GetCategoryValue) and the AC25/AC26 command ids called
  through ACAPI_Database, ACAPI_Environment, ... (APIDb_GetDrawingScaleID,
  now ACAPI_Drawing_GetDrawingScale) are shown as older names of the current
  function when the match is unambiguous, and as their own rows otherwise.
- The C++ API (ACAPI:: and IFCAPI:: namespaces in Support/Modules/*/ACAPI):
  every exported class and its public member functions, and exported free
  functions.

Status of an item: done (used on main), merged (used on main but not in the
latest release), in progress (added by an open pull request), claimed (named
by an open issue), open, not applicable or skipped (both with a reason).

Standard library only. Usage:

    python tools/generate_api_coverage.py \
        --devkit 25=Build/DevKits/AC25/Support ... --devkit 30=Build/DevKits/AC30/Support \
        --sources archicad-addon/Sources \
        [--release-sources <sources of the latest release tag>] \
        [--github-data github.json] \
        [--rules tools/api_coverage_rules.json] \
        --out-dir out

github.json (optional, written by the workflow from the GitHub API):
    {"generatedAt": "...",
     "openPullRequests": [{"number": 1, "title": "...", "addedText": "..."}],
     "openIssues": [{"number": 2, "title": "...", "body": "..."}]}

Writes out/api-coverage.json, out/api-coverage.md and out/api-coverage-summary.md.
"""

import argparse
import collections
import datetime
import json
import pathlib
import re

REPO_URL = 'https://github.com/ENZYME-APD/tapir-archicad-automation'

C_EXPORT_RE = re.compile (r'(?:__APIEXPORT|\b[A-Z][A-Z0-9_]*EXPORT)\b[^;(]*?\b(ACAPI_[A-Za-z0-9_]+)\s*\(')
# Prefixes of the AC25/26 command ids (APIDb_GetDrawingScaleID, APINavigator_GetNavigatorItemID, ...).
OLD_ID_PREFIXES = ('APIDb_', 'APIEnv_', 'APIAny_', 'APIIo_', 'APIDo_', 'APIEng_', 'APIProp_', 'APIMod_', 'APINavigator_')
OLD_ID_RE = re.compile (r'\b((?:' + '|'.join (OLD_ID_PREFIXES) + r')[A-Za-z0-9]+ID)\b\s*=')
BRIEF_RE = re.compile (r'@brief\s+(.*?)(?:\n\s*\*?\s*\n|\n\s*\*?\s*@|\*/)', re.S)
IDENT_RE = re.compile (r'\b[A-Za-z_][A-Za-z0-9_]*\b')
# A #define with its name and its body, continuation lines included.
DEFINE_RE = re.compile (r'^[ \t]*#[ \t]*define[ \t]+([A-Za-z_][A-Za-z0-9_]*)((?:[^\n]*\\\n)*[^\n]*)', re.M)
# The start of an inline function: inline GSErrCode ACAPI_View_Zoom (
INLINE_WRAPPER_RE = re.compile (r'\binline\b[^;{}()]*?\b([A-Za-z_][A-Za-z0-9_]*)\s*\(')
EXPORT_MACRO_RE = re.compile (r'\b[A-Z][A-Z0-9_]*(?:EXPORT|DLL_EXPORT)[A-Z0-9_]*\b')
CPP_KEYWORDS = {'if', 'for', 'while', 'switch', 'return', 'sizeof', 'operator', 'decltype', 'static_assert',
                'alignas', 'noexcept', 'template', 'typename', 'explicit', 'friend', 'using', 'typedef'}
STATUS_ORDER = ['done', 'merged', 'in progress', 'claimed', 'open', 'skipped', 'not applicable']
STATUS_MARK = {
    'done': '✅', 'merged': '🔀', 'in progress': '🚧', 'claimed': '🙋',
    'open': '⬜', 'skipped': '⛔', 'not applicable': '➖',
}


def read_text (path):
    return path.read_text (encoding='utf-8', errors='replace')


def brief_before (text, position):
    before = text[max (0, position - 4000):position]
    start = before.rfind ('/**')
    if start == -1 or '*/' not in before[start:] or before[start:].count ('*/') > 1:
        return ''
    found = BRIEF_RE.search (before[start:])
    return ' '.join (found.group (1).replace ('*', ' ').split ()) if found else ''


def strip_comments (text):
    text = re.sub (r'/\*.*?\*/', lambda m: '\n' * m.group (0).count ('\n'), text, flags=re.S)
    return re.sub (r'//[^\n]*', '', text)


# --------------------------------------------------------------------- C API --

def read_c_api (support):
    """Exported ACAPI_* functions and the AC25/26 command ids of one DevKit."""
    functions, old_ids = {}, {}
    headers = list ((support / 'Inc').rglob ('*.h')) + list ((support / 'Modules').glob ('*API/*.h*'))
    for header in sorted (headers):
        text = read_text (header)
        for match in C_EXPORT_RE.finditer (text):
            functions.setdefault (match.group (1), brief_before (text, match.start ()))
        for match in OLD_ID_RE.finditer (text):
            line_end = text.find ('\n', match.end ())
            comment = text[match.end ():line_end]
            old_ids.setdefault (match.group (1), ' '.join (comment.split ('//', 1)[-1].split ()) if '//' in comment else '')
    return functions, old_ids


def function_stem (name):
    """ACAPI_Element_GetCategoryValue -> GetCategoryValue; APIDb_GetDrawingScaleID -> GetDrawingScale."""
    if name.startswith ('ACAPI_'):
        parts = name.split ('_', 2)
        return parts[2] if len (parts) == 3 else parts[1]
    stem = name.split ('_', 1)[1]
    return stem[:-2] if stem.endswith ('ID') else stem


# ------------------------------------------------------------------- C++ API --

def read_cpp_api (support):
    """Exported classes, their public member functions and exported free functions."""
    items = {}
    headers = sorted (p for p in (support / 'Modules').rglob ('*.hpp') if '/ACAPI/' in p.as_posix () + '/')
    headers += sorted ((support / 'Modules').glob ('*API/ACAPI_*.hpp'))
    for header in headers:
        module = header.relative_to (support / 'Modules').parts[0]
        raw = read_text (header)
        text = strip_comments (raw)
        # Walk the braces, keeping a stack of (kind, name, exported, access).
        stack = []
        statement = ''
        i = 0
        while i < len (text):
            ch = text[i]
            if ch == '{':
                head = statement.strip ()
                kind, name, exported = 'block', '', False
                ns = re.search (r'\bnamespace\s+([A-Za-z_][A-Za-z0-9_:]*)\s*$', head)
                cls = re.search (r'\b(class|struct)\s+((?:[A-Z][A-Z0-9_]*\s+)*)([A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z_][A-Za-z0-9_]*)*)\s*(?:final\s*)?(?::(?!:)[^{]*)?$', head)
                if ns:
                    kind, name = 'namespace', ns.group (1)
                elif cls and '(' not in head.split (cls.group (0))[0][-1:]:
                    kind, name = 'class', cls.group (3)
                    exported = bool (EXPORT_MACRO_RE.search (cls.group (2) or '')) or \
                        (bool (stack) and stack[-1][0] == 'class' and stack[-1][2])
                    if cls.group (1) == 'class':
                        access = 'private'
                    else:
                        access = 'public'
                    stack.append ([kind, name, exported, access])
                    statement = ''
                    i += 1
                    continue
                elif re.search (r'\)\s*(const)?\s*(noexcept)?\s*(override)?\s*(final)?\s*(->[^{]*)?$', head) or head.endswith ('='):
                    kind = 'body'
                record_statement (items, stack, head + ';', module, header.name, raw) if kind == 'body' else None
                stack.append ([kind, name, False, 'public'])
                statement = ''
            elif ch == '}':
                if stack:
                    stack.pop ()
                statement = ''
            elif ch == ';':
                record_statement (items, stack, statement + ';', module, header.name, raw)
                statement = ''
            elif ch == ':' and stack and stack[-1][0] == 'class':
                label = statement.strip ()
                if label in ('public', 'protected', 'private'):
                    stack[-1][3] = label
                    statement = ''
                else:
                    statement += ch
            elif ch == '#' and (i == 0 or text[i - 1] == '\n'):
                end = text.find ('\n', i)
                while end != -1 and text[end - 1] == '\\':
                    end = text.find ('\n', end + 1)
                i = len (text) if end == -1 else end
                continue
            else:
                statement += ch
            i += 1
    return items


def record_statement (items, stack, statement, module, header_name, raw):
    if any (frame[0] == 'body' for frame in stack):
        return
    statement = ' '.join (statement.split ())
    if '(' not in statement or statement.startswith (('using ', 'typedef ', 'friend ', 'static_assert', 'template<>')):
        return
    namespaces = [part for frame in stack if frame[0] == 'namespace' for part in frame[1].split ('::')]
    if not namespaces or namespaces[0] not in ('ACAPI', 'IFCAPI') or {'Impl', 'impl', 'details', 'detail'} & set (namespaces):
        return
    namespaces = [n for n in namespaces if not re.fullmatch (r'v\d+', n)]
    before_paren = statement.split ('(', 1)[0]
    names = IDENT_RE.findall (before_paren)
    if not names:
        return
    name = names[-1]
    if name in CPP_KEYWORDS or name.startswith ('_'):
        return
    classes = [frame for frame in stack if frame[0] == 'class']
    namespace = '::'.join (namespaces)
    if classes:
        cls = classes[-1]
        if not cls[2] or cls[3] != 'public' or '~' in before_paren or '= delete' in statement:
            return
        class_path = '::'.join (frame[1] for frame in classes)
        if name == cls[1].split ('::')[-1]:
            return  # constructors: covered by the class row
        key = f'{namespace}::{class_path}::{name}'
        items.setdefault (key, {'module': module, 'header': header_name, 'class': f'{namespace}::{class_path}',
                                'name': name, 'brief': ''})
    else:
        if not EXPORT_MACRO_RE.search (before_paren) and 'inline' not in before_paren and 'template' not in before_paren:
            return
        key = f'{namespace}::{name}'
        items.setdefault (key, {'module': module, 'header': header_name, 'class': '', 'name': name, 'brief': ''})


# --------------------------------------------------------------- Tapir usage --

def inline_wrappers (text):
    """The inline functions of a header: (name, identifiers of its body, start, end)."""
    wrappers = []
    for match in INLINE_WRAPPER_RE.finditer (text):
        i, depth = match.end (), 1
        while i < len (text) and depth:
            depth += {'(': 1, ')': -1}.get (text[i], 0)
            i += 1
        rest = re.match (r'[\s\w]*\{', text[i:])
        if not rest:
            continue  # a declaration, not a definition
        i += rest.end ()
        body_start, depth = i, 1
        while i < len (text) and depth:
            depth += {'{': 1, '}': -1}.get (text[i], 0)
            i += 1
        wrappers.append ((match.group (1), set (IDENT_RE.findall (text[body_start:i])), match.start (), i))
    return wrappers


def read_sources (sources):
    files = {}
    for path in sorted (sources.rglob ('*')):
        if path.suffix in ('.cpp', '.hpp', '.h', '.c'):
            text = strip_comments (read_text (path))
            scanned, defines = text, []
            if path.name == 'MigrationHelper.hpp':
                # Its #defines and inline wrappers map a current name to its AC25/26
                # name (#define ACAPI_Markup_Create ACAPI_MarkUp_Create): defining one
                # is not a use by itself; used_c_names follows it from a call of its name.
                defines = [(m.group (1), set (IDENT_RE.findall (m.group (2)))) for m in DEFINE_RE.finditer (text)]
                scanned = DEFINE_RE.sub ('', text)
                wrappers = inline_wrappers (scanned)
                defines += [(name, body) for name, body, _, _ in wrappers]
                for _, _, start, end in reversed (wrappers):
                    scanned = scanned[:start] + scanned[end:]
            files[path.name] = {
                'idents': set (IDENT_RE.findall (scanned)),
                'defines': defines,
                'includes': set (re.findall (r'#\s*include\s*[<"]([^">]+)[">]', text)),
                'text': text,
            }
    return files


def used_c_names (files):
    used = collections.defaultdict (set)
    for name, data in files.items ():
        for ident in data['idents']:
            if ident.startswith (('ACAPI_',) + OLD_ID_PREFIXES):
                used[ident].add (name)
    # A called macro or wrapper uses what it expands to: on AC25/26 a call of
    # ACAPI_Markup_Create is a call of ACAPI_MarkUp_Create, and a call of
    # TAPIR_Element_SetCategoryValue one of ACAPI_Category_SetCategoryValue.
    defines = [d for data in files.values () for d in data['defines']]
    called = set ().union (*(data['idents'] for data in files.values ()))
    changed = True
    while changed:
        changed = False
        for macro, body in defines:
            if macro in called and not body <= called:
                called |= body
                changed = True
    for ident in called:
        if ident.startswith (('ACAPI_',) + OLD_ID_PREFIXES) and ident not in used:
            used[ident].add ('MigrationHelper.hpp')
    return used


def cpp_used (item, files):
    """A member is used when a file that uses its class (or includes its header) calls it by name."""
    short_class = item['class'].split ('::')[-1] if item['class'] else ''
    call = re.compile (r'\b' + re.escape (item['name']) + r'\s*[(<]')
    for data in files.values ():
        if item['name'] not in data['idents']:
            continue
        uses_header = any (inc.endswith ('/' + item['header']) or inc == item['header'] for inc in data['includes'])
        uses_class = short_class and short_class in data['idents']
        if not (uses_header or uses_class):
            continue
        if item['class'] and not short_class:
            continue
        if call.search (data['text']):
            return True
    return False


# --------------------------------------------------------------------- build --

def build (args):
    versions = sorted (args.devkit)
    c_functions = {}  # name -> {versions, brief}
    old_ids = {}      # id -> {versions, comment}
    cpp_items = {}
    for version in versions:
        support = args.devkit[version]
        functions, ids = read_c_api (support)
        for name, brief in functions.items ():
            entry = c_functions.setdefault (name, {'versions': set (), 'brief': ''})
            entry['versions'].add (version)
            entry['brief'] = brief or entry['brief']
        for name, comment in ids.items ():
            entry = old_ids.setdefault (name, {'versions': set (), 'comment': comment})
            entry['versions'].add (version)
        for key, item in read_cpp_api (support).items ():
            entry = cpp_items.setdefault (key, dict (item, versions=set ()))
            entry['versions'].add (version)

    latest = versions[-1]
    current = {n for n, e in c_functions.items () if latest in e['versions']}
    by_stem = collections.defaultdict (list)
    for name in current:
        by_stem[function_stem (name)].append (name)

    # Older names: functions gone from the latest DevKit and AC25/26 command ids.
    aliases = collections.defaultdict (list)
    standalone_old = []
    for name, entry in sorted (c_functions.items ()):
        if name in current:
            continue
        candidates = by_stem.get (function_stem (name), [])
        if len (candidates) == 1:
            aliases[candidates[0]].append (name)
        else:
            standalone_old.append (name)
    unmatched_ids = []
    for name in sorted (old_ids):
        candidates = by_stem.get (function_stem (name), [])
        if len (candidates) == 1:
            aliases[candidates[0]].append (name)
        else:
            unmatched_ids.append (name)

    rules = json.loads (read_text (args.rules)) if args.rules else {}
    files = read_sources (args.sources)
    used = used_c_names (files)
    release_used = used_c_names (read_sources (args.release_sources)) if args.release_sources else None
    release_files = read_sources (args.release_sources) if args.release_sources else None

    github = json.loads (read_text (args.github_data)) if args.github_data else {}
    pr_text = [(pr['number'], pr.get ('title', ''), pr.get ('addedText', '')) for pr in github.get ('openPullRequests', [])]
    issue_text = [(i['number'], i.get ('title', ''), (i.get ('title', '') + '\n' + (i.get ('body') or ''))) for i in github.get ('openIssues', [])]

    pr_text = [(n, t, x, set (IDENT_RE.findall (x))) for n, t, x in pr_text]
    issue_text = [(n, t, x, set (IDENT_RE.findall (x))) for n, t, x in issue_text]

    def mentioned (names, haystacks):
        found = []
        for number, title, text, idents in haystacks:
            if any ((n in idents) if '::' not in n else (n in text) for n in names):
                found.append ({'number': number, 'title': title})
        return found

    def rule_for (item_id, group):
        for section in ('skip', 'notApplicable'):
            data = rules.get (section, {})
            reason = data.get ('items', {}).get (item_id) or data.get ('groups', {}).get (group)
            if not reason:
                reason = next ((r for p, r in data.get ('patterns', {}).items () if re.search (p, item_id)), None)
            if reason:
                return ('skipped' if section == 'skip' else 'not applicable'), reason
        return None, None

    items = []

    def add (item_id, kind, group, brief, item_versions, names, is_used, was_released, search_names):
        status, reason = rule_for (item_id, group)
        prs = mentioned (search_names, pr_text)
        issues = mentioned (search_names, issue_text)
        if not status:
            if is_used:
                status = 'merged' if was_released is False else 'done'
            elif prs:
                status = 'in progress'
            elif issues:
                status = 'claimed'
            else:
                status = 'open'
        items.append ({
            'id': item_id, 'kind': kind, 'group': group, 'brief': brief,
            'versions': sorted (item_versions), 'olderNames': [n for n in names if n != item_id],
            'status': status, 'reason': reason or '',
            'pullRequests': prs, 'issues': issues,
        })

    for name in sorted (current):
        names = [name] + sorted (aliases.get (name, []))
        item_versions = set (c_functions[name]['versions'])
        for alias in names[1:]:
            item_versions |= (c_functions.get (alias) or old_ids.get (alias))['versions']
        is_used = any (n in used for n in names)
        released = None if release_used is None else any (n in release_used for n in names)
        group = name.split ('_')[1] if name.count ('_') >= 2 else 'General'
        add (name, 'C function', group, c_functions[name]['brief'], item_versions, names, is_used, released, names)

    for name in standalone_old:
        is_used = name in used
        released = None if release_used is None else name in release_used
        group = name.split ('_')[1] if name.count ('_') >= 2 else 'General'
        add (name, 'C function (older versions only)', group, c_functions[name]['brief'],
             c_functions[name]['versions'], [name], is_used, released, [name])

    for name in unmatched_ids:
        is_used = name in used
        released = None if release_used is None else name in release_used
        group = 'AC25/26 command ids (' + name.split ('_')[0] + ')'
        add (name, 'AC25/26 command id', group, old_ids[name]['comment'], old_ids[name]['versions'],
             [name], is_used, released, [name])

    for key, item in sorted (cpp_items.items ()):
        is_used = cpp_used (item, files)
        released = None if release_files is None else cpp_used (item, release_files)
        group = 'C++ ' + item['module']
        add (key, 'C++ member function' if item['class'] else 'C++ function', group, item['brief'],
             item['versions'], [key], is_used, released, [key, item['class'] + '::' + item['name'] if item['class'] else key])

    return versions, items, github.get ('generatedAt', '')


# -------------------------------------------------------------------- output --

def link (kind, entry):
    path = 'pull' if kind == 'pr' else 'issues'
    return f'[#{entry["number"]}]({REPO_URL}/{path}/{entry["number"]} "{entry["title"].replace(chr(34), "")}")'


def write_outputs (versions, items, generated_at, out_dir):
    out_dir.mkdir (parents=True, exist_ok=True)
    now = generated_at or datetime.datetime.now (datetime.timezone.utc).strftime ('%Y-%m-%d %H:%M UTC')
    counts = collections.Counter (i['status'] for i in items)
    applicable = len (items) - counts['not applicable'] - counts['skipped']
    covered = counts['done'] + counts['merged']
    (out_dir / 'api-coverage.json').write_text (json.dumps (
        {'generatedAt': now, 'versions': versions, 'counts': counts, 'items': items}, indent=1, ensure_ascii=False), encoding='utf-8')

    groups = collections.defaultdict (list)
    for item in items:
        groups[item['group']].append (item)
    group_order = sorted (groups, key=lambda g: (g.startswith (('C++', 'AC25/26')), g.startswith ('AC25/26'), -len (groups[g]), g))

    summary = [
        f'**{covered} of {applicable}** applicable Archicad API items are used by Tapir '
        f'({100 * covered // max (applicable, 1)} %): '
        + ', '.join (f'{STATUS_MARK[s]} {counts[s]} {s}' for s in STATUS_ORDER if counts[s]) + '.',
        '',
        '| Part of the API | Used | Applicable | ' + ' | '.join (STATUS_MARK[s] for s in STATUS_ORDER) + ' |',
        '|---|---|---|' + '---|' * len (STATUS_ORDER),
    ]
    parts = collections.OrderedDict ([
        ('C functions', lambda i: i['kind'] == 'C function'),
        ('C functions of Archicad 25/26 only, and their command ids', lambda i: i['kind'] in ('C function (older versions only)', 'AC25/26 command id')),
        ('C++ API (ACAPI::, IFCAPI::)', lambda i: i['kind'].startswith ('C++')),
    ])
    for part, test in parts.items ():
        c = collections.Counter (i['status'] for i in items if test (i))
        part_applicable = sum (c.values ()) - c['not applicable'] - c['skipped']
        summary.append (f'| {part} | {100 * (c["done"] + c["merged"]) // max (part_applicable, 1)} % | {part_applicable} | '
                        + ' | '.join (str (c[s] or '') for s in STATUS_ORDER) + ' |')
    summary += ['', f'Archicad {versions[0]}–{versions[-1]} DevKits; updated {now}.']
    (out_dir / 'api-coverage-summary.md').write_text ('\n'.join (summary) + '\n', encoding='utf-8')

    lines = [
        '# Archicad API coverage',
        '',
        'Every function of the Archicad API (C functions, the C++ API and the Archicad 25/26 command ids), '
        'and whether the Tapir Add-On uses it. Generated by `tools/generate_api_coverage.py`; do not edit by hand.',
        '',
    ] + summary + [
        '',
        '| | Status |',
        '|---|---|',
        '| ✅ | used on `main` |',
        '| 🔀 | used on `main`, not yet in a release |',
        '| 🚧 | an open pull request adds it |',
        '| 🙋 | an open issue names it: someone is on it, check there before starting |',
        '| ⬜ | open: file an "Archicad API request" issue naming it to claim it |',
        '| ⛔ | a maintainer decided not to expose it (reason given) |',
        '| ➖ | not usable from a JSON command (reason given) |',
        '',
        'Names are the newest ones; older names (renamed functions, AC25/26 command ids) are listed with them. '
        '"Used" means Tapir calls it somewhere, not that every option is exposed. '
        'C++ member functions count as used when a file that uses their class calls a function of that name.',
        '',
        '| Group | ' + ' | '.join (STATUS_MARK[s] for s in STATUS_ORDER) + ' | Total |',
        '|---|' + '---|' * (len (STATUS_ORDER) + 1),
    ]
    for group in group_order:
        c = collections.Counter (i['status'] for i in groups[group])
        anchor = re.sub (r'[^a-z0-9 -]', '', group.lower ()).replace (' ', '-')
        lines.append (f'| [{group}](#{anchor}) | ' + ' | '.join (str (c[s] or '') for s in STATUS_ORDER) + f' | {len (groups[group])} |')
    lines.append ('')

    for group in group_order:
        lines += [f'## {group}', '']
        for item in groups[group]:
            text = f'- {STATUS_MARK[item["status"]]} `{item["id"]}`'
            if item['versions'] != versions:
                text += f' <sub>AC{item["versions"][0]}' + (f'–{item["versions"][-1]}' if len (item['versions']) > 1 else '') + '</sub>'
            if item['brief']:
                text += f' – {item["brief"]}'
            if item['olderNames']:
                text += ' <sub>(was ' + ', '.join (f'`{n}`' for n in item['olderNames']) + ')</sub>'
            refs = [link ('pr', p) for p in item['pullRequests']] + [link ('issue', i) for i in item['issues']]
            if refs:
                text += ' — ' + ', '.join (refs)
            if item['reason']:
                text += f' *({item["reason"]})*'
            lines.append (text)
        lines.append ('')
    (out_dir / 'api-coverage.md').write_text ('\n'.join (lines), encoding='utf-8')
    return counts


def main ():
    parser = argparse.ArgumentParser ()
    parser.add_argument ('--devkit', action='append', required=True, metavar='VERSION=SUPPORT_DIR')
    parser.add_argument ('--sources', required=True, type=pathlib.Path)
    parser.add_argument ('--release-sources', type=pathlib.Path)
    parser.add_argument ('--github-data', type=pathlib.Path)
    parser.add_argument ('--rules', type=pathlib.Path)
    parser.add_argument ('--out-dir', required=True, type=pathlib.Path)
    args = parser.parse_args ()
    args.devkit = {int (v): pathlib.Path (p) for v, p in (d.split ('=', 1) for d in args.devkit)}
    versions, items, generated_at = build (args)
    counts = write_outputs (versions, items, generated_at, args.out_dir)
    print (f'{len (items)} items: ' + ', '.join (f'{counts[s]} {s}' for s in STATUS_ORDER if counts[s]))


if __name__ == '__main__':
    main ()
