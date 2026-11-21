/* The Community page (/community/): the Momentum figures, the top contributors and the
   growth chart. It reads community.json from the site-data branch on raw.githubusercontent.com
   (never GitHub's API), written daily by tools/generate_community_data.py
   (.github/workflows/site_data.yml). Only counts, never the text of a message:
       {"version": 1,
        "months": [{"month": "2026-10", "commits": 1861, "issuesOpened": 41, "pullRequestsMerged": 37,
                    "discordMessages": 512, "discordMembers": 637}, ...],
        "totals": {"commits": 4321, "discordMessages": 12345, "discordMembers": 678,
                   "discordMembersChange": {"diff": 23, "days": 30}, "participants": 234},
        "topContributors": [{"name": "Jane Doe", "login": "janedoe", "commits": 120, "reports": 6,
                             "tests": 3}, ...]}
   Every key but "version" is optional, and so is every key inside "totals".
   months: the months follow one another without gaps, oldest first, from the first month of the
   project; the current, unfinished month is left out, or its values read as a drop. The chart
   stacks commits, issues opened plus pull requests merged, and Discord messages per month, and
   draws discordMembers (the members at the end of each month) as a line on its own scale on the
   right. A series is drawn only when every month has a whole number of at least 0 for it; the
   table under the chart lists every number. The chart reads out one month at a time: the pointer
   or a finger snaps to the nearest month, the Left/Right arrow keys move month by month, Home and
   End jump to the first and last month, and Esc, or a tap or click elsewhere, hides it. It shows a
   guide line, a dot on each drawn series and a tooltip with the month's values (a value the data
   does not have reads "–", never 0) and the total of the stack; a key or a tap also writes them to
   the aria-live region #growth-readout. It rests on the latest month when the page opens. Each
   legend entry is a toggle button (aria-pressed) that hides or shows its series, and the stack and
   its left scale are drawn again for the layers shown; the last stacked layer shown stays on.
   totals: each figure is shown only when it is a whole number of at least 0. discordMembersChange
   is the change in members between the two member counts compared, "days" apart ("+23 in 30
   days"); it is left out unless both are whole numbers and days is at least 1.
   topContributors: per person, "commits" (co-authored commits included), "reports" (issues they
   opened that a merged fix closed) and "tests" (pull requests they tested in Archicad, passed or
   failed), and "login", their GitHub login, which links the name to their GitHub profile. Every
   one of these is optional: an older file has only "name" and "commits", and then only the
   commits are shown. A count that is not a whole number of at least 0 counts as 0, and an entry
   without any count is left out. Bots are left out (Claude, claude[bot], github-actions[bot] and
   any name or login ending in [bot]), and a person listed under two names is counted once: names
   that only differ in case or accents are one person, and so are the names in ALIASES. The five
   with the most commits, reports and tests together are shown, each with the counts above 0 as
   separate numbers ("44 commits · 11 reports · 24 tests"), never their sum.
   When the file cannot be read, or a part of it does not look right, that part stays hidden: a
   number that can't be read is left out, not shown wrong. Every text shown is in the page's HTML
   (data-change, data-commit(s), data-report(s), data-test(s), data-chart-label, data-months and
   data-month-label and data-total-label among them), and assets/site.js translates it
   (assets/i18n/<language>/community.js), so this starts once the page is translated
   (TapirI18n.ready) and then reads the page's language and those texts. */
(function () {
    'use strict';

    var COMMUNITY_URL = 'https://raw.githubusercontent.com/ENZYME-APD/tapir-archicad-automation/site-data/community.json';
    var MONTH_PATTERN = /^([0-9]{4})-(0[1-9]|1[0-2])$/;
    var SVG_NS = 'http://www.w3.org/2000/svg';
    // Numbers are written the way the page's language writes them (1,234 in English, 1 234 in French);
    // read in start (), once the translation has set <html lang>.
    var LOCALE = 'en';

    // Names one person committed under, by their key (see personKey), and the name to show.
    var ALIASES = { 'tlorantfy': 'Tibor Lorántfy' };

    var section = document.getElementById ('activity-section');
    var momentum = document.getElementById ('momentum-section');
    if (!section || !momentum || !window.fetch || !window.matchMedia) {
        return;
    }
    var table = document.getElementById ('activity-table');
    // The month names and how a month and its year are written ("{month} {year}"), read in start ().
    var monthNames = [];
    var monthLabelPattern = '{month} {year}';
    var phone = window.matchMedia ('(max-width: 640px)');
    var allMonths = null;

    function format (text, values) {
        return String (text || '').replace (/\{(\w+)\}/g, function (match, key) {
            return Object.prototype.hasOwnProperty.call (values, key) ? String (values[key]) : match;
        });
    }

    function isCount (value) {
        return typeof value === 'number' && isFinite (value) && value >= 0 && Math.floor (value) === value;
    }

    function monthName (month) {
        var parts = MONTH_PATTERN.exec (month);
        var name = monthNames[parseInt (parts[2], 10) - 1];
        return name || parts[2];
    }

    function monthNumber (month) {
        var parts = MONTH_PATTERN.exec (month);
        return parseInt (parts[1], 10) * 12 + parseInt (parts[2], 10) - 1;
    }

    function monthLabel (month) {
        return format (monthLabelPattern, { month: monthName (month), year: MONTH_PATTERN.exec (month)[1] });
    }

    // The months of the file, oldest first, or null when they don't look right.
    function readMonths (data) {
        if (!data || data.version !== 1 || !Array.isArray (data.months) || data.months.length === 0) {
            return null;
        }
        var months = [];
        for (var i = 0; i < data.months.length; i++) {
            var entry = data.months[i];
            if (!entry || typeof entry !== 'object' || typeof entry.month !== 'string' || !MONTH_PATTERN.test (entry.month)) {
                return null;
            }
            months.push (entry);
        }
        months.sort (function (a, b) { return a.month < b.month ? -1 : (a.month > b.month ? 1 : 0); });
        // Months one after another, without gaps or repeats, so every bar stands for its month.
        for (var j = 1; j < months.length; j++) {
            if (monthNumber (months[j].month) !== monthNumber (months[j - 1].month) + 1) {
                return null;
            }
        }
        return months;
    }

    function svgElement (name, attributes, text) {
        var element = document.createElementNS (SVG_NS, name);
        Object.keys (attributes).forEach (function (key) {
            element.setAttribute (key, String (attributes[key]));
        });
        if (text !== undefined) {
            element.textContent = text;
        }
        return element;
    }

    // The stacked series, bottom to top, and the line on its own scale. "github" is the sum of
    // issuesOpened and pullRequestsMerged, drawn only when both are there.
    var LAYERS = ['commits', 'github', 'discordMessages'];
    var LINE = 'discordMembers';

    function valueOf (month, key) {
        if (key === 'github') {
            return isCount (month.issuesOpened) && isCount (month.pullRequestsMerged) ? month.issuesOpened + month.pullRequestsMerged : null;
        }
        return isCount (month[key]) ? month[key] : null;
    }

    function complete (months, key) {
        return months.every (function (month) { return valueOf (month, key) !== null; });
    }

    // A round top for an axis, at least the largest value and close above it.
    function niceMax (value) {
        if (value <= 0) {
            return 1;
        }
        var power = Math.pow (10, Math.floor (Math.log10 (value)));
        var steps = [1, 1.2, 1.5, 2, 2.5, 3, 4, 5, 6, 8, 10];
        for (var i = 0; i < steps.length; i++) {
            if (steps[i] * power >= value) {
                return steps[i] * power;
            }
        }
        return 10 * power;
    }

    // The nearest value of the 1, 2, 5 series, for the ticks of the square-root scale.
    function niceRound (value) {
        if (value <= 0) {
            return 0;
        }
        var power = Math.pow (10, Math.floor (Math.log10 (value)));
        var best = power;
        [1, 2, 5, 10].forEach (function (step) {
            if (Math.abs (step * power - value) < Math.abs (best - value)) {
                best = step * power;
            }
        });
        return best;
    }

    function shortNumber (value) {
        return value >= 10000 ? Math.round (value / 1000) + 'k' : value.toLocaleString (LOCALE);
    }

    // A smooth path through the points that never overshoots them (monotone cubic, Fritsch and
    // Carlson), so a month is never drawn above or below its count. 'M' starts it, 'L' joins it on.
    function smoothPath (points, start) {
        var n = points.length;
        var d = start + points[0][0].toFixed (1) + ',' + points[0][1].toFixed (1);
        if (n < 3) {
            for (var k = 1; k < n; k++) {
                d += 'L' + points[k][0].toFixed (1) + ',' + points[k][1].toFixed (1);
            }
            return d;
        }
        var slopes = [];
        for (var i = 0; i < n - 1; i++) {
            slopes.push ((points[i + 1][1] - points[i][1]) / (points[i + 1][0] - points[i][0]));
        }
        // The tangent at each point: the harmonic mean of its two slopes (0 at a peak or a
        // trough), which keeps the curve monotone; the end points take their one slope.
        var tangents = [slopes[0]];
        for (var m = 1; m < n - 1; m++) {
            tangents.push (slopes[m - 1] * slopes[m] <= 0 ? 0 : 2 / (1 / slopes[m - 1] + 1 / slopes[m]));
        }
        tangents.push (slopes[n - 2]);
        for (var t = 0; t < n - 1; t++) {
            var dx = (points[t + 1][0] - points[t][0]) / 3;
            d += 'C' + (points[t][0] + dx).toFixed (1) + ',' + (points[t][1] + tangents[t] * dx).toFixed (1) + ' ' +
                (points[t + 1][0] - dx).toFixed (1) + ',' + (points[t + 1][1] - tangents[t + 1] * dx).toFixed (1) + ' ' +
                points[t + 1][0].toFixed (1) + ',' + points[t + 1][1].toFixed (1);
        }
        return d;
    }

    function drawGrowth (container, months, layers, withLine) {
        var small = phone.matches;
        var width = small ? 380 : 760;
        var height = small ? 280 : 360;
        var left = small ? 44 : 48;
        var right = withLine ? (small ? 36 : 48) : 12;
        var top = 16;
        var bottom = height - 34;
        var plotWidth = width - left - right;
        var count = months.length;
        var x = function (index) { return left + (count > 1 ? plotWidth * index / (count - 1) : plotWidth / 2); };

        var stacks = months.map (function (month) {
            var sum = 0;
            return layers.map (function (key) {
                var below = sum;
                sum += valueOf (month, key);
                return [below, sum];
            });
        });
        // The left scale grows by the square root: a month with a quarter of the top count is
        // drawn at half the height, so the early years stay readable next to the latest months.
        var leftMax = niceMax (Math.max.apply (null, stacks.map (function (stack) { return stack[stack.length - 1][1]; })));
        var y = function (value) { return bottom - (bottom - top) * Math.sqrt (value / leftMax); };

        var svg = svgElement ('svg', {
            viewBox: '0 0 ' + width + ' ' + height,
            role: 'img',
            'aria-label': format (section.getAttribute ('data-chart-label'), {
                first: monthLabel (months[0].month),
                last: monthLabel (months[count - 1].month)
            })
        });

        // One gradient per layer: full colour at the top, lighter towards the axis.
        var defs = svgElement ('defs', {});
        layers.forEach (function (key) {
            var gradient = svgElement ('linearGradient', { id: 'growth-fill-' + key, x1: 0, x2: 0, y1: 0, y2: 1 });
            gradient.appendChild (svgElement ('stop', { offset: '0%', 'class': 'stop stop-' + key }));
            gradient.appendChild (svgElement ('stop', { offset: '100%', 'class': 'stop stop-' + key + ' stop-low' }));
            defs.appendChild (gradient);
        });
        svg.appendChild (defs);

        // Grid and left scale: 0, then the round values at about a quarter, half and all of the height.
        var ticks = [0];
        [0.0625, 0.25, 1].forEach (function (share) {
            var value = share === 1 ? leftMax : niceRound (leftMax * share);
            if (value > ticks[ticks.length - 1]) {
                ticks.push (value);
            }
        });
        ticks.forEach (function (value) {
            var level = y (value);
            svg.appendChild (svgElement ('line', { 'class': value === 0 ? 'axis' : 'grid', x1: left, x2: width - right, y1: level.toFixed (1), y2: level.toFixed (1) }));
            svg.appendChild (svgElement ('text', { x: left - 6, y: (level + 4).toFixed (1), 'text-anchor': 'end' }, shortNumber (value)));
        });

        // The stacked areas, top layer last so its edge reads on top.
        layers.forEach (function (key, layer) {
            var upper = months.map (function (month, index) { return [x (index), y (stacks[index][layer][1])]; });
            var lower = months.map (function (month, index) { return [x (index), y (stacks[index][layer][0])]; }).reverse ();
            svg.appendChild (svgElement ('path', {
                'class': 'layer layer-' + key,
                fill: 'url(#growth-fill-' + key + ')',
                d: smoothPath (upper, 'M') + smoothPath (lower, 'L') + 'Z'
            }));
            svg.appendChild (svgElement ('path', { 'class': 'edge edge-' + key, d: smoothPath (upper, 'M') }));
        });

        // The line on its own, linear right scale.
        var lineMax = 1;
        var yLine = null;
        if (withLine) {
            lineMax = niceMax (Math.max.apply (null, months.map (function (month) { return month[LINE]; })));
            yLine = function (value) { return bottom - (bottom - top) * value / lineMax; };
            [0.5, 1].forEach (function (share) {
                svg.appendChild (svgElement ('text', { 'class': 'line-scale', x: width - right + 6, y: (yLine (lineMax * share) + 4).toFixed (1) }, shortNumber (lineMax * share)));
            });
            var points = months.map (function (month, index) { return [x (index), yLine (month[LINE])]; });
            svg.appendChild (svgElement ('path', { 'class': 'line-halo', d: smoothPath (points, 'M') }));
            svg.appendChild (svgElement ('path', { 'class': 'line', d: smoothPath (points, 'M') }));
            var lastValue = months[count - 1][LINE];
            svg.appendChild (svgElement ('circle', { 'class': 'line-dot', cx: x (count - 1).toFixed (1), cy: yLine (lastValue).toFixed (1), r: 4 }));
        }

        // A year under each January and under the first month, unless the next January's year
        // would touch it.
        var lastLabel = -Infinity;
        months.forEach (function (month, index) {
            var parts = MONTH_PATTERN.exec (month.month);
            var nextJanuary = parts[2] === '01' ? index : index + 13 - parseInt (parts[2], 10);
            if (index === 0 && nextJanuary < count && x (nextJanuary) - x (0) < 64) {
                return;
            }
            if ((index === 0 || parts[2] === '01') && x (index) - lastLabel >= 34) {
                lastLabel = x (index);
                svg.appendChild (svgElement ('line', { 'class': 'tick', x1: x (index).toFixed (1), x2: x (index).toFixed (1), y1: bottom, y2: bottom + 5 }));
                svg.appendChild (svgElement ('text', { x: x (index).toFixed (1), y: bottom + 20, 'text-anchor': index === 0 ? 'start' : 'middle' }, parts[1]));
            }
        });

        // The read-out's guide line and dots, drawn on top and moved by showReadout ().
        var readout = svgElement ('g', { 'class': 'readout', 'aria-hidden': 'true', visibility: 'hidden' });
        readout.appendChild (svgElement ('line', { 'class': 'guide', x1: 0, x2: 0, y1: top, y2: bottom }));
        layers.forEach (function (key) {
            readout.appendChild (svgElement ('circle', { 'class': 'focus-dot focus-' + key, 'data-key': key, r: 4 }));
        });
        if (withLine) {
            readout.appendChild (svgElement ('circle', { 'class': 'focus-dot focus-' + LINE, 'data-key': LINE, r: 4 }));
        }
        svg.appendChild (readout);

        container.textContent = '';
        container.appendChild (svg);
        return {
            svg: svg, readout: readout, months: months, layers: layers, withLine: withLine, stacks: stacks,
            width: width, left: left, plotWidth: plotWidth, x: x, y: y, yLine: withLine ? yLine : null
        };
    }

    function fillTable (months, keys) {
        var headRow = table.tHead.rows[0];
        Array.prototype.forEach.call (headRow.querySelectorAll ('th[data-key]'), function (th) {
            th.hidden = keys.indexOf (th.getAttribute ('data-key')) < 0;
        });
        var body = table.tBodies[0];
        body.textContent = '';
        months.slice ().reverse ().forEach (function (month) {
            var row = body.insertRow ();
            var th = document.createElement ('th');
            th.scope = 'row';
            th.textContent = monthLabel (month.month);
            row.appendChild (th);
            keys.forEach (function (key) {
                row.insertCell ().textContent = month[key].toLocaleString (LOCALE);
            });
        });
    }

    // ---- The read-out: the numbers of one month, by pointer, touch or keyboard ----

    // The series the legend hides (key: true), the month the read-out shows (its index, or null),
    // the chart drawn last (what drawGrowth returns) and the series the data has.
    var hiddenKeys = {};
    var selected = null;
    var chart = null;
    var drawable = { layers: [], withLine: false };

    // The name of a series as the page writes it: the table's heading, or the legend's for the
    // stacked issues and pull requests.
    function seriesName (key) {
        var element = key === 'github' ?
            section.querySelector ('.growth-legend [data-key="github"] button') :
            table.tHead.querySelector ('th[data-key="' + key + '"]');
        return element ? element.textContent.replace (/\s+/g, ' ').trim () : key;
    }

    function readoutValue (value) {
        return value === null ? '–' : value.toLocaleString (LOCALE);
    }

    // The rows of the read-out for one month: every series drawn, and a series the data has for
    // only some months, whose missing months read "–", never 0. Then the total of the stack drawn.
    function readoutRows (index) {
        var month = chart.months[index];
        var rows = [];
        LAYERS.concat ([LINE]).forEach (function (key) {
            var drawn = key === LINE ? chart.withLine : chart.layers.indexOf (key) >= 0;
            var partial = drawable.layers.indexOf (key) < 0 && !(key === LINE && drawable.withLine) &&
                chart.months.some (function (entry) { return valueOf (entry, key) !== null; });
            if (drawn || partial) {
                rows.push ({ key: key, name: seriesName (key), value: valueOf (month, key) });
            }
        });
        var stack = chart.stacks[index];
        rows.push ({ key: null, name: section.getAttribute ('data-total-label') || 'Total', value: stack[stack.length - 1][1] });
        return rows;
    }

    function swatch (key) {
        var element = document.createElement ('i');
        element.className = key === LINE ? 'swatch swatch-line' : 'swatch layer-' + key;
        return element;
    }

    // Shows the numbers of month index (the guide line, the dots and the tooltip); announce: also
    // writes them to the live region, for a screen reader.
    function showReadout (index, announce) {
        var count = chart.months.length;
        index = Math.max (0, Math.min (count - 1, index));
        selected = index;
        var at = chart.x (index);
        var guide = chart.readout.querySelector ('.guide');
        guide.setAttribute ('x1', at.toFixed (1));
        guide.setAttribute ('x2', at.toFixed (1));
        Array.prototype.forEach.call (chart.readout.querySelectorAll ('.focus-dot'), function (dot) {
            var key = dot.getAttribute ('data-key');
            var level = key === LINE ? chart.yLine (chart.months[index][LINE]) : chart.y (chart.stacks[index][chart.layers.indexOf (key)][1]);
            dot.setAttribute ('cx', at.toFixed (1));
            dot.setAttribute ('cy', level.toFixed (1));
        });
        chart.readout.setAttribute ('visibility', 'visible');

        var label = monthLabel (chart.months[index].month);
        var rows = readoutRows (index);
        var tip = document.getElementById ('growth-tip');
        tip.textContent = '';
        var title = document.createElement ('strong');
        title.textContent = label;
        tip.appendChild (title);
        rows.forEach (function (row) {
            var line = document.createElement ('span');
            line.className = row.key ? 'tip-row' : 'tip-row tip-total';
            var name = document.createElement ('span');
            name.className = 'tip-name';
            if (row.key) {
                name.appendChild (swatch (row.key));
            }
            name.appendChild (document.createTextNode (row.name));
            var value = document.createElement ('span');
            value.className = 'tip-value';
            value.textContent = readoutValue (row.value);
            line.appendChild (name);
            line.appendChild (value);
            tip.appendChild (line);
        });
        // Beside the guide line, never over it, and inside the chart: on its right, or on its left
        // in the right half, and at most as wide as the room on that side.
        var share = at / chart.width * 100;
        var flip = share > 50;
        var room = flip ? share : 100 - share;
        tip.style.left = flip ? 'auto' : 'calc(' + share.toFixed (2) + '% + 10px)';
        tip.style.right = flip ? 'calc(' + (100 - share).toFixed (2) + '% + 10px)' : 'auto';
        tip.style.maxWidth = 'calc(' + room.toFixed (2) + '% - 14px)';
        tip.hidden = false;

        if (announce) {
            document.getElementById ('growth-readout').textContent = label + ': ' + rows.map (function (row) {
                return row.name + ' ' + readoutValue (row.value);
            }).join ('; ');
        }
    }

    function hideReadout () {
        selected = null;
        if (chart) {
            chart.readout.setAttribute ('visibility', 'hidden');
        }
        var tip = document.getElementById ('growth-tip');
        if (tip) {
            tip.hidden = true;
        }
    }

    // The month nearest to a pointer.
    function monthAt (event) {
        var box = chart.svg.getBoundingClientRect ();
        if (!box.width) {
            return null;
        }
        var position = (event.clientX - box.left) * chart.width / box.width;
        var count = chart.months.length;
        return count > 1 ? Math.round ((position - chart.left) / chart.plotWidth * (count - 1)) : 0;
    }

    function listenToChart (frame) {
        frame.addEventListener ('pointermove', function (event) {
            if (!chart || (event.pointerType !== 'mouse' && event.buttons === 0)) {
                return;
            }
            var index = monthAt (event);
            if (index !== null && index !== selected) {
                showReadout (index, false);
            }
        });
        frame.addEventListener ('pointerdown', function (event) {
            var index = chart ? monthAt (event) : null;
            if (index !== null) {
                showReadout (index, true);
            }
        });
        frame.addEventListener ('pointerleave', function (event) {
            if (event.pointerType === 'mouse' && document.activeElement !== frame) {
                hideReadout ();
            }
        });
        frame.addEventListener ('keydown', function (event) {
            if (!chart) {
                return;
            }
            var last = chart.months.length - 1;
            var from = selected === null ? last : selected;
            var index = null;
            if (event.key === 'ArrowLeft') {
                index = selected === null ? last : from - 1;
            } else if (event.key === 'ArrowRight') {
                index = selected === null ? last : from + 1;
            } else if (event.key === 'Home') {
                index = 0;
            } else if (event.key === 'End') {
                index = last;
            } else if (event.key === 'Escape') {
                if (selected !== null) {
                    hideReadout ();
                    document.getElementById ('growth-readout').textContent = '';
                    event.preventDefault ();
                }
                return;
            } else {
                return;
            }
            event.preventDefault ();
            showReadout (index, true);
        });
        // A tap or a click anywhere else hides it.
        document.addEventListener ('pointerdown', function (event) {
            if (selected !== null && !frame.contains (event.target) && !event.target.closest ('.growth-legend')) {
                hideReadout ();
            }
        });
    }

    // A legend entry hides or shows its series; at least one stacked layer stays shown.
    function listenToLegend () {
        Array.prototype.forEach.call (section.querySelectorAll ('.growth-legend [data-key] button'), function (button) {
            button.addEventListener ('click', function () {
                var key = button.closest ('[data-key]').getAttribute ('data-key');
                if (!hiddenKeys[key] && key !== LINE) {
                    var shown = drawable.layers.filter (function (layer) { return !hiddenKeys[layer]; });
                    if (shown.length <= 1) {
                        return;
                    }
                }
                hiddenKeys[key] = !hiddenKeys[key];
                render ();
            });
        });
    }

    function render () {
        if (!allMonths) {
            return;
        }
        var months = allMonths;
        drawable = {
            layers: LAYERS.filter (function (key) { return complete (months, key); }),
            withLine: complete (months, LINE)
        };
        if (drawable.layers.length === 0) {
            section.hidden = true;
            return;
        }
        var layers = drawable.layers.filter (function (key) { return !hiddenKeys[key]; });
        var withLine = drawable.withLine && !hiddenKeys[LINE];
        var shownLayers = layers.length;
        Array.prototype.forEach.call (section.querySelectorAll ('.growth-legend [data-key]'), function (item) {
            var key = item.getAttribute ('data-key');
            item.hidden = key === LINE ? !drawable.withLine : drawable.layers.indexOf (key) < 0;
            var button = item.querySelector ('button');
            if (button) {
                button.setAttribute ('aria-pressed', hiddenKeys[key] ? 'false' : 'true');
                // The last stacked layer shown cannot be hidden.
                button.setAttribute ('aria-disabled', key !== LINE && !hiddenKeys[key] && shownLayers <= 1 ? 'true' : 'false');
            }
        });
        var frame = document.getElementById ('growth-chart');
        chart = drawGrowth (frame, months, layers, withLine);
        var tip = document.createElement ('div');
        tip.id = 'growth-tip';
        tip.className = 'growth-tip';
        tip.setAttribute ('aria-hidden', 'true');
        tip.hidden = true;
        frame.appendChild (tip);
        if (selected !== null) {
            showReadout (selected, false);
        }
        var keys = ['commits', 'issuesOpened', 'pullRequestsMerged', 'discordMessages', LINE].filter (function (key) {
            return months.every (function (month) { return isCount (month[key]); });
        });
        fillTable (months, keys);
        section.hidden = false;
    }

    // ---- Momentum ----

    function personKey (name) {
        return name.normalize ('NFD').replace (/[\u0300-\u036f]/g, '').toLowerCase ().replace (/\s+/g, ' ').trim ();
    }

    function isBot (name) {
        var key = personKey (name);
        return key === 'claude' || /\[bot\]$/.test (key);
    }

    // What a contributor is counted for, in the order they are shown.
    var CONTRIBUTIONS = ['commits', 'reports', 'tests'];
    // A GitHub login: letters, digits and single hyphens, at most 39 characters.
    var LOGIN_PATTERN = /^[A-Za-z0-9](?:[A-Za-z0-9]|-(?=[A-Za-z0-9])){0,38}$/;

    function total (person) {
        return person.commits + person.reports + person.tests;
    }

    function topContributors (list) {
        if (!Array.isArray (list)) {
            return [];
        }
        var aliasOf = {};
        Object.keys (ALIASES).forEach (function (alias) {
            aliasOf[personKey (alias)] = ALIASES[alias];
        });
        var people = {};
        list.forEach (function (entry) {
            if (!entry || typeof entry.name !== 'string' || !entry.name.trim () || isBot (entry.name)) {
                return;
            }
            if (!CONTRIBUTIONS.some (function (kind) { return isCount (entry[kind]); })) {
                return;
            }
            if (typeof entry.login === 'string' && isBot (entry.login)) {
                return;
            }
            var login = typeof entry.login === 'string' && LOGIN_PATTERN.test (entry.login) ? entry.login : null;
            var alias = aliasOf[personKey (entry.name)];
            var name = alias || entry.name.trim ();
            var key = personKey (name);
            var person = people[key] || (people[key] = { name: name, login: null, commits: 0, reports: 0, tests: 0, shownBy: -1 });
            var counts = {};
            CONTRIBUTIONS.forEach (function (kind) {
                counts[kind] = isCount (entry[kind]) ? entry[kind] : 0;
                person[kind] += counts[kind];
            });
            var weight = counts.commits + counts.reports + counts.tests;
            // An alias's name wins; otherwise the spelling with the most contributions. The login
            // goes with the name shown, else it is the first one found.
            if (alias) {
                person.name = alias;
                person.shownBy = Infinity;
                person.login = person.login || login;
            } else if (weight > person.shownBy) {
                person.name = name;
                person.shownBy = weight;
                person.login = login || person.login;
            } else {
                person.login = person.login || login;
            }
        });
        return Object.keys (people).map (function (key) { return people[key]; }).filter (function (person) {
            return total (person) > 0;
        }).sort (function (a, b) {
            return total (b) - total (a) || a.name.localeCompare (b.name, LOCALE);
        }).slice (0, 5);
    }

    // "44 commits", "1 report": the page's text for one count, in the singular for 1.
    function countText (list, kind, count) {
        var attribute = count === 1 ? 'data-' + kind.replace (/s$/, '') : 'data-' + kind;
        return format (list.getAttribute (attribute) || list.getAttribute ('data-' + kind), { count: count.toLocaleString (LOCALE) });
    }

    function renderMomentum (data) {
        var totals = data && data.version === 1 && data.totals && typeof data.totals === 'object' ? data.totals : {};
        var shown = 0;
        var figures = momentum.querySelectorAll ('[data-total]');
        for (var i = 0; i < figures.length; i++) {
            var key = figures[i].getAttribute ('data-total');
            var value = totals[key];
            if (!isCount (value)) {
                continue;
            }
            figures[i].querySelector ('strong').textContent = value.toLocaleString (LOCALE);
            if (key === 'discordMembers') {
                var change = totals.discordMembersChange;
                var changeElement = figures[i].querySelector ('.change');
                if (change && typeof change.diff === 'number' && Math.floor (change.diff) === change.diff && isCount (change.days) && change.days >= 1) {
                    changeElement.textContent = format (momentum.getAttribute ('data-change'), {
                        sign: change.diff > 0 ? '+' : (change.diff < 0 ? '\u2212' : ''),
                        diff: Math.abs (change.diff).toLocaleString (LOCALE),
                        days: change.days
                    });
                }
            }
            figures[i].hidden = false;
            shown++;
        }
        var people = data && data.version === 1 ? topContributors (data.topContributors) : [];
        var box = document.getElementById ('top-contributors');
        var list = box.querySelector ('ol');
        list.textContent = '';
        people.forEach (function (person) {
            var item = document.createElement ('li');
            var name = document.createElement (person.login ? 'a' : 'span');
            name.className = 'name';
            name.textContent = person.name;
            if (person.login) {
                name.href = 'https://github.com/' + person.login;
            }
            // Each count above 0 on its own, never their sum.
            var counts = document.createElement ('span');
            counts.className = 'counts';
            CONTRIBUTIONS.filter (function (kind) { return person[kind] > 0; }).forEach (function (kind, index) {
                if (index > 0) {
                    counts.appendChild (document.createTextNode (' · '));
                }
                var count = document.createElement ('span');
                count.textContent = countText (list, kind, person[kind]);
                counts.appendChild (count);
            });
            item.appendChild (name);
            item.appendChild (counts);
            list.appendChild (item);
        });
        box.hidden = people.length === 0;
        document.getElementById ('momentum-figures').hidden = shown === 0;
        momentum.hidden = shown === 0 && people.length === 0;
    }

    function start () {
        LOCALE = document.documentElement.lang || 'en';
        monthNames = (section.getAttribute ('data-months') || '').split (/\s+/);
        monthLabelPattern = section.getAttribute ('data-month-label') || monthLabelPattern;
        fetch (COMMUNITY_URL, { cache: 'no-cache' }).then (function (response) {
            if (!response.ok) {
                throw new Error (String (response.status));
            }
            return response.json ();
        }).then (function (data) {
            try {
                renderMomentum (data);
            } catch (e) {
                momentum.hidden = true;
            }
            allMonths = readMonths (data);
            // The first frame already shows numbers: the read-out rests on the latest month.
            if (allMonths) {
                selected = allMonths.length - 1;
            }
            render ();
            listenToChart (document.getElementById ('growth-chart'));
            listenToLegend ();
            if (phone.addEventListener) {
                phone.addEventListener ('change', render);
            } else if (phone.addListener) {
                phone.addListener (render);
            }
        }).catch (function () {
            section.hidden = true;
            momentum.hidden = true;
        });
    }

    // The labels and the language are read from the page, so it starts once it is translated.
    if (window.TapirI18n && typeof window.TapirI18n.ready === 'function') {
        window.TapirI18n.ready (start);
    } else {
        start ();
    }
}) ();
