# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

"""
Solid Operation Explorer

Lists the solid element operation links (target, operation, operator) of the
elements, highlights the targets and operators, and finds the links that have
no effect any more because the two elements do not touch. The chosen links can
be removed.
"""

import tkinter as tk
from tkinter import ttk

from utilities.archicad import (batched, get_details_of_elements, get_elements, item_errors,
                                run_tapir, select_elements)
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Solid Operation Explorer"
TARGET_COLOR = [0, 110, 220, 255]
OPERATOR_COLOR = [240, 120, 0, 255]
DIMMED_COLOR = [200, 200, 200, 160]
# the element types that can take part in solid element operations
SOLID_TYPES = ["Wall", "Column", "Beam", "Slab", "Roof", "Shell", "Morph", "Mesh", "Object", "Lamp", "Stair", "CurtainWall"]
OPERATION_NAMES = {
    "Subtraction": "Subtraction",
    "SubtractionUpwards": "Subtraction, upwards",
    "SubtractionDownwards": "Subtraction, downwards",
    "Intersection": "Intersection",
    "Addition": "Addition",
}
NO_EFFECT = "Do not touch"
TOLERANCE = 0.001  # m


def boxes_touch(target: dict, operator: dict, operation: str) -> bool:
    """ Whether the operator can still change the target. Upwards and downwards
    subtractions extrude the operator, so for them only the plan and the side
    towards the target count. """
    if target["xMin"] > operator["xMax"] + TOLERANCE or operator["xMin"] > target["xMax"] + TOLERANCE:
        return False
    if target["yMin"] > operator["yMax"] + TOLERANCE or operator["yMin"] > target["yMax"] + TOLERANCE:
        return False
    if operation == "SubtractionUpwards":
        return target["zMax"] >= operator["zMin"] - TOLERANCE
    if operation == "SubtractionDownwards":
        return target["zMin"] <= operator["zMax"] + TOLERANCE
    return target["zMin"] <= operator["zMax"] + TOLERANCE and operator["zMin"] <= target["zMax"] + TOLERANCE


class SolidOperationExplorer:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Lists the solid element operations. Targets are highlighted in blue, operators in orange. "
            "Links between elements that do not touch have no effect and can usually be removed.",
            min_width=720, min_height=520)
        self.links: list[dict] = []
        self.listed: list[dict] = []
        self._build_body()
        self.window.add_button("Select in Archicad", self.select_listed)
        self.remove_button = self.window.add_button("Remove links", self.remove_links)
        self.window.add_button("Refresh", self.refresh, primary=True)

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(2, weight=1)

        options = ttk.LabelFrame(body, text="Elements", padding=(10, 6))
        options.grid(row=0, column=0, sticky="ew")
        self.scope = tk.StringVar(value="project")
        for column, (value, text) in enumerate([("selection", "Selection"), ("story", "Current story"),
                                                ("project", "Whole project")]):
            ttk.Radiobutton(options, text=text, value=value, variable=self.scope,
                            command=self.window.guarded(self.refresh)).grid(
                row=0, column=column, sticky="w", padx=(0 if column == 0 else 10, 0))
        self.only_no_effect = tk.BooleanVar(value=False)
        ttk.Checkbutton(options, text="Show only the links whose elements do not touch", variable=self.only_no_effect,
                        command=self.window.guarded(self._show)).grid(row=1, column=0, columnspan=3, sticky="w", pady=(6, 0))

        stats = ttk.Frame(body, padding=(0, 10, 0, 6))
        stats.grid(row=1, column=0, sticky="ew")
        self.stat_labels = {}
        for column, (key, caption) in enumerate([("links", "links"), ("targets", "targets"),
                                                 ("operators", "operators"), ("no_effect", "do not touch")]):
            stats.columnconfigure(column, weight=1)
            value = ttk.Label(stats, text="–", style="Big.TLabel")
            value.grid(row=0, column=column, sticky="w")
            ttk.Label(stats, text=caption, style="Muted.TLabel").grid(row=1, column=column, sticky="w")
            self.stat_labels[key] = value

        self.tree = make_tree(body, [
            ("target", "Target", 170, "w"),
            ("operation", "Operation", 150, "w"),
            ("operator", "Operator", 170, "w"),
            ("layer", "Operator layer", 140, "w"),
            ("status", "Status", 110, "w"),
        ], height=12)
        self.tree.container.grid(row=2, column=0, sticky="nsew")
        self.tree.tag_configure("no_effect", foreground="#a15c00")
        self.tree.bind("<<TreeviewSelect>>", lambda _e: self.window.guarded(self._highlight)())
        self.tree.bind("<Double-1>", lambda _e: self.window.guarded(self.select_clicked)())

    # ---- reading ----

    def refresh(self) -> None:
        with self.window.busy("Reading the elements..."):
            elements = get_elements(self.scope.get(), SOLID_TYPES)
            found: dict[tuple[str, str], dict] = {}
            for chunk in batched(elements):
                self.window.set_status(f"Reading the solid element operations... {len(found)} links found")
                result = run_tapir("GetSolidElementLinks", {"elements": chunk}).get("solidLinks", [])
                for element, entry in zip(chunk, result):
                    guid = element["elementId"]["guid"]
                    for link in entry.get("solidLinksWithTheGivenTarget", []):
                        found[(guid, link["operatorId"]["guid"])] = link
                    for link in entry.get("solidLinksWithTheGivenOperator", []):
                        found[(link["targetId"]["guid"], guid)] = link

            self.window.set_status(f"Reading the {len(found)} linked element pairs...")
            guids = sorted({guid for pair in found for guid in pair})
            element_ids = [{"elementId": {"guid": guid}} for guid in guids]
            details = dict(zip(guids, get_details_of_elements(element_ids, ["type", "id", "layerIndex"])))
            boxes = {}
            for chunk in batched(element_ids):
                result = run_tapir("Get3DBoundingBoxes", {"elements": chunk}).get("boundingBoxes3D", [])
                boxes.update({e["elementId"]["guid"]: b.get("boundingBox3D") for e, b in zip(chunk, result)})
            layers = {a["index"]: a["name"] for a in run_tapir("GetAttributesByType", {"attributeType": "Layer"}).get("attributes", [])}

        def describe(guid: str) -> str:
            detail = details.get(guid, {})
            if "error" in detail or not detail:
                return "(unreadable)"
            return f"{detail.get('id') or '(no ID)'}  ·  {detail.get('type', '')}"

        self.links = []
        for (target, operator), link in found.items():
            target_box, operator_box = boxes.get(target), boxes.get(operator)
            no_effect = bool(target_box and operator_box and not boxes_touch(target_box, operator_box, link["operation"]))
            operator_detail = details.get(operator, {})
            self.links.append({
                "targetId": {"guid": target},
                "operatorId": {"guid": operator},
                "target": describe(target),
                "operator": describe(operator),
                "operation": OPERATION_NAMES.get(link["operation"], link["operation"]),
                "layer": layers.get(int(operator_detail.get("layerIndex", -1)), ""),
                "no_effect": no_effect,
            })
        self.links.sort(key=lambda l: (not l["no_effect"], l["target"], l["operator"]))

        self.stat_labels["links"].configure(text=str(len(self.links)))
        self.stat_labels["targets"].configure(text=str(len({l["targetId"]["guid"] for l in self.links})))
        self.stat_labels["operators"].configure(text=str(len({l["operatorId"]["guid"] for l in self.links})))
        no_effect = sum(l["no_effect"] for l in self.links)
        self.stat_labels["no_effect"].configure(text=str(no_effect), foreground="#a15c00" if no_effect else "#1b7f3b")
        self._show()
        if not self.links:
            self.window.set_status("There are no solid element operations here.", "ok")
        elif no_effect:
            self.window.set_status(f"{no_effect} link(s) connect elements that do not touch.", "warning")
        else:
            self.window.set_status("Every link connects elements that touch.", "ok")

    def _show(self) -> None:
        self.listed = [l for l in self.links if l["no_effect"] or not self.only_no_effect.get()]
        self.tree.delete(*self.tree.get_children())
        for index, link in enumerate(self.listed):
            self.tree.insert("", "end", iid=str(index), tags=("no_effect",) if link["no_effect"] else (), values=(
                link["target"], link["operation"], link["operator"], link["layer"],
                NO_EFFECT if link["no_effect"] else "Touch"))
        self.remove_button.state(["!disabled"] if self.listed else ["disabled"])
        self._highlight()

    # ---- actions ----

    def _chosen(self) -> list[dict]:
        rows = self.tree.selection()
        return [self.listed[int(r)] for r in rows] if rows else self.listed

    def _highlight(self) -> None:
        colors: dict[str, list[int]] = {}
        for link in self._chosen():
            colors.setdefault(link["targetId"]["guid"], TARGET_COLOR)
            colors[link["operatorId"]["guid"]] = OPERATOR_COLOR
        if not colors:
            self.clear_highlight()
            return
        run_tapir("HighlightElements", {
            "elements": [{"elementId": {"guid": guid}} for guid in colors],
            "highlightedColors": list(colors.values()),
            "wireframe3D": False,
            "nonHighlightedColor": DIMMED_COLOR,
        })

    @staticmethod
    def _elements_of(links: list[dict]) -> list[dict]:
        guids = dict.fromkeys(g for l in links for g in (l["targetId"]["guid"], l["operatorId"]["guid"]))
        return [{"elementId": {"guid": guid}} for guid in guids]

    def select_listed(self) -> None:
        elements = self._elements_of(self._chosen())
        select_elements(elements)
        self.window.set_status(f"Selected {len(elements)} element(s) in Archicad.", "ok")

    def select_clicked(self) -> None:
        row = self.tree.focus()
        if row:
            select_elements(self._elements_of([self.listed[int(row)]]))
            self.window.set_status("Selected the target and the operator in Archicad.", "ok")

    def remove_links(self) -> None:
        rows = self.tree.selection()
        if not rows:
            self.window.set_status("Select the links to remove in the list first.", "warning")
            return
        links = [self.listed[int(r)] for r in rows]
        if not self.window.ask_ok_cancel(SCRIPT_NAME, f"Remove {len(links)} solid element operation link(s)?\n\n"
                                                      "The elements stay; only their links are removed. "
                                                      "You can undo it in Archicad."):
            return
        with self.window.busy(f"Removing {len(links)} link(s)..."):
            results = run_tapir("RemoveSolidElementLinks", {"solidLinks": [
                {"targetId": l["targetId"], "operatorId": l["operatorId"]} for l in links]}).get("executionResults", [])
        errors = item_errors(results)
        self.refresh()
        if errors:
            self.window.set_status(f"Removed {len(links) - len(errors)} of {len(links)} link(s). First error: {errors[0]}", "warning")
        else:
            self.window.set_status(f"Removed {len(links)} link(s).", "ok")

    def clear_highlight(self) -> None:
        run_tapir("HighlightElements", {"elements": [], "highlightedColors": []})

    def run(self) -> None:
        self.window.run(on_start=self.refresh, on_close=self.clear_highlight)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    SolidOperationExplorer().run()
