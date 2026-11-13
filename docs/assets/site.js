/* The shared behaviour of every page of Tapir's site: the language menu and the translations, the
   Display menu, the site search, the header's menu on phones, a copy button on code blocks, and the live figures. The pages are complete
   without it; it only adds to them. The texts it shows are in the page's HTML (the copy button's
   labels are data-copy and data-copied on the <html> element), or, for the menus it builds (the
   phone menu's button, the Display panel and the search panel), English here, translated by common.js like the rest.

   Display: the visitor's look (Standard or Legacy), mode (System, Light, Dark or High contrast),
   accent (the look's own, Blue, Purple or Orange), text size (100, 115, 130 or 90 %), font (the
   look's own, System or Readable), spacing (Comfortable or Compact) and motion (System, Reduced or
   Full), kept in localStorage under tapir-display as an object of the settings that differ from the
   default, e.g. {"style":"legacy","theme":"dark","size":"115"}, and applied as data-style,
   data-theme, data-accent, data-size, data-font, data-spacing and data-motion on <html> (no attribute
   for a default). High contrast is one look, the same whichever Look and Accent are chosen: with it,
   neither data-style nor data-accent is set (the stored choices stay, for the other modes), and the
   panel's Accent is disabled; the other settings apply in it too. motion.js and home.js read
   data-motion, and follow a change of it (tapir-display-change). The head's inline script applies
   them before the first paint, moves an old tapir-theme choice into tapir-display, sets the
   theme-color metas and loads Fraunces for Legacy's own font only and Atkinson Hyperlegible for
   Readable only; this script does the same when the choice changes. Keep the two in step
   (DISPLAY_SETTINGS, THEME_COLORS, LEGACY_FONTS, READABLE_FONTS). A page that draws with colours
   listens for tapir-display-change on document and draws again.

   Translations: the English stays in the HTML. In another language, the inner HTML of every element
   marked data-i18n, and the attributes an element names in data-i18n-attrs="<attribute> ...", are
   looked up by their English text (whitespace collapsed) in that language's dictionaries, in
   assets/i18n/<language>/: common.js for what every page shares, plus one file per name in the
   <html> element's data-i18n-pages="<name> ...", which also sets the page's <html lang> unless the
   element has data-i18n-partial (a page that stays mostly English). What a dictionary lacks stays English. The parts
   every page shares (SHARED_TEXT, SHARED_ATTRS) are translated without being marked. Do not nest
   data-i18n elements. The language is ?lang=<code> in the address, else the one chosen in the menu,
   else the browser's, else English; the head's inline script hides the page (data-i18n-loading)
   until the translation is applied, at most two seconds. Page scripts use window.TapirI18n:
   language, t (english) for a text they write, apply (element) for HTML they add, and
   ready (callback), which runs once the page is translated.

   Live figures: an element with data-live="<name> [<name> ...]" is hidden in the HTML and only
   shown once every value it names was read; each [data-live-value="<name>"] inside it gets the value.
   A figure that cannot be read stays hidden, so the site never shows a wrong number. None of them
   uses GitHub's API:
       commands         the number of Tapir commands, from archicad-addon/command_definitions.js
       archicad-count   the number of supported Archicad versions  } from archicad-addon/devkits.json
       archicad-range   e.g. "25–30"                               } on main
       coverage         the Archicad API coverage, e.g. "99%", from the coverage workflow's badge
       release          the latest release, e.g. "1.8.0", from shields.io's release badge
       scripts          the number of scripts in the marketplace catalogue (site-data branch)
   The values read are kept for an hour in sessionStorage, so moving between pages reads them once. */
(function () {
    'use strict';

    var DISPLAY_KEY = 'tapir-display';
    var OLD_THEME_KEY = 'tapir-theme';
    // Each setting: its key in tapir-display, the <html> attribute, the panel's legend, and its options
    // (value, label, and the label in Legacy when it differs), the default first, which sets no
    // attribute. inContrast: false for the settings High contrast sets no attribute for.
    var DISPLAY_SETTINGS = [
        { key: 'style', attribute: 'data-style', legend: 'Look', inContrast: false, options: [['standard', 'Standard'], ['legacy', 'Legacy']] },
        { key: 'theme', attribute: 'data-theme', legend: 'Mode', inContrast: true, options: [['system', 'System'], ['light', 'Light'], ['dark', 'Dark'], ['contrast', 'High contrast']] },
        { key: 'accent', attribute: 'data-accent', legend: 'Accent', inContrast: false, options: [['look', 'Green', 'Ochre'], ['blue', 'Blue'], ['purple', 'Purple'], ['orange', 'Orange']] },
        { key: 'size', attribute: 'data-size', legend: 'Text size', inContrast: true, options: [['100', '100%'], ['115', '115%'], ['130', '130%'], ['90', '90%']] },
        { key: 'font', attribute: 'data-font', legend: 'Font', inContrast: true, options: [['look', 'Inter', 'Fraunces & Inter'], ['system', 'System'], ['readable', 'Readable']] },
        { key: 'spacing', attribute: 'data-spacing', legend: 'Spacing', inContrast: true, options: [['comfortable', 'Comfortable'], ['compact', 'Compact']] },
        { key: 'motion', attribute: 'data-motion', legend: 'Motion', inContrast: true, options: [['system', 'System'], ['reduced', 'Reduced'], ['full', 'Full']] }
    ];
    // The browser bar colour (the header's): [under prefers-color-scheme: dark, otherwise].
    var THEME_COLORS = { legacy: ['#2D3A30', '#2D3A30'], contrast: ['#FFFFFF', '#FFFFFF'], standard: { system: ['#11161D', '#FFFFFF'], light: ['#FFFFFF', '#FFFFFF'], dark: ['#11161D', '#11161D'] } };
    var LEGACY_FONTS = 'https://fonts.googleapis.com/css2?family=Fraunces:ital,opsz,wght@0,9..144,500;0,9..144,700;0,9..144,900;1,9..144,500;1,9..144,700&display=swap';
    var READABLE_FONTS = 'https://fonts.googleapis.com/css2?family=Atkinson+Hyperlegible:ital,wght@0,400;0,700;1,400;1,700&display=swap';
    var CACHE_KEY = 'tapir-live-v1';
    var CACHE_MS = 60 * 60 * 1000;
    var REPO = 'ENZYME-APD/tapir-archicad-automation';
    var RAW_BASE = 'https://raw.githubusercontent.com/' + REPO + '/';
    var SLUG_PATTERN = /^[a-z0-9]+(-[a-z0-9]+)*$/;
    var root = document.documentElement;
    var darkQuery = window.matchMedia ? window.matchMedia ('(prefers-color-scheme: dark)') : null;

    // The site's root, from this script's own address (…/assets/site.js), so it works at any depth
    // and under any path the site is served from.
    var siteRoot = (function () {
        var script = document.currentScript;
        var src = script && script.src ? script.src : '';
        return src ? src.replace (/assets\/site\.js(?:[?#].*)?$/, '') : null;
    }) ();

    // ---- Languages ----

    var LANG_KEY = 'tapir-lang';
    // code: the ?lang= value and the folder under assets/i18n/; tag: the <html lang> it sets.
    var LANGUAGES = [
        { code: 'en', tag: 'en', name: 'English' },
        { code: 'de', tag: 'de', name: 'Deutsch' },
        { code: 'fr', tag: 'fr', name: 'Français' },
        { code: 'es', tag: 'es', name: 'Español' },
        { code: 'it', tag: 'it', name: 'Italiano' },
        { code: 'zh', tag: 'zh-Hans', name: '中文 (简体)' },
        { code: 'ja', tag: 'ja', name: '日本語' },
        // Fraunces and Inter have no Devanagari: the font stacks in site.css fall back to these, loaded
        // only for Hindi (fonts is a Google Fonts css2 query).
        { code: 'hi', tag: 'hi', name: 'हिन्दी', fonts: 'family=Noto+Serif+Devanagari:wght@500;700;900&family=Noto+Sans+Devanagari:wght@400;500;600;700&display=swap' }
    ];
    // The header and footer every page shares, translated as if they were marked data-i18n.
    var SHARED_TEXT = '.skip-link, .site-nav a, .site-header .btn, .site-header .visually-hidden, ' +
        '.site-footer .lab, .site-footer .note, .site-footer .fine, .site-footer li a';
    var SHARED_ATTRS = [
        ['html', 'data-copy data-copied'],
        ['.site-nav', 'aria-label']
    ];
    var NAME_PATTERN = /^[a-z0-9]+(-[a-z0-9]+)*$/;
    var dictionary = {};
    var readyCallbacks = [];
    var translationReady = false;

    function findLanguage (code) {
        code = typeof code === 'string' ? code.toLowerCase () : '';
        for (var i = 0; i < LANGUAGES.length; i++) {
            if (LANGUAGES[i].code === code) {
                return LANGUAGES[i];
            }
        }
        return null;
    }

    function storedLanguage () {
        try {
            return findLanguage (window.localStorage.getItem (LANG_KEY));
        } catch (e) {
            return null;
        }
    }

    function storeLanguage (code) {
        try {
            window.localStorage.setItem (LANG_KEY, code);
        } catch (e) {}
    }

    function addressLanguage () {
        var match = /[?&]lang=([A-Za-z]{2})(?:[&#]|$)/.exec (window.location.search);
        return match ? findLanguage (match[1]) : null;
    }

    function browserLanguage () {
        var wanted = window.navigator.languages && window.navigator.languages.length ? window.navigator.languages : [window.navigator.language];
        for (var i = 0; i < wanted.length; i++) {
            var language = findLanguage (String (wanted[i] || '').split ('-')[0]);
            if (language) {
                return language;
            }
        }
        return null;
    }

    function currentLanguage () {
        var fromAddress = addressLanguage ();
        if (fromAddress) {
            // A link with ?lang= is a choice, kept for the next pages.
            storeLanguage (fromAddress.code);
            return fromAddress;
        }
        return storedLanguage () || browserLanguage () || LANGUAGES[0];
    }

    function switchLanguage (code) {
        storeLanguage (code);
        if (/[?&]lang=/.test (window.location.search) && window.URLSearchParams) {
            var params = new URLSearchParams (window.location.search);
            params.set ('lang', code);
            window.location.search = params.toString ();
        } else {
            window.location.reload ();
        }
    }

    // Loads the fonts a language's script needs and the site's own fonts lack.
    function loadLanguageFonts (language) {
        if (!language.fonts) {
            return;
        }
        var link = document.createElement ('link');
        link.rel = 'stylesheet';
        link.href = 'https://fonts.googleapis.com/css2?' + language.fonts;
        document.head.appendChild (link);
    }

    // Loads common.js and the page's own dictionaries, then calls done, also when some are missing.
    function loadTranslations (language, done) {
        var finished = false;
        function finish () {
            if (!finished) {
                finished = true;
                done ();
            }
        }
        if (language.code === 'en' || !siteRoot) {
            finish ();
            return;
        }
        loadLanguageFonts (language);
        var names = ['common'].concat ((root.getAttribute ('data-i18n-pages') || '').split (/\s+/).filter (function (name) {
            return NAME_PATTERN.test (name) && name !== 'common';
        }));
        var pending = names.length;
        names.forEach (function (name) {
            var script = document.createElement ('script');
            script.src = siteRoot + 'assets/i18n/' + language.code + '/' + name + '.js';
            script.onload = script.onerror = function () {
                pending--;
                if (pending === 0) {
                    finish ();
                }
            };
            document.head.appendChild (script);
        });
        setTimeout (finish, 3000);
    }

    // The dictionaries push their objects into window.TAPIR_I18N.
    function readDictionaries () {
        var loaded = Array.isArray (window.TAPIR_I18N) ? window.TAPIR_I18N : [];
        loaded.forEach (function (entries) {
            if (!entries || typeof entries !== 'object') {
                return;
            }
            Object.keys (entries).forEach (function (key) {
                if (typeof entries[key] === 'string') {
                    dictionary[key.replace (/\s+/g, ' ').trim ()] = entries[key];
                }
            });
        });
    }

    // The translation of an English text, keeping a leading and a trailing space; null if none.
    function translate (text) {
        var key = String (text).replace (/\s+/g, ' ').trim ();
        if (!key || !Object.prototype.hasOwnProperty.call (dictionary, key)) {
            return null;
        }
        return (/^\s/.test (text) ? ' ' : '') + dictionary[key] + (/\s$/.test (text) ? ' ' : '');
    }

    function translateAttributes (element, names) {
        element.tapirTranslatedAttrs = element.tapirTranslatedAttrs || {};
        names.split (/\s+/).forEach (function (name) {
            if (!name || element.tapirTranslatedAttrs[name] || !element.hasAttribute (name)) {
                return;
            }
            var translated = translate (element.getAttribute (name));
            if (translated !== null) {
                element.setAttribute (name, translated);
            }
            element.tapirTranslatedAttrs[name] = true;
        });
    }

    function matchingElements (scope, selector) {
        var found = Array.prototype.slice.call (scope.querySelectorAll (selector));
        if (scope.nodeType === 1 && scope.matches && scope.matches (selector)) {
            found.unshift (scope);
        }
        return found;
    }

    // Translates the marked elements in scope (an element, or the whole document); each only once.
    function applyTranslations (scope) {
        if (!scope || !Object.keys (dictionary).length) {
            return;
        }
        var texts = matchingElements (scope, '[data-i18n]');
        if (scope === document) {
            texts = texts.concat (matchingElements (document, SHARED_TEXT));
        }
        texts.forEach (function (element) {
            if (element.tapirTranslated) {
                return;
            }
            element.tapirTranslated = true;
            var translated = translate (element.innerHTML);
            if (translated !== null) {
                element.innerHTML = translated;
                // For screen readers on a page whose own text is still English (<html lang> stays en).
                element.setAttribute ('lang', language.tag);
            }
        });
        matchingElements (scope, '[data-i18n-attrs]').forEach (function (element) {
            translateAttributes (element, element.getAttribute ('data-i18n-attrs'));
        });
        if (scope === document) {
            SHARED_ATTRS.forEach (function (shared) {
                matchingElements (document, shared[0]).forEach (function (element) {
                    translateAttributes (element, shared[1]);
                });
            });
        }
    }

    function setUpLanguageMenu (current) {
        var toggle = document.getElementById ('display-button');
        var place = toggle ? toggle.parentNode : document.querySelector ('.site-header .right');
        if (!place) {
            return;
        }
        var select = document.createElement ('select');
        select.id = 'lang-select';
        select.className = 'toggle lang-select';
        select.setAttribute ('aria-label', translate ('Language') || 'Language');
        LANGUAGES.forEach (function (language) {
            var option = document.createElement ('option');
            option.value = language.code;
            option.lang = language.tag;
            option.textContent = language.name;
            option.selected = language.code === current.code;
            select.appendChild (option);
        });
        select.addEventListener ('change', function () {
            switchLanguage (select.value);
        });
        place.insertBefore (select, toggle || place.firstChild);
    }

    var language = currentLanguage ();

    window.TapirI18n = {
        language: language.code,
        t: function (text) {
            var translated = translate (text);
            return translated === null ? text : translated;
        },
        apply: function (element) {
            applyTranslations (element);
        },
        ready: function (callback) {
            if (translationReady) {
                callback ();
            } else {
                readyCallbacks.push (callback);
            }
        }
    };

    // ---- Display: look and mode ----

    function isOption (setting, value) {
        return setting.options.some (function (option) { return option[0] === value; });
    }

    // Only the known settings with a known, non-default value.
    function cleanDisplay (settings) {
        var clean = {};
        DISPLAY_SETTINGS.forEach (function (setting) {
            var value = settings ? settings[setting.key] : null;
            if (typeof value === 'string' && value !== setting.options[0][0] && isOption (setting, value)) {
                clean[setting.key] = value;
            }
        });
        return clean;
    }

    // The stored settings; moves an old tapir-theme choice into them (the head's script normally did).
    function storedDisplay () {
        var settings = {};
        try {
            settings = JSON.parse (window.localStorage.getItem (DISPLAY_KEY) || '{}') || {};
        } catch (e) {
            settings = {};
        }
        try {
            var oldTheme = window.localStorage.getItem (OLD_THEME_KEY);
            if (oldTheme !== null) {
                if ((oldTheme === 'light' || oldTheme === 'dark') && !settings.theme) {
                    settings.theme = oldTheme;
                }
                window.localStorage.setItem (DISPLAY_KEY, JSON.stringify (cleanDisplay (settings)));
                window.localStorage.removeItem (OLD_THEME_KEY);
            }
        } catch (e) {}
        return cleanDisplay (settings);
    }

    function storeDisplay (settings) {
        try {
            if (Object.keys (settings).length) {
                window.localStorage.setItem (DISPLAY_KEY, JSON.stringify (settings));
            } else {
                window.localStorage.removeItem (DISPLAY_KEY);
            }
        } catch (e) {}
    }

    function loadFonts (id, href) {
        if (document.getElementById (id)) {
            return;
        }
        var link = document.createElement ('link');
        link.rel = 'stylesheet';
        link.id = id;
        link.href = href;
        document.head.appendChild (link);
    }

    function applyDisplay (settings) {
        var contrast = settings.theme === 'contrast';
        DISPLAY_SETTINGS.forEach (function (setting) {
            if (settings[setting.key] && (setting.inContrast || !contrast)) {
                root.setAttribute (setting.attribute, settings[setting.key]);
            } else {
                root.removeAttribute (setting.attribute);
            }
        });
        var legacy = settings.style === 'legacy' && !contrast;
        var colors = contrast ? THEME_COLORS.contrast : legacy ? THEME_COLORS.legacy : THEME_COLORS.standard[settings.theme || 'system'];
        var metas = document.querySelectorAll ('meta[name="theme-color"]');
        for (var i = 0; i < metas.length && i < colors.length; i++) {
            metas[i].setAttribute ('content', colors[i]);
        }
        // Fraunces is only for Legacy's own font, Atkinson Hyperlegible only for Readable; once loaded, a
        // font stays, unused, if the visitor chooses something else.
        if (legacy && !settings.font) {
            loadFonts ('font-legacy', LEGACY_FONTS);
        }
        if (settings.font === 'readable') {
            loadFonts ('font-readable', READABLE_FONTS);
        }
        var event;
        try {
            event = new CustomEvent ('tapir-display-change', { detail: settings });
        } catch (e) {
            event = document.createEvent ('CustomEvent');
            event.initCustomEvent ('tapir-display-change', false, false, settings);
        }
        document.dispatchEvent (event);
    }

    // The Display button opens a dialog with one fieldset of radio buttons per setting; a change applies
    // at once. A popover under the button on wide screens, a sheet from the bottom where the header has its
    // menu (CSS).
    function setUpDisplayMenu () {
        var button = document.getElementById ('display-button');
        // The head's inline script already applied the stored choice; this repeats it in case it could not run.
        var settings = storedDisplay ();
        applyDisplay (settings);
        if (darkQuery) {
            // In System mode a page that draws with colours draws again when the system's mode changes.
            var onSystemChange = function () {
                if (!settings.theme) {
                    applyDisplay (settings);
                }
            };
            if (darkQuery.addEventListener) {
                darkQuery.addEventListener ('change', onSystemChange);
            } else if (darkQuery.addListener) {
                darkQuery.addListener (onSystemChange);
            }
        }
        if (!button) {
            return;
        }
        function text (english) {
            return translate (english) || english;
        }

        var panel = document.createElement ('div');
        panel.className = 'display-panel';
        panel.id = 'display-panel';
        panel.hidden = true;
        panel.setAttribute ('role', 'dialog');
        panel.setAttribute ('aria-labelledby', 'display-title');
        var head = document.createElement ('div');
        head.className = 'display-head';
        var title = document.createElement ('p');
        title.className = 'display-title';
        title.id = 'display-title';
        title.textContent = text ('Display');
        var done = document.createElement ('button');
        done.type = 'button';
        done.className = 'display-done';
        done.textContent = text ('Done');
        head.appendChild (title);
        head.appendChild (done);
        panel.appendChild (head);

        var inputs = [];
        // The labels that differ in Legacy (the accent's own: Green or Ochre; the font's own: Inter or
        // Fraunces & Inter): [text node, label, Legacy's].
        var lookLabels = [];
        // The fieldsets of the settings High contrast leaves out of the page, disabled while it is chosen.
        var notInContrast = [];
        DISPLAY_SETTINGS.forEach (function (setting) {
            var fieldset = document.createElement ('fieldset');
            var legend = document.createElement ('legend');
            legend.textContent = text (setting.legend);
            fieldset.appendChild (legend);
            if (setting.key === 'accent') {
                notInContrast.push (fieldset);
            }
            var options = document.createElement ('div');
            options.className = 'display-options';
            setting.options.forEach (function (option) {
                var label = document.createElement ('label');
                var input = document.createElement ('input');
                input.type = 'radio';
                input.name = 'display-' + setting.key;
                input.value = option[0];
                input.tapirSetting = setting;
                label.appendChild (input);
                var labelText = document.createTextNode (text (option[1]));
                label.appendChild (labelText);
                if (option[2]) {
                    lookLabels.push ([labelText, text (option[1]), text (option[2])]);
                }
                options.appendChild (label);
                inputs.push (input);
                input.addEventListener ('change', function () {
                    if (!input.checked) {
                        return;
                    }
                    if (option[0] === setting.options[0][0]) {
                        delete settings[setting.key];
                    } else {
                        settings[setting.key] = option[0];
                    }
                    applyDisplay (settings);
                    storeDisplay (settings);
                    update ();
                });
            });
            fieldset.appendChild (options);
            panel.appendChild (fieldset);
        });

        var reset = document.createElement ('button');
        reset.type = 'button';
        reset.className = 'display-reset';
        reset.textContent = text ('Reset to defaults');
        panel.appendChild (reset);
        document.body.appendChild (panel);

        // Checks the radio button of each setting's current value (a label shows it checked, also in
        // browsers without :has), names the accent's own after the chosen look, and disables Accent
        // in High contrast, where it does not apply.
        function update () {
            inputs.forEach (function (input) {
                var current = settings[input.tapirSetting.key] || input.tapirSetting.options[0][0];
                input.checked = input.value === current;
                input.parentNode.classList.toggle ('checked', input.checked);
            });
            // High contrast draws Standard's fonts whatever the Look, so the font's own is Inter there.
            var legacyShown = settings.style === 'legacy' && settings.theme !== 'contrast';
            lookLabels.forEach (function (entry) {
                entry[0].nodeValue = legacyShown ? entry[2] : entry[1];
            });
            notInContrast.forEach (function (fieldset) {
                fieldset.disabled = settings.theme === 'contrast';
            });
        }

        function isOpen () {
            return !panel.hidden;
        }

        function place () {
            var rect = button.getBoundingClientRect ();
            panel.style.top = Math.max (8, rect.bottom + 8) + 'px';
            panel.style.right = Math.max (16, document.documentElement.clientWidth - rect.right) + 'px';
        }

        function open () {
            update ();
            place ();
            panel.hidden = false;
            button.setAttribute ('aria-expanded', 'true');
            var checked = panel.querySelector ('input:checked') || panel.querySelector ('input');
            if (checked) {
                checked.focus ();
            }
        }

        // Back to the Display button, or to the phone menu's button when the menu holding it is closed.
        function close (focusButton) {
            if (!isOpen ()) {
                return;
            }
            panel.hidden = true;
            button.setAttribute ('aria-expanded', 'false');
            if (focusButton) {
                var menuButton = document.querySelector ('.site-header .menu-button');
                (button.offsetParent === null && menuButton ? menuButton : button).focus ();
            }
        }

        button.setAttribute ('aria-haspopup', 'dialog');
        button.setAttribute ('aria-expanded', 'false');
        button.setAttribute ('aria-controls', panel.id);
        button.addEventListener ('click', function () {
            if (isOpen ()) {
                close (true);
            } else {
                open ();
            }
        });
        done.addEventListener ('click', function () {
            close (true);
        });
        reset.addEventListener ('click', function () {
            settings = {};
            applyDisplay (settings);
            storeDisplay (settings);
            update ();
        });
        document.addEventListener ('keydown', function (event) {
            if ((event.key === 'Escape' || event.key === 'Esc') && isOpen ()) {
                close (true);
            }
        });
        // A tap on text sends no click on iOS, so outside the panel it listens to pointerdown.
        document.addEventListener (window.PointerEvent ? 'pointerdown' : 'click', function (event) {
            if (isOpen () && !panel.contains (event.target) && !button.contains (event.target)) {
                close (true);
            }
        });
        // Tabbing out of the panel closes it too, leaving the focus where it went.
        panel.addEventListener ('focusout', function (event) {
            var next = event.relatedTarget;
            if (next && !panel.contains (next) && next !== button) {
                close (false);
            }
        });
        window.addEventListener ('resize', function () {
            if (isOpen ()) {
                place ();
            }
        });
        update ();
        button.hidden = false;
    }

    // ---- Search (#2774) ----

    // A search button at the start of the header's .right (on phones, next to the menu button) opens a
    // panel that searches the whole site: every page and its headings, every command and every built-in
    // script. The index is the site's own llms-full.txt (written by tools/generate_llm_files.py, which
    // the weekly release runs), read the first time the panel opens, so nothing has to be kept in step
    // by hand. A heading's result links to its page with a text fragment (#:~:text=), which scrolls to
    // it in the browsers that have them and is ignored by the others. "/" or Ctrl+K (Cmd+K) opens it.
    var SEARCH_SITE = 'https://archicad-api.com/';
    var SEARCH_MAX_RESULTS = 40;

    function searchPlainText (markdown) {
        return markdown
            .replace (/!?\[([^\]]*)\]\([^)]*\)/g, '$1')
            .replace (/[*`]+/g, '')
            .replace (/\s+/g, ' ')
            .trim ();
    }

    // The entries of llms-full.txt: { title, kind, context, url, text }, kind 'Page', 'Section',
    // 'Command' or 'Built-in script' (shown, translated); context is the page or the command group.
    function parseSearchIndex (source, base) {
        var entries = [];
        var lines = source.split (/\r?\n/);
        var mode = null;
        var inFence = false;
        var page = null;
        var section = null;
        var group = '';
        function localUrl (url) {
            return url.indexOf (SEARCH_SITE) === 0 ? base + url.substring (SEARCH_SITE.length) : url;
        }
        function fragment (text) {
            return '#:~:text=' + encodeURIComponent (text).replace (/-/g, '%2D');
        }
        lines.forEach (function (line) {
            if (/^```/.test (line)) {
                inFence = !inFence;
                return;
            }
            if (inFence) {
                return;
            }
            var match;
            if (line === '---') {
                mode = null;
                page = null;
                section = null;
                return;
            }
            if ((match = /^Source: (\S+)\s*$/.exec (line))) {
                mode = 'page';
                page = { url: localUrl (match[1]), title: null };
                section = null;
                return;
            }
            if (/^# Command reference/.test (line)) {
                mode = 'commands';
                return;
            }
            if (mode === null && /^## Scripts in the Tapir palette/.test (line)) {
                mode = 'scripts';
                return;
            }
            if (mode === 'page') {
                if ((match = /^#{2,4} (.+)$/.exec (line))) {
                    var title = searchPlainText (match[1]);
                    if (!title) {
                        return;
                    }
                    if (page.title === null) {
                        page.title = title;
                        section = { title: title, kind: 'Page', context: '', url: page.url, text: section ? section.text : '' };
                    } else {
                        section = { title: title, kind: 'Section', context: page.title, url: page.url + fragment (title), text: '' };
                    }
                    entries.push (section);
                } else if (line.trim ()) {
                    if (!section) {
                        // The text above the page's first heading (e.g. its tagline) belongs to the page.
                        section = { text: '' };
                    }
                    section.text += (section.text ? ' ' : '') + searchPlainText (line);
                }
            } else if (mode === 'commands') {
                // The first group's heading ends the paragraph above it, on the same line.
                if ((match = /(?:^|\s)## (.+)$/.exec (line))) {
                    group = searchPlainText (match[1]);
                } else if ((match = /^- \*\*(\w+)\*\* \(since [^)]*\): (.*)$/.exec (line))) {
                    entries.push ({ title: match[1], kind: 'Command', context: group, url: base + 'archicad-addon/#' + match[1], text: searchPlainText (match[2]) });
                }
            } else if (mode === 'scripts') {
                if ((match = /^- \*\*(.+?)\*\*: (.*)$/.exec (line))) {
                    var link = /\(\[source\]\(([^)]+)\)\)\s*$/.exec (match[2]);
                    entries.push ({
                        title: searchPlainText (match[1]),
                        kind: 'Built-in script',
                        context: '',
                        url: link ? link[1] : base + 'scripts/',
                        text: searchPlainText (link ? match[2].substring (0, link.index) : match[2])
                    });
                }
            }
        });
        entries.forEach (function (entry) {
            entry.titleLower = entry.title.toLowerCase ();
            entry.contextLower = entry.context.toLowerCase ();
            entry.textLower = entry.text.toLowerCase ();
        });
        return entries;
    }

    // The entries that have every word of the query, best first: a title that is the query, then one
    // that starts with it, then by where the words were found (title, page or group, text).
    function searchEntries (entries, query) {
        var phrase = query.toLowerCase ().replace (/\s+/g, ' ').trim ();
        var words = phrase.split (' ').filter (function (word) {
            return word;
        });
        if (!words.length) {
            return [];
        }
        var squashed = phrase.replace (/ /g, '');
        var found = [];
        entries.forEach (function (entry, index) {
            var score = 0;
            for (var i = 0; i < words.length; i++) {
                var word = words[i];
                if (entry.titleLower.indexOf (word) !== -1) {
                    score += 10;
                } else if (entry.contextLower.indexOf (word) !== -1) {
                    score += 3;
                } else if (entry.textLower.indexOf (word) !== -1) {
                    score += 1;
                } else {
                    return;
                }
            }
            if (entry.titleLower === phrase || entry.titleLower === squashed) {
                score += 100;
            } else if (entry.titleLower.indexOf (phrase) === 0 || entry.titleLower.indexOf (squashed) === 0) {
                score += 40;
            } else if (entry.titleLower.indexOf (phrase) !== -1 || entry.titleLower.indexOf (squashed) !== -1) {
                score += 20;
            }
            if (entry.kind === 'Page') {
                score += 2;
            }
            found.push ({ entry: entry, score: score, index: index });
        });
        found.sort (function (a, b) {
            return b.score - a.score || a.index - b.index;
        });
        return found;
    }

    // Appends text to an element with each word of the query in a <mark>, as text nodes only.
    function appendHighlighted (element, text, words) {
        var lower = text.toLowerCase ();
        var position = 0;
        while (position < text.length) {
            var next = -1;
            var length = 0;
            words.forEach (function (word) {
                var at = lower.indexOf (word, position);
                if (at !== -1 && (next === -1 || at < next || (at === next && word.length > length))) {
                    next = at;
                    length = word.length;
                }
            });
            if (next === -1) {
                element.appendChild (document.createTextNode (text.substring (position)));
                return;
            }
            if (next > position) {
                element.appendChild (document.createTextNode (text.substring (position, next)));
            }
            var mark = document.createElement ('mark');
            mark.textContent = text.substr (next, length);
            element.appendChild (mark);
            position = next + length;
        }
    }

    // About 160 characters of the text around the first word of the query found in it.
    function searchSnippet (entry, words) {
        var text = entry.text;
        if (text.length <= 160) {
            return text;
        }
        var at = -1;
        words.forEach (function (word) {
            var found = entry.textLower.indexOf (word);
            if (found !== -1 && (at === -1 || found < at)) {
                at = found;
            }
        });
        var start = at > 60 ? text.lastIndexOf (' ', at - 50) + 1 : 0;
        var end = text.indexOf (' ', start + 160);
        return (start > 0 ? '… ' : '') + text.substring (start, end === -1 ? text.length : end) + (end === -1 ? '' : ' …');
    }

    function setUpSearch () {
        var right = document.querySelector ('.site-header .right');
        if (!right || !siteRoot || !window.fetch) {
            return;
        }
        function text (english) {
            return translate (english) || english;
        }

        var button = document.createElement ('button');
        button.type = 'button';
        button.id = 'search-button';
        button.className = 'toggle search-button';
        button.innerHTML = '<svg viewBox="0 0 16 16" width="16" height="16" aria-hidden="true" focusable="false"><g fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round"><circle cx="7" cy="7" r="4.75"/><path d="M10.5 10.5 14 14"/></g></svg>';
        var buttonLabel = document.createElement ('span');
        buttonLabel.className = 'visually-hidden';
        buttonLabel.textContent = text ('Search');
        button.appendChild (buttonLabel);
        right.insertBefore (button, right.firstChild);

        var panel = document.createElement ('div');
        panel.className = 'search-panel';
        panel.id = 'search-panel';
        panel.hidden = true;
        panel.setAttribute ('role', 'dialog');
        panel.setAttribute ('aria-label', text ('Search the site'));
        var form = document.createElement ('form');
        form.className = 'search-form';
        form.setAttribute ('role', 'search');
        var input = document.createElement ('input');
        input.type = 'search';
        input.className = 'search-input';
        input.setAttribute ('aria-label', text ('Search the site'));
        input.placeholder = text ('Search pages, commands and scripts');
        input.autocomplete = 'off';
        input.spellcheck = false;
        input.setAttribute ('aria-controls', 'search-results');
        var done = document.createElement ('button');
        done.type = 'button';
        done.className = 'display-done search-done';
        done.textContent = text ('Done');
        form.appendChild (input);
        form.appendChild (done);
        panel.appendChild (form);
        var status = document.createElement ('p');
        status.className = 'search-status';
        status.setAttribute ('role', 'status');
        panel.appendChild (status);
        var list = document.createElement ('ul');
        list.className = 'search-results';
        list.id = 'search-results';
        panel.appendChild (list);
        document.body.appendChild (panel);

        var entries = null;
        var loading = false;
        var failed = false;

        function load () {
            if (entries || loading) {
                return;
            }
            loading = true;
            failed = false;
            render ();
            window.fetch (siteRoot + 'llms-full.txt').then (function (response) {
                if (!response.ok) {
                    throw new Error (String (response.status));
                }
                return response.text ();
            }).then (function (source) {
                entries = parseSearchIndex (source, siteRoot);
                loading = false;
                render ();
            }, function () {
                loading = false;
                failed = true;
                render ();
            });
        }

        function render () {
            list.textContent = '';
            var query = input.value;
            if (!query.trim ()) {
                status.textContent = '';
                return;
            }
            if (failed) {
                status.textContent = text ('The search could not be loaded. Check your connection and try again.');
                return;
            }
            if (!entries) {
                status.textContent = text ('Loading…');
                return;
            }
            var found = searchEntries (entries, query);
            var words = query.toLowerCase ().split (/\s+/).filter (function (word) {
                return word;
            });
            if (!found.length) {
                status.textContent = text ('Nothing found.');
                return;
            }
            status.textContent = found.length === 1 ? text ('1 result') : found.length + ' ' + text ('results');
            found.slice (0, SEARCH_MAX_RESULTS).forEach (function (result) {
                var entry = result.entry;
                var item = document.createElement ('li');
                var link = document.createElement ('a');
                link.href = entry.url;
                var title = document.createElement ('span');
                title.className = 'search-title';
                appendHighlighted (title, entry.title, words);
                link.appendChild (title);
                var meta = document.createElement ('span');
                meta.className = 'search-meta';
                meta.textContent = text (entry.kind) + (entry.context ? ' · ' + entry.context : '');
                link.appendChild (meta);
                var snippet = searchSnippet (entry, words);
                if (snippet) {
                    var body = document.createElement ('span');
                    body.className = 'search-snippet';
                    appendHighlighted (body, snippet, words);
                    link.appendChild (body);
                }
                item.appendChild (link);
                list.appendChild (item);
            });
        }

        function isOpen () {
            return !panel.hidden;
        }

        function open () {
            panel.hidden = false;
            button.setAttribute ('aria-expanded', 'true');
            load ();
            input.focus ();
            input.select ();
        }

        function close (focusButton) {
            if (!isOpen ()) {
                return;
            }
            panel.hidden = true;
            button.setAttribute ('aria-expanded', 'false');
            if (focusButton) {
                button.focus ();
            }
        }

        function results () {
            return Array.prototype.slice.call (list.querySelectorAll ('a'));
        }

        button.setAttribute ('aria-haspopup', 'dialog');
        button.setAttribute ('aria-expanded', 'false');
        button.setAttribute ('aria-controls', panel.id);
        button.addEventListener ('click', function () {
            if (isOpen ()) {
                close (true);
            } else {
                open ();
            }
        });
        done.addEventListener ('click', function () {
            close (true);
        });
        input.addEventListener ('input', render);
        // Enter opens the first result.
        form.addEventListener ('submit', function (event) {
            event.preventDefault ();
            var first = results ()[0];
            if (first) {
                close (false);
                window.location.href = first.href;
            }
        });
        // The arrow keys move between the field and the results.
        panel.addEventListener ('keydown', function (event) {
            if (event.key !== 'ArrowDown' && event.key !== 'ArrowUp' && event.key !== 'Down' && event.key !== 'Up') {
                return;
            }
            var links = results ();
            if (!links.length) {
                return;
            }
            var down = event.key === 'ArrowDown' || event.key === 'Down';
            var at = links.indexOf (document.activeElement);
            event.preventDefault ();
            if (at === -1) {
                (down ? links[0] : input).focus ();
            } else if (down) {
                links[Math.min (at + 1, links.length - 1)].focus ();
            } else if (at === 0) {
                input.focus ();
            } else {
                links[at - 1].focus ();
            }
        });
        // A result may only scroll this page, so following one closes the panel.
        list.addEventListener ('click', function (event) {
            var target = event.target;
            while (target && target !== list) {
                if (target.tagName === 'A') {
                    close (false);
                    return;
                }
                target = target.parentNode;
            }
        });
        document.addEventListener ('keydown', function (event) {
            if ((event.key === 'Escape' || event.key === 'Esc') && isOpen ()) {
                close (true);
                return;
            }
            if (isOpen () || event.altKey || event.defaultPrevented) {
                return;
            }
            var typing = event.target && (event.target.isContentEditable || /^(INPUT|TEXTAREA|SELECT)$/.test (event.target.tagName));
            if ((event.key === 'k' || event.key === 'K') && (event.ctrlKey || event.metaKey) && !event.shiftKey) {
                event.preventDefault ();
                open ();
            } else if (event.key === '/' && !typing && !event.ctrlKey && !event.metaKey) {
                event.preventDefault ();
                open ();
            }
        });
        // A tap on text sends no click on iOS, so outside the panel it listens to pointerdown.
        document.addEventListener (window.PointerEvent ? 'pointerdown' : 'click', function (event) {
            if (isOpen () && !panel.contains (event.target) && !button.contains (event.target)) {
                close (false);
            }
        });
        // Tabbing out of the panel closes it too, leaving the focus where it went.
        panel.addEventListener ('focusout', function (event) {
            var next = event.relatedTarget;
            if (next && !panel.contains (next) && next !== button) {
                close (false);
            }
        });
    }

    // ---- The header's menu on phones ----

    // Below 640 px, and on short wide screens such as a phone in landscape (#2717), the nav, the
    // language menu and the Display button move into a panel under the header, opened by a menu button
    // next to Download; on other screens they stay where they are. Without this script the nav stays
    // a row of links under the logo. The query is the one of site.css's header blocks; keep them the same.
    var MENU_QUERY = '(max-width: 640px), (max-width: 1024px) and (max-height: 500px)';

    function setUpMenu () {
        var header = document.querySelector ('.site-header');
        var nav = header ? header.querySelector ('.site-nav') : null;
        var right = header ? header.querySelector ('.right') : null;
        if (!nav || !right || !window.matchMedia || header.querySelector ('.menu-button')) {
            return;
        }
        var narrow = window.matchMedia (MENU_QUERY);

        var menu = document.createElement ('div');
        menu.className = 'site-menu';
        menu.id = 'site-menu';
        nav.parentNode.insertBefore (menu, nav);
        menu.appendChild (nav);
        var tools = document.createElement ('div');
        tools.className = 'menu-tools';
        menu.appendChild (tools);

        var button = document.createElement ('button');
        button.type = 'button';
        button.className = 'menu-button';
        button.setAttribute ('aria-controls', menu.id);
        button.setAttribute ('aria-expanded', 'false');
        var icon = document.createElement ('span');
        icon.className = 'menu-icon';
        icon.setAttribute ('aria-hidden', 'true');
        var label = document.createElement ('span');
        label.className = 'visually-hidden';
        label.textContent = translate ('Menu') || 'Menu';
        button.appendChild (icon);
        button.appendChild (label);
        right.appendChild (button);

        // The language menu and the Display button, in their order, and what follows them in .right.
        var moved = Array.prototype.filter.call (right.children, function (child) {
            return child.id === 'lang-select' || child.id === 'display-button';
        });
        var movedBefore = moved.length ? moved[moved.length - 1].nextSibling : null;
        header.classList.add ('has-menu');

        function isOpen () {
            return header.classList.contains ('menu-open');
        }

        function open () {
            header.classList.add ('menu-open');
            button.setAttribute ('aria-expanded', 'true');
            var first = menu.querySelector ('a[href], select, button');
            if (first) {
                first.focus ();
            }
        }

        function close (focusButton) {
            if (!isOpen ()) {
                return;
            }
            header.classList.remove ('menu-open');
            button.setAttribute ('aria-expanded', 'false');
            if (focusButton) {
                button.focus ();
            }
        }

        // The search button stays in the header: next to the menu button on phones (#2774), first otherwise.
        var search = document.getElementById ('search-button');

        function place () {
            if (search && search.parentNode === right) {
                right.insertBefore (search, narrow.matches ? button : right.firstChild);
            }
            if (narrow.matches) {
                moved.forEach (function (element) {
                    tools.appendChild (element);
                });
            } else {
                close (false);
                moved.forEach (function (element) {
                    right.insertBefore (element, movedBefore);
                });
            }
        }

        button.addEventListener ('click', function () {
            if (isOpen ()) {
                close (true);
            } else {
                open ();
            }
        });
        document.addEventListener ('keydown', function (event) {
            if ((event.key === 'Escape' || event.key === 'Esc') && isOpen ()) {
                close (true);
            }
        });
        // A tap on a link closes it (it may only scroll this page), and so does a tap outside it.
        menu.addEventListener ('click', function (event) {
            var target = event.target;
            while (target && target !== menu) {
                if (target.tagName === 'A') {
                    close (false);
                    return;
                }
                target = target.parentNode;
            }
        });
        // A tap on text sends no click on iOS, so outside the panel it listens to pointerdown.
        document.addEventListener (window.PointerEvent ? 'pointerdown' : 'click', function (event) {
            if (isOpen () && !menu.contains (event.target) && !button.contains (event.target)) {
                close (false);
            }
        });
        // Tabbing out of the panel closes it too.
        menu.addEventListener ('focusout', function (event) {
            var next = event.relatedTarget;
            if (next && !menu.contains (next) && next !== button) {
                close (false);
            }
        });

        if (narrow.addEventListener) {
            narrow.addEventListener ('change', place);
        } else if (narrow.addListener) {
            narrow.addListener (place);
        }
        place ();
    }

    // ---- Copy buttons, also on code blocks a page adds later (the command reference's examples) ----

    function addCopyButton (pre, label, doneLabel) {
        if (pre.parentNode && pre.parentNode.classList && pre.parentNode.classList.contains ('code-block')) {
            return;
        }
        var wrapper = document.createElement ('div');
        wrapper.className = 'code-block';
        pre.parentNode.insertBefore (wrapper, pre);
        wrapper.appendChild (pre);
        var button = document.createElement ('button');
        button.type = 'button';
        button.className = 'copy-button';
        button.textContent = label;
        wrapper.appendChild (button);
        button.addEventListener ('click', function () {
            navigator.clipboard.writeText (pre.innerText.replace (/\n$/, '')).then (function () {
                button.textContent = doneLabel || label;
                setTimeout (function () { button.textContent = label; }, 2000);
            }, function () {});
        });
    }

    function setUpCopyButtons () {
        var label = root.getAttribute ('data-copy');
        var doneLabel = root.getAttribute ('data-copied');
        var main = document.querySelector ('main');
        if (!label || !main || !navigator.clipboard || !navigator.clipboard.writeText) {
            return;
        }
        function scan (node) {
            if (node.nodeType !== 1) {
                return;
            }
            if (node.tagName === 'PRE') {
                addCopyButton (node, label, doneLabel);
                return;
            }
            var blocks = node.querySelectorAll ('pre');
            for (var i = 0; i < blocks.length; i++) {
                addCopyButton (blocks[i], label, doneLabel);
            }
        }
        scan (main);
        if (window.MutationObserver) {
            new MutationObserver (function (records) {
                records.forEach (function (record) {
                    Array.prototype.forEach.call (record.addedNodes, scan);
                });
            }).observe (main, { childList: true, subtree: true });
        }
    }

    // ---- Live figures ----

    function readCache () {
        try {
            var cached = JSON.parse (window.sessionStorage.getItem (CACHE_KEY) || 'null');
            if (cached && typeof cached.time === 'number' && Date.now () - cached.time < CACHE_MS && cached.values) {
                return cached;
            }
        } catch (e) {}
        return { time: Date.now (), values: {} };
    }

    function writeCache (cache) {
        try {
            window.sessionStorage.setItem (CACHE_KEY, JSON.stringify (cache));
        } catch (e) {}
    }

    function fetchJson (url) {
        return fetch (url, { cache: 'no-cache' }).then (function (response) {
            if (!response.ok) {
                throw new Error (String (response.status));
            }
            return response.json ();
        });
    }

    function countCommands (groups) {
        if (!Array.isArray (groups)) {
            return null;
        }
        var count = 0;
        for (var i = 0; i < groups.length; i++) {
            if (!groups[i] || !Array.isArray (groups[i].commands)) {
                return null;
            }
            count += groups[i].commands.length;
        }
        return count > 0 ? { commands: count.toLocaleString ('en-US') } : null;
    }

    // Each source gives an object of values, or null when it could not be read.
    var SOURCES = {
        commands: function () {
            if (window.gCommands) {
                return Promise.resolve (countCommands (window.gCommands));
            }
            if (!siteRoot) {
                return Promise.resolve (null);
            }
            return new Promise (function (resolve) {
                var script = document.createElement ('script');
                script.src = siteRoot + 'archicad-addon/command_definitions.js';
                script.async = true;
                script.onload = function () {
                    try {
                        resolve (countCommands (window.gCommands));
                    } catch (e) {
                        resolve (null);
                    }
                };
                script.onerror = function () { resolve (null); };
                document.head.appendChild (script);
            });
        },
        archicad: function () {
            return fetchJson (RAW_BASE + 'main/archicad-addon/devkits.json').then (function (data) {
                var versions = (data && Array.isArray (data.devkits) ? data.devkits : []).map (function (devkit) {
                    return devkit ? devkit.archicad : null;
                }).filter (function (version) {
                    return Number.isInteger (version) && version > 0 && version < 1000;
                });
                if (versions.length === 0) {
                    return null;
                }
                versions.sort (function (a, b) { return a - b; });
                return {
                    'archicad-count': String (versions.length),
                    'archicad-range': String (versions[0]) + (versions.length > 1 ? '–' + versions[versions.length - 1] : '')
                };
            });
        },
        coverage: function () {
            // The badge the coverage workflow publishes, e.g. "99 % (2177/2179)".
            return fetchJson (RAW_BASE + 'api-coverage/api-coverage-badge.json').then (function (data) {
                var match = data && typeof data.message === 'string' ? /^\s*(\d{1,3}(?:\.\d+)?)\s*%/.exec (data.message) : null;
                return match ? { coverage: match[1] + '%' } : null;
            });
        },
        release: function () {
            return fetchJson ('https://img.shields.io/github/v/release/' + REPO + '.json').then (function (data) {
                var match = data && typeof data.value === 'string' ? /^v?(\d+\.\d+\.\d+)$/.exec (data.value.trim ()) : null;
                return match ? { release: match[1] } : null;
            });
        },
        scripts: function () {
            // The distinct slugs in the catalogue; left out until it exists and lists a script.
            return fetchJson (RAW_BASE + 'site-data/catalogue.json').then (function (catalogue) {
                if (!catalogue || catalogue.version !== 1 || !Array.isArray (catalogue.scripts)) {
                    return null;
                }
                var seen = {};
                var count = 0;
                catalogue.scripts.forEach (function (entry) {
                    if (entry && typeof entry.slug === 'string' && SLUG_PATTERN.test (entry.slug) && !seen[entry.slug]) {
                        seen[entry.slug] = true;
                        count++;
                    }
                });
                return count > 0 ? { scripts: count.toLocaleString ('en-US') } : null;
            });
        }
    };
    var SOURCE_OF = { 'commands': 'commands', 'archicad-count': 'archicad', 'archicad-range': 'archicad', 'coverage': 'coverage', 'release': 'release', 'scripts': 'scripts' };

    function setUpLiveFigures () {
        var elements = Array.prototype.slice.call (document.querySelectorAll ('[data-live]'));
        if (!elements.length) {
            return;
        }
        var cache = readCache ();
        var values = cache.values;

        function fill () {
            elements.forEach (function (element) {
                var names = element.getAttribute ('data-live').split (/\s+/).filter (Boolean);
                var ready = names.length > 0 && names.every (function (name) { return typeof values[name] === 'string'; });
                if (!ready) {
                    return;
                }
                var targets = element.querySelectorAll ('[data-live-value]');
                for (var i = 0; i < targets.length; i++) {
                    var name = targets[i].getAttribute ('data-live-value');
                    if (typeof values[name] === 'string') {
                        targets[i].textContent = values[name];
                    }
                }
                element.hidden = false;
            });
        }

        var wanted = {};
        elements.forEach (function (element) {
            element.getAttribute ('data-live').split (/\s+/).forEach (function (name) {
                if (SOURCE_OF[name] && typeof values[name] !== 'string') {
                    wanted[SOURCE_OF[name]] = true;
                }
            });
        });
        fill ();
        Object.keys (wanted).forEach (function (source) {
            if (source !== 'commands' && !window.fetch) {
                return;
            }
            var read;
            try {
                read = SOURCES[source] ();
            } catch (e) {
                return;
            }
            read.then (function (result) {
                if (!result) {
                    return;
                }
                Object.keys (result).forEach (function (name) {
                    values[name] = result[name];
                });
                writeCache (cache);
                fill ();
            }).catch (function () {});
        });
    }

    function init () {
        // The rest reads the translated labels and fills the translated live figures, so it waits.
        loadTranslations (language, function () {
            readDictionaries ();
            if (language.code !== 'en') {
                applyTranslations (document);
                // Only a page with its own dictionaries is in that language; the others stay English, and
                // so does a data-i18n-partial page, which translates only some of its parts.
                if (root.hasAttribute ('data-i18n-pages') && !root.hasAttribute ('data-i18n-partial')) {
                    root.setAttribute ('lang', language.tag);
                }
            }
            root.removeAttribute ('data-i18n-loading');
            setUpLanguageMenu (language);
            setUpDisplayMenu ();
            setUpSearch ();
            setUpMenu ();
            setUpCopyButtons ();
            setUpLiveFigures ();
            translationReady = true;
            readyCallbacks.splice (0).forEach (function (callback) {
                try {
                    callback ();
                } catch (e) {}
            });
        });
    }

    if (document.readyState === 'loading') {
        document.addEventListener ('DOMContentLoaded', init);
    } else {
        init ();
    }
}) ();
