/* The shared behaviour of every page of Tapir's site: the language menu and the translations, the
   dark mode toggle, the header's menu on phones, a copy button on code blocks, and the live figures. The pages are complete
   without it; it only adds to them. Every text it shows is in the page's HTML, so the pages can be
   translated: the toggle's two words are its data-dark and data-light attributes, the copy button's
   labels are data-copy and data-copied on the <html> element.

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

    var STORAGE_KEY = 'tapir-theme';
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
        ['.site-nav', 'aria-label'],
        ['#theme-toggle', 'data-dark data-light']
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
        var toggle = document.getElementById ('theme-toggle');
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

    // ---- Dark mode ----

    function storedTheme () {
        try {
            var theme = window.localStorage.getItem (STORAGE_KEY);
            return theme === 'light' || theme === 'dark' ? theme : null;
        } catch (e) {
            return null;
        }
    }

    function storeTheme (theme) {
        try {
            window.localStorage.setItem (STORAGE_KEY, theme);
        } catch (e) {}
    }

    function isDark () {
        var theme = root.getAttribute ('data-theme');
        if (theme === 'light' || theme === 'dark') {
            return theme === 'dark';
        }
        return !!(darkQuery && darkQuery.matches);
    }

    function setUpThemeToggle () {
        var toggle = document.getElementById ('theme-toggle');
        if (!toggle) {
            return;
        }
        // The head's inline script already applied a stored choice; this repeats it in case it could not run.
        var theme = storedTheme ();
        if (theme) {
            root.setAttribute ('data-theme', theme);
        }
        var word = toggle.querySelector ('.toggle-word') || toggle;
        function update () {
            // The button names the theme it switches to.
            word.textContent = isDark () ? toggle.getAttribute ('data-light') : toggle.getAttribute ('data-dark');
        }
        toggle.addEventListener ('click', function () {
            var next = isDark () ? 'light' : 'dark';
            root.setAttribute ('data-theme', next);
            storeTheme (next);
            update ();
        });
        if (darkQuery) {
            var onChange = function () {
                if (!root.hasAttribute ('data-theme')) {
                    update ();
                }
            };
            if (darkQuery.addEventListener) {
                darkQuery.addEventListener ('change', onChange);
            } else if (darkQuery.addListener) {
                darkQuery.addListener (onChange);
            }
        }
        update ();
        toggle.hidden = false;
    }

    // ---- The header's menu on phones ----

    // Below 640 px (as in site.css) the nav, the language menu and the dark mode toggle move into a
    // panel under the header, opened by a menu button next to Download; on wider screens they stay
    // where they are. Without this script the nav stays a row of links under the logo.
    var NARROW_QUERY = '(max-width: 640px)';

    function setUpMenu () {
        var header = document.querySelector ('.site-header');
        var nav = header ? header.querySelector ('.site-nav') : null;
        var right = header ? header.querySelector ('.right') : null;
        if (!nav || !right || !window.matchMedia || header.querySelector ('.menu-button')) {
            return;
        }
        var narrow = window.matchMedia (NARROW_QUERY);

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

        // The language menu and the toggle, in their order, and what follows them in .right.
        var moved = Array.prototype.filter.call (right.children, function (child) {
            return child.id === 'lang-select' || child.id === 'theme-toggle';
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

        function place () {
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
            setUpThemeToggle ();
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
