/* The scripts (/scripts/): the schedule of scripts with search and filters, and one script at
   ?s=<slug>. It reads catalogue.json from the site-data branch on raw.githubusercontent.com (never
   GitHub's API):
       {"version": 1, "scripts": [{slug, name, summary, category, tags, author, license, entry,
                                   versions: [{version, archicadVersions, minTapirVersion, changelog,
                                               commit, url, sha256}, ...]}, ...]}
   The listing fields are those of marketplace/<slug>/listing.json (see marketplace/README.md), where
   versions are listed newest first. The page does not rely on that order: it sorts the versions by
   their number (x.y.z), and the highest one is the script's current version.
   Optional fields it also shows when present: "changes" ("model", "read-only" or "files", from the
   script's # /// tapir header, on the script or on its newest version) and "screenshots" (file
   names in the script's screenshots/ folder).

   A script's ID in the schedule is its category's code (data-code in the page's category list) and
   its place in that category, by name: QA-01, QA-02, ...

   A script's README, screenshots and code are read at the commit its newest version names. Anything
   that is missing or does not look right is left out rather than shown wrong; when the catalogue
   cannot be read, the page keeps saying the scripts are opening soon. Every text shown is in the
   page's HTML. */
(function () {
    'use strict';

    var REPO = 'ENZYME-APD/tapir-archicad-automation';
    var CATALOGUE_URL = 'https://raw.githubusercontent.com/' + REPO + '/site-data/catalogue.json';
    var RAW_BASE = 'https://raw.githubusercontent.com/' + REPO + '/';
    var GITHUB_BASE = 'https://github.com/' + REPO + '/';

    var SLUG_PATTERN = /^[a-z0-9]+(-[a-z0-9]+)*$/;
    var ENTRY_PATTERN = /^[A-Za-z0-9_][A-Za-z0-9_-]*\.py$/;
    var COMMIT_PATTERN = /^[0-9a-f]{7,40}$/;
    var SHA256_PATTERN = /^[0-9a-f]{64}$/;
    var VERSION_PATTERN = /^[0-9]+\.[0-9]+\.[0-9]+$/;
    var GITHUB_LOGIN_PATTERN = /^[A-Za-z0-9](?:[A-Za-z0-9-]{0,38})$/;
    var SCREENSHOT_PATTERN = /^(?:screenshots\/)?([A-Za-z0-9_][A-Za-z0-9_.-]*\.(?:png|jpe?g))$/i;
    var CHANGES = ['model', 'files', 'read-only'];
    var MAX_SCREENSHOTS = 5;

    function byId (id) {
        return document.getElementById (id);
    }

    function isString (value) {
        return typeof value === 'string' && value.trim () !== '';
    }

    // RegExp.test turns a missing value into "undefined", which some patterns would accept.
    function matches (pattern, value) {
        return typeof value === 'string' && pattern.test (value);
    }

    function format (text, values) {
        return String (text || '').replace (/\{(\w+)\}/g, function (match, key) {
            return Object.prototype.hasOwnProperty.call (values, key) ? String (values[key]) : match;
        });
    }

    function fetchResponse (url) {
        return fetch (url, { cache: 'no-cache' }).then (function (response) {
            if (!response.ok) {
                throw new Error (String (response.status));
            }
            return response;
        });
    }

    // "1.10.0" after "1.9.2": the parts compare as numbers.
    function compareVersions (a, b) {
        var partsA = a.split ('.').map (Number);
        var partsB = b.split ('.').map (Number);
        for (var i = 0; i < 3; i++) {
            if (partsA[i] !== partsB[i]) {
                return partsA[i] - partsB[i];
            }
        }
        return 0;
    }

    // [27, 28, 29, 30] -> "27–30", [25, 27, 28] -> "25, 27–28"
    function versionRanges (versions) {
        var ranges = [];
        versions.forEach (function (v) {
            var last = ranges[ranges.length - 1];
            if (last && v === last[1] + 1) {
                last[1] = v;
            } else {
                ranges.push ([v, v]);
            }
        });
        return ranges.map (function (r) { return r[0] === r[1] ? String (r[0]) : r[0] + '–' + r[1]; }).join (', ');
    }

    // ---- The catalogue, checked field by field ----

    function cleanVersion (version) {
        if (!version || !matches (VERSION_PATTERN, version.version) || !isString (version.minTapirVersion) || !isString (version.changelog)) {
            return null;
        }
        var archicadVersions = Array.isArray (version.archicadVersions) ? version.archicadVersions.filter (function (v) {
            return Number.isInteger (v) && v >= 25 && v < 100;
        }) : [];
        if (archicadVersions.length === 0) {
            return null;
        }
        archicadVersions.sort (function (a, b) { return a - b; });
        return {
            version: version.version,
            archicadVersions: archicadVersions,
            minTapirVersion: version.minTapirVersion,
            changelog: version.changelog,
            commit: matches (COMMIT_PATTERN, version.commit) ? version.commit : null,
            sha256: typeof version.sha256 === 'string' && SHA256_PATTERN.test (version.sha256.toLowerCase ()) ? version.sha256.toLowerCase () : null,
            changes: version.changes
        };
    }

    function cleanScript (script, categories) {
        if (!script || !matches (SLUG_PATTERN, script.slug) || !isString (script.name) || !isString (script.summary)) {
            return null;
        }
        if (!Object.prototype.hasOwnProperty.call (categories, script.category)) {
            return null;
        }
        if (!script.author || !isString (script.author.name) || !isString (script.license) || !matches (ENTRY_PATTERN, script.entry)) {
            return null;
        }
        var versions = Array.isArray (script.versions) ? script.versions.map (cleanVersion) : [];
        if (versions.length === 0 || versions.indexOf (null) !== -1) {
            return null;
        }
        // Newest first, whatever order the catalogue has; a version number listed twice is an error.
        versions.sort (function (a, b) { return compareVersions (b.version, a.version); });
        for (var v = 1; v < versions.length; v++) {
            if (compareVersions (versions[v - 1].version, versions[v].version) === 0) {
                return null;
            }
        }
        var newest = versions[0];
        var changes = [newest.changes, script.changes].filter (function (value) {
            return CHANGES.indexOf (value) !== -1;
        })[0] || null;
        var screenshots = [];
        if (Array.isArray (script.screenshots)) {
            script.screenshots.forEach (function (file) {
                var match = typeof file === 'string' ? SCREENSHOT_PATTERN.exec (file) : null;
                if (match && screenshots.length < MAX_SCREENSHOTS) {
                    screenshots.push (match[1]);
                }
            });
        }
        return {
            slug: script.slug,
            name: script.name,
            summary: script.summary,
            category: script.category,
            tags: Array.isArray (script.tags) ? script.tags.filter (isString) : [],
            author: {
                name: script.author.name,
                github: matches (GITHUB_LOGIN_PATTERN, script.author.github) ? script.author.github : null
            },
            license: script.license,
            entry: script.entry,
            versions: versions,
            newest: newest,
            changes: changes,
            screenshots: screenshots
        };
    }

    // The categories from the page: {key: {name, code, order}}.
    function categoriesFromPage () {
        var categories = {};
        var items = byId ('script-categories').querySelectorAll ('li[data-value]');
        for (var i = 0; i < items.length; i++) {
            categories[items[i].getAttribute ('data-value')] = {
                name: items[i].textContent.trim (),
                code: items[i].getAttribute ('data-code') || '',
                order: i
            };
        }
        return categories;
    }

    // ---- Shared bits ----

    function effectPill (changes) {
        var templates = byId ('script-effect-templates').content;
        var pill = templates.querySelector ('[data-effect="' + (changes || 'unknown') + '"]');
        return pill ? pill.cloneNode (true) : document.createTextNode ('');
    }

    // ---- The schedule ----

    function makeChips (container, options, selected, onChange) {
        var buttons = [];
        options.forEach (function (option) {
            var button = document.createElement ('button');
            button.type = 'button';
            button.className = 'chip';
            button.textContent = option.label;
            button.setAttribute ('aria-pressed', String (option.value === selected));
            button.addEventListener ('click', function () {
                buttons.forEach (function (other) {
                    other.setAttribute ('aria-pressed', String (other === button));
                });
                onChange (option.value);
            });
            buttons.push (button);
            container.appendChild (button);
        });
    }

    function showList (scripts, categories) {
        var search = byId ('script-search');
        var rowsBody = byId ('script-rows');
        var count = byId ('script-count');
        var template = byId ('script-row-template');
        var categoryChips = byId ('category-chips');
        var versionChips = byId ('version-chips');

        // By category (the page's order), then by name; the ID numbers the scripts of each category.
        scripts.sort (function (a, b) {
            return categories[a.category].order - categories[b.category].order || a.name.localeCompare (b.name, 'en');
        });
        var perCategory = {};
        scripts.forEach (function (script) {
            perCategory[script.category] = (perCategory[script.category] || 0) + 1;
            var number = perCategory[script.category];
            script.id = categories[script.category].code + '-' + (number < 10 ? '0' : '') + number;
        });

        // Only the categories and Archicad versions that have a script are offered.
        var usedCategories = {};
        var usedVersions = {};
        scripts.forEach (function (script) {
            usedCategories[script.category] = true;
            script.newest.archicadVersions.forEach (function (v) { usedVersions[v] = true; });
        });

        var rows = scripts.map (function (script) {
            var row = template.content.firstElementChild.cloneNode (true);
            row.querySelector ('.id').textContent = script.id;
            var link = row.querySelector ('.nm');
            link.textContent = script.name;
            link.href = '?s=' + encodeURIComponent (script.slug);
            row.querySelector ('.ds').textContent = script.summary;
            row.querySelector ('.cell-category').appendChild (document.createTextNode (categories[script.category].name));
            row.querySelector ('.cell-effect').appendChild (effectPill (script.changes));
            row.querySelector ('.cell-archicad').appendChild (document.createTextNode (versionRanges (script.newest.archicadVersions)));
            row.querySelector ('.cell-author').appendChild (document.createTextNode (script.author.name));
            var haystack = [script.name, script.summary, script.author.name, script.id, categories[script.category].name].concat (script.tags).join (' ').toLowerCase ();
            return { script: script, element: row, haystack: haystack };
        });

        // The filters are kept in the address (q, category, archicad), so a filtered list can be shared.
        var params = new URLSearchParams (window.location.search);
        var wantedCategory = params.get ('category') && usedCategories[params.get ('category')] ? params.get ('category') : '';
        var wantedVersion = params.get ('archicad') && usedVersions[params.get ('archicad')] ? Number (params.get ('archicad')) : null;
        search.value = params.get ('q') || '';

        makeChips (categoryChips, [{ value: '', label: categoryChips.getAttribute ('data-all') }].concat (
            Object.keys (categories).filter (function (key) { return usedCategories[key]; }).map (function (key) {
                return { value: key, label: categories[key].name };
            })), wantedCategory, function (value) { wantedCategory = value; update (); });
        var versions = Object.keys (usedVersions).map (Number).sort (function (a, b) { return b - a; });
        makeChips (versionChips, [{ value: null, label: versionChips.getAttribute ('data-all') }].concat (
            versions.map (function (v) { return { value: v, label: format (versionChips.getAttribute ('data-option'), { version: v }) }; })),
            wantedVersion, function (value) { wantedVersion = value; update (); });
        if (versions.length < 2) {
            versionChips.hidden = true;
        }

        function update () {
            var words = search.value.toLowerCase ().split (/\s+/).filter (Boolean);
            var shown = 0;
            rowsBody.textContent = '';
            rows.forEach (function (item) {
                var ok = words.every (function (word) { return item.haystack.indexOf (word) !== -1; }) &&
                    (!wantedCategory || item.script.category === wantedCategory) &&
                    (wantedVersion === null || item.script.newest.archicadVersions.indexOf (wantedVersion) !== -1);
                if (ok) {
                    rowsBody.appendChild (item.element);
                    shown++;
                }
            });
            count.textContent = shown === 0 ? count.getAttribute ('data-none') :
                shown === 1 ? count.getAttribute ('data-one') : format (count.getAttribute ('data-many'), { count: shown });

            var query = new URLSearchParams ();
            if (search.value.trim ()) {
                query.set ('q', search.value.trim ());
            }
            if (wantedCategory) {
                query.set ('category', wantedCategory);
            }
            if (wantedVersion !== null) {
                query.set ('archicad', String (wantedVersion));
            }
            var address = window.location.pathname + (query.toString () ? '?' + query.toString () : '');
            try {
                window.history.replaceState (null, '', address);
            } catch (e) {}
        }

        search.addEventListener ('input', update);
        byId ('script-filters').addEventListener ('submit', function (event) { event.preventDefault (); });

        update ();
        byId ('scripts-opening').hidden = true;
        byId ('scripts-browser').hidden = false;
    }

    // ---- A README, as a small and safe subset of Markdown, built as DOM nodes (never innerHTML) ----

    function safeUrl (href, base) {
        try {
            var url = new URL (href, base);
            return url.protocol === 'https:' || url.protocol === 'http:' || url.protocol === 'mailto:' ? url.href : null;
        } catch (e) {
            return null;
        }
    }

    function appendInline (parent, text, links) {
        // `code`, **bold**, *italic* or _italic_, ![alt](src) and [text](href).
        var pattern = /`([^`]+)`|\*\*([^*]+)\*\*|\*([^*\s][^*]*)\*|\b_([^_]+)_\b|!\[([^\]]*)\]\(([^)\s]+)[^)]*\)|\[([^\]]+)\]\(([^)\s]+)[^)]*\)/g;
        var last = 0;
        var match;
        while ((match = pattern.exec (text)) !== null) {
            if (match.index > last) {
                parent.appendChild (document.createTextNode (text.slice (last, match.index)));
            }
            var node = null;
            if (match[1] !== undefined) {
                node = document.createElement ('code');
                node.textContent = match[1];
            } else if (match[2] !== undefined) {
                node = document.createElement ('strong');
                appendInline (node, match[2], links);
            } else if (match[3] !== undefined || match[4] !== undefined) {
                node = document.createElement ('em');
                appendInline (node, match[3] !== undefined ? match[3] : match[4], links);
            } else if (match[6] !== undefined) {
                var src = safeUrl (match[6], links.raw);
                if (src && /^https:\/\/raw\.githubusercontent\.com\//.test (src)) {
                    node = document.createElement ('img');
                    node.src = src;
                    node.alt = match[5];
                    node.loading = 'lazy';
                } else {
                    node = document.createTextNode (match[5]);
                }
            } else {
                var href = safeUrl (match[8], links.page);
                if (href) {
                    node = document.createElement ('a');
                    node.href = href;
                    appendInline (node, match[7], links);
                } else {
                    node = document.createTextNode (match[7]);
                }
            }
            parent.appendChild (node);
            last = pattern.lastIndex;
        }
        if (last < text.length) {
            parent.appendChild (document.createTextNode (text.slice (last)));
        }
    }

    function renderMarkdown (container, markdown, links) {
        var lines = markdown.replace (/\r\n?/g, '\n').split ('\n');
        var i = 0;
        var paragraph = [];
        var list = null;

        function flushParagraph () {
            if (paragraph.length) {
                var p = document.createElement ('p');
                appendInline (p, paragraph.join (' '), links);
                container.appendChild (p);
                paragraph = [];
            }
        }
        function closeList () {
            list = null;
        }

        while (i < lines.length) {
            var line = lines[i];
            var fence = /^\s*(```|~~~)/.exec (line);
            if (fence) {
                flushParagraph ();
                closeList ();
                var code = [];
                i++;
                while (i < lines.length && lines[i].trim ().indexOf (fence[1]) !== 0) {
                    code.push (lines[i]);
                    i++;
                }
                var pre = document.createElement ('pre');
                var codeElement = document.createElement ('code');
                codeElement.textContent = code.join ('\n');
                pre.appendChild (codeElement);
                container.appendChild (pre);
                i++;
                continue;
            }
            var heading = /^(#{1,6})\s+(.*?)\s*#*\s*$/.exec (line);
            if (heading) {
                flushParagraph ();
                closeList ();
                // The page's h1 is the script's name and "About this script" is an h2.
                var level = Math.min (heading[1].length + 2, 6);
                var h = document.createElement ('h' + level);
                appendInline (h, heading[2], links);
                container.appendChild (h);
                i++;
                continue;
            }
            var item = /^\s*(?:([-*+])|(\d+)[.)])\s+(.*)$/.exec (line);
            if (item) {
                flushParagraph ();
                var tag = item[1] ? 'ul' : 'ol';
                if (!list || list.tagName.toLowerCase () !== tag) {
                    list = document.createElement (tag);
                    container.appendChild (list);
                }
                var li = document.createElement ('li');
                appendInline (li, item[3], links);
                list.appendChild (li);
                i++;
                continue;
            }
            if (/^\s*\|.*\|\s*$/.test (line)) {
                // Tables are shown as their text, which stays readable.
                flushParagraph ();
                closeList ();
                var rows = [];
                while (i < lines.length && /^\s*\|.*\|\s*$/.test (lines[i])) {
                    if (!/^\s*\|[\s:|-]+\|\s*$/.test (lines[i])) {
                        rows.push (lines[i].trim ());
                    }
                    i++;
                }
                var table = document.createElement ('pre');
                table.textContent = rows.join ('\n');
                container.appendChild (table);
                continue;
            }
            if (line.trim () === '' || /^\s*([-*_])(\s*\1){2,}\s*$/.test (line)) {
                flushParagraph ();
                closeList ();
                i++;
                continue;
            }
            if (list && /^\s{2,}\S/.test (line)) {
                // A list item's continuation line.
                list.lastChild.appendChild (document.createTextNode (' '));
                appendInline (list.lastChild, line.trim (), links);
                i++;
                continue;
            }
            closeList ();
            paragraph.push (line.replace (/^\s*>\s?/, '').trim ());
            i++;
        }
        flushParagraph ();
    }

    // ---- One script ----

    function bytesToHex (buffer) {
        var bytes = new Uint8Array (buffer);
        var hex = '';
        for (var i = 0; i < bytes.length; i++) {
            hex += (bytes[i] < 16 ? '0' : '') + bytes[i].toString (16);
        }
        return hex;
    }

    function showScript (script, categories) {
        var newest = script.newest;
        document.title = script.name + ' - ' + document.title;

        // The name in the page head, with its last word in italics.
        var title = byId ('detail-name');
        var words = script.name.trim ().split (/\s+/);
        var last = words.pop ();
        if (words.length) {
            title.appendChild (document.createTextNode (words.join (' ') + ' '));
        }
        var em = document.createElement ('em');
        em.textContent = last;
        title.appendChild (em);
        byId ('detail-crumb').textContent = script.name;
        byId ('detail-summary').textContent = script.summary;

        byId ('detail-category').textContent = categories[script.category].name;
        byId ('detail-effect').appendChild (effectPill (script.changes));
        var author = byId ('detail-author');
        if (script.author.github) {
            var authorLink = document.createElement ('a');
            authorLink.href = 'https://github.com/' + script.author.github;
            authorLink.textContent = script.author.name;
            author.appendChild (authorLink);
        } else {
            author.textContent = script.author.name;
        }
        byId ('detail-version').textContent = newest.version;
        byId ('detail-archicad').textContent = versionRanges (newest.archicadVersions);
        byId ('detail-tapir').textContent = newest.minTapirVersion;
        byId ('detail-license').textContent = script.license;
        var nameRefs = document.querySelectorAll ('.detail-name-ref');
        for (var n = 0; n < nameRefs.length; n++) {
            nameRefs[n].textContent = script.name;
        }
        // One sentence on what running it does: "files" is not "read only".
        if (script.changes) {
            byId ('detail-changes-' + script.changes).hidden = false;
        }

        var report = byId ('detail-report');
        var reportValues = { name: script.name, slug: script.slug, version: newest.version };
        report.href = GITHUB_BASE + 'issues/new?title=' + encodeURIComponent (format (report.getAttribute ('data-title'), reportValues)) +
            '&body=' + encodeURIComponent (format (report.getAttribute ('data-body'), reportValues));

        var versionTemplate = byId ('script-version-template');
        script.versions.forEach (function (version) {
            var item = versionTemplate.content.firstElementChild.cloneNode (true);
            item.querySelector ('.version-number').textContent = version.version;
            item.querySelector ('.version-archicad').textContent = versionRanges (version.archicadVersions);
            item.querySelector ('.version-tapir').textContent = version.minTapirVersion;
            item.querySelector ('.version-changelog').textContent = version.changelog;
            byId ('detail-versions').appendChild (item);
        });

        byId ('scripts-list-view').hidden = true;
        byId ('script-detail').hidden = false;

        // The files are read at the commit of the newest version; without one, they are left out.
        if (!newest.commit) {
            return;
        }
        var folder = 'marketplace/' + script.slug + '/';
        var rawFolder = RAW_BASE + newest.commit + '/' + folder;
        var pageFolder = GITHUB_BASE + 'blob/' + newest.commit + '/' + folder;

        var source = byId ('detail-source');
        source.href = GITHUB_BASE + 'tree/' + newest.commit + '/' + folder;
        source.hidden = false;

        fetchResponse (rawFolder + 'README.md').then (function (response) {
            return response.text ();
        }).then (function (markdown) {
            var readme = byId ('detail-readme');
            // The README's own title is the script's name, which the page head already shows.
            markdown = markdown.replace (/^\uFEFF?\s*#\s[^\n]*\n/, '');
            renderMarkdown (readme, markdown, { raw: rawFolder, page: pageFolder });
            if (readme.firstChild) {
                byId ('detail-readme-section').hidden = false;
            }
        }).catch (function () {});

        // The first screenshot is the page's picture; the others follow the README.
        var shot = byId ('detail-shot');
        var gallery = byId ('detail-screenshots');
        script.screenshots.forEach (function (file, index) {
            var url = rawFolder + 'screenshots/' + encodeURIComponent (file);
            var link = document.createElement ('a');
            link.href = url;
            var image = document.createElement ('img');
            image.src = url;
            image.alt = format (shot.getAttribute ('data-alt'), { number: index + 1, name: script.name });
            image.loading = index === 0 ? 'eager' : 'lazy';
            image.addEventListener ('error', function () {
                var holder = link.parentNode;
                link.remove ();
                if (holder && !holder.firstChild) {
                    (holder === shot ? shot : byId ('detail-screenshots-section')).hidden = true;
                }
            });
            link.appendChild (image);
            (index === 0 ? shot : gallery).appendChild (link);
        });
        shot.hidden = !shot.firstChild;
        byId ('detail-screenshots-section').hidden = !gallery.firstChild;

        // The code is shown, and offered for download, only when it matches the catalogue's SHA-256
        // (when the catalogue has one and the browser can check it).
        fetchResponse (rawFolder + encodeURIComponent (script.entry)).then (function (response) {
            return response.arrayBuffer ();
        }).then (function (buffer) {
            var check = newest.sha256 && window.crypto && window.crypto.subtle ?
                window.crypto.subtle.digest ('SHA-256', buffer).then (bytesToHex) : Promise.resolve (null);
            return check.then (function (hash) {
                if (hash !== null && hash !== newest.sha256) {
                    return;
                }
                if (hash !== null) {
                    byId ('detail-sha').textContent = newest.sha256;
                    var shaParts = document.querySelectorAll ('.detail-sha');
                    for (var s = 0; s < shaParts.length; s++) {
                        shaParts[s].hidden = false;
                    }
                }
                var text = new TextDecoder ('utf-8').decode (buffer);
                byId ('detail-code').textContent = text;
                byId ('detail-entry').textContent = script.entry;
                byId ('detail-code-section').hidden = false;
                var download = byId ('detail-download');
                download.href = URL.createObjectURL (new Blob ([buffer], { type: 'text/x-python' }));
                download.setAttribute ('download', script.entry);
                download.hidden = false;
            });
        }).catch (function () {});
    }

    function showNotFound () {
        byId ('scripts-list-view').hidden = true;
        byId ('script-not-found').hidden = false;
    }

    function init () {
        if (!window.fetch || !window.URLSearchParams || !('content' in document.createElement ('template'))) {
            return;
        }
        var wanted = new URLSearchParams (window.location.search).get ('s');
        var categories = categoriesFromPage ();
        fetchResponse (CATALOGUE_URL).then (function (response) {
            return response.json ();
        }).then (function (catalogue) {
            var scripts = [];
            var seen = {};
            if (catalogue && catalogue.version === 1 && Array.isArray (catalogue.scripts)) {
                catalogue.scripts.forEach (function (entry) {
                    var script = cleanScript (entry, categories);
                    if (script && !seen[script.slug]) {
                        seen[script.slug] = true;
                        scripts.push (script);
                    }
                });
            }
            if (wanted !== null) {
                var found = scripts.filter (function (script) { return script.slug === wanted; })[0];
                if (found) {
                    showScript (found, categories);
                } else {
                    showNotFound ();
                }
            } else if (scripts.length) {
                showList (scripts, categories);
            }
        }).catch (function () {
            if (wanted !== null) {
                showNotFound ();
            }
        });
    }

    if (document.readyState === 'loading') {
        document.addEventListener ('DOMContentLoaded', init);
    } else {
        init ();
    }
}) ();
