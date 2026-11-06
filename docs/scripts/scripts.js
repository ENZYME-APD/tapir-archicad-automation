/* The Tapir Marketplace (/scripts/): the list of scripts with search and filters, and one script at
   ?s=<slug>. It reads catalogue.json from the site-data branch on raw.githubusercontent.com (never
   GitHub's API):
       {"version": 1, "scripts": [{slug, name, summary, category, tags, author, license, entry,
                                   versions: [{version, archicadVersions, minTapirVersion, changelog,
                                               commit, url, sha256}, ...]}, ...]}
   The listing fields are those of marketplace/<slug>/listing.json (see marketplace/README.md).
   Optional fields it also shows when present: "changes" ("model", "read-only" or "files", from the
   script's # /// tapir header, on the script or on its newest version) and "screenshots" (file
   names in the script's screenshots/ folder).

   A script's README, screenshots and code are read at the commit its newest version names. Anything
   that is missing or does not look right is left out rather than shown wrong; when the catalogue
   cannot be read, the page keeps saying the Marketplace is opening. Every text shown is in the
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

    function cleanScript (script, categoryNames) {
        if (!script || !matches (SLUG_PATTERN, script.slug) || !isString (script.name) || !isString (script.summary)) {
            return null;
        }
        if (!Object.prototype.hasOwnProperty.call (categoryNames, script.category)) {
            return null;
        }
        if (!script.author || !isString (script.author.name) || !isString (script.license) || !matches (ENTRY_PATTERN, script.entry)) {
            return null;
        }
        var versions = Array.isArray (script.versions) ? script.versions.map (cleanVersion) : [];
        if (versions.length === 0 || versions.indexOf (null) !== -1) {
            return null;
        }
        var newest = versions[0];
        var changes = [script.changes, newest.changes].filter (function (value) {
            return value === 'model' || value === 'read-only' || value === 'files';
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

    function categoryNamesFromPage () {
        var names = {};
        var options = byId ('script-category').options;
        for (var i = 0; i < options.length; i++) {
            if (options[i].value) {
                names[options[i].value] = options[i].textContent;
            }
        }
        return names;
    }

    // ---- Shared bits ----

    function badgeTemplate (kind) {
        var template = byId ('script-badge-templates');
        var badge = template.content.querySelector ('[data-badge="' + kind + '"]');
        return badge ? badge.parentNode.cloneNode (true) : null;
    }

    function addBadges (list, script, categoryNames, withVersion) {
        var kinds = [];
        if (script.changes === 'model') {
            kinds.push ('model');
        } else if (script.changes) {
            kinds.push ('read-only');
        }
        kinds.push ('category');
        if (withVersion) {
            kinds.push ('version');
        }
        kinds.forEach (function (kind) {
            var item = badgeTemplate (kind);
            if (!item) {
                return;
            }
            var badge = item.querySelector ('[data-badge]');
            if (kind === 'category') {
                badge.textContent = categoryNames[script.category];
            } else if (kind === 'version') {
                badge.textContent = format (badge.textContent, { version: script.newest.version });
            }
            list.appendChild (item);
        });
    }

    function versionList (versions) {
        return versions.join (', ');
    }

    // ---- The list ----

    function showList (scripts, categoryNames) {
        var browser = byId ('scripts-browser');
        var cards = byId ('script-cards');
        var search = byId ('script-search');
        var category = byId ('script-category');
        var archicad = byId ('script-archicad');
        var count = byId ('script-count');
        var template = byId ('script-card-template');

        scripts.sort (function (a, b) { return a.name.localeCompare (b.name, 'en'); });

        // Only the categories and Archicad versions that have a script are offered.
        var usedCategories = {};
        var usedVersions = {};
        scripts.forEach (function (script) {
            usedCategories[script.category] = true;
            script.newest.archicadVersions.forEach (function (v) { usedVersions[v] = true; });
        });
        for (var i = category.options.length - 1; i > 0; i--) {
            if (!usedCategories[category.options[i].value]) {
                category.remove (i);
            }
        }
        Object.keys (usedVersions).map (Number).sort (function (a, b) { return b - a; }).forEach (function (v) {
            var option = document.createElement ('option');
            option.value = String (v);
            option.textContent = format (archicad.getAttribute ('data-option'), { version: v });
            archicad.appendChild (option);
        });

        var items = scripts.map (function (script) {
            var item = template.content.firstElementChild.cloneNode (true);
            var link = item.querySelector ('.script-name');
            link.textContent = script.name;
            link.href = '?s=' + encodeURIComponent (script.slug);
            item.querySelector ('.script-summary').textContent = script.summary;
            item.querySelector ('.script-author').textContent = script.author.name;
            item.querySelector ('.script-archicad').textContent = versionList (script.newest.archicadVersions);
            addBadges (item.querySelector ('.script-badges'), script, categoryNames, false);
            var haystack = [script.name, script.summary, script.author.name, categoryNames[script.category]].concat (script.tags).join (' ').toLowerCase ();
            return { script: script, element: item, haystack: haystack };
        });

        // The filters are kept in the address (q, category, archicad), so a filtered list can be shared.
        var params = new URLSearchParams (window.location.search);
        search.value = params.get ('q') || '';
        if (params.get ('category') && usedCategories[params.get ('category')]) {
            category.value = params.get ('category');
        }
        if (params.get ('archicad') && usedVersions[params.get ('archicad')]) {
            archicad.value = params.get ('archicad');
        }

        function update () {
            var words = search.value.toLowerCase ().split (/\s+/).filter (Boolean);
            var wantedCategory = category.value;
            var wantedVersion = archicad.value ? Number (archicad.value) : null;
            var shown = 0;
            cards.textContent = '';
            items.forEach (function (item) {
                var matches = words.every (function (word) { return item.haystack.indexOf (word) !== -1; }) &&
                    (!wantedCategory || item.script.category === wantedCategory) &&
                    (wantedVersion === null || item.script.newest.archicadVersions.indexOf (wantedVersion) !== -1);
                if (matches) {
                    cards.appendChild (item.element);
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
        category.addEventListener ('change', update);
        archicad.addEventListener ('change', update);
        byId ('script-filters').addEventListener ('submit', function (event) { event.preventDefault (); });

        update ();
        byId ('scripts-opening').hidden = true;
        browser.hidden = false;
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

    function showScript (script, categoryNames) {
        var newest = script.newest;
        document.title = script.name + ' - ' + document.title;

        byId ('detail-category').textContent = categoryNames[script.category];
        byId ('detail-name').textContent = script.name;
        byId ('detail-summary').textContent = script.summary;
        addBadges (byId ('detail-badges'), script, categoryNames, true);

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
        byId ('detail-archicad').textContent = versionList (newest.archicadVersions);
        byId ('detail-tapir').textContent = newest.minTapirVersion;
        byId ('detail-license').textContent = script.license;
        var nameRefs = document.querySelectorAll ('.detail-name-ref');
        for (var n = 0; n < nameRefs.length; n++) {
            nameRefs[n].textContent = script.name;
        }
        if (script.changes === 'model') {
            byId ('detail-changes-model').hidden = false;
        } else if (script.changes) {
            byId ('detail-changes-read-only').hidden = false;
        }

        var report = byId ('detail-report');
        var reportValues = { name: script.name, slug: script.slug, version: newest.version };
        report.href = GITHUB_BASE + 'issues/new?title=' + encodeURIComponent (format (report.getAttribute ('data-title'), reportValues)) +
            '&body=' + encodeURIComponent (format (report.getAttribute ('data-body'), reportValues));

        var versionTemplate = byId ('script-version-template');
        script.versions.forEach (function (version) {
            var item = versionTemplate.content.firstElementChild.cloneNode (true);
            item.querySelector ('.version-number').textContent = version.version;
            item.querySelector ('.version-archicad').textContent = versionList (version.archicadVersions);
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
            renderMarkdown (readme, markdown, { raw: rawFolder, page: pageFolder });
            if (readme.firstChild) {
                byId ('detail-readme-section').hidden = false;
            }
        }).catch (function () {});

        if (script.screenshots.length) {
            var gallery = byId ('detail-screenshots');
            script.screenshots.forEach (function (file, index) {
                var url = rawFolder + 'screenshots/' + encodeURIComponent (file);
                var link = document.createElement ('a');
                link.href = url;
                var image = document.createElement ('img');
                image.src = url;
                image.alt = format (gallery.getAttribute ('data-alt'), { number: index + 1, name: script.name });
                image.loading = 'lazy';
                image.addEventListener ('error', function () { link.remove (); });
                link.appendChild (image);
                gallery.appendChild (link);
            });
            byId ('detail-screenshots-section').hidden = false;
        }

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
        var categoryNames = categoryNamesFromPage ();
        fetchResponse (CATALOGUE_URL).then (function (response) {
            return response.json ();
        }).then (function (catalogue) {
            var scripts = [];
            var seen = {};
            if (catalogue && catalogue.version === 1 && Array.isArray (catalogue.scripts)) {
                catalogue.scripts.forEach (function (entry) {
                    var script = cleanScript (entry, categoryNames);
                    if (script && !seen[script.slug]) {
                        seen[script.slug] = true;
                        scripts.push (script);
                    }
                });
            }
            if (wanted !== null) {
                var found = scripts.filter (function (script) { return script.slug === wanted; })[0];
                if (found) {
                    showScript (found, categoryNames);
                } else {
                    showNotFound ();
                }
            } else if (scripts.length) {
                showList (scripts, categoryNames);
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
