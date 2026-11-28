# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# category = "element-data-properties"
# changes = "model"
# ///

"""
Door and Window Numbering by Zone

Gives every door and window an ID built from the zone it opens into, such
as 1.04-D1 or 1.04-W2, numbered within each zone, and writes it to the
Element ID or to a property you choose.

For Archicad with the Tapir Add-On.
"""

import math
import sys
import tkinter as tk
from tkinter import ttk

try:
    from utilities.archicad import ArchicadError, batched, get_details_of_elements, get_elements, item_errors, run_tapir
    from utilities.ui import ScriptWindow, make_tree, require_archicad
except ModuleNotFoundError as error:
    # The palette of a Tapir newer than 2.0.0 provides the helpers; an older one needs copies next to the script.
    if (error.name or "").split(".")[0] not in ("utilities", "aclib"):
        raise
    sys.exit("Error: the Tapir helpers utilities and aclib were not found. Update the Tapir Add-On, or copy the "
             "utilities and aclib folders of builtin-scripts "
             "(https://github.com/ENZYME-APD/tapir-archicad-automation/tree/main/builtin-scripts) next to this script.")

SCRIPT_NAME = "Door and Window Numbering by Zone"
OPENING_TYPES = ["Door", "Window"]
SCOPES = {
    "Selection": "selection",
    "Current story": "story",
    "Whole project": "project",
}
# The sides of an opening, as Archicad's "To Zone" and "From Zone" properties name them, and the
# GetRelationsOfElements field of each. The API's fields are the other way round: its toRoom is the
# zone Archicad shows as the From Zone (compared with Archicad's properties in #3071).
ZONE_SIDES = {
    "To zone": "fromRoom",
    "From zone": "toRoom",
}
ELEMENT_ID = "Element ID"


################################
# NUMBERING
################################

def clockwise_from_north(origin: tuple[float, float], point: tuple[float, float]) -> float:
    """ The angle of point seen from origin, 0 to the north (+y), growing clockwise. """
    return math.atan2(point[0] - origin[0], point[1] - origin[1]) % (2 * math.pi)


def fill_pattern(pattern: str, values: dict[str, str]) -> str:
    """ Plain replacement, so braces the user types elsewhere in the pattern never raise. """
    for key, value in values.items():
        pattern = pattern.replace("{" + key + "}", value)
    return pattern


def text_of(value) -> str:
    if value is None:
        return ""
    return value if isinstance(value, str) else str(value)


################################
# SCRIPT
################################

class DoorWindowNumbering:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Every door and window gets an ID built from the zone it opens into: the zone on the side you put "
            "first, or the other side when there is none. In the patterns {zone} is the zone number, {name} the "
            "zone name (also used for {zone} when the zone has no number), {n} the running number within the "
            "zone and {id} the current Element ID. The new ID goes to the Element ID or to the property you "
            "choose. Nothing is changed until you press Rename and confirm.",
            min_width=700, min_height=620)
        self.openings: list[dict] = []  # the doors and windows with a zone, in numbering order
        self.without_zone: list[dict] = []
        self.properties: dict[str, dict] = {}  # the writable text properties by their label, to their propertyId
        self.read_target = ELEMENT_ID  # the target the current values were read from
        self._build_body()
        self.window.add_button("Read elements", self.read_elements)
        self.rename_button = self.window.add_button("Rename", self.rename, primary=True)
        self.rename_button.state(["disabled"])

    def _build_body(self) -> None:
        body = self.window.body
        body.columnconfigure(0, weight=1)
        body.rowconfigure(2, weight=3)
        body.rowconfigure(4, weight=1)

        options = ttk.LabelFrame(body, text="Numbering", padding=(10, 6))
        options.grid(row=0, column=0, sticky="ew")
        self.scope = tk.StringVar(value="Current story")
        self.side = tk.StringVar(value="To zone")
        self.door_pattern = tk.StringVar(value="{zone}-D{n}")
        self.window_pattern = tk.StringVar(value="{zone}-W{n}")
        self.start = tk.StringVar(value="1")
        self.digits = tk.StringVar(value="1")
        self.unique = tk.BooleanVar(value=False)
        self.repeat_pattern = tk.StringVar(value=".{k}")
        self.repeat_digits = tk.StringVar(value="2")
        self.target = tk.StringVar(value=ELEMENT_ID)
        self.target_box = ttk.Combobox(options, textvariable=self.target, values=[ELEMENT_ID], state="readonly", width=34)
        fields = [
            ("Elements:", ttk.Combobox(options, textvariable=self.scope, values=list(SCOPES), state="readonly", width=14)),
            ("Zone side first:", ttk.Combobox(options, textvariable=self.side, values=list(ZONE_SIDES), state="readonly", width=12)),
            ("Doors:", ttk.Entry(options, textvariable=self.door_pattern, width=18)),
            ("Start at:", ttk.Spinbox(options, textvariable=self.start, from_=0, to=9999, width=6)),
            ("Windows:", ttk.Entry(options, textvariable=self.window_pattern, width=18)),
            ("Digits:", ttk.Spinbox(options, textvariable=self.digits, from_=1, to=6, width=6)),
        ]
        for i, (label, widget) in enumerate(fields):
            ttk.Label(options, text=label).grid(row=i // 2, column=(i % 2) * 2, sticky="w", pady=2, padx=(0 if i % 2 == 0 else 16, 0))
            widget.grid(row=i // 2, column=(i % 2) * 2 + 1, sticky="w", padx=(6, 0), pady=2)

        repeats = ttk.Frame(options)
        repeats.grid(row=3, column=0, columnspan=4, sticky="w", pady=2)
        ttk.Checkbutton(repeats, text="Number repeated IDs, adding:", variable=self.unique).pack(side="left")
        ttk.Entry(repeats, textvariable=self.repeat_pattern, width=8).pack(side="left", padx=(6, 0))
        ttk.Label(repeats, text="Digits of {k}:").pack(side="left", padx=(16, 0))
        ttk.Spinbox(repeats, textvariable=self.repeat_digits, from_=1, to=6, width=6).pack(side="left", padx=(6, 0))

        ttk.Label(options, text="Write to:").grid(row=4, column=0, sticky="w", pady=2)
        self.target_box.grid(row=4, column=1, columnspan=3, sticky="w", padx=(6, 0), pady=2)

        for var in (self.door_pattern, self.window_pattern, self.start, self.digits,
                    self.unique, self.repeat_pattern, self.repeat_digits):
            var.trace_add("write", lambda *_: self._update_preview())
        # a new scope, zone side or target needs a new read, not only a new preview
        for var in (self.scope, self.side, self.target):
            var.trace_add("write", lambda *_: self._clear())

        ttk.Label(body, text="Doors and windows within each zone are numbered clockwise around the zone stamp, "
                             "starting from the north. Zones with the same number continue the count, "
                             "from north to south. With Number repeated IDs, an ID given to more than one "
                             "element gets {k} = 1, 2, ... added.",
                  style="Muted.TLabel", wraplength=660).grid(row=1, column=0, sticky="w", pady=(6, 4))

        self.tree = make_tree(body, [
            ("zone", "Zone", 160, "w"),
            ("type", "Type", 70, "w"),
            ("elementId", "Element ID", 110, "w"),
            ("current", "Current value", 110, "w"),
            ("new", "New value", 120, "w"),
            ("status", "Status", 140, "w"),
        ], height=10)
        self.tree.container.grid(row=2, column=0, sticky="nsew")

        self.without_zone_label = ttk.Label(body, text="Without a zone (not renamed):")
        self.without_zone_label.grid(row=3, column=0, sticky="w", pady=(8, 2))
        self.without_zone_tree = make_tree(body, [
            ("current", "Element ID", 160, "w"),
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

    @staticmethod
    def _digits_of(var: tk.StringVar) -> int:
        try:
            return max(1, min(6, int(var.get())))
        except ValueError:
            return 1

    def _target_label(self) -> str:
        return "the Element ID" if self.read_target == ELEMENT_ID else f"the property {self.read_target}"

    def _assign_new_ids(self) -> bool:
        """ Fills in each opening's newId; False when the options are not valid. """
        start = self._start_number()
        if start is None:
            return False
        digits = self._digits_of(self.digits)
        patterns = {"Door": self.door_pattern.get(), "Window": self.window_pattern.get()}
        counters: dict[tuple[str, str], int] = {}
        for item in self.openings:
            # numbered per zone number, not per zone element, so two zones with the same number do not get the same IDs
            key = (item["zoneKey"], item["type"])
            offset = counters.get(key, 0)
            counters[key] = offset + 1
            item["newId"] = fill_pattern(patterns[item["type"]], {
                "zone": item["zoneNumber"], "name": item["zoneName"], "n": f"{start + offset:0{digits}d}",
                "id": item["readId"]})

        if self.unique.get():
            # an ID given to more than one element gets the repeat pattern added, counted in numbering order
            uses: dict[str, int] = {}
            for item in self.openings:
                uses[item["newId"]] = uses.get(item["newId"], 0) + 1
            repeat_digits = self._digits_of(self.repeat_digits)
            counts: dict[str, int] = {}
            for item in self.openings:
                base = item["newId"]
                if uses[base] > 1:
                    counts[base] = counts.get(base, 0) + 1
                    item["newId"] = base + fill_pattern(self.repeat_pattern.get(), {"k": f"{counts[base]:0{repeat_digits}d}"})
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
            if item["newId"] == item["current"]:
                status = "renamed" if item.get("renamedTo") == item["newId"] else "unchanged"
            else:
                to_change += 1
                if item.get("failedTo") == item["newId"]:
                    status = "not changed: " + item["failure"]
                else:
                    status = "duplicate new ID" if item["newId"] in duplicates else ""
            self.tree.insert("", "end", iid=str(index), values=(
                item["zoneLabel"], item["type"], item["currentId"] or "(empty)", item["current"] or "(empty)",
                item["newId"], status))

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
        message = f"{to_change} of {len(self.openings)} doors and windows get a new value in {self._target_label()}."
        if self.without_zone:
            message += f" {len(self.without_zone)} have no zone."
        if duplicates:
            self.window.set_status(message + " Some new IDs repeat: put {zone} and {n} in both patterns, "
                                             "or tick Number repeated IDs.", "warning")
        else:
            self.window.set_status(message)

    # ---- reading ----

    def load_properties(self) -> None:
        """ Fills Write to with the Element ID and the project's editable single text properties. """
        with self.window.busy("Reading the properties..."):
            properties = run_tapir("GetAllProperties").get("properties", [])
        self.properties = {}
        writable = [p for p in properties if "propertyId" in p and p.get("propertyIsEditable")
                    and p.get("propertyValueType") == "String" and p.get("propertyCollectionType", "Single") == "Single"]
        for p in sorted(writable, key=lambda p: (p.get("propertyType") != "Custom", p.get("propertyGroupName", "").lower(),
                                                  p.get("propertyName", "").lower())):
            self.properties[f"{p.get('propertyGroupName', '')} / {p.get('propertyName', '')}"] = p["propertyId"]
        self.target_box.configure(values=[ELEMENT_ID] + list(self.properties))

    def _read_property(self, elements: list[dict], property_id: dict) -> list[str | None]:
        """ The value of the property for each element; None where the element does not have it. """
        values: list[str | None] = []
        for chunk in batched(elements):
            answer = run_tapir("GetPropertyValuesOfElements", {
                "elements": chunk, "properties": [{"propertyId": property_id}]}).get("propertyValuesForElements", [])
            answer += [{}] * (len(chunk) - len(answer))
            for entry in answer:
                found = (entry.get("propertyValues") or [{}])[0]
                if "propertyValue" in found:
                    values.append(text_of(found["propertyValue"].get("value")))
                elif "error" in found or "error" in entry:
                    values.append(None)
                else:
                    values.append("")
        return values

    def read_elements(self) -> None:
        self.openings = []
        self.without_zone = []
        target = self.target.get()
        if target != ELEMENT_ID and target not in self.properties:
            self.target.set(ELEMENT_ID)  # clears the preview; the read starts again with the Element ID
            target = ELEMENT_ID
        self.read_target = target
        first_side = ZONE_SIDES[self.side.get()]
        other_side = next(field for field in ZONE_SIDES.values() if field != first_side)
        side_of = {field: label.split()[0].lower() for label, field in ZONE_SIDES.items()}  # "to" or "from"

        with self.window.busy("Reading the doors and windows..."):
            elements = get_elements(SCOPES[self.scope.get()], OPENING_TYPES)
            details = get_details_of_elements(elements, ["type", "id", "floorIndex"])
            relations = []
            for chunk in batched(elements):
                relations += run_tapir("GetRelationsOfElements", {"elements": chunk}).get("relations", [])
            if target == ELEMENT_ID:
                current_values = [None] * len(elements)
            else:
                current_values = self._read_property([{"elementId": e["elementId"]} for e in elements], self.properties[target])

        if not elements:
            self._update_preview()
            self.window.set_status("There are no doors or windows in " + self.scope.get().lower() + ".", "warning")
            return

        candidates = []
        relation_errors = []
        relations += [{}] * (len(elements) - len(relations))
        for element, detail, relation, value in zip(elements, details, relations, current_values):
            item = {
                "elementId": element["elementId"],
                "type": detail.get("type", ""),
                "currentId": detail.get("id", ""),
                # {id} is the Element ID as read, so a rename does not feed the pattern its own result
                "readId": detail.get("id", ""),
                "floorIndex": detail.get("floorIndex", 0),
            }
            if "error" in detail:
                item["reason"] = "cannot be read"
                self.without_zone.append(item)
                continue
            if target == ELEMENT_ID:
                item["current"] = item["currentId"]
            elif value is None:
                item["reason"] = f"has no property {target}"
                self.without_zone.append(item)
                continue
            else:
                item["current"] = value
            if "error" in relation:
                relation_errors.append(relation["error"])
                item["reason"] = "relations cannot be read"
                self.without_zone.append(item)
                continue
            sides = relation.get("openingRelations", {})
            field = first_side if sides.get(first_side) else other_side
            zone = sides.get(field)
            if zone is None:
                item["reason"] = "no zone on either side"
                self.without_zone.append(item)
                continue
            item["zoneGuid"] = zone["guid"]
            item["side"] = side_of[field]
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
            item["zoneStamp"] = zone["stamp"]
            item["zoneLabel"] = " ".join(part for part in (zone["number"], zone["name"]) if part) + f" ({item['side']})"
            if "boundingBox3D" in box:
                bb = box["boundingBox3D"]
                center = ((bb["xMin"] + bb["xMax"]) / 2, (bb["yMin"] + bb["yMax"]) / 2)
                item["angle"] = clockwise_from_north(zone["stamp"], center)
            else:
                item["angle"] = 0.0
            self.openings.append(item)

        # Zones that share a number on a story are numbered one after the other, from north to south and
        # then from west to east by their zone stamps, so the openings of one zone are not mixed with another's.
        self.openings.sort(key=lambda item: (item["zoneKey"], item["type"], item["floorIndex"],
                                             -item["zoneStamp"][1], item["zoneStamp"][0], item["zoneGuid"],
                                             item["angle"], item["currentId"]))
        self.without_zone.sort(key=lambda item: (item["type"], item["currentId"]))
        self._update_preview()

    # ---- writing ----

    def rename(self) -> None:
        if not self._assign_new_ids():
            return
        changes = [item for item in self.openings if item["newId"] != item["current"]]
        if not changes:
            return
        duplicates = self._duplicate_new_ids()
        question = f"Change {self._target_label()} of {len(changes)} doors and windows?"
        if duplicates:
            question += f"\n\n{len(duplicates)} new IDs are used more than once."
        question += "\n\nThis can be undone in Archicad with Undo."
        if not self.window.ask_ok_cancel(SCRIPT_NAME, question):
            return

        results = []
        with self.window.busy(f"Renaming {len(changes)} doors and windows..."):
            for chunk in batched(changes):
                if self.read_target == ELEMENT_ID:
                    results += run_tapir("SetElementInfoStrings", {"elements": [
                        {"elementId": item["elementId"], "infoString": item["newId"]} for item in chunk
                    ]}).get("executionResults", [])
                else:
                    property_id = self.properties[self.read_target]
                    results += run_tapir("SetPropertyValuesOfElements", {"elementPropertyValues": [
                        {"elementId": item["elementId"], "propertyId": property_id, "propertyValue": {"value": item["newId"]}}
                        for item in chunk
                    ]}).get("executionResults", [])

        # the results follow the order of changes
        failed = len(changes) - len(results)  # a missing result counts as not changed
        for item, result in zip(changes, results):
            if "error" in result or result.get("success") is False:
                failed += 1
                item["failedTo"] = item["newId"]
                item["failure"] = (item_errors([result]) or ["not written"])[0]
            else:
                item["current"] = item["newId"]
                if self.read_target == ELEMENT_ID:
                    item["currentId"] = item["newId"]
                item["renamedTo"] = item["newId"]
                item.pop("failedTo", None)
        self._update_preview()
        if failed:
            first_failed = next(i for i, item in enumerate(self.openings) if item["newId"] != item["current"])
            self.tree.see(str(first_failed))
            self.window.set_status(f"Renamed {len(changes) - failed} of {len(changes)}. {failed} could not be changed: "
                                   "their status in the list says why.", "warning")
        else:
            self.window.set_status(f"Wrote the new value of {len(changes)} doors and windows to {self._target_label()}.", "ok")

    def load_and_read(self) -> None:
        try:
            self.load_properties()
        except ArchicadError as e:
            # numbering into the Element ID still works without the property list
            print(f"Error: {e}")
        self.read_elements()

    def run(self) -> None:
        self.window.run(on_start=self.load_and_read)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    DoorWindowNumbering().run()
