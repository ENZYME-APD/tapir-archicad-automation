/* The shared behaviour of every page of Tapir's site: the dark mode toggle, closing the Docs menu,
   and a copy button on code blocks. The pages are complete without it; it only adds to them.
   Every text it shows is in the page's HTML, so the pages can be translated:
   the toggle's label is in the header, the copy button's labels are data-copy and data-copied
   on the <html> element. */
(function () {
    'use strict';

    var STORAGE_KEY = 'tapir-theme';
    var root = document.documentElement;
    var darkQuery = window.matchMedia ? window.matchMedia ('(prefers-color-scheme: dark)') : null;

    function storedTheme () {
        try {
            var theme = window.localStorage.getItem (STORAGE_KEY);
            return theme === 'light' || theme === 'dark' ? theme : null;
        } catch (e) {
            return null;
        }
    }

    function storeTheme (theme) {
        try {
            window.localStorage.setItem (STORAGE_KEY, theme);
        } catch (e) {}
    }

    function isDark () {
        var theme = root.getAttribute ('data-theme');
        if (theme === 'light' || theme === 'dark') {
            return theme === 'dark';
        }
        return !!(darkQuery && darkQuery.matches);
    }

    function setUpThemeToggle () {
        var toggle = document.getElementById ('theme-toggle');
        if (!toggle) {
            return;
        }
        // The head's inline script already applied a stored choice; this repeats it in case it could not run.
        var theme = storedTheme ();
        if (theme) {
            root.setAttribute ('data-theme', theme);
        }
        function update () {
            toggle.setAttribute ('aria-pressed', String (isDark ()));
        }
        toggle.addEventListener ('click', function () {
            var next = isDark () ? 'light' : 'dark';
            root.setAttribute ('data-theme', next);
            storeTheme (next);
            update ();
        });
        if (darkQuery) {
            var onChange = function () {
                if (!root.hasAttribute ('data-theme')) {
                    update ();
                }
            };
            if (darkQuery.addEventListener) {
                darkQuery.addEventListener ('change', onChange);
            } else if (darkQuery.addListener) {
                darkQuery.addListener (onChange);
            }
        }
        update ();
        toggle.hidden = false;
    }

    function setUpNavMenus () {
        var menus = document.querySelectorAll ('.nav-menu');
        if (!menus.length) {
            return;
        }
        function closeAll (except) {
            for (var i = 0; i < menus.length; i++) {
                if (menus[i] !== except) {
                    menus[i].open = false;
                }
            }
        }
        document.addEventListener ('click', function (event) {
            var menu = event.target.closest ? event.target.closest ('.nav-menu') : null;
            closeAll (menu);
        });
        document.addEventListener ('keydown', function (event) {
            if (event.key !== 'Escape') {
                return;
            }
            for (var i = 0; i < menus.length; i++) {
                if (menus[i].open) {
                    menus[i].open = false;
                    menus[i].querySelector ('summary').focus ();
                }
            }
        });
        // Leaving the menu with Tab closes it.
        for (var i = 0; i < menus.length; i++) {
            menus[i].addEventListener ('focusout', function (event) {
                var menu = event.currentTarget;
                if (event.relatedTarget && !menu.contains (event.relatedTarget)) {
                    menu.open = false;
                }
            });
        }
    }

    function setUpCopyButtons () {
        var label = root.getAttribute ('data-copy');
        var doneLabel = root.getAttribute ('data-copied');
        if (!label || !navigator.clipboard || !navigator.clipboard.writeText) {
            return;
        }
        var blocks = document.querySelectorAll ('main pre');
        for (var i = 0; i < blocks.length; i++) {
            var pre = blocks[i];
            if (pre.parentNode.classList && pre.parentNode.classList.contains ('code-block')) {
                continue;
            }
            var wrapper = document.createElement ('div');
            wrapper.className = 'code-block';
            pre.parentNode.insertBefore (wrapper, pre);
            wrapper.appendChild (pre);
            var button = document.createElement ('button');
            button.type = 'button';
            button.className = 'copy-button';
            button.textContent = label;
            wrapper.appendChild (button);
            button.addEventListener ('click', (function (pre, button) {
                return function () {
                    navigator.clipboard.writeText (pre.innerText.replace (/\n$/, '')).then (function () {
                        button.textContent = doneLabel || label;
                        setTimeout (function () { button.textContent = label; }, 2000);
                    }, function () {});
                };
            }) (pre, button));
        }
    }

    function init () {
        setUpThemeToggle ();
        setUpNavMenus ();
        setUpCopyButtons ();
    }

    if (document.readyState === 'loading') {
        document.addEventListener ('DOMContentLoaded', init);
    } else {
        init ();
    }
}) ();
