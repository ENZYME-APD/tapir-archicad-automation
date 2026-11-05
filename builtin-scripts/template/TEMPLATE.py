# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

'''
Selected Elements Lister
by <your name>

A starting point for a Tapir palette script: a window that lists the elements
selected in Archicad. Copy this file next to the utilities/ and aclib/ folders
(into builtin-scripts/, or into a local script folder that holds copies of
them), rename it and change it. See builtin-scripts/README.md.
'''

from utilities.archicad import get_details_of_elements, run_tapir
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Selected Elements Lister"


class SelectedElementsLister:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Lists the elements selected in Archicad. Select other elements and click Refresh.")
        body = self.window.body
        body.rowconfigure(0, weight=1)
        self.tree = make_tree(body, [
            ("id", "Element ID", 160, "w"),
            ("type", "Type", 120, "w"),
            ("guid", "GUID", 280, "w"),
        ])
        self.tree.container.grid(row=0, column=0, sticky="nsew")
        self.window.add_button("Refresh", self.refresh, primary=True)

    def refresh(self) -> None:
        with self.window.busy("Reading the selection..."):
            elements = run_tapir("GetSelectedElements").get("elements", [])
            details = get_details_of_elements(elements, ["id", "type"])
        self.tree.delete(*self.tree.get_children())
        for element, detail in zip(elements, details):
            self.tree.insert("", "end", values=(
                detail.get("id") or "(no ID)", detail.get("type", ""), element["elementId"]["guid"]))
        self.window.set_status(f"{len(elements)} element(s) selected.", "ok" if elements else "info")

    def run(self) -> None:
        self.window.run(on_start=self.refresh)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    SelectedElementsLister().run()
