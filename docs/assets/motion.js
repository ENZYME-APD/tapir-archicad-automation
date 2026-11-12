/* Motion prototype for the Tapir site (see motion.css).
   The Display menu's Motion (data-motion on <html>, see site.js) decides whether it runs: Reduced
   never, Full always, System (no attribute) unless the system asks for reduced motion. ?nomotion
   turns it off whatever was chosen. A change of choice, or of the system's setting, applies at once:
   switching off removes html.motion, so everything shows as it is; switching on starts it then
   (without the splash, which only opens a visit). */
(function () {
    'use strict';
    var root = document.documentElement;
    var query = window.matchMedia ? matchMedia ('(prefers-reduced-motion: reduce)') : null;
    var blocked = !('IntersectionObserver' in window) || /nomotion/.test (location.search + location.hash);
    var started = false;

    function wanted () {
        var choice = root.getAttribute ('data-motion');
        return !blocked && choice !== 'reduced' && (choice === 'full' || !(query && query.matches));
    }

    function update () {
        if (!wanted ()) { root.classList.remove ('motion'); return; }
        if (started) { root.classList.add ('motion'); return; }
        started = true; start (false);
    }

    // The inline splash script makes the same checks; drop a splash it set anyway.
    if (wanted ()) { started = true; start (root.classList.contains ('m-splash')); }
    else root.classList.remove ('m-splash', 'm-font');
    if (blocked) return;
    document.addEventListener ('tapir-display-change', update);
    if (query && query.addEventListener) query.addEventListener ('change', update);
    else if (query && query.addListener) query.addListener (update);

    function start (splashing) {
    root.classList.add ('motion');
    if (splashing) root.classList.add ('m-hold');
    var held = [];

    // 1. Reveal groups: each child gets a stagger delay within its parent.
    var groups = '.ways > *, .teasers > *, .videos > *, .comm > *, .doc-grid > *, .uses-grid > li, .flow > div, .figures > div, .stack .box, .timeline > li, .share-steps li, .spec dl > *';
    // Not .schedule-wrap: a script fills it after the observer starts (the scripts list) and it can be far
    // taller than a phone's screen, so it shows as soon as it is there (#2508).
    var singles = '.sec-head, .term, .tblock, .shot, .callout, main section > h2, main section > p';
    var els = [];
    document.querySelectorAll (groups).forEach (function (el) {
        var i = Array.prototype.indexOf.call (el.parentNode.children, el);
        el.style.setProperty ('--d', Math.min (i, 6) * 0.07 + 's'); el.classList.add ('rv'); els.push (el);
    });
    document.querySelectorAll (singles).forEach (function (el) { if (!el.classList.contains ('rv') && !el.closest ('.page-head')) { el.classList.add ('rv'); els.push (el); } });
    document.querySelectorAll ('.dim').forEach (function (el) { els.push (el); });

    var io = new IntersectionObserver (function (entries) {
        entries.forEach (function (e) {
            if (!e.isIntersecting) return;
            // An element taller than 60% of the window shows as soon as any of it is on screen: its next
            // threshold (a few % of an element thousands of px tall) may never fit in the screen (#2508).
            // A shorter one waits for 12% of it, or a quarter of the screen's height of it.
            if (e.boundingClientRect.height <= innerHeight * 0.6 && e.intersectionRatio < 0.12 && e.intersectionRect.height < innerHeight * 0.25) return;
            show (e.target);
        });
    }, { rootMargin: '0px 0px -8% 0px', threshold: [0, 0.01, 0.02, 0.05, 0.12] });
    els.forEach (function (el) {
        // Anything already on screen at load shows at once, without a flash.
        var r = el.getBoundingClientRect ();
        if (r.top < innerHeight * 0.9 && r.bottom > 0) { if (splashing) held.push (el); else show (el); }
        else io.observe (el);
    });

    function show (el) { el.classList.add ('in'); io.unobserve (el); if (el.matches ('.figures > div')) countUp (el.querySelector ('strong')); if (el.matches ('.term')) typeOut (el); }
    if (splashing) {
        // The elements on screen at load stay hidden under the splash and come in as it lifts.
        held.forEach (function (el) { el.classList.remove ('in'); });
        splash (function () { root.classList.remove ('m-hold'); held.forEach (function (el, i) { setTimeout (function () { show (el); }, 60 * Math.min (i, 6)); }); });
    }

    // 11. Splash: wait for the page (images, fonts, translation), then fly the logo into the header.
    function splash (done) {
        try { sessionStorage.setItem ('tapir-splash', '1'); } catch (e) {}
        var t0 = performance.now (), lifted = false;
        function ready () {
            var fonts = document.fonts && document.fonts.ready ? document.fonts.ready : Promise.resolve ();
            fonts.then (function () {
                var wait = Math.max (0, 1000 - (performance.now () - t0));
                (function check () { if (root.hasAttribute ('data-i18n-loading') || !root.classList.contains ('m-font')) return setTimeout (check, 50); setTimeout (idle, wait); }) ();
            });
        }
        if (document.readyState === 'complete') ready (); else addEventListener ('load', ready);
        setTimeout (lift, 4000);
        // Start the flight only when the main thread is quiet, so the first frames are not dropped.
        function idle () { (window.requestIdleCallback || function (f) { setTimeout (f, 1); }) (function () { requestAnimationFrame (function () { requestAnimationFrame (lift); }); }, { timeout: 400 }); }
        function lift () {
            if (lifted) return; lifted = true;
            var brand = document.querySelector ('.site-header .brand');
            var r = brand && brand.getBoundingClientRect ();
            // Where the splash really is: a fixed probe at 50%/50%, the same box the splash is centred
            // in. innerWidth/innerHeight differ from it on phones (address bar) and in frames.
            var probe = document.createElement ('div');
            probe.style.cssText = 'position:fixed;left:50%;top:50%;width:0;height:0;visibility:hidden;pointer-events:none';
            document.body.appendChild (probe);
            var c = probe.getBoundingClientRect (); probe.remove ();
            if (r && r.bottom > 0 && r.width) {
                // Line up the left edges (the mark) and the vertical middles.
                var cs = getComputedStyle (root, '::after'), sw = (parseFloat (cs.width) + parseFloat (cs.paddingLeft)) || r.width / (24 / 56);
                root.style.setProperty ('--m-dx', (r.left + sw * (24 / 56) / 2 - c.left).toFixed (2) + 'px');
                root.style.setProperty ('--m-dy', (r.top + r.height / 2 - c.top).toFixed (2) + 'px');
            } else {
                // The header is off screen (a reload part way down): the splash only fades.
                root.classList.add ('m-fade');
            }
            root.classList.add ('m-still');
            getComputedStyle (root, '::after').transform;  // commit the still frame
            requestAnimationFrame (function () { requestAnimationFrame (function () { root.classList.add ('m-fly'); }); });
            setTimeout (done, 380);
            // Remove the splash when its flight has really ended (slow phones lag behind the clock).
            var ended = false;
            function end () {
                if (ended) return; ended = true;
                root.classList.remove ('m-splash', 'm-still', 'm-fly', 'm-fade', 'm-font');
                root.style.removeProperty ('--m-dx'); root.style.removeProperty ('--m-dy');
            }
            root.addEventListener ('transitionend', function te (e) {
                if (e.target === root && e.pseudoElement === '::after' && (e.propertyName === 'transform' || e.propertyName === 'opacity')) { root.removeEventListener ('transitionend', te); setTimeout (end, 30); }
            });
            setTimeout (end, 2000);
        }
    }

    // 5. Count-up for figures: animates the first number in the text node, keeps the rest (sup, %, +).
    function countUp (strong) {
        if (!strong || strong.dataset.mCounted) return; strong.dataset.mCounted = '1';
        var node = null; strong.childNodes.forEach (function (n) { if (!node && n.nodeType === 3 && /\d/.test (n.nodeValue)) node = n; });
        if (!node) return;
        var m = /([\d,.]+)/.exec (node.nodeValue); if (!m) return;
        // Only plain digits or en-US grouping (12,345): a translated page writes 12.345 or 12 345, leave those alone.
        if (!/^(\d+|\d{1,3}(,\d{3})+)$/.test (m[1])) return;
        var target = parseFloat (m[1].replace (/,/g, '')); if (!isFinite (target) || target < 2) return;
        var comma = m[1].indexOf (',') >= 0, before = node.nodeValue.slice (0, m.index), after = node.nodeValue.slice (m.index + m[1].length);
        var t0 = null, dur = 1200;
        function fmt (v) { v = Math.round (v); return comma ? v.toLocaleString ('en-US') : String (v); }
        function step (t) { if (!t0) t0 = t; var p = Math.min (1, (t - t0) / dur); p = 1 - Math.pow (1 - p, 3); node.nodeValue = before + fmt (target * p) + after; if (p < 1) requestAnimationFrame (step); }
        requestAnimationFrame (step);
    }

    // 7. Terminal: wrap each code line and reveal them one by one, with a caret at the end.
    //    A long block types faster (about a second and a half in all), and the rest shows at once
    //    when the reader scrolls to the end of the box (or scrolls inside it) before it is done, so
    //    a reader who scrolls faster than it types never looks at an empty box (#2723).
    function typeOut (term) {
        var pre = term.querySelector ('pre'); if (!pre || pre.dataset.mTyped) return; pre.dataset.mTyped = '1';
        var lines = pre.innerHTML.split ('\n'); if (lines.length < 2) return;
        pre.innerHTML = lines.map (function (l) { return '<span class="ln">' + (l || ' ') + '</span>'; }).join ('');
        var spans = pre.querySelectorAll ('.ln'), i = 0, timer = 0, caret = document.createElement ('span'); caret.className = 'caret';
        var pace = Math.max (12, Math.min (100, 1500 / spans.length));
        function finish () {
            removeEventListener ('scroll', onScroll, true); clearTimeout (timer);
            for (; i < spans.length; i++) spans[i].classList.add ('on');
            spans[spans.length - 1].appendChild (caret);
        }
        function onScroll (e) {
            if (e.target === pre || pre.getBoundingClientRect ().bottom <= innerHeight) finish ();
        }
        addEventListener ('scroll', onScroll, { capture: true, passive: true });
        (function next () {
            if (i >= spans.length) { removeEventListener ('scroll', onScroll, true); return; }
            spans[i].classList.add ('on'); spans[i].appendChild (caret); i++;
            timer = setTimeout (next, pace * (0.7 + Math.random () * 0.6));
        }) ();
    }

    // 6. Timeline, Legacy look only: each step's segment runs from 12px below its number to 12px
    //    above the next one (measured from the layout, so transforms of the reveal do not move it)
    //    and fills as the reading line, 60% down the window, passes it. The Standard look has none.
    var steps = [].slice.call (document.querySelectorAll ('.timeline > li'));
    function placeSteps () {
        if (root.getAttribute ('data-style') !== 'legacy') return;
        var reading = innerHeight * 0.6;
        steps.forEach (function (li) {
            var next = li.nextElementSibling, n = li.querySelector (':scope > .n'), nextN = next && next.querySelector (':scope > .n');
            if (!n || !nextN) return;
            // In li's padding box, where the segment is positioned (li and the timeline are positioned).
            var top = n.offsetTop + n.offsetHeight + 12;
            var end = (next.offsetTop + next.clientTop) - (li.offsetTop + li.clientTop) + nextN.offsetTop - 12;
            var h = Math.max (0, end - top);
            var p = h ? Math.max (0, Math.min (1, (reading - (li.getBoundingClientRect ().top + li.clientTop + top)) / h)) : 0;
            li.style.setProperty ('--seg-top', top + 'px');
            li.style.setProperty ('--seg-h', h + 'px');
            li.style.setProperty ('--prog', (p * 100).toFixed (1) + '%');
        });
    }
    // 8. Header shadow.
    var header = document.querySelector ('.site-header');
    function onScroll () {
        if (header) header.classList.toggle ('m-scrolled', scrollY > 8);
        if (steps.length) placeSteps ();
    }
    addEventListener ('scroll', onScroll, { passive: true }); onScroll ();
    if (steps.length) {
        addEventListener ('resize', placeSteps);
        document.addEventListener ('tapir-display-change', placeSteps);
        if (document.fonts && document.fonts.ready) document.fonts.ready.then (placeSteps);
        if ('ResizeObserver' in window) { var ro = new ResizeObserver (placeSteps); steps.forEach (function (li) { ro.observe (li); }); }
    }

    // Safety net: an element still hidden on screen 1.5 s after the last scroll (or after load, for
    // content a script fills or moves later) is shown, whatever the observer missed (#2508).
    var net = 0;
    function sweep () {
        if (root.classList.contains ('m-hold')) return arm ();  // the splash shows the held ones itself
        els.forEach (function (el) {
            if (el.classList.contains ('in')) return;
            var r = el.getBoundingClientRect ();
            if (r.top < innerHeight && r.bottom > 0 && (r.width || r.height)) show (el);
        });
    }
    function arm () { clearTimeout (net); net = setTimeout (sweep, 1500); }
    addEventListener ('scroll', arm, { passive: true }); arm ();

    // 4. CAD crosshair with coordinates in the page head (fine pointers only).
    if (matchMedia ('(pointer: fine)').matches) document.querySelectorAll ('.page-head').forEach (function (head) {
        if (getComputedStyle (head).position === 'static') head.style.position = 'relative';
        head.style.overflow = 'hidden';
        var c = document.createElement ('div'), x = document.createElement ('div'), y = document.createElement ('div');
        c.className = 'm-coords'; x.className = 'm-cross-x'; y.className = 'm-cross-y';
        [x, y, c].forEach (function (n) { n.setAttribute ('aria-hidden', 'true'); head.appendChild (n); });
        head.addEventListener ('pointermove', function (e) {
            var r = head.getBoundingClientRect (), px = e.clientX - r.left, py = e.clientY - r.top;
            head.classList.add ('m-hover');
            x.style.top = py + 'px'; y.style.left = px + 'px';
            c.style.left = px + 'px'; c.style.top = py + 'px';
            c.textContent = 'X ' + (px * 10).toFixed (0) + '  Y ' + ((r.height - py) * 10).toFixed (0) + ' mm';
        });
        head.addEventListener ('pointerleave', function () { head.classList.remove ('m-hover'); });
    });
    }  // start
}) ();
