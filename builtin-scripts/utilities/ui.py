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


def _use_own_taskbar_entry() -> None:
    """ Lets Windows show the window's own icon in the taskbar.

    Without an application ID of its own, Windows groups the window with Python
    and shows Python's icon in the taskbar, whatever the window's icon is.
    """
    if sys.platform != "win32":
        return
    try:
        import ctypes
        ctypes.windll.shell32.SetCurrentProcessExplicitAppUserModelID("ENZYME-APD.Tapir.Script")
    except (AttributeError, OSError):
        pass


def set_tapir_icon(root: tk.Tk) -> None:
    """ Shows the Tapir logo instead of Tk's feather in the window's title bar and taskbar.

    default=True gives it to the dialogs the script opens later too. Only cosmetic,
    so a Tk that cannot read the PNG leaves the feather instead of stopping the script.
    """
    try:
        root._tapir_icon = tk.PhotoImage(master=root, data=_TAPIR_ICON_PNG_B64)
        root.iconphoto(True, root._tapir_icon)
    except Exception:  # noqa: BLE001 - never worth stopping a script over its icon
        pass


def _pick_theme(style: ttk.Style) -> None:
    preferred = {"win32": "vista", "darwin": "aqua"}.get(sys.platform, "clam")
    if preferred in style.theme_names():
        style.theme_use(preferred)


class ScriptWindow:
    def __init__(self, title: str, description: str, min_width: int = 460, min_height: int = 320,
                 keep_on_top: bool = True):
        _enable_dpi_awareness()
        _use_own_taskbar_entry()
        self.root = tk.Tk()
        set_tapir_icon(self.root)
        self.root.title(title)
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
    _use_own_taskbar_entry()
    root = tk.Tk()
    set_tapir_icon(root)
    root.withdraw()
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


# The Tapir logo (128x128 PNG, base64), the same as in archicad-addon/Workflows/sync_attributes_gui.py.
# It is embedded rather than shipped as a file because installing a marketplace script copies only
# the helper files ScriptManager.cpp lists next to it.
_TAPIR_ICON_PNG_B64 = (
    "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"
)
