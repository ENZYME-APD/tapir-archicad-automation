/* The Community page's map of where people use Tapir (#map-section). It reads user-map.json from
   the site-data branch on raw.githubusercontent.com, written daily by tools/generate_user_map.py
   (.github/workflows/site_data.yml). Only towns and counts, never a name or a message:
       {"version": 1,
        "totals": {"people": 7, "places": 7, "countries": 5},
        "places": [{"name": "Maribor", "country": "Slovenia", "countryCode": "SI",
                    "lat": 46.56, "lon": 15.65, "people": 1}, ...],
        "countries": [{"name": "Germany", "code": "DE", "people": 3}, ...]}
   The land is world.svg, dots of Natural Earth's 1:110m land in the Equal Earth projection, drawn
   as a CSS mask so it takes the theme's colour; the towns are drawn over it in the same projection
   (SCALE, TX, TY are the numbers written in world.svg). Each town is a dot, larger the more people
   are there (by area); the pointer, a tap or the keyboard (Tab) names it. The list under the map
   names every country with its count, so nothing is only in the picture. When the file cannot be
   read, or holds no place, the section stays hidden. Its texts are in the page's HTML
   (data-place(s), data-country/-ies, data-map-label), translated by assets/site.js. */
(function () {
    'use strict';

    var MAP_URL = 'https://raw.githubusercontent.com/ENZYME-APD/tapir-archicad-automation/site-data/user-map.json';
    var SVG_NS = 'http://www.w3.org/2000/svg';
    var WIDTH = 1000, HEIGHT = 437, SCALE = 184.7316, TX = 500, TY = 241.3912;

    var section = document.getElementById ('map-section');
    if (!section || !window.fetch) {
        return;
    }

    function format (text, values) {
        return String (text || '').replace (/\{(\w+)\}/g, function (match, key) {
            return Object.prototype.hasOwnProperty.call (values, key) ? String (values[key]) : match;
        });
    }

    function isCount (value) {
        return typeof value === 'number' && isFinite (value) && value >= 0 && Math.floor (value) === value;
    }

    // d3-geo's Equal Earth, with world.svg's scale and translation.
    function project (lon, lat) {
        var A1 = 1.340264, A2 = -0.081106, A3 = 0.000893, A4 = 0.003796, M = Math.sqrt (3) / 2;
        var lambda = lon * Math.PI / 180, phi = lat * Math.PI / 180;
        var l = Math.asin (M * Math.sin (phi)), l2 = l * l, l6 = l2 * l2 * l2;
        var x = lambda * Math.cos (l) / (M * (A1 + 3 * A2 * l2 + l6 * (7 * A3 + 9 * A4 * l2)));
        var y = l * (A1 + A2 * l2 + l6 * (A3 + A4 * l2));
        return [TX + SCALE * x, TY - SCALE * y];
    }

    function element (name, attributes) {
        var node = document.createElementNS (SVG_NS, name);
        for (var key in attributes) {
            node.setAttribute (key, attributes[key]);
        }
        return node;
    }

    function plural (count, one, many) {
        return format (count === 1 ? section.getAttribute (one) : section.getAttribute (many),
                       { count: count.toLocaleString (document.documentElement.lang || 'en') });
    }

    // A country in the page's language (España, Allemagne), from its code; else the English name.
    function countryName (code, english) {
        try {
            if (window.Intl && Intl.DisplayNames && /^[A-Z]{2}$/.test (code || '')) {
                return new Intl.DisplayNames ([document.documentElement.lang || 'en'], { type: 'region' }).of (code) || english;
            }
        } catch (e) {}
        return english;
    }

    function draw (data) {
        var places = (Array.isArray (data.places) ? data.places : []).filter (function (p) {
            return p && typeof p.name === 'string' && typeof p.country === 'string' && isCount (p.people) && p.people > 0 &&
                typeof p.lat === 'number' && typeof p.lon === 'number' && Math.abs (p.lat) <= 90 && Math.abs (p.lon) <= 180;
        });
        if (!places.length) {
            return;
        }
        var totals = data.totals || {};
        var countries = (Array.isArray (data.countries) ? data.countries : []).filter (function (c) {
            return c && typeof c.name === 'string' && isCount (c.people) && c.people > 0;
        });

        Array.prototype.forEach.call (document.querySelectorAll ('#map-figures [data-total]'), function (figure) {
            var value = totals[figure.getAttribute ('data-total')];
            if (isCount (value)) {
                figure.querySelector ('strong').textContent = value.toLocaleString (document.documentElement.lang || 'en');
                figure.hidden = false;
            }
        });

        var canvas = document.getElementById ('map-canvas');
        canvas.setAttribute ('aria-label', format (section.getAttribute ('data-map-label'), {
            people: isCount (totals.people) ? totals.people : places.length,
            countries: isCount (totals.countries) ? totals.countries : countries.length
        }));
        var svg = element ('svg', { viewBox: '0 0 ' + WIDTH + ' ' + HEIGHT, 'class': 'map-towns', focusable: 'false' });
        var label = document.createElement ('p');
        label.className = 'map-label';
        label.hidden = true;
        label.setAttribute ('aria-hidden', 'true');

        function show (marker, text, point) {
            label.textContent = text;
            label.hidden = false;
            label.style.left = (point[0] / WIDTH * 100) + '%';
            label.style.top = (point[1] / HEIGHT * 100) + '%';
            Array.prototype.forEach.call (svg.querySelectorAll ('.active'), function (m) { m.classList.remove ('active'); });
            marker.classList.add ('active');
        }
        function hide () {
            label.hidden = true;
            Array.prototype.forEach.call (svg.querySelectorAll ('.active'), function (m) { m.classList.remove ('active'); });
        }

        // The largest first, so a small town next to a big one stays on top and can be pointed at.
        places.slice ().sort (function (a, b) { return b.people - a.people; }).forEach (function (place) {
            var point = project (place.lon, place.lat);
            var radius = 5 + 3.5 * (Math.sqrt (place.people) - 1);
            var text = format (place.people === 1 ? section.getAttribute ('data-place') : section.getAttribute ('data-places'),
                               { place: place.name, country: countryName (place.countryCode, place.country), count: place.people });
            var marker = element ('g', { 'class': 'town', tabindex: '0', role: 'img', 'aria-label': text });
            marker.appendChild (element ('circle', { 'class': 'halo', cx: point[0].toFixed (1), cy: point[1].toFixed (1), r: (radius * 2.2).toFixed (1) }));
            marker.appendChild (element ('circle', { 'class': 'dot', cx: point[0].toFixed (1), cy: point[1].toFixed (1), r: radius.toFixed (1) }));
            // A larger, invisible target for fingers.
            marker.appendChild (element ('circle', { 'class': 'hit', cx: point[0].toFixed (1), cy: point[1].toFixed (1), r: Math.max (radius, 12).toFixed (1) }));
            marker.addEventListener ('pointerenter', function () { show (marker, text, point); });
            marker.addEventListener ('pointerleave', hide);
            marker.addEventListener ('focus', function () { show (marker, text, point); });
            marker.addEventListener ('blur', hide);
            marker.addEventListener ('click', function (event) { event.stopPropagation (); show (marker, text, point); });
            svg.appendChild (marker);
        });
        document.addEventListener ('click', hide);
        document.addEventListener ('keydown', function (event) {
            if (event.key === 'Escape') {
                hide ();
            }
        });
        canvas.appendChild (svg);
        canvas.appendChild (label);

        var list = document.getElementById ('country-list');
        var most = countries.length ? countries[0].people : 1;
        countries.forEach (function (country) {
            var item = document.createElement ('li');
            var name = document.createElement ('span');
            name.className = 'name';
            name.textContent = countryName (country.code, country.name);
            var count = document.createElement ('span');
            count.className = 'count';
            count.textContent = plural (country.people, 'data-country', 'data-countries');
            var bar = document.createElement ('i');
            bar.className = 'bar';
            bar.setAttribute ('aria-hidden', 'true');
            bar.style.width = Math.max (4, country.people / most * 100) + '%';
            item.appendChild (name);
            item.appendChild (count);
            item.appendChild (bar);
            list.appendChild (item);
        });
        list.hidden = !countries.length;
        section.hidden = false;
    }

    function start () {
        fetch (MAP_URL, { cache: 'no-cache' }).then (function (response) {
            return response.ok ? response.json () : null;
        }).then (function (data) {
            if (data && typeof data === 'object') {
                draw (data);
            }
        }).catch (function () {});
    }

    if (window.TapirI18n && typeof window.TapirI18n.ready === 'function') {
        window.TapirI18n.ready (start);
    } else {
        start ();
    }
}) ();
