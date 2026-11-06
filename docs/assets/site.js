/* The shared behaviour of every page of Tapir's site: the dark mode toggle, a copy button on code
   blocks, and the live figures. The pages are complete without it; it only adds to them.
   Every text it shows is in the page's HTML, so the pages can be translated: the toggle's two words
   are its data-dark and data-light attributes, the copy button's labels are data-copy and
   data-copied on the <html> element.

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
        setUpThemeToggle ();
        setUpCopyButtons ();
        setUpLiveFigures ();
    }

    if (document.readyState === 'loading') {
        document.addEventListener ('DOMContentLoaded', init);
    } else {
        init ();
    }
}) ();
