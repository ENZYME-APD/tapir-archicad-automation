/* The activity charts of the Community page (/community/). It reads community.json from the
   site-data branch on raw.githubusercontent.com (never GitHub's API). Only counts per month,
   never the text of a message:
       {"version": 1,
        "months": [{"month": "2026-10", "issuesOpened": 41, "pullRequestsMerged": 37,
                    "discordMessages": 512}, ...]}
   The months follow one another without gaps; the current, unfinished month is best left out,
   or its bar reads as a drop. Each series key is optional. A series is drawn only when every month shown has a whole number
   of at least 0 for it; the page shows the last 12 months (6 on phones). When the file cannot be
   read, or nothing in it looks right, the section stays hidden: a number that can't be read is
   left out, not shown wrong. Every text shown is in the page's HTML. */
(function () {
    'use strict';

    var COMMUNITY_URL = 'https://raw.githubusercontent.com/ENZYME-APD/tapir-archicad-automation/site-data/community.json';
    var MONTH_PATTERN = /^([0-9]{4})-(0[1-9]|1[0-2])$/;
    var SVG_NS = 'http://www.w3.org/2000/svg';

    var section = document.getElementById ('activity-section');
    if (!section || !window.fetch || !window.matchMedia) {
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

    fetch (COMMUNITY_URL, { cache: 'no-cache' }).then (function (response) {
        if (!response.ok) {
            throw new Error (String (response.status));
        }
        return response.json ();
    }).then (function (data) {
        allMonths = readMonths (data);
        render ();
        if (phone.addEventListener) {
            phone.addEventListener ('change', render);
        } else if (phone.addListener) {
            phone.addListener (render);
        }
    }).catch (function () {
        section.hidden = true;
    });
}) ();
