# Translations of the site

The site is written in English; `assets/site.js` translates it in the browser into the language
picked in the header's language menu (or given as `?lang=<code>` in the address, or the browser's
language on a first visit). Languages: `de` Deutsch, `fr` Français, `es` Español, `it` Italiano,
`zh` 中文 (简体), `ja` 日本語, `hi` हिन्दी. The list is `LANGUAGES` in `assets/site.js`.

A language whose script the site's fonts (Fraunces, Inter, JetBrains Mono) lack names the web fonts
to load for it in its `fonts` entry there (Hindi: Noto Serif and Noto Sans Devanagari), and adds them
to the font stacks in `assets/site.css`, after the Latin fonts.

Hindi is standard Hindi in Devanagari, written for architects: common technical words stay in
English as Indian architects use them (Archicad, script, BIM, property, layer, API, JSON, ...).

## The dictionaries

One folder per language, each with:

- `common.js`: what every page shares (header, footer, the dark mode and copy buttons). These parts
  are translated without being marked (`SHARED_TEXT` and `SHARED_ATTRS` in `site.js`).
- one file per page or page group, loaded by the pages that name it in their `<html>` element:
  `<html ... data-i18n-pages="home">` loads `<language>/home.js` too. Only a page with
  `data-i18n-pages` gets the language in its `<html lang>`; on the others each translated element
  gets its own `lang`, as the rest of the page is still English.
- `data-i18n-partial` on the `<html>` element, next to `data-i18n-pages`, is for a page that stays
  mostly English: it loads the page's dictionaries but keeps `<html lang="en">`, and each translated
  element gets its own `lang`. The reference pages (`archicad-addon/`, `api-coverage/`,
  `gh-coverage/`) use it with `reference.js`: their heads, headings and introductions are
  translated, while the commands, schemas and checklists stay English.
- A script that fills in a part of a marked element (as `api-coverage/` fills `#archicad-range` in
  its introduction) does so in `TapirI18n.ready`: the translation looks the element up by its
  English HTML and replaces it.

Each file pushes one object of English → translation:

```js
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Get started': 'Loslegen',
    'Every command, <em>documented.</em>': 'Jeder Befehl, <em>dokumentiert.</em>'
});
```

## Marking a page

- `data-i18n` on an element translates its inner HTML. The key is that HTML as the browser reads
  it back (whitespace collapsed to single spaces, entities such as `&rarr;` as their characters,
  attributes in double quotes); the translation keeps the same tags and attributes. Do not nest
  `data-i18n` elements.
- `data-i18n-attrs="alt title aria-label"` translates those attributes by their English value.
- A script that writes text uses `TapirI18n.t ('English text')`, one that adds marked HTML calls
  `TapirI18n.apply (element)`, and one that runs before the translation is applied waits with
  `TapirI18n.ready (callback)`.
- The translation of the page does not reach into a `<template>`: a script that copies one marks its
  texts as usual and translates them first with `TapirI18n.apply (template.content)`, as
  `scripts/scripts.js` does.

A text a dictionary lacks stays English, so a page can be translated piece by piece. Command names,
JSON keys, code and Archicad's own terms stay as Archicad shows them in English.
