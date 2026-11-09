/* The home page (/): the floor-plan demo and the command ticker. The live figures are read by
   assets/site.js. The page is complete without it: without JavaScript the plan shows the numbered
   doors and the palette bar (script menu and Run) stays hidden. Every text it shows is in the
   page's HTML, translated there (data-i18n, see assets/i18n/README.md); the script names stay
   English, as in the palette. Under
   prefers-reduced-motion the demo shows each result at once and the ticker stands still. */
(function () {
    'use strict';

    function byId (id) {
        return document.getElementById (id);
    }

    function reducedMotion () {
        return !!(window.matchMedia && window.matchMedia ('(prefers-reduced-motion: reduce)').matches);
    }

    // ---- The floor-plan demo ----

    function setUpPlan () {
        var bar = byId ('plan-bar');
        var select = byId ('plan-script');
        var runButton = byId ('plan-run');
        var plan = byId ('plan');
        if (!bar || !select || !runButton || !plan) {
            return;
        }
        var zones = plan.querySelectorAll ('.zone');
        var tags = plan.querySelectorAll ('.tagnum');
        var numbering = byId ('plan-numbering');
        var check = byId ('plan-check');
        var legend = byId ('plan-legend');
        var status = byId ('plan-status');
        var statusText = byId ('plan-status-text');
        var runningText = plan.getAttribute ('data-running') || '';
        var timers = [];

        // Moss shades: the Property Highlighter only colours the view. Ochre is kept for what a
        // script changes in the model (the door numbers) and for the check's markers. The colours
        // are home.css's --plan-heat-* properties, given by these classes, so a zone follows the
        // site's look when it changes without being drawn again.
        var heatClasses = ['heat-low', 'heat-mid', 'heat-high'];

        function heatClass (area) {
            return area < 15 ? 'heat-low' : area <= 25 ? 'heat-mid' : 'heat-high';
        }

        function colourZone (zone) {
            zone.classList.add (heatClass (Number (zone.getAttribute ('data-area'))));
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

        // Each label of the check gets a box as wide as its (translated) text, on the side of its
        // marker that data-side names, kept inside the plan. Measured while the check is shown.
        function fitLabels () {
            Array.prototype.forEach.call (check.querySelectorAll ('.plan-label'), function (label) {
                var box = label.querySelector ('rect');
                var text = label.querySelector ('text');
                if (!box || !text || !text.getComputedTextLength) {
                    return;
                }
                var width = Math.ceil (text.getComputedTextLength ()) + 18;
                var x = Number (label.getAttribute ('data-x'));
                var left = label.getAttribute ('data-side') === 'left' ? x - width : x;
                left = Math.max (4, Math.min (left, 556 - width));
                box.setAttribute ('x', left);
                box.setAttribute ('width', width);
                text.setAttribute ('x', left + 8);
            });
        }

        function colourZones () {
            Array.prototype.forEach.call (zones, colourZone);
        }

        function run (option) {
            var kind = option.value;
            var name = option.textContent;
            var done = option.getAttribute ('data-done');
            timers.forEach (clearTimeout);
            timers = [];
            status.classList.remove ('running');
            runButton.removeAttribute ('aria-busy');
            plan.classList.toggle ('heat', kind === 'heat');

            for (var i = 0; i < zones.length; i++) {
                zones[i].classList.remove.apply (zones[i].classList, heatClasses);
            }
            for (var j = 0; j < tags.length; j++) {
                tags[j].classList.remove ('hidden-tag');
            }
            numbering.style.display = kind === 'number' ? '' : 'none';
            check.style.display = kind === 'check' ? '' : 'none';
            legend.style.display = kind === 'heat' ? '' : 'none';
            if (kind === 'check') {
                fitLabels ();
                // Measured again once the web fonts are in: until then the text is set in a
                // narrower fallback font, and the box would come out too short (seen on phones).
                if (document.fonts && document.fonts.ready) {
                    document.fonts.ready.then (fitLabels);
                }
            }

            if (reducedMotion ()) {
                if (kind === 'heat') {
                    colourZones ();
                }
                setStatus (name, done);
                return;
            }

            status.classList.add ('running');
            runButton.setAttribute ('aria-busy', 'true');
            setStatus (name, runningText);
            if (kind === 'number') {
                Array.prototype.forEach.call (tags, function (tag, index) {
                    tag.classList.add ('hidden-tag');
                    later (function () { tag.classList.remove ('hidden-tag'); }, 300 + index * 260);
                });
            } else if (kind === 'heat') {
                Array.prototype.forEach.call (zones, function (zone, index) {
                    later (function () { colourZone (zone); }, 200 + index * 180);
                });
            }
            later (function () {
                status.classList.remove ('running');
                runButton.removeAttribute ('aria-busy');
                setStatus (name, done);
            }, 2000);
        }

        // As in the Tapir palette: the menu picks a script, Run runs it.
        runButton.addEventListener ('click', function () {
            var option = select.options[select.selectedIndex];
            if (option) {
                run (option);
            }
        });
        if (document.fonts && document.fonts.addEventListener) {
            document.fonts.addEventListener ('loadingdone', function () {
                if (check.style.display !== 'none') {
                    fitLabels ();
                }
            });
        }
        bar.hidden = false;
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
        // The demo reads its texts (data-running, data-done) from the HTML, so it waits until
        // assets/site.js has translated them.
        if (window.TapirI18n) {
            window.TapirI18n.ready (function () {
                setUpPlan ();
                setUpTicker ();
            });
            return;
        }
        setUpPlan ();
        setUpTicker ();
    }

    if (document.readyState === 'loading') {
        document.addEventListener ('DOMContentLoaded', init);
    } else {
        init ();
    }
}) ();
