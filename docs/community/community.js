/* The Community page (/community/): the Momentum figures, the top code contributors and the
   activity charts. It reads community.json from the site-data branch on raw.githubusercontent.com
   (never GitHub's API). Only counts, never the text of a message:
       {"version": 1,
        "months": [{"month": "2026-10", "issuesOpened": 41, "pullRequestsMerged": 37,
                    "discordMessages": 512}, ...],
        "totals": {"commits": 4321, "discordMessages": 12345, "discordMembers": 678,
                   "discordMembersChange": {"diff": 23, "days": 30}, "participants": 234},
        "topContributors": [{"name": "Jane Doe", "commits": 120}, ...]}
   Every key but "version" is optional, and so is every key inside "totals".
   months: the months follow one another without gaps; the current, unfinished month is best left
   out, or its bar reads as a drop. A series is drawn only when every month shown has a whole number
   of at least 0 for it; the page shows the last 12 months (6 on phones).
   totals: each figure is shown only when it is a whole number of at least 0. discordMembersChange
   is the change in members between the two member counts compared, "days" apart ("+23 in 30
   days"); it is left out unless both are whole numbers and days is at least 1.
   topContributors: commits per person. Bots are left out (Claude, claude[bot], github-actions[bot]
   and any name ending in [bot]), and a person listed under two names is counted once: names that
   only differ in case or accents are one person, and so are the names in ALIASES. The five with
   the most commits are shown.
   When the file cannot be read, or a part of it does not look right, that part stays hidden: a
   number that can't be read is left out, not shown wrong. Every text shown is in the page's HTML. */
(function () {
    'use strict';

    var COMMUNITY_URL = 'https://raw.githubusercontent.com/ENZYME-APD/tapir-archicad-automation/site-data/community.json';
    var MONTH_PATTERN = /^([0-9]{4})-(0[1-9]|1[0-2])$/;
    var SVG_NS = 'http://www.w3.org/2000/svg';

    // Names one person committed under, by their key (see personKey), and the name to show.
    var ALIASES = { 'tlorantfy': 'Tibor Lorántfy' };

    var section = document.getElementById ('activity-section');
    var momentum = document.getElementById ('momentum-section');
    if (!section || !momentum || !window.fetch || !window.matchMedia) {
        return;
    }
    var figures = Array.prototype.slice.call (section.querySelectorAll ('figure[data-series]'));
    var table = document.getElementById ('activity-table');
    var monthNames = (section.getAttribute ('data-months') || '').split (/\s+/);
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
        return monthName (month) + ' ' + MONTH_PATTERN.exec (month)[1];
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

    function drawChart (container, months, key, label) {
        var width = 300;
        var height = 132;
        var top = 18;
        var bottom = 100;
        var slot = width / months.length;
        var barWidth = Math.max (4, Math.min (28, slot * 0.62));
        var max = 0;
        var maxIndex = 0;
        months.forEach (function (month, index) {
            if (month[key] >= max) {
                max = month[key];
                maxIndex = index;
            }
        });

        var svg = svgElement ('svg', {
            viewBox: '0 0 ' + width + ' ' + height,
            role: 'img',
            'aria-label': format (section.getAttribute ('data-chart-label'), {
                series: label,
                first: monthLabel (months[0].month),
                last: monthLabel (months[months.length - 1].month)
            })
        });
        svg.appendChild (svgElement ('line', { 'class': 'axis', x1: 0, x2: width, y1: bottom + 0.5, y2: bottom + 0.5 }));

        var last = months.length - 1;
        months.forEach (function (month, index) {
            var value = month[key];
            var barHeight = max > 0 ? Math.max (value > 0 ? 1 : 0, (bottom - top) * value / max) : 0;
            var x = slot * index + (slot - barWidth) / 2;
            var centre = slot * index + slot / 2;
            svg.appendChild (svgElement ('rect', {
                'class': 'bar',
                x: x.toFixed (2),
                y: (bottom - barHeight).toFixed (2),
                width: barWidth.toFixed (2),
                height: barHeight.toFixed (2),
                rx: 2
            }));
            // The value is written above the highest and the newest bar; the table has the rest.
            if (index === maxIndex || index === last) {
                svg.appendChild (svgElement ('text', {
                    'class': 'value', x: centre.toFixed (2), y: (bottom - barHeight - 5).toFixed (2), 'text-anchor': 'middle'
                }, String (value)));
            }
            svg.appendChild (svgElement ('text', {
                x: centre.toFixed (2), y: bottom + 14, 'text-anchor': 'middle'
            }, monthName (month.month)));
            var parts = MONTH_PATTERN.exec (month.month);
            if (index === 0 || parts[2] === '01') {
                svg.appendChild (svgElement ('text', {
                    x: centre.toFixed (2), y: bottom + 28, 'text-anchor': 'middle'
                }, parts[1]));
            }
        });

        container.textContent = '';
        container.appendChild (svg);
    }

    function fillTable (months, series) {
        var headRow = table.tHead.rows[0];
        while (headRow.cells.length > 1) {
            headRow.deleteCell (1);
        }
        series.forEach (function (item) {
            var th = document.createElement ('th');
            th.scope = 'col';
            th.textContent = item.label;
            headRow.appendChild (th);
        });
        var body = table.tBodies[0];
        body.textContent = '';
        months.slice ().reverse ().forEach (function (month) {
            var row = body.insertRow ();
            var th = document.createElement ('th');
            th.scope = 'row';
            th.textContent = monthLabel (month.month);
            row.appendChild (th);
            series.forEach (function (item) {
                row.insertCell ().textContent = String (month[item.key]);
            });
        });
    }

    function render () {
        if (!allMonths) {
            return;
        }
        var months = allMonths.slice (-(phone.matches ? 6 : 12));
        var series = [];
        figures.forEach (function (figure) {
            var key = figure.getAttribute ('data-series');
            var caption = figure.querySelector ('figcaption');
            var complete = months.every (function (month) { return isCount (month[key]); });
            figure.hidden = !complete;
            if (complete) {
                var label = caption ? caption.textContent : key;
                series.push ({ key: key, label: label });
                drawChart (figure.querySelector ('.activity-chart'), months, key, label);
            }
        });
        if (series.length === 0) {
            section.hidden = true;
            return;
        }
        fillTable (months, series);
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
            return b.commits - a.commits || a.name.localeCompare (b.name, 'en');
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
            figures[i].querySelector ('strong').textContent = value.toLocaleString ('en-US');
            if (key === 'discordMembers') {
                var change = totals.discordMembersChange;
                var changeElement = figures[i].querySelector ('.change');
                if (change && typeof change.diff === 'number' && Math.floor (change.diff) === change.diff && isCount (change.days) && change.days >= 1) {
                    changeElement.textContent = format (momentum.getAttribute ('data-change'), {
                        sign: change.diff > 0 ? '+' : (change.diff < 0 ? '\u2212' : ''),
                        diff: Math.abs (change.diff).toLocaleString ('en-US'),
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
            commits.textContent = format (list.getAttribute ('data-commits'), { count: person.commits.toLocaleString ('en-US') });
            item.appendChild (name);
            item.appendChild (commits);
            list.appendChild (item);
        });
        box.hidden = people.length === 0;
        document.getElementById ('momentum-figures').hidden = shown === 0;
        momentum.hidden = shown === 0 && people.length === 0;
    }

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
}) ();
