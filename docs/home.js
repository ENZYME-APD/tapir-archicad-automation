/* The home page (/): the live figures, the floor-plan demo and the command ticker.
   The page is complete without it. Every figure is hidden in the HTML and only shown once it was
   read, so the page never shows a wrong number; none of them uses GitHub's API. Every text it shows
   is in the page's HTML. Under prefers-reduced-motion the demo shows each result at once and the
   ticker stands still. */
(function () {
    'use strict';

    var REPO = 'ENZYME-APD/tapir-archicad-automation';
    var RAW_BASE = 'https://raw.githubusercontent.com/' + REPO + '/';
    var SLUG_PATTERN = /^[a-z0-9]+(-[a-z0-9]+)*$/;

    function byId (id) {
        return document.getElementById (id);
    }

    function reducedMotion () {
        return !!(window.matchMedia && window.matchMedia ('(prefers-reduced-motion: reduce)').matches);
    }

    // ---- Live figures ----

    function show (id, number, label) {
        byId (id + '-number').textContent = number;
        if (label) {
            byId (id + '-label').textContent = label;
        }
        byId (id).hidden = false;
    }

    function fetchJson (url) {
        return fetch (url, { cache: 'no-cache' }).then (function (response) {
            if (!response.ok) {
                throw new Error (String (response.status));
            }
            return response.json ();
        });
    }

    function loadFigures () {
        // Commands: counted from the command reference's own data, published with this site.
        var script = document.createElement ('script');
        script.src = 'archicad-addon/command_definitions.js';
        script.async = true;
        script.onload = function () {
            try {
                var groups = window.gCommands;
                if (!Array.isArray (groups)) {
                    return;
                }
                var count = 0;
                for (var i = 0; i < groups.length; i++) {
                    if (!groups[i] || !Array.isArray (groups[i].commands)) {
                        return;
                    }
                    count += groups[i].commands.length;
                }
                if (count > 0) {
                    show ('stat-commands', count.toLocaleString ('en-US'));
                }
            } catch (e) {}
        };
        document.body.appendChild (script);

        if (!window.fetch) {
            return;
        }

        // Supported Archicad versions: the list the builds and the release use.
        fetchJson (RAW_BASE + 'main/archicad-addon/devkits.json').then (function (data) {
            var versions = (data && Array.isArray (data.devkits) ? data.devkits : []).map (function (devkit) {
                return devkit ? devkit.archicad : null;
            }).filter (function (version) {
                return Number.isInteger (version);
            });
            if (versions.length === 0) {
                return;
            }
            versions.sort (function (a, b) { return a - b; });
            var range = String (versions[0]) + (versions.length > 1 ? '–' + versions[versions.length - 1] : '');
            show ('stat-versions', String (versions.length), 'Supported Archicad versions (' + range + ')');
            byId ('hero-versions-range').textContent = range;
            byId ('hero-versions').hidden = false;
        }).catch (function () {});

        // Archicad API coverage: the badge the coverage workflow publishes, e.g. "99 % (2177/2179)".
        fetchJson (RAW_BASE + 'api-coverage/api-coverage-badge.json').then (function (data) {
            var match = data && typeof data.message === 'string' ? /^\s*(\d+(?:\.\d+)?)\s*%/.exec (data.message) : null;
            if (match) {
                show ('stat-coverage', match[1] + '%');
            }
        }).catch (function () {});

        // Marketplace scripts: the distinct slugs in the catalogue on the site-data branch. Left out
        // until the catalogue exists and lists at least one script.
        fetchJson (RAW_BASE + 'site-data/catalogue.json').then (function (catalogue) {
            if (!catalogue || catalogue.version !== 1 || !Array.isArray (catalogue.scripts)) {
                return;
            }
            var seen = {};
            var count = 0;
            catalogue.scripts.forEach (function (entry) {
                if (entry && typeof entry.slug === 'string' && SLUG_PATTERN.test (entry.slug) && !seen[entry.slug]) {
                    seen[entry.slug] = true;
                    count++;
                }
            });
            if (count > 0) {
                show ('stat-scripts', count.toLocaleString ('en-US'));
            }
        }).catch (function () {});
    }

    // ---- The floor-plan demo ----

    function setUpPlan () {
        var tabs = byId ('plan-tabs');
        var plan = byId ('plan');
        if (!tabs || !plan) {
            return;
        }
        var buttons = tabs.querySelectorAll ('button');
        var zones = plan.querySelectorAll ('.zone');
        var tags = plan.querySelectorAll ('.tagnum');
        var numbering = byId ('plan-numbering');
        var check = byId ('plan-check');
        var legend = byId ('plan-legend');
        var status = byId ('plan-status');
        var statusText = byId ('plan-status-text');
        var runningText = plan.getAttribute ('data-running') || '';
        var timers = [];

        function heatColor (area) {
            return area < 15 ? '#E9DCC6' : area <= 25 ? '#D4A876' : '#8A6235';
        }

        function setStatus (name, text) {
            statusText.textContent = '';
            var bold = document.createElement ('b');
            bold.textContent = name;
            statusText.appendChild (bold);
            statusText.appendChild (document.createTextNode (' · ' + text));
        }

        function later (fn, ms) {
            timers.push (setTimeout (fn, ms));
        }

        function colourZones () {
            for (var i = 0; i < zones.length; i++) {
                zones[i].setAttribute ('fill', heatColor (Number (zones[i].getAttribute ('data-area'))));
            }
        }

        function run (button) {
            var kind = button.getAttribute ('data-demo');
            var name = button.getAttribute ('data-name');
            var done = button.getAttribute ('data-done');
            timers.forEach (clearTimeout);
            timers = [];
            status.classList.remove ('running');

            for (var i = 0; i < zones.length; i++) {
                zones[i].setAttribute ('fill', '#FFFFFF');
            }
            for (var j = 0; j < tags.length; j++) {
                tags[j].classList.remove ('hidden-tag');
            }
            numbering.style.display = kind === 'number' ? '' : 'none';
            check.style.display = kind === 'check' ? '' : 'none';
            legend.style.display = kind === 'heat' ? '' : 'none';

            if (reducedMotion ()) {
                if (kind === 'heat') {
                    colourZones ();
                }
                setStatus (name, done);
                return;
            }

            status.classList.add ('running');
            setStatus (name, runningText);
            if (kind === 'number') {
                Array.prototype.forEach.call (tags, function (tag, index) {
                    tag.classList.add ('hidden-tag');
                    later (function () { tag.classList.remove ('hidden-tag'); }, 300 + index * 260);
                });
            } else if (kind === 'heat') {
                Array.prototype.forEach.call (zones, function (zone, index) {
                    later (function () {
                        zone.setAttribute ('fill', heatColor (Number (zone.getAttribute ('data-area'))));
                    }, 200 + index * 180);
                });
            }
            later (function () {
                status.classList.remove ('running');
                setStatus (name, done);
            }, 2000);
        }

        Array.prototype.forEach.call (buttons, function (button) {
            button.addEventListener ('click', function () {
                Array.prototype.forEach.call (buttons, function (other) {
                    other.setAttribute ('aria-pressed', String (other === button));
                });
                run (button);
            });
        });
        tabs.hidden = false;
    }

    // ---- The command ticker: its names twice, so the loop has no seam. ----

    function setUpTicker () {
        var track = byId ('ticker-track');
        if (!track || reducedMotion ()) {
            return;
        }
        var names = Array.prototype.slice.call (track.children);
        names.forEach (function (name) {
            track.appendChild (name.cloneNode (true));
        });
        track.classList.add ('moving');
    }

    function init () {
        loadFigures ();
        setUpPlan ();
        setUpTicker ();
    }

    if (document.readyState === 'loading') {
        document.addEventListener ('DOMContentLoaded', init);
    } else {
        init ();
    }
}) ();
