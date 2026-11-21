# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# category = "utilities"
# archicad-versions = [25, 26, 27, 28, 29, 30]
# min-tapir-version = "1.8.0"
# changes = "model"
# ///

"""
Tapir Note

Records the manual steps of a task that a script cannot do, and hands them to
a team member as an Archicad Issue: one issue named after the task, tagged
with the person it is for, with one comment per step and the selected
elements attached, so the team member finds it in the Issue Manager. In a
Teamwork project everyone sees it after Send; Export as BCF... writes it to a
.bcfzip file for someone outside the project.
"""

import datetime
import getpass
import os
import re
import sys
import tkinter as tk
from tkinter import filedialog, ttk

try:
    from utilities.archicad import ArchicadError, run_tapir
    from utilities.ui import ScriptWindow, require_archicad
except ModuleNotFoundError as error:
    # The palette of a Tapir newer than 2.0.0 provides the helpers; an older one needs copies next to the script.
    if (error.name or "").split(".")[0] not in ("utilities", "aclib"):
        raise
    sys.exit("Error: the Tapir helpers utilities and aclib were not found. Update the Tapir Add-On, or copy the "
             "utilities and aclib folders of builtin-scripts "
             "(https://github.com/ENZYME-APD/tapir-archicad-automation/tree/main/builtin-scripts) next to this script.")

SCRIPT_NAME = "Tapir Note"
DEFAULT_TAG = "Tapir Note"
# A list mark the user typed: "-", "*" or "•", or a number with "." or ")" that is not a decimal (1.5 m stays).
STEP_MARK = re.compile(r"^(?:[-*\u2022]+\s*|\d+[.)](?!\d)\s*)")


def current_user() -> str:
    try:
        return getpass.getuser()
    except Exception:  # noqa: BLE001 - no user name is not a reason to stop
        return ""


def steps_from_text(text: str) -> list[str]:
    """ One step per non-empty line; a leading "1." or "-" the user typed is dropped,
    as the script numbers the steps itself. """
    steps = []
    for line in text.splitlines():
        step = STEP_MARK.sub("", line.strip()).strip()
        if step:
            steps.append(step)
    return steps


class TapirNote:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Write down the steps someone has to do by hand, one per line, and create a note: an Archicad "
            "Issue with one comment per step, for the person you name, with the selected elements attached.",
            min_width=560, min_height=520)
        self.issue_id: dict | None = None
        self.issue_name = ""
        self._build_body()
        self.export_button = self.window.add_button("Export as BCF...", self.export_bcf)
        self.export_button.state(["disabled"])
        self.window.add_button("Create note", self.create_note, primary=True)

    def _build_body(self) -> None:
        body = self.window.body
        body.columnconfigure(1, weight=1)
        body.rowconfigure(3, weight=1)

        ttk.Label(body, text="Task").grid(row=0, column=0, sticky="w", padx=(0, 8))
        self.task = tk.StringVar()
        ttk.Entry(body, textvariable=self.task).grid(row=0, column=1, sticky="ew")

        ttk.Label(body, text="For").grid(row=1, column=0, sticky="w", padx=(0, 8), pady=(6, 0))
        self.assignee = tk.StringVar()
        ttk.Entry(body, textvariable=self.assignee).grid(row=1, column=1, sticky="ew", pady=(6, 0))

        ttk.Label(body, text="From").grid(row=2, column=0, sticky="w", padx=(0, 8), pady=(6, 0))
        self.author = tk.StringVar(value=current_user())
        ttk.Entry(body, textvariable=self.author).grid(row=2, column=1, sticky="ew", pady=(6, 0))

        steps = ttk.LabelFrame(body, text="Steps to do by hand, one per line", padding=(8, 6))
        steps.grid(row=3, column=0, columnspan=2, sticky="nsew", pady=(10, 0))
        steps.columnconfigure(0, weight=1)
        steps.rowconfigure(0, weight=1)
        self.steps = tk.Text(steps, height=10, wrap="word", undo=True)
        self.steps.grid(row=0, column=0, sticky="nsew")
        scroll = ttk.Scrollbar(steps, orient="vertical", command=self.steps.yview)
        scroll.grid(row=0, column=1, sticky="ns")
        self.steps.configure(yscrollcommand=scroll.set)

        self.attach = tk.BooleanVar(value=True)
        ttk.Checkbutton(body, text="Attach the selected elements", variable=self.attach).grid(
            row=4, column=0, columnspan=2, sticky="w", pady=(8, 0))

    # ---- actions ----

    def create_note(self) -> None:
        task = self.task.get().strip()
        steps = steps_from_text(self.steps.get("1.0", "end"))
        if not task:
            self.window.set_status("Give the task a name.", "warning")
            return
        if not steps:
            self.window.set_status("Write at least one step.", "warning")
            return
        assignee = self.assignee.get().strip()
        author = self.author.get().strip() or SCRIPT_NAME

        with self.window.busy("Creating the note..."):
            elements = run_tapir("GetSelectedElements").get("elements", []) if self.attach.get() else []
            issue = run_tapir("CreateIssue", {"name": task, "tagText": f"For {assignee}" if assignee else DEFAULT_TAG})
            issue_id = issue["issueId"]
            failures = []
            header = f"Manual steps for {assignee}." if assignee else "Manual steps."
            header += f" Written by {author} on {datetime.date.today().isoformat()}."
            comments = [header] + [f"Step {index} of {len(steps)}: {step}" for index, step in enumerate(steps, 1)]
            for index, text in enumerate(comments):
                self.window.set_status(f"Adding the steps... {index + 1} / {len(comments)}")
                try:
                    run_tapir("AddCommentToIssue", {"issueId": issue_id, "author": author, "status": "Info", "text": text})
                except ArchicadError as e:
                    failures.append(f"{text[:40]}: {e}")
            attached = 0
            if elements:
                try:
                    run_tapir("AttachElementsToIssue", {"issueId": issue_id, "elements": elements, "type": "Highlight"})
                    attached = len(elements)
                except ArchicadError as e:
                    failures.append(f"attaching the selected elements: {e}")

        self.issue_id = issue_id
        self.issue_name = task
        self.export_button.state(["!disabled"])
        for failure in failures:
            print(f"Not added: {failure}")
        summary = f"Created the issue \"{task}\" with {len(steps)} step(s)"
        summary += f" and {attached} attached element(s)." if attached else "."
        if failures:
            self.window.set_status(summary + f" {len(failures)} part(s) could not be added; see the palette output.", "warning")
        else:
            self.window.set_status(summary + " Find it in the Issue Manager; in Teamwork, Send to share it.", "ok")

    def export_bcf(self) -> None:
        if self.issue_id is None:
            self.window.set_status("Create a note first.", "warning")
            return
        name = "".join(c if c.isalnum() or c in " -_" else "_" for c in self.issue_name).strip() or "Tapir Note"
        path = filedialog.asksaveasfilename(parent=self.window.root, title="Export the note as BCF",
                                            initialfile=f"{name}.bcfzip", defaultextension=".bcfzip",
                                            filetypes=[("BCF file", "*.bcfzip"), ("All files", "*.*")])
        if not path:
            return
        with self.window.busy("Exporting the note..."):
            run_tapir("ExportIssuesToBCF", {"issues": [{"issueId": self.issue_id}], "exportPath": path,
                                            "useExternalId": False, "alignBySurveyPoint": True})
        self.window.set_status(f"Exported the note to {os.path.basename(path)}.", "ok")

    def run(self) -> None:
        self.window.run()


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    TapirNote().run()
