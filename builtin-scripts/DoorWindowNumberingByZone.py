# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

##############################################
# DOOR AND WINDOW NUMBERING BY ZONE
# For Archicad with the Tapir Add-On
#
# Gives every door and window an Element ID built from the zone it opens
# into, such as 1.04-D1 or 1.04-W2, numbered within each zone.
###############################################

import math
import tkinter as tk
from tkinter import ttk

from utilities.archicad import ArchicadError, batched, get_details_of_elements, get_elements, item_errors, run_tapir
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Door and Window Numbering by Zone"
OPENING_TYPES = ["Door", "Window"]
SCOPES = {
    "Selection": "selection",
    "Current story": "story",
    "Whole project": "project",
}


################################
# NUMBERING
################################

def clockwise_from_north(origin: tuple[float, float], point: tuple[float, float]) -> float:
    """ The angle of point seen from origin, 0 to the north (+y), growing clockwise. """
    return math.atan2(point[0] - origin[0], point[1] - origin[1]) % (2 * math.pi)


def fill_pattern(pattern: str, zone_number: str, zone_name: str, number: str) -> str:
    """ Plain replacement, so braces the user types elsewhere in the pattern never raise. """
    return pattern.replace("{zone}", zone_number).replace("{name}", zone_name).replace("{n}", number)


################################
# SCRIPT
################################

class DoorWindowNumbering:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Every door and window gets an Element ID built from the zone it opens into: the zone on its "
            "\"to\" side, or the \"from\" side when there is none. In the patterns {zone} is the zone number, "
            "{name} the zone name (also used for {zone} when the zone has no number) and {n} the running "
            "number within the zone. Nothing is changed until you "
            "press Rename and confirm.",
            min_width=640, min_height=560)
        self.openings: list[dict] = []  # the doors and windows with a zone, in numbering order
        self.without_zone: list[dict] = []
        self._build_body()
        self.window.add_button("Read elements", self.read_elements)
        self.rename_button = self.window.add_button("Rename", self.rename, primary=True)
        self.rename_button.state(["disabled"])

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(2, weight=3)
        body.rowconfigure(4, weight=1)

        options = ttk.LabelFrame(body, text="Numbering", padding=(10, 6))
        options.grid(row=0, column=0, sticky="ew")
        self.scope = tk.StringVar(value="Current story")
        self.door_pattern = tk.StringVar(value="{zone}-D{n}")
        self.window_pattern = tk.StringVar(value="{zone}-W{n}")
        self.start = tk.StringVar(value="1")
        self.digits = tk.StringVar(value="1")
        fields = [
            ("Elements:", ttk.Combobox(options, textvariable=self.scope, values=list(SCOPES), state="readonly", width=14)),
            ("Start at:", ttk.Spinbox(options, textvariable=self.start, from_=0, to=9999, width=6)),
            ("Doors:", ttk.Entry(options, textvariable=self.door_pattern, width=18)),
            ("Digits:", ttk.Spinbox(options, textvariable=self.digits, from_=1, to=6, width=6)),
            ("Windows:", ttk.Entry(options, textvariable=self.window_pattern, width=18)),
        ]
        for i, (label, widget) in enumerate(fields):
            ttk.Label(options, text=label).grid(row=i // 2, column=(i % 2) * 2, sticky="w", pady=2, padx=(0 if i % 2 == 0 else 16, 0))
            widget.grid(row=i // 2, column=(i % 2) * 2 + 1, sticky="w", padx=(6, 0), pady=2)
        for var in (self.door_pattern, self.window_pattern, self.start, self.digits):
            var.trace_add("write", lambda *_: self._update_preview())
        # a new scope needs a new read, not only a new preview
        self.scope.trace_add("write", lambda *_: self._clear())

        ttk.Label(body, text="Doors and windows within each zone are numbered clockwise around the zone stamp, "
                             "starting from the north.", style="Muted.TLabel").grid(row=1, column=0, sticky="w", pady=(6, 4))

        self.tree = make_tree(body, [
            ("zone", "Zone", 160, "w"),
            ("type", "Type", 70, "w"),
            ("current", "Current ID", 120, "w"),
            ("new", "New ID", 120, "w"),
            ("status", "Status", 140, "w"),
        ], height=10)
        self.tree.container.grid(row=2, column=0, sticky="nsew")

        self.without_zone_label = ttk.Label(body, text="Without a zone (not renamed):")
        self.without_zone_label.grid(row=3, column=0, sticky="w", pady=(8, 2))
        self.without_zone_tree = make_tree(body, [
            ("current", "Current ID", 160, "w"),
            ("type", "Type", 70, "w"),
            ("reason", "Reason", 260, "w"),
        ], height=4)
        self.without_zone_tree.container.grid(row=4, column=0, sticky="nsew")

    # ---- options ----

    def _start_number(self) -> int | None:
        try:
            value = int(self.start.get())
        except ValueError:
            return None
        return value if value >= 0 else None

    def _digits(self) -> int:
        try:
            return max(1, min(6, int(self.digits.get())))
        except ValueError:
            return 1

    def _assign_new_ids(self) -> bool:
        """ Fills in each opening's newId; False when the options are not valid. """
        start = self._start_number()
        if start is None:
            return False
        digits = self._digits()
        patterns = {"Door": self.door_pattern.get(), "Window": self.window_pattern.get()}
        counters: dict[tuple[str, str], int] = {}
        for item in self.openings:
            # numbered per zone number, not per zone element, so two zones with the same number do not get the same IDs
            key = (item["zoneKey"], item["type"])
            offset = counters.get(key, 0)
            counters[key] = offset + 1
            item["newId"] = fill_pattern(patterns[item["type"]], item["zoneNumber"], item["zoneName"], f"{start + offset:0{digits}d}")
        return True

    def _duplicate_new_ids(self) -> set[str]:
        seen, duplicates = set(), set()
        for item in self.openings:
            (duplicates if item["newId"] in seen else seen).add(item["newId"])
        return duplicates

    def _clear(self) -> None:
        self.openings = []
        self.without_zone = []
        self._update_preview()
        self.window.set_status("Press Read elements to read the doors and windows.")

    def _update_preview(self) -> None:
        self.tree.delete(*self.tree.get_children())
        self.rename_button.state(["disabled"])
        if not self._assign_new_ids():
            self.window.set_status("The start number must be a whole number, 0 or more.", "warning")
            return
        duplicates = self._duplicate_new_ids()
        to_change = 0
        for index, item in enumerate(self.openings):
            if item["newId"] == item["currentId"]:
                status = "renamed" if item.get("renamedTo") == item["newId"] else "unchanged"
            else:
                to_change += 1
                if item.get("failedTo") == item["newId"]:
                    status = "not changed: " + item["failure"]
                else:
                    status = "duplicate new ID" if item["newId"] in duplicates else ""
            self.tree.insert("", "end", iid=str(index), values=(
                item["zoneLabel"], item["type"], item["currentId"] or "(empty)", item["newId"], status))

        self.without_zone_tree.delete(*self.without_zone_tree.get_children())
        for index, item in enumerate(self.without_zone):
            self.without_zone_tree.insert("", "end", iid=str(index), values=(
                item["currentId"] or "(empty)", item["type"], item["reason"]))
        self.without_zone_label.configure(text=f"Without a zone (not renamed): {len(self.without_zone)}")

        if not self.openings:
            if self.without_zone:
                self.window.set_status(f"None of the {len(self.without_zone)} doors and windows has a zone to number them after.", "warning")
            return
        if to_change:
            self.rename_button.state(["!disabled"])
        message = f"{to_change} of {len(self.openings)} doors and windows get a new ID."
        if self.without_zone:
            message += f" {len(self.without_zone)} have no zone."
        if duplicates:
            self.window.set_status(message + " Some new IDs repeat: put {zone} and {n} in both patterns.", "warning")
        else:
            self.window.set_status(message)

    # ---- actions ----

    def read_elements(self) -> None:
        self.openings = []
        self.without_zone = []
        with self.window.busy("Reading the doors and windows..."):
            elements = get_elements(SCOPES[self.scope.get()], OPENING_TYPES)
            details = get_details_of_elements(elements, ["type", "id", "floorIndex"])
            relations = []
            for chunk in batched(elements):
                relations += run_tapir("GetRelationsOfElements", {"elements": chunk}).get("relations", [])

        if not elements:
            self._update_preview()
            self.window.set_status("There are no doors or windows in " + self.scope.get().lower() + ".", "warning")
            return

        candidates = []
        relation_errors = []
        for element, detail, relation in zip(elements, details, relations):
            item = {
                "elementId": element["elementId"],
                "type": detail.get("type", ""),
                "currentId": detail.get("id", ""),
                "floorIndex": detail.get("floorIndex", 0),
            }
            if "error" in detail:
                item["reason"] = "cannot be read"
                self.without_zone.append(item)
                continue
            if "error" in relation:
                relation_errors.append(relation["error"])
                item["reason"] = "relations cannot be read"
                self.without_zone.append(item)
                continue
            sides = relation.get("openingRelations", {})
            zone = sides.get("toRoom") or sides.get("fromRoom")
            if zone is None:
                item["reason"] = "no zone on either side"
                self.without_zone.append(item)
                continue
            item["zoneGuid"] = zone["guid"]
            candidates.append(item)

        if relation_errors and len(relation_errors) == len(elements):
            self._update_preview()
            first = item_errors([{"error": relation_errors[0]}])[0]
            self.window.set_status(f"The zones of the doors and windows cannot be read: {first}", "error")
            return

        zone_guids = list({item["zoneGuid"] for item in candidates})
        with self.window.busy(f"Reading {len(zone_guids)} zones and placing {len(candidates)} openings..."):
            zone_details = get_details_of_elements([{"elementId": {"guid": guid}} for guid in zone_guids], ["details"])
            boxes = []
            for chunk in batched([{"elementId": item["elementId"]} for item in candidates]):
                boxes += run_tapir("Get3DBoundingBoxes", {"elements": chunk}).get("boundingBoxes3D", [])

        zones = {}
        for guid, detail in zip(zone_guids, zone_details):
            if "error" in detail:
                continue
            zone = detail.get("details", {})
            stamp = zone.get("stampPosition") or {"x": 0.0, "y": 0.0}
            number = (zone.get("numberStr") or "").strip()
            name = (zone.get("name") or "").strip()
            zones[guid] = {"number": number, "name": name, "stamp": (stamp["x"], stamp["y"])}

        boxes += [{}] * (len(candidates) - len(boxes))  # an opening without a box is numbered first in its zone
        for item, box in zip(candidates, boxes):
            zone = zones.get(item["zoneGuid"])
            if zone is None:
                item["reason"] = "its zone cannot be read"
                self.without_zone.append(item)
                continue
            if not zone["number"] and not zone["name"]:
                item["reason"] = "its zone has no number and no name"
                self.without_zone.append(item)
                continue
            # a zone without a number is numbered by its name
            item["zoneNumber"] = zone["number"] or zone["name"]
            item["zoneName"] = zone["name"]
            item["zoneKey"] = item["zoneNumber"]
            item["zoneLabel"] = " ".join(part for part in (zone["number"], zone["name"]) if part)
            if "boundingBox3D" in box:
                bb = box["boundingBox3D"]
                center = ((bb["xMin"] + bb["xMax"]) / 2, (bb["yMin"] + bb["yMax"]) / 2)
                item["angle"] = clockwise_from_north(zone["stamp"], center)
            else:
                item["angle"] = 0.0
            self.openings.append(item)

        self.openings.sort(key=lambda item: (item["zoneKey"], item["type"], item["floorIndex"], item["angle"], item["currentId"]))
        self.without_zone.sort(key=lambda item: (item["type"], item["currentId"]))
        self._update_preview()

    def rename(self) -> None:
        if not self._assign_new_ids():
            return
        changes = [item for item in self.openings if item["newId"] != item["currentId"]]
        if not changes:
            return
        duplicates = self._duplicate_new_ids()
        question = f"Change the Element ID of {len(changes)} doors and windows?"
        if duplicates:
            question += f"\n\n{len(duplicates)} new IDs are used more than once."
        question += "\n\nThis can be undone in Archicad with Undo."
        if not self.window.ask_ok_cancel(SCRIPT_NAME, question):
            return

        results = []
        with self.window.busy(f"Renaming {len(changes)} doors and windows..."):
            for chunk in batched(changes):
                results += run_tapir("SetElementInfoStrings", {"elements": [
                    {"elementId": item["elementId"], "infoString": item["newId"]} for item in chunk
                ]}).get("executionResults", [])

        # the results follow the order of changes
        failed = len(changes) - len(results)  # a missing result counts as not changed
        for item, result in zip(changes, results):
            if "error" in result:
                failed += 1
                item["failedTo"] = item["newId"]
                item["failure"] = item_errors([result])[0]
            else:
                item["currentId"] = item["newId"]
                item["renamedTo"] = item["newId"]
                item.pop("failedTo", None)
        self._update_preview()
        if failed:
            first_failed = next(i for i, item in enumerate(self.openings) if item["newId"] != item["currentId"])
            self.tree.see(str(first_failed))
            self.window.set_status(f"Renamed {len(changes) - failed} of {len(changes)}. {failed} could not be changed: "
                                   "their status in the list says why.", "warning")
        else:
            self.window.set_status(f"Renamed {len(changes)} doors and windows.", "ok")

    def run(self) -> None:
        self.window.run(on_start=self.read_elements)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    DoorWindowNumbering().run()
