# Website style guide

The rules for every page of <https://archicad-api.com> (the folder [`docs/`](../docs/)), for people and
for AI assistants alike. A new page, a new section or a change to an existing one follows them; a page
that breaks one is fixed, not copied. When a rule here and the code disagree, the code in
[`docs/assets/site.css`](../docs/assets/site.css) is what the browser draws: fix whichever is wrong, in
the same pull request.

The words **must** and **never** are rules; **prefer** is the default, which a page may leave with a reason
written in a comment next to the code.

Contents: [1. The looks](#1-the-looks) · [2. Files and the page skeleton](#2-files-and-the-page-skeleton) ·
[3. Colour](#3-colour) · [4. Type](#4-type) · [5. Spacing and layout](#5-spacing-and-layout) ·
[6. Page structure](#6-page-structure) · [7. Components](#7-components) · [8. Numbering and labels](#8-numbering-and-labels) ·
[9. Writing](#9-writing) · [10. Links, buttons and calls to action](#10-links-buttons-and-calls-to-action) ·
[11. Accessibility](#11-accessibility) · [12. Translation](#12-translation) · [13. Motion](#13-motion) ·
[14. Checklist for a change](#14-checklist-for-a-change) · [15. Known inconsistencies](#15-known-inconsistencies)

## 1. The looks

The site has one default look and one optional one. Both must keep working.

- **Standard** (the default, "the standard theme of today"): white surfaces, Inter for all text,
  JetBrains Mono only for code and command names, no textures, thin light lines, soft shadows, 8 px
  corners on controls and 12 px on cards. Its rules are the last part of `site.css`, each starting with
  `:root:not([data-style="legacy"])`, and the last part of each page's own stylesheet.
- **Legacy** (`data-style="legacy"` on `<html>`, chosen in the Display menu): the old architect's
  drawing set: cream paper with a 24 px grid, Fraunces headings, moss page heads, ochre accents, ink
  lines and hard offset shadows. It is the first part of `site.css`. Nothing new is designed for Legacy
  first; it only has to stay readable.

On top of the look, the **Display menu** (`assets/site.js`) sets attributes on `<html>` that every page
must respect:

| Attribute | Values | What it changes |
| --- | --- | --- |
| `data-theme` | none (follows the system), `light`, `dark`, `contrast` | Mode. `contrast` is High contrast: black on white, one look, no accent. |
| `data-accent` | none (green in Standard, ochre in Legacy), `blue`, `purple`, `orange` | The accent colour of links, buttons, focus and badges. |
| `data-size` | none (100 %), `90`, `115`, `130` | The root font size; every text size scales with it. |
| `data-font` | none, `system`, `readable` | The font stacks (`--body`, `--mono`, `--display`). |
| `data-spacing` | none, `compact` | The row padding of the long lists only. |
| `data-motion` | none (follows the system), `reduced`, `full` | Animations and transitions. |

So a page **must**: take every colour, font and shadow from a custom property (section 3), size text in
`rem` (section 4), give its own colours for dark mode and High contrast when it has any, and check its
new parts in Light, Dark and High contrast, in both looks, and at Text size 130 %.

## 2. Files and the page skeleton

- Shared look: `docs/assets/site.css` (with `site.js`), animation: `docs/assets/motion.css` (with
  `motion.js`). A page's own stylesheet sits next to it (`home.css`, `community/community.css`,
  `scripts/scripts.css`); the reference pages share `assets/coverage.css`, and the command reference has
  `archicad-addon/style.css`.
- Prefer a shared class in `site.css` to a page stylesheet, and a page stylesheet to anything else.
  A shape used on two pages belongs in `site.css`.
- **Never** use a `style="…"` attribute in a page's HTML, and never a `<style>` element. The only
  exceptions are SVG drawing attributes (`fill`, `font-size` and the like on SVG elements) and
  `display: none` on an SVG group a script shows later.
- A page stylesheet follows the order of `site.css`: its Legacy rules first, then a block commented
  "The Standard look (see the end of site.css)", then its rules for the Display menu (Accent, High
  contrast, Font, Spacing, Motion) when it has any. Every rule that draws a line, a corner, a colour, a
  font or a label case in the Legacy part has its Standard counterpart there.
- Every page repeats, byte for byte, the blocks between these comments, copied from an existing page
  (paths adjusted with `../`):
  - `<!-- site-assets -->` … `<!-- /site-assets -->` in the head: the favicon, the two `theme-color`
    metas (`#11161D` for dark first, then `#FFFFFF`), the Google Fonts link for Inter (400, 500, 600, 700)
    and JetBrains Mono (400, 500, 700), `site.css`, the inline Display script, the inline translation
    script, `site.js`, `motion.css` with `motion.js`, and the inline splash script.
  - `<!-- site-header -->` … `<!-- /site-header -->`: the skip link, the brand with both marks, the six
    nav links (the current section's link has `aria-current="true"`), the Display button and the
    Download button.
  - `<!-- site-footer -->` … `<!-- /site-footer -->`: the title block (Project, Product, Docs,
    Community, Revision and Licence) and the fine print.
- The head also has, in this order: `charset`, `viewport`, `<title data-i18n>`, the `description` meta
  (with `data-i18n-attrs="content"`), the `canonical` link, the `BreadcrumbList` JSON-LD matching the
  crumb, and the link-preview metas (`og:type`, `og:site_name`, `og:title` equal to the title,
  `og:description` equal to the description, `og:url` equal to the canonical URL, `twitter:card`).
- Indent HTML by 4 spaces, starting at `<main>`'s children (`<main>` itself and `<header>`/`<footer>`
  at column 0, their children indented once).
- The pages' copies for AI assistants (`index.md`, `llms.txt`, `llms-full.txt`, `sitemap.xml`) are
  generated: after changing a page, run `python tools/generate_llm_files.py`, never edit them by hand.

## 3. Colour

Every colour comes from a custom property. A literal colour (`#…`, `rgb()`, `rgba()`) is only allowed
inside the blocks that define those properties (`:root…{ --name: … }`), never in a rule that draws.

Standard, the tokens to use (light / dark / High contrast):

| Property | Light | Dark | High contrast | Use |
| --- | --- | --- | --- | --- |
| `--bg` | `#FFFFFF` | `#0B0F14` | `#FFFFFF` | Page background |
| `--surface` | `#FFFFFF` | `#11161D` | `#FFFFFF` | Cards, panels, header, footer |
| `--subtle` | `#F7F8FA` | `#0E1318` | `#FFFFFF` | Page heads, hovered rows, screenshot frames |
| `--fg` | `#111827` | `#E6EAF0` | `#000000` | Text and headings |
| `--muted` | `#4B5563` | `#9AA4B2` | `#333333` | Secondary text: descriptions, captions, labels |
| `--line` | `#E5E7EB` | `#232B36` | `#000000` | Borders of cards and panels, row dividers |
| `--border-strong` | `#D1D5DB` | `#3A4553` | `#000000` | Borders of controls (buttons, chips, inputs) |
| `--link` | `#1F6B4E` | `#4CB88A` | `#0B4FA8` | Links, taglines, step numbers, the current nav item |
| `--link-hover` | `#164D38` | `#6FCBA3` | `#000000` | Hovered links |
| `--accent-fill` / `--on-accent` | `#1F6B4E` / white | `#4CB88A` / `#0B0F14` | black / white | Filled buttons, pressed chips, the add-on box, video play buttons |
| `--badge` | accent at 10 % | accent at 10 % | `#E6E6E6` | Pills, tinted captions, checked options |
| `--code-bg` / `--code-fg` | `#F3F4F6` / `#111827` | `#1A212B` / `#E6EAF0` | `#F2F2F2` / black | Inline `code` |
| `--pre-*`, `--copy-*` | | | | Code blocks, the terminal (`.term`) and their copy button |
| `--warning` | `#B45309` | `#F59E0B` | `#7C2D12` | Warnings only |

Rules:

- Text is `--fg` or `--muted`, links are `--link`. Never `--slate` or a grey literal for text.
- An accent colour is `--link` (for text and thin marks) or `--accent-fill` (for filled shapes).
  **Never** `--ochre`, `--moss` or another Legacy brand colour in a Standard rule: they are Legacy's and
  ignore the Display menu's Accent. A Legacy rule that uses `--ochre` needs a Standard rule using
  `--accent-fill` (fills) or `--link` (text, dots, bars).
- Text on a filled accent is `--on-accent`, never a literal `#FFFFFF` (in Dark it is near black).
- A page's own palette (a chart's layers, the coverage ramp) is defined once per mode as properties
  (`--layer-*`, `--track`, `--done`, …) in three blocks: light, dark (both selectors, see the top of
  `site.css`), and High contrast in greys/black. Prefer shades of the accent and of `--muted`'s grey.
- Contrast: every text pair passes WCAG AA (4.5:1, 3:1 for text 24 px and larger) in Light and Dark,
  and AAA (7:1) in High contrast. Write the measured ratio in the comment above a new colour.
- Dark mode is written with both selectors `site.css` names at its top, so it applies when chosen and
  under the system's dark mode, but never in High contrast.

## 4. Type

Fonts: `--body` (Inter) for all text and headings in Standard; `--mono` (JetBrains Mono) only for code,
command and parameter names, keyboard keys and file names. `--display` is Fraunces in Legacy and Inter in
Standard: use it for headings and big numbers so Legacy keeps its serif. Never name a font family in a
rule; always use these three properties (the Display menu's Font replaces them).

Sizes are in `rem` (1 rem = 16 px at 100 %), never `px`, `em` for a whole element, or `vw` alone. The
only `px` text is inside drawings (the floor plan, the growth chart) and the header's brand.

The scale. Every text size **must** be one of these; the sizes in between (`.65625`, `.6875`,
`.71875`, `.78125`, `.84375`, `.90625`, `1.125`, `1.25`, `1.3125rem`) were the drift that part 5 of
#2765 removed and are not used again:

| Step | Size | Use |
| --- | --- | --- |
| Label | `.75rem` (12 px) | Taglines, dimension lines, table heads, footer labels, field labels in `.spec` |
| Small | `.8125rem` (13 px) | Breadcrumbs, captions, code in code blocks, figure labels, fine print |
| Note | `.875rem` (14 px) | Card notes, small buttons, chips, footer links, the text of a `.flow` step |
| Nav | `.9375rem` (15 px) | Header nav, buttons |
| Body | `1rem` (16 px) | Body text, line height 1.6 |
| Lead | `1.0625rem` (17 px) | Section introductions (`.sec-head p`) |
| Head lead | `1.1875rem` (19 px) | The introduction under a page's `h1` |
| h4 | `1.0625rem` | |
| h3 | `1.375rem` (22 px) | Card titles, step titles under a section head |
| Sub-heading | `1.625rem` (26 px) | `.ref-sub`; a card title in a three-card row (`.comm`) |
| Big card | `1.875rem` (30 px) | Teaser titles, step titles of a page that is only steps |
| h2 | `clamp(2rem, 4.6vw, 3.5rem)` | A section's heading (`.sec-head h2`) |
| h1 | `clamp(2.75rem, 7vw, 5.5rem)` | The page head's `h1` |

On phones (below 640 px) `site.css` steps the large sizes down (h1 `2.25rem`, section h2 `1.75rem`,
teaser and card titles `1.375rem`); a page never sets its own phone size for a shared class.

Weights (Standard): 400 body, 500 labels and nav, 600 pills, chips and table cells that name a thing,
700 headings, buttons, big numbers and step numbers. Legacy's 900 and 500-italic headings are Legacy's
only. `em` in a heading is normal (upright) in Standard and italic in Legacy, automatically: never style
it yourself.

Case: labels are written in sentence case in the HTML. Legacy turns taglines, dimension lines, table
heads and step labels to capitals with `text-transform`; Standard shows them as written. **Never** type
capitals for emphasis (`READ`, `FREE`): they shout in Standard. Exceptions are real acronyms (API, JSON,
MCP, IFC, BIM, CI).

Numbers that are compared (figures, counts in tables) use `font-variant-numeric: tabular-nums`.

## 5. Spacing and layout

All layout spacing is a multiple of 4 px: `4, 8, 12, 16, 20, 24, 32, 40, 48, 56, 72`. The inner
padding of controls (buttons, chips, pills, inputs, the copy button) may use the even values in between.
Spacing stays in `px` (so the Display menu's Text size changes text only).

| Where | Desktop | Phone (below 640 px) |
| --- | --- | --- |
| Content width | `.wrap`, 1200 px, 16 px side padding | the same |
| Page head padding | 56 px top, 48 px bottom | 28 / 24 px |
| Page body (`.page-body`) | 56 px top, 24 px bottom | 24 / 16 px |
| Section | 56 px top, 32 px bottom; 32 px top after a `.dim` in a `.page-body` or as the first section (a section in a plain `.wrap`, on the home page, gets it with `.after-dim`) | 28 / 16 px; 20 px |
| Dimension line (`.dim`) | 40 px above | 24 px |
| Section head to content | 40 px | 20 px |
| Space between two blocks inside a section | 32 px (24 px after a grid or a `.flow`, 12 px between the first blocks of a page body): `.gap-top`, `.gap-top-sm`, `.gap-top-xs` | the same |
| Gap between cards in a grid | 20 px | 16 px |
| Card padding (`.way`, `.teaser`, `.share-steps li`) | 24 px | 18 px |
| Cell padding (`.flow`, `.uses-grid`, `.figures`, `.spec`) | 20 px | 16 px |
| Footer | 72 px above | 40 px |

Grids collapse to one column below 900 px (two columns for `.uses-grid`), as `site.css` does for
`.two`, `.ways`, `.teasers`, `.doc-grid`, `.uses`, `.videos`, `.share-steps ol` and `.flow`. A new grid
joins those lists rather than adding its own breakpoint. Breakpoints are 900, 640 and 400 px, and the
phone header's `(max-width: 640px), (max-width: 1024px) and (max-height: 500px)`; no others.

Corners (Standard): `--radius-control` (8 px) for buttons, chips, inputs, the copy button, callouts and
code blocks; `--radius-card` (12 px) for cards, panels, the terminal, tables in a frame and screenshots;
999 px only for pills. Never a literal radius in a Standard rule.

Lines: 1 px `--line` for cards and dividers, 1 px `--border-strong` for controls, the callout's 4 px
accent edge. Legacy's 1.5 px ink line (`1.5px solid var(--fg)`) is Legacy's only.

Shadows: `--shadow-soft` (on a hovered card, through `--shadow-lift`), `--shadow-menu` for popups. None
in Dark or High contrast (the properties are `none` there). Never a literal `box-shadow` in a Standard rule.

## 6. Page structure

Every page has, in order:

1. The header (section 2).
2. `<main id="main">` with a **page head**: `<div class="page-head"><div class="wrap">` holding the
   breadcrumb `<p class="crumb" data-i18n>`, exactly one `<h1>`, and one introduction `<p>` (two at
   most). The home page's `.hero` is the only other kind of page head; the scripts page's script and
   "not found" views, which are not pages of their own, use `.page-head` with an `h2 class="h1"`.
   `.page-head.ochre` is for the Share page only (Legacy draws it hatched; Standard draws it as any other).
3. `<div class="wrap page-body">` with the sections.
4. The footer.

The breadcrumb is `Tapir / <section> / <page>`, every part but the last a link, the section being the
nav item the page sits under (the same one that has `aria-current`): Docs for the references, guides
and limits, Community for tutorials and the roadmap. The JSON-LD `BreadcrumbList` repeats it.

Sections:

- Each top-level part of the page body is a `<section>` with `aria-labelledby` pointing at its `h2`, or
  `aria-label` when it has none.
- The first section opens the body directly. **Every later section is preceded by a dimension line**:
  `<div class="dim" aria-hidden="true"><i></i><span data-i18n>…</span><i></i></div>`.
- A section opens with a **section head** `<div class="sec-head"><h2 id="…" data-i18n>…</h2><p data-i18n>…</p></div>`
  (on the reference pages `.sec-head.ref`), or, when the dimension line already says what it is, a
  `<h2 class="visually-hidden">` with the dimension line's text. A section that is a `.two` with its
  text in one column and a code block in the other puts its `h2` in the text column as `h2.col-head`.
- A heading inside a section is `h3` (cards, steps under a section head) or, on the reference pages,
  `.ref-sub`. Never skip a level, and never size a heading with anything but these classes.
- The last section of a page is usually a **next step**: a dimension line "Next step" (on a tutorial,
  "Next tutorial") and one `.teaser.dark` with a tagline, an `h3`, a sentence and one or two buttons.

## 7. Components

Use these from `site.css` before making a new one; a new component that a second page needs goes there.

| Component | Markup | For |
| --- | --- | --- |
| Section head | `.sec-head > h2 + p` | Every titled section |
| Dimension line | `.dim > i + span + i` | Between sections |
| Teaser | `a.teaser > .tagline + h3 + p + .go` | A card that is one link to another page |
| Next step | `div.teaser.dark > .tagline + h3 + p + .ctas` | The end of a page |
| Way | `article.way > .who + h3 + p + a` (an icon `svg` optional) | A card with its own links or list |
| Grids | `.ways`, `.comm`, `.teasers` (3 columns), `.doc-grid`, `.two` (2) | Cards in a row |
| Uses grid | `ul.uses-grid` (4 columns) or `.uses-grid.three` | Short facts in a frame of cells |
| Figures | `.figures > div > strong + span` | Live numbers |
| Flow | `.flow` (5), `.flow-3`, `.flow-4` `> div > b + strong + span` | A short sequence or a set of parallel points |
| Timeline | `ol.timeline > li > span.n + .body` | Numbered steps with a paragraph or more each |
| Share steps | `.share-steps ol > li > b + strong + span` | Three big numbered steps |
| Schedule | `.schedule-wrap > table.schedule` | Tables; on phones each row is a card (`data-label` on each cell) |
| Spec | `.spec > dl` | Key/value facts of one item |
| Callout | `p.callout` with `strong.callout-label` | One note the reader must not miss |
| Prose | `.prose` | Running text, at most 46 em wide |
| Terminal | `.term > .bar + pre` | Code with a title bar |
| Pills, chips | `.pill`, `.chip` (a button with `aria-pressed`) | Tags, filters |
| Keyboard | `.kbd` | A key or a button name in Archicad or Grasshopper |
| Video | `a.video > .frame + .text` in `ul.videos` | Video links |

A component is used whole: no extra wrapper, no inline style, no page-specific restyling of a shared
class (a page that needs a variant adds a modifier class in `site.css`).

## 8. Numbering and labels

- A step number is the plain number: `1`, `2`, `3`, never `01`, `1.`, `Step 1` or a letter. The same in
  `.timeline .n`, `.flow b` and `.share-steps li b`.
- A step number is decorative (the list or the order says it) and has `aria-hidden="true"`.
- Number with `.flow b` only when the points are a sequence. Points that are not a sequence get a
  one-word label in sentence case (`Read`, `Free`), or no `b` at all.
- Ordered content is an `<ol>`; unordered points are a `<ul>` or a grid of cards.
- Rankings (the contributor list) use the CSS counter of their component, not typed numbers.
- Taglines (`.tagline`, `.way .who`) are one to four words in sentence case, naming the kind of thing
  ("Requests", "In progress", "Idea"), without a full stop.

## 9. Writing

- The site is written in plain, short English sentences, for architects first and developers second.
  Name what a reader sees in Archicad, with Archicad's own English terms.
- **Page titles** (`<title>` and `og:title`): `<What the page is> | Tapir`. The home page alone starts
  with `Tapir – `. Never ` - Tapir` or `– <tagline>`.
- **h1 and section h2**: a short phrase whose last words are in `<em>` and end with a full stop:
  `Get <em>started.</em>`, `How it <em>works.</em>`. One `<em>` per heading, at the end.
- **h3, step titles, card titles, button and link texts**: sentence case, no full stop.
- **Introductions** (`.page-head p`, `.sec-head p`): one or two sentences, ending with a full stop.
- **Dimension lines**: two to six words, sentence case, no full stop, saying what the section is
  ("Install and share", "Next step").
- Command and parameter names in `<code>` (or `.mono` inside a sentence that is a translation key and
  must stay readable), linked to the command reference (`../archicad-addon/#<CommandName>`) on first use.
- Numbers as digits; Archicad versions as "Archicad 29"; dates as "5 July 2022".
- British spelling is not required; be consistent within a page.

## 10. Links, buttons and calls to action

- `.btn-primary` is the one main action of a block; `.btn-ghost` (on a dark or subtle panel) and
  `.btn-line` (on the page) are the secondary ones. At most one primary button per block.
- `.btn-sm` in the header, in a next-step teaser and in rows of several buttons.
- A button's text starts with a verb: "Download", "Open the reference", "Ask on Discord".
- A teaser's `.go` text is "<Verb> the <thing>": "See the requests", "Read the guide", "Open the board".
- Links inside text are underlined (the browser's default with `text-underline-offset: 3px`); links
  that are a whole card or a list of navigation are not.
- External links open in the same tab; never `target="_blank"` without a reason.

## 11. Accessibility

- One `h1` per page; headings in order; every section labelled (`aria-labelledby` or `aria-label`).
- Decorative things (dimension lines, step numbers, icons) have `aria-hidden="true"`; an icon-only
  button has a `.visually-hidden` label.
- Focus is always visible: never remove the outline. A component on a dark or filled background sets
  `--focus` to a colour that passes 3:1 on it.
- Touch targets in the phone layout are at least 44 px high.
- Contrast as in section 3; never convey a state by colour alone (pressed chips also have
  `aria-pressed`, the coverage swatches have a text label).
- Images have `alt` (empty for decoration); videos have a text title and description.

## 12. Translation

- Every text a reader sees has `data-i18n` (its inner HTML is the key) or `data-i18n-attrs`, and is
  translated in all seven dictionaries under `docs/assets/i18n/<language>/` (format and rules in
  [`docs/assets/i18n/README.md`](../docs/assets/i18n/README.md)).
- Changing the HTML inside a `data-i18n` element changes its key: update the key, and the translation
  if its meaning or tags changed, in every language in the same pull request. A key no dictionary has
  stays English.
- A style change never needs a translation change: keep presentation out of the marked HTML (classes
  on the marked element itself are not part of the key; tags inside it are).

## 13. Motion

- Transitions are `.15s ease` on `transform` and `box-shadow` (the lift of a card or button), nothing
  longer except the splash and the reveal animations in `motion.css`.
- Every animation stops under the Display menu's Motion Reduced, and under the system's reduced motion
  unless Motion is Full: add a new animated selector to the reduced-motion blocks at the end of the
  stylesheet it is in.

## 14. Checklist for a change

Before a pull request that touches `docs/`:

- [ ] No `style=` attribute or `<style>` element added; no literal colour, font, radius or shadow in a
      rule that draws.
- [ ] Every new text size on the scale (section 4), every spacing on the 4 px scale (section 5).
- [ ] Every Legacy rule of a new component has its Standard counterpart, and dark and High contrast
      colours where it has its own.
- [ ] Looked at in Light, Dark and High contrast, Standard and Legacy, 640 px wide and 130 % text.
- [ ] The page skeleton blocks are identical to the other pages'.
- [ ] Headings, titles, numbers and labels written as sections 8 and 9 say.
- [ ] Every changed `data-i18n` key updated in all seven languages.
- [ ] `python tools/generate_llm_files.py` run and its output committed.

## 15. Known inconsistencies

The audit of the site against these rules (November 2026, #2765). Each item names the pull request part
of #2765 that fixes it; strike it from this list in that part.

**Inline styles**: fixed in part 2. The `style="…"` attributes of `ai/`, `grasshopper/`, `start/`,
`about/`, `tutorials/`, `404.html` and the home page are now the shared classes `.gap-top`,
`.gap-top-sm`, `.gap-top-xs`, `.col-head`, `.nowrap` and `.after-dim` (the home page's figures section
now gets the 32 px of section 5 instead of 28 px), and `<main>`'s children on `api-coverage/` and
`gh-coverage/` are indented like every other page's.

**Numbering and labels** (part 3):

- Step numbers are `1, 2, 3` in every `.timeline` and in `.share-steps`, but `01, 02, 03` in the `.flow`
  blocks of `ai/index.html` (twice) and `community/index.html`.
- Step numbers have `aria-hidden="true"` in `.timeline` and `.share-steps`, but not in `.flow`.
- `.flow` labels typed in capitals: `READ`, `CHANGE`, `CREATE`, `PROJECT` and `REFRESH`, `EXECUTE`,
  `CREATE` (`grasshopper/index.html`), `FREE`, `OPEN`, `SIGNED`, `WEEKLY` (`about/index.html`); in the
  Standard look, which shows labels as written, they are the only capitals on the site.

**Wording** (part 3):

- Page titles: most end with ` | Tapir`, but `roadmap/` ("Tapir roadmap – Archicad automation"),
  `community/` ("Community – Archicad automation with Tapir"), `about/` ("About Tapir – open-source
  Archicad add-on") and `404.html` ("Page not found - Tapir", with a hyphen) do not.
- The section heads of `api-coverage/` and `gh-coverage/` ("Overview", "Checklist") lack the
  `<em>…</em>.` ending every other section head has, including the command reference's ("All <em>commands.</em>").

**Page stylesheets that still draw the Legacy look in Standard** (part 4):

- `community/community.css`, the map ("On the map", added after the Standard look): `.map-card` keeps
  the 1.5 px ink border and square corners, the town dots, their halo and the country bars are
  `--ochre` (so they ignore the Accent), and `.map-label` has square corners. No Standard rule exists
  for any of them.
- `assets/coverage.css`, the phone filter sheet of the coverage pages: `.chip-filter`'s 1.5 px ink top
  line and `.sheet-done`'s pill shape and ink border have no Standard rule, while the Display menu's
  sheet, its twin, has 1 px lines and an 8 px Done button.
- `home.css`: the floor plan's Run button sets `color: #FFFFFF` in Standard, a literal instead of a
  property.

**Spacing and type off the scale** (part 5): fixed. Card and cell paddings, the phone gaps, the list
indents and every text size in `site.css`, `motion.css`, `coverage.css` and the page stylesheets are on
the scales of sections 4 and 5, and the command reference sizes its text in `rem`.
