"""Writes the machine-readable copies of the site, so an AI can learn Tapir from it.

Everything is generated from what the site and the repository already hold, without Archicad:

  docs/llms.txt          the llms.txt index (https://llmstxt.org): what Tapir is, how a command is
                         sent, and a link to the Markdown copy of every page
  docs/llms-full.txt     every page and a compact reference of every command, in one file
  docs/commands.json     every command: group, version it was added in, description, input and
                         output schemas, the Grasshopper components that send it and its Python
                         example; with the shared schema definitions
  docs/commands.md       the command reference in Markdown, with parameters and results
  docs/<page>/index.md   the text of each page (its <main>), in Markdown
  docs/sitemap.xml, docs/robots.txt

Sources: the pages under docs/, docs/archicad-addon/command_definitions.js and
common_schema_definitions.js (written by tools/generate_addon_docs.py, so run that first), the
examples in archicad-addon/Examples, the Grasshopper plugin's sources (read the way
tools/generate_gh_coverage.py reads them), the built-in scripts, archicad-addon/devkits.json and
docs/CNAME for the site's address. The output has no dates, so it only changes when its sources do.

Usage: python tools/generate_llm_files.py [--check]
  --check  do not write, exit with 1 when a file is out of date
"""

import argparse
import html
import html.parser
import json
import pathlib
import re
import sys
import urllib.parse

REPO_ROOT = pathlib.Path (__file__).resolve ().parent.parent
DOCS = REPO_ROOT / 'docs'
sys.path.insert (0, str (REPO_ROOT / 'tools'))

import generate_gh_coverage  # noqa: E402  (read_plugin, used_commands, components_of)

GITHUB = 'https://github.com/ENZYME-APD/tapir-archicad-automation'
RAW = 'https://raw.githubusercontent.com/ENZYME-APD/tapir-archicad-automation'
DISCORD = 'https://discord.gg/NAnSennmpY'

# The pages, in the order an AI should read them, with the section of llms.txt each goes in.
# Pages drawn by script from live data (the coverage checklists) point to that data instead.
PAGES = [
    ('', 'Docs'), ('start/', 'Docs'), ('docs/', 'Docs'), ('scripts/', 'Docs'), ('scripts/share/', 'Docs'),
    ('grasshopper/', 'Docs'), ('ai/', 'Docs'), ('tutorials/', 'Docs'),
    ('community/', 'Project'), ('roadmap/', 'Project'), ('about/', 'Project'),
]
LIVE_PAGES = [
    ('archicad-addon/', 'The command reference, also as commands.md and commands.json.'),
    ('api-coverage/', 'Which parts of the Archicad API Tapir uses, updated hourly; data: '
                      + RAW + '/api-coverage/api-coverage.md'),
    ('gh-coverage/', 'Which Tapir commands have a Grasshopper component; data: '
                     + RAW + '/api-coverage/gh-coverage.md'),
]


def all_pages ():
    """PAGES, then every other hand-written page under docs/ (a folder with an index.html that
    is not a LIVE_PAGES page and not under assets/), as 'Docs', so a new page is in llms.txt and
    the sitemap without an edit here."""
    known = {p for p, _ in PAGES} | {p for p, _ in LIVE_PAGES}
    found = sorted (path.parent.relative_to (DOCS).as_posix () + '/' for path in DOCS.rglob ('index.html')
                    if path.parent != DOCS and 'assets' not in path.relative_to (DOCS).parts)
    return PAGES + [(p, 'Docs') for p in found if p not in known]


def marketplace_slugs ():
    """The marketplace folders (those with a listing.json); each is a page of the Scripts page."""
    return sorted (path.parent.name for path in (REPO_ROOT / 'marketplace').glob ('*/listing.json'))


def site_url ():
    cname = DOCS / 'CNAME'
    if cname.exists ():
        return 'https://' + cname.read_text (encoding='utf-8').strip ().strip ('/') + '/'
    return 'https://enzyme-apd.github.io/tapir-archicad-automation/'


# ------------------------------------------------------------- HTML -> Markdown --

class MarkdownWriter (html.parser.HTMLParser):
    """The text of a page's <main> as Markdown: headings, paragraphs, lists, links (absolute),
    code, tables. Leaves out scripts, styles, SVG, templates, and what is hidden from everyone
    (the hidden attribute, aria-hidden); visually hidden headings stay, they name sections."""

    SKIP = {'script', 'style', 'svg', 'template', 'noscript', 'button', 'form', 'select', 'dialog'}
    VOID = {'br', 'img', 'hr', 'input', 'meta', 'link', 'source', 'wbr'}
    BLOCKS = {'p', 'div', 'section', 'article', 'header', 'aside', 'figure', 'figcaption', 'details',
              'summary', 'blockquote', 'dl', 'dt', 'dd', 'table', 'ul', 'ol'}

    def __init__ (self, base):
        super ().__init__ (convert_charrefs=True)
        self.base = base
        self.out = []
        self.in_main = 0
        self.skip = 0
        self.stack = []
        self.lists = []
        self.pre = 0
        self.href = []
        self.marks = []
        self.row = None
        self.rows = []
        self.cell = None

    def absolute (self, href):
        # Fragments too: the Markdown copy is read away from the page, so #section names the page.
        if re.match (r'^[a-z]+:', href):
            return href
        return urllib.parse.urljoin (self.base, href)

    def target (self):
        return self.cell if self.cell is not None else self.out

    def text (self, value):
        self.target ().append (value)

    def open_mark (self, mark):
        """Writes the opening of a link or emphasis and remembers where, for close_mark."""
        target = self.target ()
        if mark != '[' and target and target[-1].endswith ('*'):
            target.append (' ')     # two bold labels in a row (<strong>01</strong><strong>Post it</strong>)
        self.marks.append ((target, len (target)))
        target.append (mark)

    def close_mark (self, mark, href=None):
        """Closes what open_mark opened. Leaves out empty emphasis and links without text; a link
        around blocks (a card: tagline, heading, text) becomes a link on its heading instead,
        as a Markdown link cannot hold blocks."""
        if not self.marks:
            return
        target, start = self.marks.pop ()
        inner = ''.join (target[start + 1:])
        if not inner.strip ():
            del target[start:]
            target.append (inner)
        elif href is not None and '\n' in inner.strip ():
            heading = re.search (r'^(#+ )(.+)$', inner, re.M)
            if heading:
                inner = inner[:heading.start (2)] + f'[{heading.group (2).strip ()}]({href})' + inner[heading.end (2):]
            else:
                inner += f'\n\n<{href}>'
            target[start:] = ['\n\n' + inner.strip () + '\n\n']
        elif href is None:
            # Markdown emphasis may not start or end with a space: move it outside.
            lead, trail = inner[:len (inner) - len (inner.lstrip ())], inner[len (inner.rstrip ()):]
            target[start:] = [lead + target[start] + inner.strip () + mark + trail]
        else:
            target.append (mark)

    def block (self):
        if self.cell is None:
            self.out.append ('\n\n')
        else:
            self.cell.append (' ')      # a table cell stays on one line, its blocks apart

    def handle_starttag (self, tag, attrs):
        attrs = dict (attrs)
        if tag == 'main':
            self.in_main += 1
            return
        if not self.in_main:
            return
        if tag in self.VOID:
            if tag == 'br':
                self.text ('\n')
            return
        hidden = 'hidden' in attrs or attrs.get ('aria-hidden') == 'true'
        if self.skip or tag in self.SKIP or hidden:
            if hidden and not self.skip and not self.pre:
                self.text (' ')     # a separator drawn for the eye (·) still separates the words
            self.stack.append ((tag, 'skip'))
            self.skip += 1
            return
        self.stack.append ((tag, None))
        if re.fullmatch (r'h[1-6]', tag):
            # A heading that opens a list item (numbered steps) replaces the item's marker.
            index = len (self.out) - 1
            while index >= 0 and not self.out[index].strip ():
                index -= 1
            if index >= 0 and re.fullmatch (r'\n *(- |\d+\. )', self.out[index]):
                self.out[index] = '\n'
            self.block ()
            self.text ('#' * int (tag[1]) + ' ')
        elif tag in ('ul', 'ol'):
            self.lists.append ([tag, 0])
            self.block ()
        elif tag == 'li':
            kind = self.lists[-1] if self.lists else ['ul', 0]
            kind[1] += 1
            self.out.append ('\n' + '  ' * (len (self.lists) - 1) + ('- ' if kind[0] == 'ul' else f'{kind[1]}. '))
        elif tag == 'pre':
            self.pre += 1
            self.out.append ('\n\n```\n')
        elif tag == 'code' and not self.pre:
            self.text ('`')
        elif tag in ('strong', 'b'):
            self.open_mark ('**')
        elif tag in ('em', 'i'):
            self.open_mark ('*')
        elif tag == 'a' and attrs.get ('href'):
            self.href.append (self.absolute (attrs['href']))
            self.open_mark ('[')
        elif tag == 'a':
            self.href.append (None)
        elif tag == 'table':
            self.rows = []
        elif tag == 'tr':
            self.row = []
        elif tag in ('td', 'th'):
            self.cell = []
        elif tag in self.BLOCKS:
            self.block ()

    def handle_endtag (self, tag):
        if tag == 'main':
            self.in_main = max (0, self.in_main - 1)
            return
        if not self.in_main or tag in self.VOID:
            return
        while self.stack:
            open_tag, mark = self.stack.pop ()
            if mark == 'skip':
                self.skip -= 1
            elif not self.skip:
                self.close (open_tag)
            if open_tag == tag:
                break

    def close (self, tag):
        if re.fullmatch (r'h[1-6]', tag):
            self.block ()
        elif tag in ('ul', 'ol'):
            if self.lists:
                self.lists.pop ()
            self.block ()
        elif tag == 'pre':
            self.pre -= 1
            self.out.append ('\n```\n\n')
        elif tag == 'code' and not self.pre:
            self.text ('`')
        elif tag in ('strong', 'b'):
            self.close_mark ('**')
        elif tag in ('em', 'i'):
            self.close_mark ('*')
        elif tag == 'a':
            href = self.href.pop () if self.href else None
            if href:
                self.close_mark (f']({href})', href)
        elif tag in ('td', 'th'):
            if self.row is not None and self.cell is not None:
                self.row.append (re.sub (r'\s+', ' ', ''.join (self.cell)).strip ().replace ('|', '\\|'))
            self.cell = None
        elif tag == 'tr':
            if self.row:
                self.rows.append (self.row)
            self.row = None
        elif tag == 'table':
            if self.rows:
                width = max (len (r) for r in self.rows)
                lines = ['| ' + ' | '.join (r + [''] * (width - len (r))) + ' |' for r in self.rows]
                lines.insert (1, '|' + ' --- |' * width)
                self.out.append ('\n\n' + '\n'.join (lines) + '\n\n')
            self.rows = []
        elif tag in self.BLOCKS:
            self.block ()

    def handle_data (self, data):
        if not self.in_main or self.skip:
            return
        if self.pre:
            self.out.append (data)
        else:
            self.text (re.sub (r'\s+', ' ', data))

    def markdown (self):
        text = ''.join (self.out)
        lines = [line.rstrip () for line in text.split ('\n')]
        result, blank, fence = [], 0, False
        for line in lines:
            if line.startswith ('```'):
                fence = not fence
            if not fence:
                line = re.sub (r'^ +(?=\S)', lambda m: m.group (0) if re.match (r' *(- |\d+\. )', line) else '', line)
                line = re.sub (r'^( *(?:-|\d+\.)) {2,}', r'\1 ', line)    # a list marker before a skipped icon
                line = re.sub (r'(?<=\S) {2,}', ' ', line)
            if not line.strip () and not fence:
                blank += 1
                if blank > 1:
                    continue
            else:
                blank = 0
            result.append (line)
        return '\n'.join (result).strip () + '\n'


def page_markdown (path, base):
    writer = MarkdownWriter (base)
    writer.feed (path.read_text (encoding='utf-8'))
    return writer.markdown ()


def page_meta (path):
    text = path.read_text (encoding='utf-8')
    # The first <title> is the page's (an SVG on the page can have its own); both tags may carry
    # data-i18n attributes for the translations.
    title = re.search (r'<title\b[^>]*>(.*?)</title>', text, re.S)
    description = re.search (r'<meta\s[^>]*\bname="description"[^>]*\bcontent="([^"]*)"', text)
    title = html.unescape (title.group (1)).strip () if title else path.parent.name
    title = re.sub (r'\s+-\s+Tapir( documentation)?$', '', title)
    return title, html.unescape (description.group (1)) if description else ''


# ---------------------------------------------------------------- the commands --

def read_js_json (path, prefix):
    text = path.read_text (encoding='utf-8').strip ()
    if not text.startswith (prefix):
        raise SystemExit (f'{path} does not start with {prefix!r}: run python tools/generate_addon_docs.py')
    return json.loads (text[len (prefix):].rstrip (';'))


def schema_type (node, definitions):
    if not isinstance (node, dict):
        return 'any'
    if '$ref' in node:
        return node['$ref'].split ('/')[-1]
    for key in ('oneOf', 'anyOf'):
        if key in node:
            return ' or '.join (schema_type (n, definitions) for n in node[key])
    kind = node.get ('type', 'object' if 'properties' in node else 'any')
    if isinstance (kind, list):
        kind = ' or '.join (kind)
    if kind == 'array':
        return 'array of ' + schema_type (node.get ('items', {}), definitions)
    if 'enum' in node:
        return kind + ' (' + ', '.join (json.dumps (v) for v in node['enum'][:12]) + (', ...' if len (node['enum']) > 12 else '') + ')'
    return kind


def schema_fields (node, definitions, depth=0, prefix=''):
    """One line per property, nested objects and arrays of objects one level down."""
    if not isinstance (node, dict):
        return []
    lines = []
    required = set (node.get ('required', []))
    for name, child in (node.get ('properties') or {}).items ():
        description = (child.get ('description') or '').strip () if isinstance (child, dict) else ''
        line = f'{"  " * depth}- `{prefix}{name}` ({schema_type (child, definitions)}{", required" if name in required else ""})'
        lines.append (line + (': ' + re.sub (r'\s+', ' ', description) if description else ''))
        if depth < 1 and isinstance (child, dict):
            inner = child.get ('items') if child.get ('type') == 'array' else child
            if isinstance (inner, dict) and inner.get ('properties'):
                lines += schema_fields (inner, definitions, depth + 1)
    return lines


def read_examples ():
    examples = {}
    for path in sorted ((REPO_ROOT / 'archicad-addon' / 'Examples').glob ('*/*.py')):
        if path.parent.name == 'aclib' or path.name == 'aclib.py':
            continue
        code = path.read_text (encoding='utf-8').replace ('\r\n', '\n')
        doc = re.match (r'\s*(?:"""(.*?)"""|\'\'\'(.*?)\'\'\')', code, re.S)
        examples.setdefault (path.stem, {
            'path': path.relative_to (REPO_ROOT).as_posix (),
            'description': re.sub (r'\s+', ' ', (doc.group (1) or doc.group (2))).strip () if doc else '',
            'code': code,
        })
    return examples


def read_grasshopper (names):
    classes = generate_gh_coverage.read_plugin (REPO_ROOT / 'grasshopper-plugin' / 'TapirGrasshopperPlugin')
    used = generate_gh_coverage.used_commands (classes, set (names))
    return {command: generate_gh_coverage.components_of (classes, owners) for command, owners in used.items ()}


def read_commands (site):
    groups = read_js_json (DOCS / 'archicad-addon' / 'command_definitions.js', 'var gCommands = ')
    definitions = read_js_json (DOCS / 'archicad-addon' / 'common_schema_definitions.js', 'var gSchemaDefinitions = ')
    examples = read_examples ()
    names = [c['name'] for g in groups for c in g['commands']]
    grasshopper = read_grasshopper (names)
    commands = []
    for group in groups:
        for command in group['commands']:
            name = command['name']
            example = examples.get (name)
            commands.append ({
                'name': name,
                'group': group['name'],
                'version': command['version'],
                'description': command['description'],
                'inputSchema': command.get ('inputScheme'),
                'outputSchema': command.get ('outputScheme'),
                'grasshopperComponents': grasshopper.get (name, []),
                'example': None if not example else {
                    'path': example['path'],
                    'url': f'{GITHUB}/blob/main/{example["path"]}',
                    'description': example['description'],
                    'code': example['code'],
                },
                'docsUrl': f'{site}archicad-addon/#{name}',
            })
    return commands, definitions


# ------------------------------------------------------------------- outputs --

def call_section (site):
    return f"""## How a Tapir command is sent

Archicad (with a project open) answers JSON on `http://127.0.0.1:19723`. Tapir's commands go
through Archicad's own `API.ExecuteAddOnCommand`, in the namespace `TapirCommand`:

```json
{{"command": "API.ExecuteAddOnCommand",
 "parameters": {{"addOnCommandId": {{"commandNamespace": "TapirCommand", "commandName": "GetAddOnVersion"}},
                "addOnCommandParameters": {{}}}}}}
```

The answer is `{{"succeeded": true, "result": {{"addOnCommandResponse": {{...}}}}}}`. Python's standard
library is enough (`urllib.request`); the official `archicad` package and the Tapir Grasshopper
plugin send the same JSON. Every command, with its parameters and an example, is in
[commands.md]({site}commands.md) and [commands.json]({site}commands.json).
"""


def intro (site, commands, versions):
    return f"""# Tapir

> Tapir is a free, open-source (MIT) add-on for Graphisoft Archicad {versions[0]}–{versions[-1]}, by
> ENZYME-APD and its community. It adds {len (commands)} JSON commands to Archicad's own JSON API, a palette
> that runs Python scripts in one click and a Grasshopper plugin (package `tapir` on the Rhino Package
> Manager); community MCP servers let AI assistants drive Archicad through it. A new release ships
> every Monday.

- Site: {site}
- Source, issues and releases: {GITHUB}
- Download (Tapir Installer for Windows and macOS): {GITHUB}/releases/latest
- Community (Discord): {DISCORD}
- Tapir started in 2022 as R&D at Enzyme, an architecture practice; the add-on is built on Tibor
  Lorántfy's archicad-additional-json-commands template.
"""


def builtin_scripts ():
    scripts = []
    for path in sorted ((REPO_ROOT / 'builtin-scripts').glob ('*.py')):
        text = path.read_text (encoding='utf-8')
        name = re.search (r'^SCRIPT_NAME\s*=\s*"([^"]+)"', text, re.M)
        header = re.search (r'^#{10,}\n(.*?)^#{10,}', text, re.S | re.M)
        docstring = re.search (r'^(?:\s*#[^\n]*\n)*\s*("""|\'\'\')(.*?)\1', text, re.S)
        about = ''
        if docstring:     # the module docstring; ##### lines further down are section banners
            paragraphs = [p.strip () for p in docstring.group (2).strip ().split ('\n\n')]
            if not name and paragraphs:
                name = re.match (r'(.*)', paragraphs[0])
            # The first paragraph after the title that is not a credit (by, modified by, a link).
            body = [p for p in paragraphs[1:] if not re.search (r'^(by |modified by|https?://)', p, re.I | re.M)]
            about = re.sub (r'\s+', ' ', body[0] if body else '').strip ()
        elif header:
            lines = [re.sub (r'^#\s?', '', l).strip () for l in header.group (1).splitlines ()]
            body = [l for l in lines if l and not l.isupper () and not l.startswith ('For Archicad') and not l.startswith ('Created by')]
            about = ' '.join (body)
        scripts.append ((name.group (1).strip () if name else path.stem, about, path.relative_to (REPO_ROOT).as_posix ()))
    return scripts


def command_markdown (commands, definitions, compact):
    out, group = [], None
    for c in commands:
        if c['group'] != group:
            group = c['group']
            out.append (f'\n## {group}\n')
        if compact:
            params = schema_fields (c['inputSchema'], definitions) if c['inputSchema'] else []
            names = ', '.join (re.match (r'- `([^`]+)`', p).group (1) for p in params if p.startswith ('- `'))
            out.append (f'- **{c["name"]}** (since {c["version"]}): {c["description"]}' + (f' Parameters: {names}.' if names else ''))
            continue
        out.append (f'### {c["name"]}\n')
        out.append (f'{c["description"]}\n')
        out.append (f'Group: {c["group"]}. Added in Tapir {c["version"]}. Docs: {c["docsUrl"]}\n')
        params = schema_fields (c['inputSchema'], definitions) if c['inputSchema'] else []
        out.append ('Parameters:\n\n' + '\n'.join (params) + '\n' if params else 'Parameters: none.\n')
        result = schema_fields (c['outputSchema'], definitions) if c['outputSchema'] else []
        out.append ('Returns:\n\n' + '\n'.join (result) + '\n' if result else '')
        if c['grasshopperComponents']:
            out.append ('Grasshopper: ' + ', '.join (f'`{n}`' for n in c['grasshopperComponents']) + '\n')
        if c['example']:
            out.append (f'Example: [{c["example"]["path"]}]({c["example"]["url"]})'
                        + (f': {c["example"]["description"]}' if c['example']['description'] else '') + '\n')
    return '\n'.join (out).strip () + '\n'


def definitions_markdown (definitions):
    out = ['## Shared schema definitions\n', 'Types that parameters and results refer to by name.\n']
    for name, node in definitions.items ():
        fields = schema_fields (node, definitions)
        kind = schema_type (node, definitions)
        description = (node.get ('description') or '').strip () if isinstance (node, dict) else ''
        out.append (f'### {name}\n\n{kind}' + (f': {description}' if description else '') + '\n' + ('\n'.join (fields) + '\n' if fields else ''))
    return '\n'.join (out)


def build ():
    site = site_url ()
    versions = [d['archicad'] for d in json.loads ((REPO_ROOT / 'archicad-addon' / 'devkits.json').read_text (encoding='utf-8'))['devkits']]
    commands, definitions = read_commands (site)
    files = {}

    pages = []
    for path, section in all_pages ():
        source = DOCS / path / 'index.html'
        if not source.exists ():
            continue
        title, description = page_meta (source)
        markdown = page_markdown (source, site + path)
        files[f'{path}index.md'] = f'<!-- Generated by tools/generate_llm_files.py from {path}index.html. -->\n\n' + markdown
        pages.append ((path, section, title, description, markdown))

    # commands.json and commands.md
    files['commands.json'] = json.dumps ({
        'version': 1,
        'archicadVersions': versions,
        'howToCall': {'url': 'http://127.0.0.1:19723', 'command': 'API.ExecuteAddOnCommand',
                      'commandNamespace': 'TapirCommand'},
        'commands': commands,
        'schemaDefinitions': definitions,
    }, indent=1, ensure_ascii=False) + '\n'
    files['commands.md'] = ('<!-- Generated by tools/generate_llm_files.py from docs/archicad-addon. -->\n\n'
                            f'# Tapir commands\n\nAll {len (commands)} commands Tapir adds to Archicad, by group.\n\n'
                            + call_section (site) + '\n' + command_markdown (commands, definitions, compact=False)
                            + '\n' + definitions_markdown (definitions))

    scripts = builtin_scripts ()
    scripts_md = '## Scripts in the Tapir palette\n\n' + '\n'.join (
        f'- **{name}**: {about} ([source]({GITHUB}/blob/main/{path}))' for name, about, path in scripts) + '\n'

    # llms.txt
    lines = [intro (site, commands, versions), call_section (site)]
    for section in ('Docs', 'Project'):
        lines.append (f'## {section}\n')
        for path, sec, title, description, _ in pages:
            if sec == section:
                lines.append (f'- [{title}]({site}{path}index.md): {description}')
        lines.append ('')
    lines.append ('## Reference\n')
    lines.append (f'- [Commands in Markdown]({site}commands.md): every command with its parameters, results, Grasshopper components and example.')
    lines.append (f'- [Commands as JSON]({site}commands.json): the same, with the full JSON schemas and the example code.')
    for path, about in LIVE_PAGES:
        lines.append (f'- [{path.strip ("/")}]({site}{path}): {about}')
    lines.append ('')
    lines.append ('## Optional\n')
    lines.append (f'- [Everything in one file]({site}llms-full.txt): all pages, the palette scripts and a one-line reference of every command.')
    lines.append (f'- [Repository guide for AI assistants]({GITHUB}/blob/main/CLAUDE.md): how the add-on, the plugin and the bots are built.')
    files['llms.txt'] = '\n'.join (lines).rstrip () + '\n'

    # llms-full.txt
    full = [intro (site, commands, versions), call_section (site)]
    for path, _, title, _, markdown in pages:
        full.append (f'\n---\n\nSource: {site}{path}\n\n' + re.sub (r'^(#+) ', r'#\1 ', markdown, flags=re.M))
    full.append ('\n---\n\n' + scripts_md)
    full.append ('\n---\n\n# Command reference (one line each)\n\nParameters and results of each: '
                 f'{site}commands.md. ' + command_markdown (commands, definitions, compact=True))
    files['llms-full.txt'] = '\n'.join (full).rstrip () + '\n'

    # sitemap.xml and robots.txt
    urls = ([p for p, _ in all_pages () if (DOCS / p / 'index.html').exists ()] + [p for p, _ in LIVE_PAGES]
            + [f'scripts/?s={slug}' for slug in marketplace_slugs ()])
    files['sitemap.xml'] = ('<?xml version="1.0" encoding="UTF-8"?>\n'
                            '<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">\n'
                            + ''.join (f'  <url><loc>{site}{u}</loc></url>\n' for u in urls)
                            + '</urlset>\n')
    files['robots.txt'] = ('# Everyone, AI crawlers included, is welcome to read and learn from this site.\n'
                           'User-agent: *\nAllow: /\n\n'
                           f'Sitemap: {site}sitemap.xml\n'
                           f'# For AI assistants: {site}llms.txt\n')
    return files, len (commands), sum (1 for c in commands if c['example']), sum (1 for c in commands if c['grasshopperComponents'])


def main ():
    parser = argparse.ArgumentParser (description=__doc__.split ('\n')[0])
    parser.add_argument ('--check', action='store_true', help='exit with 1 when a file is out of date, write nothing')
    args = parser.parse_args ()
    files, count, with_example, with_component = build ()
    changed = []
    for name, content in sorted (files.items ()):
        path = DOCS / name
        if not path.exists () or path.read_text (encoding='utf-8') != content:
            changed.append (name)
            if not args.check:
                path.parent.mkdir (parents=True, exist_ok=True)
                with open (path, 'w', encoding='utf-8', newline='\n') as f:
                    f.write (content)
    summary = f'{count} commands ({with_example} with an example, {with_component} with a Grasshopper component), {len (files)} files'
    if not changed:
        print (f'The LLM files are up to date: {summary}.')
        return 0
    if args.check:
        print ('Out of date: ' + ', '.join (changed) + '. Run python tools/generate_llm_files.py')
        return 1
    print (f'Wrote {", ".join (changed)}: {summary}.')
    return 0


if __name__ == '__main__':
    sys.exit (main ())
