""" A common window layout for the built-in scripts.

Every script window has the same parts: a header with the script's name and a
short explanation, a body the script fills in, and a footer with a status line,
a "Keep on top" switch and the action buttons. Only tkinter/ttk is used, so the
scripts need no extra packages.
"""

import contextlib
import glob
import os
import sys
import tkinter as tk
import traceback
from tkinter import font as tkfont, messagebox, ttk
from typing import Callable

from utilities.archicad import ArchicadError, is_alive

# The Tapir icon (docs/assets/favicon.svg, the site's favicon, at 64 and 32 pixels) for the title bar,
# the taskbar and the Dock, instead of Tk's feather (#2735).
_TAPIR_ICON_64_PNG = (
    "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"
)
_TAPIR_ICON_32_PNG = (
    "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"
)


STATUS_COLORS = {
    "info": "#555555",
    "ok": "#1b7f3b",
    "warning": "#a15c00",
    "error": "#c62828",
}


def _enable_dpi_awareness() -> None:
    """ Without this, Windows scales the window up as a bitmap and it looks blurry. """
    if sys.platform != "win32":
        return
    try:
        import ctypes
        ctypes.windll.shcore.SetProcessDpiAwareness(1)
    except (AttributeError, OSError):
        pass


def _point_tcl_to_base_python() -> None:
    """ Lets Tcl/Tk find their scripts when the script runs in a virtual environment.

    uv runs each script in its own virtual environment. The macOS builds of its
    Python look for Tcl's init.tcl next to the running executable, which is the
    environment's, not the Python installation that has lib/tcl8.6 and lib/tk8.6:
    tk.Tk() then fails with "Can't find a usable init.tcl". TCL_LIBRARY and
    TK_LIBRARY point Tcl/Tk at the installation's copies. Windows Python sets
    them itself, and a value set by the user is kept.
    """
    if sys.platform == "win32":
        return
    for variable, pattern, marker in (("TCL_LIBRARY", "tcl8.*", "init.tcl"), ("TK_LIBRARY", "tk8.*", "tk.tcl")):
        if os.environ.get(variable):
            continue
        found = sorted(glob.glob(os.path.join(sys.base_prefix, "lib", pattern, marker)))
        if found:
            os.environ[variable] = os.path.dirname(found[-1])


_point_tcl_to_base_python()


def set_tapir_icon(window: tk.Misc) -> list:
    """ Gives window, and every window opened after it, the Tapir icon instead of Tk's feather.
    Returns the images, which the caller keeps as long as the window lives. The icon is cosmetic:
    a Tk that cannot show it leaves the window as it was. """
    try:
        images = [tk.PhotoImage(master=window, data=data) for data in (_TAPIR_ICON_64_PNG, _TAPIR_ICON_32_PNG)]
        window.iconphoto(True, *images)
        return images
    except tk.TclError:
        return []


def _pick_theme(style: ttk.Style) -> None:
    preferred = {"win32": "vista", "darwin": "aqua"}.get(sys.platform, "clam")
    if preferred in style.theme_names():
        style.theme_use(preferred)


class ScriptWindow:
    def __init__(self, title: str, description: str, min_width: int = 460, min_height: int = 320,
                 keep_on_top: bool = True):
        _enable_dpi_awareness()
        self.root = tk.Tk()
        self.root.title(title)
        self._icons = set_tapir_icon(self.root)
        self.root.minsize(min_width, min_height)

        self.style = ttk.Style(self.root)
        _pick_theme(self.style)
        family = tkfont.nametofont("TkDefaultFont").actual("family")
        self.style.configure("Title.TLabel", font=(family, 14, "bold"))
        self.style.configure("Description.TLabel", foreground="#555555")
        self.style.configure("Big.TLabel", font=(family, 22, "bold"))
        self.style.configure("Muted.TLabel", foreground="#777777")

        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(2, weight=1)

        header = ttk.Frame(self.root, padding=(14, 12, 14, 8))
        header.grid(row=0, column=0, sticky="ew")
        header.columnconfigure(0, weight=1)
        ttk.Label(header, text=title, style="Title.TLabel").grid(row=0, column=0, sticky="w")
        self._description = ttk.Label(header, text=description, style="Description.TLabel", justify="left")
        self._description.grid(row=1, column=0, sticky="ew", pady=(4, 0))
        header.bind("<Configure>", lambda e: self._description.configure(wraplength=max(200, e.width - 28)))

        ttk.Separator(self.root).grid(row=1, column=0, sticky="new")

        self.body = ttk.Frame(self.root, padding=(14, 10))
        self.body.grid(row=2, column=0, sticky="nsew")
        self.body.columnconfigure(0, weight=1)

        ttk.Separator(self.root).grid(row=3, column=0, sticky="ew")

        footer = ttk.Frame(self.root, padding=(14, 8))
        footer.grid(row=4, column=0, sticky="ew")
        footer.columnconfigure(0, weight=1)
        self._status = ttk.Label(footer, text="", foreground=STATUS_COLORS["info"])
        self._status.grid(row=0, column=0, columnspan=2, sticky="w", pady=(0, 6))

        self.keep_on_top = tk.BooleanVar(value=keep_on_top)
        ttk.Checkbutton(footer, text="Keep on top", variable=self.keep_on_top,
                        command=self._apply_topmost).grid(row=1, column=0, sticky="w")
        self._buttons = ttk.Frame(footer)
        self._buttons.grid(row=1, column=1, sticky="e")
        self._button_widgets: list[ttk.Button] = []
        self._apply_topmost()

    def _apply_topmost(self) -> None:
        self.root.attributes("-topmost", self.keep_on_top.get())

    def add_button(self, text: str, command: Callable[[], None], primary: bool = False) -> ttk.Button:
        button = ttk.Button(self._buttons, text=text, command=self.guarded(command),
                            default="active" if primary else "normal")
        button.pack(side="left", padx=(6, 0))
        self._button_widgets.append(button)
        if primary:
            self.root.bind("<Return>", lambda _e: button.invoke())
        return button

    def set_status(self, text: str, kind: str = "info") -> None:
        self._status.configure(text=text, foreground=STATUS_COLORS.get(kind, STATUS_COLORS["info"]))
        self.root.update_idletasks()

    @contextlib.contextmanager
    def busy(self, text: str):
        """ Shows text in the status line and disables the buttons while a long call runs. """
        # instate, not b["state"]: the -state option does not follow b.state([...])
        was_disabled = [b.instate(["disabled"]) for b in self._button_widgets]
        for b in self._button_widgets:
            b.state(["disabled"])
        self.root.configure(cursor="watch")
        self.set_status(text)
        self.root.update()
        try:
            yield
        finally:
            for b, disabled in zip(self._button_widgets, was_disabled):
                if b.winfo_exists():
                    b.state(["disabled"] if disabled else ["!disabled"])
            self.root.configure(cursor="")

    def guarded(self, action: Callable[[], None]) -> Callable[[], None]:
        """ Wraps an action so a failure is shown in the window instead of killing it. """
        def run():
            try:
                action()
            except ArchicadError as e:
                self.set_status(str(e), "error")
                print(f"Error: {e}")
            except Exception as e:  # noqa: BLE001 - the window must stay usable
                traceback.print_exc()
                self.set_status(f"Unexpected error: {type(e).__name__}: {e}", "error")
        return run

    def ask_ok_cancel(self, title: str, message: str) -> bool:
        return messagebox.askokcancel(title, message, parent=self.root)

    def run(self, on_start: Callable[[], None] | None = None, on_close: Callable[[], None] | None = None,
            can_close: Callable[[], bool] | None = None) -> None:
        """ can_close may keep the window open, e.g. after asking the user. """
        def close():
            if can_close and not can_close():
                return
            if on_close:
                self.guarded(on_close)()
            self.root.destroy()
        self.root.protocol("WM_DELETE_WINDOW", close)
        self.root.bind("<Escape>", lambda _e: close())
        if on_start:
            self.root.after(50, self.guarded(on_start))
        self.root.mainloop()


def require_archicad(script_name: str) -> None:
    """ Stops with a clear message when the Archicad the script was started for does not answer. """
    if is_alive():
        return
    root = tk.Tk()
    root.withdraw()
    icons = set_tapir_icon(root)  # noqa: F841 - kept until the message is closed
    messagebox.showerror(script_name, "Cannot connect to Archicad.\n\n"
                                      "Start this script from the Tapir palette of a running Archicad.")
    root.destroy()
    sys.exit(1)


def make_tree(parent: tk.Misc, columns: list[tuple[str, str, int, str]], height: int = 10,
              show_tree: bool = False) -> ttk.Treeview:
    """ A Treeview with a vertical scrollbar. columns: (id, heading, width, anchor). """
    frame = ttk.Frame(parent)
    frame.columnconfigure(0, weight=1)
    frame.rowconfigure(0, weight=1)
    tree = ttk.Treeview(frame, columns=[c[0] for c in columns], height=height,
                        show="tree headings" if show_tree else "headings", selectmode="extended")
    for column_id, heading, width, anchor in columns:
        tree.heading(column_id, text=heading, anchor=anchor)
        tree.column(column_id, width=width, anchor=anchor, stretch=column_id == columns[0][0] and not show_tree)
    scrollbar = ttk.Scrollbar(frame, orient="vertical", command=tree.yview)
    tree.configure(yscrollcommand=scrollbar.set)
    tree.grid(row=0, column=0, sticky="nsew")
    scrollbar.grid(row=0, column=1, sticky="ns")
    tree.container = frame
    return tree
