""" A common window layout for the built-in scripts.

Every script window has the same parts: a header with the script's name and a
short explanation, a body the script fills in, and a footer with a status line,
a "Keep on top" switch and the action buttons. Only tkinter/ttk is used, so the
scripts need no extra packages.
"""

import contextlib
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


def _pick_theme(style: ttk.Style) -> None:
    preferred = {"win32": "vista", "darwin": "aqua"}.get(sys.platform, "clam")
    if preferred in style.theme_names():
        style.theme_use(preferred)


class ScriptWindow:
    def __init__(self, title: str, description: str, min_width: int = 460, min_height: int = 320):
        _enable_dpi_awareness()
        self.root = tk.Tk()
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

        self.keep_on_top = tk.BooleanVar(value=True)
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

    def run(self, on_start: Callable[[], None] | None = None, on_close: Callable[[], None] | None = None) -> None:
        def close():
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
