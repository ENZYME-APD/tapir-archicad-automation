# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

"""
Batch Publisher

Publishes the chosen Publisher Sets in one go, now or on a schedule (every N
minutes or hours, or daily at a given time on the chosen weekdays). Before
publishing it can receive the Teamwork changes and update the drawings on the
layouts. The schedule runs while this window is open. The settings are saved
per project, the runs are logged in the window and in a CSV file next to the
settings (Documents/Tapir).
"""

import csv
import datetime
import json
import os
import re
import time
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, ttk

from utilities.archicad import ArchicadError, is_alive, run_command, run_tapir
from utilities.navigator_utils import get_unique_navigator_items_from_tree
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Batch Publisher"
CHECKED, UNCHECKED = "☑", "☐"
WEEKDAYS = ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"]
UNITS = {"minutes": 1, "hours": 60}


def settings_folder() -> Path:
    """ Documents/Tapir, where Tapir keeps its own config.json as well. """
    documents = Path.home() / "Documents"
    return (documents if documents.is_dir() else Path.home()) / "Tapir"


SETTINGS_FILE = settings_folder() / "BatchPublisher.json"
LOG_FILE = settings_folder() / "BatchPublisher log.csv"


def safe_folder_name(name: str) -> str:
    return re.sub(r'[<>:"/\\|?*\x00-\x1f]', "_", name).strip(" .") or "Publisher Set"


def parse_time(text: str) -> tuple[int, int] | None:
    match = re.fullmatch(r"\s*(\d{1,2})[:.](\d{2})\s*", text)
    if not match or int(match.group(1)) > 23 or int(match.group(2)) > 59:
        return None
    return int(match.group(1)), int(match.group(2))


def next_daily_run(after: datetime.datetime, at: tuple[int, int], weekdays: set[int]) -> datetime.datetime | None:
    """ The first moment after the given one at the time of day on one of the weekdays (0 = Monday). """
    for days in range(8):
        day = after.date() + datetime.timedelta(days=days)
        candidate = datetime.datetime.combine(day, datetime.time(*at))
        if candidate > after and candidate.weekday() in weekdays:
            return candidate
    return None


def format_duration(seconds: float) -> str:
    seconds = int(round(seconds))
    hours, rest = divmod(seconds, 3600)
    minutes, seconds = divmod(rest, 60)
    return f"{hours}:{minutes:02d}:{seconds:02d}" if hours else f"{minutes}:{seconds:02d}"


class BatchPublisher:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Publishes the checked Publisher Sets now or on a schedule. The schedule runs while this window is open; "
            "the Tapir palette runs no other script meanwhile. Archicad waits with the publishing while a dialog "
            "is open in it.",
            min_width=640, min_height=680, keep_on_top=False)
        self.project_key = "untitled"
        self.project_name = ""
        self.is_teamwork = False
        self.publisher_sets: list[str] = []
        self.checked: dict[str, bool] = {}
        self.next_run: datetime.datetime | None = None
        self._build_body()
        self.window.add_button("Publish now", self.publish_now)
        # not the primary button: Enter in the time field must not start the schedule
        self.schedule_button = self.window.add_button("Start schedule", self.toggle_schedule)

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(0, weight=1)
        body.rowconfigure(4, weight=1)

        sets = ttk.LabelFrame(body, text="Publisher Sets", padding=(10, 6))
        sets.grid(row=0, column=0, sticky="nsew")
        sets.columnconfigure(0, weight=1)
        sets.rowconfigure(0, weight=1)
        self.set_tree = make_tree(sets, [("check", "Publish", 64, "center"), ("name", "Publisher Set", 300, "w")], height=4)
        self.set_tree.column("name", stretch=True)
        self.set_tree.column("check", stretch=False)
        self.set_tree.container.grid(row=0, column=0, columnspan=3, sticky="nsew")
        self.set_tree.bind("<ButtonRelease-1>", self._on_set_click)
        self.set_tree.bind("<space>", lambda _e: self._toggle_set(self.set_tree.focus()))
        ttk.Button(sets, text="Check all", command=lambda: self._set_all(True)).grid(row=1, column=1, pady=(6, 0))
        ttk.Button(sets, text="Uncheck all", command=lambda: self._set_all(False)).grid(row=1, column=2, padx=(6, 0), pady=(6, 0))

        before = ttk.LabelFrame(body, text="Before publishing", padding=(10, 6))
        before.grid(row=1, column=0, sticky="ew", pady=(8, 0))
        self.receive = tk.BooleanVar(value=False)
        self.receive_box = ttk.Checkbutton(before, text="Receive the changes of the Teamwork project", variable=self.receive)
        self.receive_box.grid(row=0, column=0, sticky="w")
        self.update_drawings = tk.BooleanVar(value=False)
        ttk.Checkbutton(before, text="Update the drawings on the layouts", variable=self.update_drawings).grid(
            row=1, column=0, sticky="w", pady=(4, 0))

        output = ttk.LabelFrame(body, text="Output", padding=(10, 6))
        output.grid(row=2, column=0, sticky="ew", pady=(8, 0))
        output.columnconfigure(1, weight=1)
        self.output_mode = tk.StringVar(value="own")
        ttk.Radiobutton(output, text="Where each Publisher Set publishes to", value="own", variable=self.output_mode).grid(
            row=0, column=0, columnspan=3, sticky="w")
        ttk.Radiobutton(output, text="Into this folder:", value="folder", variable=self.output_mode).grid(
            row=1, column=0, sticky="w", pady=(4, 0))
        self.folder = tk.StringVar()
        ttk.Entry(output, textvariable=self.folder).grid(row=1, column=1, sticky="ew", padx=(6, 0), pady=(4, 0))
        ttk.Button(output, text="Browse...", command=self._browse).grid(row=1, column=2, padx=(6, 0), pady=(4, 0))
        self.subfolder = tk.BooleanVar(value=True)
        ttk.Checkbutton(output, text="A new subfolder for each run (date and time), with one folder per Publisher Set",
                        variable=self.subfolder).grid(row=2, column=0, columnspan=3, sticky="w", pady=(4, 0))

        schedule = ttk.LabelFrame(body, text="Schedule", padding=(10, 6))
        schedule.grid(row=3, column=0, sticky="ew", pady=(8, 0))
        self.schedule_mode = tk.StringVar(value="daily")
        self.interval = tk.StringVar(value="60")
        self.unit = tk.StringVar(value="minutes")
        self.daily_time = tk.StringVar(value="22:00")
        self.weekdays = [tk.BooleanVar(value=i < 5) for i in range(7)]
        self.schedule_widgets = [
            ttk.Radiobutton(schedule, text="Every", value="interval", variable=self.schedule_mode),
            ttk.Spinbox(schedule, textvariable=self.interval, from_=1, to=1440, width=6),
            ttk.Combobox(schedule, textvariable=self.unit, values=list(UNITS), width=8, state="readonly"),
            ttk.Radiobutton(schedule, text="Daily at", value="daily", variable=self.schedule_mode),
            ttk.Entry(schedule, textvariable=self.daily_time, width=7),
        ]
        for column, widget in enumerate(self.schedule_widgets[:3]):
            widget.grid(row=0, column=column, sticky="w", padx=(0 if column == 0 else 6, 0))
        self.schedule_widgets[3].grid(row=1, column=0, sticky="w", pady=(6, 0))
        self.schedule_widgets[4].grid(row=1, column=1, sticky="w", padx=(6, 0), pady=(6, 0))
        ttk.Label(schedule, text="on").grid(row=1, column=2, sticky="w", padx=(6, 0), pady=(6, 0))
        days = ttk.Frame(schedule)
        days.grid(row=1, column=3, sticky="w", padx=(6, 0), pady=(6, 0))
        for index, (name, var) in enumerate(zip(WEEKDAYS, self.weekdays)):
            box = ttk.Checkbutton(days, text=name, variable=var)
            box.grid(row=0, column=index, sticky="w", padx=(0, 4))
            self.schedule_widgets.append(box)
        self.next_label = ttk.Label(schedule, text="The schedule is off.", style="Muted.TLabel")
        self.next_label.grid(row=2, column=0, columnspan=4, sticky="w", pady=(8, 0))

        log = ttk.LabelFrame(body, text="Runs", padding=(10, 6))
        log.grid(row=4, column=0, sticky="nsew", pady=(8, 0))
        log.columnconfigure(0, weight=1)
        log.rowconfigure(0, weight=1)
        self.log_tree = make_tree(log, [("time", "Time", 130, "w"), ("step", "Publisher Set / step", 200, "w"),
                                        ("result", "Result", 220, "w"), ("duration", "Took", 60, "e")], height=5)
        self.log_tree.column("time", stretch=False)
        self.log_tree.column("result", stretch=True)
        self.log_tree.container.grid(row=0, column=0, sticky="nsew")
        self.log_tree.tag_configure("failed", foreground="#c62828")
        self.log_tree.tag_configure("ok", foreground="#1b7f3b")

    # ---- publisher sets ----

    def load(self) -> None:
        info = run_tapir("GetProjectInfo")
        self.is_teamwork = bool(info.get("isTeamwork"))
        self.project_name = info.get("projectName", "")
        self.project_key = info.get("projectLocation") or info.get("projectPath") or self.project_name or "untitled"
        self.publisher_sets = run_command("API.GetPublisherSetNames").get("publisherSetNames", [])
        self._load_settings()
        self.receive_box.state(["!disabled"] if self.is_teamwork else ["disabled"])
        if not self.is_teamwork:
            self.receive.set(False)
        self.set_tree.delete(*self.set_tree.get_children())
        for index, name in enumerate(self.publisher_sets):
            self.set_tree.insert("", "end", iid=str(index), values=("", name))
        self._refresh_marks()
        if not self.publisher_sets:
            self.window.set_status("The project has no Publisher Sets.", "warning")
        else:
            self.window.set_status(f"{len(self.publisher_sets)} Publisher Set(s). Check the ones to publish.")

    def _refresh_marks(self) -> None:
        for index, name in enumerate(self.publisher_sets):
            self.set_tree.set(str(index), "check", CHECKED if self.checked.get(name) else UNCHECKED)

    def _toggle_set(self, row: str) -> None:
        if row:
            name = self.publisher_sets[int(row)]
            self.checked[name] = not self.checked.get(name)
            self._refresh_marks()

    def _on_set_click(self, event) -> None:
        if self.set_tree.identify_region(event.x, event.y) == "cell":
            self._toggle_set(self.set_tree.identify_row(event.y))

    def _set_all(self, state: bool) -> None:
        self.checked = {name: state for name in self.publisher_sets}
        self._refresh_marks()

    def _checked_sets(self) -> list[str]:
        return [name for name in self.publisher_sets if self.checked.get(name)]

    def _browse(self) -> None:
        folder = filedialog.askdirectory(parent=self.window.root, title="Publish into", initialdir=self.folder.get() or None)
        if folder:
            self.folder.set(folder)
            self.output_mode.set("folder")

    # ---- settings ----

    def _settings(self) -> dict:
        return {
            "publisherSets": self._checked_sets(),
            "receive": self.receive.get(),
            "updateDrawings": self.update_drawings.get(),
            "outputMode": self.output_mode.get(),
            "folder": self.folder.get(),
            "subfolder": self.subfolder.get(),
            "scheduleMode": self.schedule_mode.get(),
            "interval": self.interval.get(),
            "unit": self.unit.get(),
            "time": self.daily_time.get(),
            "weekdays": [i for i, var in enumerate(self.weekdays) if var.get()],
        }

    @staticmethod
    def _read_settings_file() -> dict:
        try:
            with open(SETTINGS_FILE, encoding="utf-8") as file:
                data = json.load(file)
            return data if isinstance(data, dict) else {}
        except (OSError, ValueError):
            return {}

    def _load_settings(self) -> None:
        settings = self._read_settings_file().get(self.project_key)
        if not isinstance(settings, dict):
            self.checked = {name: True for name in self.publisher_sets}
            return
        self.checked = {name: name in settings.get("publisherSets", []) for name in self.publisher_sets}
        for var, key in ((self.receive, "receive"), (self.update_drawings, "updateDrawings"), (self.subfolder, "subfolder")):
            var.set(bool(settings.get(key, var.get())))
        for var, key in ((self.output_mode, "outputMode"), (self.folder, "folder"), (self.schedule_mode, "scheduleMode"),
                         (self.interval, "interval"), (self.unit, "unit"), (self.daily_time, "time")):
            var.set(str(settings.get(key, var.get())))
        weekdays = settings.get("weekdays")
        if isinstance(weekdays, list):
            for index, var in enumerate(self.weekdays):
                var.set(index in weekdays)

    def save_settings(self) -> None:
        data = self._read_settings_file()
        data[self.project_key] = self._settings()
        try:
            SETTINGS_FILE.parent.mkdir(parents=True, exist_ok=True)
            with open(SETTINGS_FILE, "w", encoding="utf-8") as file:
                json.dump(data, file, indent=2, ensure_ascii=False)
        except OSError as e:
            print(f"Cannot save the settings to {SETTINGS_FILE}: {e}")

    # ---- publishing ----

    def _log(self, step: str, result: str, ok: bool, seconds: float | None = None) -> None:
        now = datetime.datetime.now()
        duration = format_duration(seconds) if seconds is not None else ""
        self.log_tree.insert("", 0, values=(now.strftime("%Y-%m-%d %H:%M"), step, result, duration),
                             tags=("ok" if ok else "failed",))
        print(f"{now:%Y-%m-%d %H:%M:%S}  {step}: {result} {duration}")
        try:
            new_file = not LOG_FILE.exists()
            LOG_FILE.parent.mkdir(parents=True, exist_ok=True)
            with open(LOG_FILE, "a", newline="", encoding="utf-8-sig" if new_file else "utf-8") as file:
                writer = csv.writer(file)
                if new_file:
                    writer.writerow(["Time", "Project", "Publisher Set / step", "Result", "Seconds"])
                writer.writerow([now.isoformat(timespec="seconds"), self.project_name, step, result,
                                 "" if seconds is None else round(seconds)])
        except OSError as e:
            print(f"Cannot write the log file {LOG_FILE}: {e}")

    def _output_folder(self) -> str | None:
        """ The folder to publish into, or None for each set's own; checked before a run. """
        if self.output_mode.get() != "folder":
            return None
        folder = self.folder.get().strip()
        if not folder:
            raise ValueError("Choose the folder to publish into.")
        if not os.path.isdir(folder):
            raise ValueError(f"The folder does not exist: {folder}")
        return folder

    def _update_drawings(self) -> int:
        tree = run_command("API.GetNavigatorItemTree", {"navigatorTreeId": {"type": "LayoutBook"}})
        layouts: dict = {}
        get_unique_navigator_items_from_tree(tree["navigatorTree"]["rootItem"].get("children", []), layouts, ["LayoutItem"])
        if not layouts:
            return 0
        databases = run_tapir("GetDatabaseIdFromNavigatorItemId", {
            "navigatorItemIds": [{"navigatorItemId": item["navigatorItemId"]} for item in layouts.values()]}).get("databases", [])
        databases = [d for d in databases if "databaseId" in d]
        drawings = run_tapir("GetElementsByType", {"elementType": "Drawing", "databases": databases}).get("elements", [])
        if drawings:
            run_tapir("UpdateDrawings", {"elements": drawings})
        return len(drawings)

    def publish(self, trigger: str) -> None:
        sets = self._checked_sets()
        if not sets:
            self.window.set_status("Check at least one Publisher Set.", "warning")
            return
        try:
            folder = self._output_folder()
        except ValueError as e:
            self._log(trigger, f"Not published: {e}", False)
            self.window.set_status(str(e), "error")
            return
        self.save_settings()
        failures = 0
        with self.window.busy(f"Publishing {len(sets)} Publisher Set(s)..."):
            run_folder = folder
            if folder and self.subfolder.get():
                run_folder = os.path.join(folder, datetime.datetime.now().strftime("%Y-%m-%d %H-%M"))
            if self.receive.get() and self.is_teamwork:
                self.window.set_status("Receiving the Teamwork changes...")
                failures += not self._step("Receive changes", self._receive)
            if self.update_drawings.get():
                self.window.set_status("Updating the drawings...")
                failures += not self._step("Update drawings", lambda: f"Updated {self._update_drawings()} drawing(s)")
            for index, name in enumerate(sets, start=1):
                self.window.set_status(f"Publishing {name} ({index} of {len(sets)})...")
                failures += not self._step(name, lambda: self._publish_set(name, run_folder, len(sets)))
        if failures and not is_alive():
            self.stop_schedule()
            self.window.set_status("Archicad does not answer any more; the schedule is stopped.", "error")
        elif failures:
            self.window.set_status(f"{trigger}: done with {failures} failed step(s). See the runs below.", "warning")
        else:
            self.window.set_status(f"{trigger}: published {len(sets)} Publisher Set(s).", "ok")

    @staticmethod
    def _receive() -> str:
        run_tapir("TeamworkReceive")
        return "Received"

    def _publish_set(self, name: str, run_folder: str | None, set_count: int) -> str:
        parameters = {"publisherSetName": name}
        if run_folder:
            # several sets in one folder would overwrite each other's files
            path = os.path.join(run_folder, safe_folder_name(name)) if self.subfolder.get() or set_count > 1 else run_folder
            os.makedirs(path, exist_ok=True)
            parameters["outputPath"] = path
        run_tapir("PublishPublisherSet", parameters)
        return f"Published to {parameters.get('outputPath', 'its own folder')}"

    def _step(self, step: str, action) -> bool:
        started = time.monotonic()
        try:
            result = action()
        except (ArchicadError, OSError) as e:
            self._log(step, f"Failed: {e}", False, time.monotonic() - started)
            return False
        self._log(step, str(result), True, time.monotonic() - started)
        return True

    def publish_now(self) -> None:
        self.publish("Publish now")
        self._show_next_run()

    # ---- schedule ----

    def _compute_next_run(self, after: datetime.datetime) -> datetime.datetime:
        if self.schedule_mode.get() == "interval":
            try:
                amount = int(self.interval.get())
            except ValueError:
                amount = 0
            if amount < 1:
                raise ValueError("The interval must be a whole number, 1 or more.")
            return after + datetime.timedelta(minutes=amount * UNITS.get(self.unit.get(), 1))
        at = parse_time(self.daily_time.get())
        if at is None:
            raise ValueError("Give the time of day as HH:MM, e.g. 22:00.")
        weekdays = {i for i, var in enumerate(self.weekdays) if var.get()}
        if not weekdays:
            raise ValueError("Check at least one weekday.")
        return next_daily_run(after, at, weekdays)

    def toggle_schedule(self) -> None:
        if self.next_run:
            self.stop_schedule()
            self.window.set_status("The schedule is stopped.")
            return
        if not self._checked_sets():
            self.window.set_status("Check at least one Publisher Set.", "warning")
            return
        try:
            self._output_folder()
            self.next_run = self._compute_next_run(datetime.datetime.now())
        except ValueError as e:
            self.window.set_status(str(e), "error")
            return
        self.save_settings()
        for widget in self.schedule_widgets:
            widget.state(["disabled"])
        self.schedule_button.configure(text="Stop schedule")
        self._log("Schedule", f"Started, first run {self.next_run:%a %Y-%m-%d %H:%M}", True)
        self.window.set_status("The schedule is on. Keep this window open.", "ok")
        self._show_next_run()

    def stop_schedule(self) -> None:
        if self.next_run:
            self._log("Schedule", "Stopped", True)
        self.next_run = None
        for widget in self.schedule_widgets:
            widget.state(["!disabled"])
        self.schedule_button.configure(text="Start schedule")
        self._show_next_run()

    def _show_next_run(self) -> None:
        if not self.next_run:
            self.next_label.configure(text="The schedule is off.", foreground="#777777")
            return
        left = (self.next_run - datetime.datetime.now()).total_seconds()
        self.next_label.configure(text=f"Next run: {self.next_run:%a %Y-%m-%d %H:%M}  (in {format_duration(max(0, left))})",
                                  foreground="#1b7f3b")

    def _tick(self) -> None:
        try:
            if self.next_run and datetime.datetime.now() >= self.next_run:
                self.window.guarded(lambda: self.publish("Scheduled run"))()
                if self.next_run:
                    try:
                        self.next_run = self._compute_next_run(datetime.datetime.now())
                    except ValueError as e:
                        self.stop_schedule()
                        self.window.set_status(str(e), "error")
            self._show_next_run()
        finally:
            self.window.root.after(1000, self._tick)

    def can_close(self) -> bool:
        return not self.next_run or self.window.ask_ok_cancel(
            SCRIPT_NAME, "The schedule stops when this window closes. Close it?")

    def on_close(self) -> None:
        if self.publisher_sets:
            self.save_settings()

    def run(self) -> None:
        self.window.root.after(1000, self._tick)
        self.window.run(on_start=self.load, on_close=self.on_close, can_close=self.can_close)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    BatchPublisher().run()
