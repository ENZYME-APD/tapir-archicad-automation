"""'gui': a small window for testers. Lists the open pull requests with their checks, builds and
last results; tests the selected ones or everything due; tests what is labelled every pollMinutes
unless paused; shows what happens. Uses the same functions as the command line (tkinter, standard
library). The runner never touches an Archicad that is open: close it and the next test runs.
"""
from __future__ import annotations

import queue
import subprocess
import sys
import threading
import time
import tkinter as tk
import webbrowser
from pathlib import Path
from tkinter import ttk

import tapir_test_runner as r


class RunnerWindow:
    def __init__(self, root: tk.Tk, cfg: dict):
        self.root, self.cfg = root, cfg
        self.events: queue.Queue = queue.Queue()
        self.busy = False
        self.stop_requested = False
        self.paused = tk.BooleanVar(value=False)
        self.post = tk.BooleanVar(value=True)
        self.every = tk.BooleanVar(value=False)
        self.next_pass = time.monotonic() + 5
        versions = ", ".join("AC" + str(a["version"]) for a in cfg["archicads"]) or "no Archicad configured"

        root.title("Tapir test runner")
        root.geometry("980x560")
        root.minsize(720, 420)
        top = ttk.Frame(root, padding=(10, 8))
        top.pack(fill="x")
        self.status = ttk.Label(top, text=f"{versions} - starting")
        self.status.pack(side="left")
        ttk.Checkbutton(top, text="Pause automatic testing", variable=self.paused).pack(side="right")

        columns = ("pr", "title", "branch", "checks", "builds", "last")
        widths = (60, 330, 150, 60, 120, 240)
        frame = ttk.Frame(root, padding=(10, 0))
        frame.pack(fill="both", expand=True)
        self.table = ttk.Treeview(frame, columns=columns, show="headings", selectmode="extended", height=10)
        for name, width in zip(columns, widths):
            self.table.heading(name, text=name.capitalize() if name != "pr" else "PR")
            self.table.column(name, width=width, stretch=name in ("title", "last"))
        self.table.tag_configure("untestable", foreground="gray")
        self.table.pack(side="left", fill="both", expand=True)
        bar = ttk.Scrollbar(frame, orient="vertical", command=self.table.yview)
        bar.pack(side="right", fill="y")
        self.table.configure(yscrollcommand=bar.set)
        self.table.bind("<Double-1>", lambda _e: self.open_selected())

        buttons = ttk.Frame(root, padding=(10, 6))
        buttons.pack(fill="x")
        self.actions = [ttk.Button(buttons, text="Test selected", command=self.test_selected),
                        ttk.Button(buttons, text="Test everything due", command=self.test_due),
                        ttk.Button(buttons, text="Refresh", command=self.refresh),
                        ttk.Button(buttons, text="Open on GitHub", command=self.open_selected),
                        ttk.Button(buttons, text="Put my add-on back", command=self.restore)]
        for button in self.actions:
            button.pack(side="left", padx=(0, 6))
        self.stop_button = ttk.Button(buttons, text="Stop after this test", command=self.request_stop, state="disabled")
        self.stop_button.pack(side="left")
        ttk.Button(buttons, text="Help", command=self.help).pack(side="right", padx=(6, 0))
        ttk.Button(buttons, text="Settings", command=self.settings).pack(side="right", padx=(6, 0))
        options = ttk.Frame(root, padding=(10, 0, 10, 6))
        options.pack(fill="x")
        ttk.Checkbutton(options, text="Post results on GitHub", variable=self.post).pack(side="left")
        ttk.Checkbutton(options, text="Run every check, not only the change's own", variable=self.every).pack(side="left", padx=12)

        self.log_box = tk.Text(root, height=9, wrap="word", state="disabled", padx=6, pady=4)
        self.log_box.pack(fill="both", padx=10, pady=(0, 10))
        self.log("Close Archicad to let tests run; your own Tapir is put back after every test.")
        self.refresh()
        root.after(200, self.pump)

    # --- the worker ------------------------------------------------------------------------------

    def work(self, label: str, job) -> None:
        if self.busy:
            self.log("busy - wait for the current test to finish")
            return
        self.busy, self.stop_requested = True, False
        self.set_buttons("disabled")
        self.events.put(("status", label))

        def target():
            try:
                job()
            except Exception as exc:  # noqa: BLE001 - shown to the tester instead of a silent thread
                text = str(exc)
                if "no GitHub token" in text or "request limit" in text:
                    text = "No working GitHub token. Click Settings and paste a token (see Help, step 3)."
                self.events.put(("log", f"stopped: {text}"))
            finally:
                self.events.put(("done", None))
        threading.Thread(target=target, daemon=True).start()

    def log(self, text: str) -> None:
        self.log_box.configure(state="normal")
        self.log_box.insert("end", time.strftime("%H:%M ") + text + "\n")
        self.log_box.see("end")
        self.log_box.configure(state="disabled")

    def pump(self) -> None:
        while True:
            try:
                kind, value = self.events.get_nowait()
            except queue.Empty:
                break
            if kind == "log":
                self.log(value)
            elif kind == "status":
                self.status.configure(text=value)
            elif kind == "rows":
                self.fill(value)
            elif kind == "done":
                self.busy = False
                self.set_buttons("normal")
                self.next_pass = time.monotonic() + self.cfg.get("pollMinutes", 15) * 60
        if not self.busy:
            if self.paused.get():
                self.status.configure(text="Automatic testing paused")
            else:
                left = max(0, int(self.next_pass - time.monotonic()))
                self.status.configure(text=f"Next automatic pass in {left // 60} min {left % 60:02d} s")
                if left == 0:
                    self.test_due(automatic=True)
        self.root.after(500, self.pump)

    def set_buttons(self, state: str) -> None:
        for button in self.actions:
            button.configure(state=state)
        self.stop_button.configure(state="normal" if state == "disabled" else "disabled")

    def request_stop(self) -> None:
        self.stop_requested = True
        self.log("stopping after the current test")

    # --- actions ---------------------------------------------------------------------------------

    def fill(self, rows: list[dict]) -> None:
        self.table.delete(*self.table.get_children())
        for p in rows:
            builds = "AC" + ",".join(p["builds"]) if p["builds"] else "none yet"
            self.table.insert("", "end", iid=str(p["number"]), tags=() if p["testable"] else ("untestable",),
                              values=(f"#{p['number']}", p["title"], p["branch"], p["checks"], builds,
                                      "; ".join(p["last"]) or "-"))

    def refresh(self) -> None:
        def job():
            rows = r.pull_rows(self.cfg)
            self.events.put(("rows", rows))
            self.events.put(("log", f"{len(rows)} open pull request(s); gray ones are in forks you don't trust"))
        self.work("Reading pull requests ...", job)

    def run(self, numbers: list[int] | None) -> None:
        def job():
            r.run_pass(self.cfg, self.post.get(), numbers, every=self.every.get(), as_brief=True,
                       log=lambda text: self.events.put(("log", text)), stop=lambda: self.stop_requested)
            self.events.put(("rows", r.pull_rows(self.cfg)))
        self.work("Testing ...", job)

    def test_selected(self) -> None:
        numbers = [int(i) for i in self.table.selection()]
        if not numbers:
            self.log("select one or more pull requests first")
            return
        self.run(numbers)

    def test_due(self, automatic: bool = False) -> None:
        if automatic:
            self.next_pass = time.monotonic() + self.cfg.get("pollMinutes", 15) * 60
        self.run(None)

    def settings(self) -> None:
        """Runs 'setup' in a console window of its own (it asks questions); the window reads the result on restart."""
        python = Path(sys.executable)
        if python.name.lower() == "pythonw.exe":  # pythonw has no console to answer in
            python = python.with_name("python.exe")
        subprocess.Popen([str(python), str(Path(r.__file__).resolve()), "setup", "--config", self.cfg["_path"]],
                         creationflags=getattr(subprocess, "CREATE_NEW_CONSOLE", 0))
        self.log("Settings opened in a new window. Close and reopen this window afterwards to use them.")

    def help(self) -> None:
        # GitHub shows the guide formatted; a local .md often opens as plain text in Notepad
        webbrowser.open(f"https://github.com/{self.cfg['repo']}/blob/main/tools/community-test-runner/GUIDE.md")

    def open_selected(self) -> None:
        for number in self.table.selection():
            webbrowser.open(f"https://github.com/{self.cfg['repo']}/pull/{number}")

    def restore(self) -> None:
        def job():
            if not r.restore_all(self.cfg, lambda text: self.events.put(("log", text))):
                self.events.put(("log", "nothing to put back: your own add-on is loaded"))
        self.work("Putting your add-on back ...", job)


def run(cfg: dict) -> int:
    root = tk.Tk()
    RunnerWindow(root, cfg)
    root.mainloop()
    return 0
