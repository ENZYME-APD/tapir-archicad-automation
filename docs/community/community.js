/* The Community page (/community/): the Momentum figures, the top code contributors and the
   growth chart. It reads community.json from the site-data branch on raw.githubusercontent.com
   (never GitHub's API), written daily by tools/generate_community_data.py
   (.github/workflows/site_data.yml). Only counts, never the text of a message:
       {"version": 1,
        "months": [{"month": "2026-10", "commits": 1861, "issuesOpened": 41, "pullRequestsMerged": 37,
                    "discordMessages": 512, "discordMembers": 637}, ...],
        "totals": {"commits": 4321, "discordMessages": 12345, "discordMembers": 678,
                   "discordMembersChange": {"diff": 23, "days": 30}, "participants": 234},
        "topContributors": [{"name": "Jane Doe", "commits": 120}, ...]}
   Every key but "version" is optional, and so is every key inside "totals".
   months: the months follow one another without gaps, oldest first, from the first month of the
   project; the current, unfinished month is left out, or its values read as a drop. The chart
   stacks commits, issues opened plus pull requests merged, and Discord messages per month, and
   draws discordMembers (the members at the end of each month) as a line on its own scale on the
   right. A series is drawn only when every month has a whole number of at least 0 for it; the
   table under the chart lists every number.
   totals: each figure is shown only when it is a whole number of at least 0. discordMembersChange
   is the change in members between the two member counts compared, "days" apart ("+23 in 30
   days"); it is left out unless both are whole numbers and days is at least 1.
   topContributors: commits per person. Bots are left out (Claude, claude[bot], github-actions[bot]
   and any name ending in [bot]), and a person listed under two names is counted once: names that
   only differ in case or accents are one person, and so are the names in ALIASES. The five with
   the most commits are shown.
   When the file cannot be read, or a part of it does not look right, that part stays hidden: a
   number that can't be read is left out, not shown wrong. Every text shown is in the page's HTML
   (data-change, data-commits, data-chart-label, data-months and data-month-label among them), and
   assets/site.js translates it (assets/i18n/<language>/community.js), so this starts once the page
   is translated (TapirI18n.ready) and then reads the page's language and those texts. */
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
        if (withLine) {
            var lineMax = niceMax (Math.max.apply (null, months.map (function (month) { return month[LINE]; })));
            var yLine = function (value) { return bottom - (bottom - top) * value / lineMax; };
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

        container.textContent = '';
        container.appendChild (svg);
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

    function render () {
        if (!allMonths) {
            return;
        }
        var months = allMonths;
        var layers = LAYERS.filter (function (key) { return complete (months, key); });
        var withLine = complete (months, LINE);
        if (layers.length === 0) {
            section.hidden = true;
            return;
        }
        Array.prototype.forEach.call (section.querySelectorAll ('.growth-legend [data-key]'), function (item) {
            var key = item.getAttribute ('data-key');
            item.hidden = key === LINE ? !withLine : layers.indexOf (key) < 0;
        });
        drawGrowth (document.getElementById ('growth-chart'), months, layers, withLine);
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
            if (!entry || typeof entry.name !== 'string' || !entry.name.trim () || !isCount (entry.commits) || isBot (entry.name)) {
                return;
            }
            var alias = aliasOf[personKey (entry.name)];
            var name = alias || entry.name.trim ();
            var key = personKey (name);
            var person = people[key] || (people[key] = { name: name, commits: 0, shownBy: -1 });
            person.commits += entry.commits;
            // An alias's name wins; otherwise the spelling with the most commits.
            if (alias) {
                person.name = alias;
                person.shownBy = Infinity;
            } else if (entry.commits > person.shownBy) {
                person.name = name;
                person.shownBy = entry.commits;
            }
        });
        return Object.keys (people).map (function (key) { return people[key]; }).filter (function (person) {
            return person.commits > 0;
        }).sort (function (a, b) {
            return b.commits - a.commits || a.name.localeCompare (b.name, LOCALE);
        }).slice (0, 5);
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
            var name = document.createElement ('span');
            name.className = 'name';
            name.textContent = person.name;
            var commits = document.createElement ('span');
            commits.className = 'commits';
            commits.textContent = format (list.getAttribute ('data-commits'), { count: person.commits.toLocaleString (LOCALE) });
            item.appendChild (name);
            item.appendChild (commits);
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
            render ();
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
