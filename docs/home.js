/* The home page (/): the floor-plan demo and the command ticker. The live figures are read by
   assets/site.js. The page is complete without it: without JavaScript the plan shows the numbered
   doors and the tabs stay hidden. Every text it shows is in the page's HTML. Under
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
        setUpPlan ();
        setUpTicker ();
    }

    if (document.readyState === 'loading') {
        document.addEventListener ('DOMContentLoaded', init);
    } else {
        init ();
    }
}) ();
