/* Motion prototype for the Tapir site (see motion.css). */
(function () {
    'use strict';
    var reduced = window.matchMedia && matchMedia ('(prefers-reduced-motion: reduce)').matches;
    var root = document.documentElement;
    // The inline splash script makes the same checks; drop a splash it set anyway.
    if (reduced || !('IntersectionObserver' in window) || /nomotion/.test (location.search + location.hash)) { root.classList.remove ('m-splash', 'm-font'); return; }
    root.classList.add ('motion');
    var splashing = root.classList.contains ('m-splash');
    if (splashing) root.classList.add ('m-hold');
    var held = [];

    // 1. Reveal groups: each child gets a stagger delay within its parent.
    var groups = '.ways > *, .teasers > *, .videos > *, .comm > *, .doc-grid > *, .uses-grid > li, .flow > div, .figures > div, .stack .box, .timeline > li, .share-steps li, .spec dl > *';
    var singles = '.sec-head, .term, .schedule-wrap, .tblock, .shot, .callout, main section > h2, main section > p';
    var els = [];
    document.querySelectorAll (groups).forEach (function (el) {
        var i = Array.prototype.indexOf.call (el.parentNode.children, el);
        el.style.setProperty ('--d', Math.min (i, 6) * 0.07 + 's'); el.classList.add ('rv'); els.push (el);
    });
    document.querySelectorAll (singles).forEach (function (el) { if (!el.classList.contains ('rv') && !el.closest ('.page-head')) { el.classList.add ('rv'); els.push (el); } });
    document.querySelectorAll ('.dim').forEach (function (el) { els.push (el); });

    var io = new IntersectionObserver (function (entries) {
        entries.forEach (function (e) {
            // 12% of an element, or a quarter of the screen's height of it: 12% of a tall element (the
            // schedule on a phone, about 7,000 px) never fits in the screen at once.
            if (!e.isIntersecting || (e.intersectionRatio < 0.12 && e.intersectionRect.height < innerHeight * 0.25)) return;
            e.target.classList.add ('in'); io.unobserve (e.target);
            if (e.target.matches ('.figures > div')) countUp (e.target.querySelector ('strong'));
            if (e.target.matches ('.term')) typeOut (e.target);
        });
    }, { rootMargin: '0px 0px -8% 0px', threshold: [0, 0.05, 0.12] });
    els.forEach (function (el) {
        // Anything already on screen at load shows at once, without a flash.
        var r = el.getBoundingClientRect ();
        if (r.top < innerHeight * 0.9 && r.bottom > 0) { if (splashing) held.push (el); else show (el); }
        else io.observe (el);
    });

    function show (el) { el.classList.add ('in'); if (el.matches ('.figures > div')) countUp (el.querySelector ('strong')); if (el.matches ('.term')) typeOut (el); }
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
    function typeOut (term) {
        var pre = term.querySelector ('pre'); if (!pre || pre.dataset.mTyped) return; pre.dataset.mTyped = '1';
        var lines = pre.innerHTML.split ('\n'); if (lines.length < 2) return;
        pre.innerHTML = lines.map (function (l) { return '<span class="ln">' + (l || ' ') + '</span>'; }).join ('');
        var spans = pre.querySelectorAll ('.ln'), i = 0, caret = document.createElement ('span'); caret.className = 'caret';
        (function next () { if (i >= spans.length) return; spans[i].classList.add ('on'); spans[i].appendChild (caret); i++; setTimeout (next, 70 + Math.random () * 60); }) ();
    }

    // 6. Timeline progress line follows the scroll.
    var tls = document.querySelectorAll ('.timeline');
    // 8. Header shadow.
    var header = document.querySelector ('.site-header');
    function onScroll () {
        if (header) header.classList.toggle ('m-scrolled', scrollY > 8);
        tls.forEach (function (tl) { var r = tl.getBoundingClientRect (); var p = Math.max (0, Math.min (1, (innerHeight * 0.6 - r.top) / r.height)); tl.style.setProperty ('--prog', (p * 100).toFixed (1) + '%'); });
    }
    addEventListener ('scroll', onScroll, { passive: true }); onScroll ();

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
}) ();
