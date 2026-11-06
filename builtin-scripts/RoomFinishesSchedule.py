# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

'''
Room Finishes Schedule

Lists the floor, wall and ceiling finish areas of every zone (or of the
selected zones), one row per zone and finish, and saves the table as CSV or
Excel. It reads only; nothing in the project is changed.

- Floor and ceiling: the zone's net area as Archicad calculates it.
- Walls: the length of the walls around the zone (Archicad's zone quantity)
  times the zone's height, less the doors and windows that open into the zone
  (width x height of each). The wall area is split by the surface the boundary
  walls show: a wall whose two sides have the same surface override, or whose
  model shows a single surface, gives that surface. When the sides differ, the
  surfaces are listed with "(side not reported)", because Archicad does not
  report which side faces the zone.

Needs Archicad 26 or later (zone and opening relations) and Tapir 1.7.3 or
later (GetQuantitiesOfElements, GetSurfaceQuantitiesOfElements).
'''

import tkinter as tk
from collections import defaultdict
from tkinter import filedialog, ttk

from utilities.archicad import (ArchicadError, batched, get_details_of_elements, get_elements, get_story_names,
                                item_errors, run_tapir)
from utilities.export import write_table
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Room Finishes Schedule"
QUANTITY_BATCH = 200
NOT_REPORTED = "(not reported)"
HEADERS = ["Story", "Zone number", "Zone name", "Finish", "Surface",
           "Length (m)", "Height (m)", "Openings (m²)", "Area (m²)"]


def guid_of(value) -> str | None:
    """ The guid in {"guid": ...}, {"attributeId": {"guid": ...}} or {"elementId": {"guid": ...}}. """
    while isinstance(value, dict):
        if "guid" in value:
            return value["guid"]
        value = value.get("attributeId") or value.get("elementId")
    return None


def overridden_surface(material: dict | None) -> str | None:
    if isinstance(material, dict) and material.get("overridden"):
        return guid_of(material.get("attributeId"))
    return None


def per_element(command: str, key: str, elements: list[dict], **parameters) -> list[dict]:
    """ A per-element command run in batches; its result items are aligned with elements. """
    results = []
    for chunk in batched(elements, QUANTITY_BATCH):
        results += run_tapir(command, {"elements": chunk, **parameters}).get(key, [])
    return results


class RoomFinishesSchedule:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Floor, wall and ceiling finish areas per zone. Wall areas are the walls around the zone "
            "times its height, less its doors and windows, grouped by the surface of the boundary walls.",
            min_width=820, min_height=480)
        self.rows: list[list] = []
        self._build_body()
        self.window.add_button("Save as...", self.save)
        self.window.add_button("Calculate", self.calculate, primary=True)

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(2, weight=1)

        options = ttk.LabelFrame(body, text="Zones", padding=(10, 6))
        options.grid(row=0, column=0, sticky="ew")
        self.scope = tk.StringVar(value="project")
        for column, (value, text) in enumerate((("project", "Whole project"), ("story", "Current story"),
                                                ("selection", "Selection"))):
            ttk.Radiobutton(options, text=text, value=value, variable=self.scope).grid(
                row=0, column=column, sticky="w", padx=(0 if column == 0 else 10, 0))

        self.summary_label = ttk.Label(body, text="Press Calculate to read the zones.", padding=(0, 8, 0, 6))
        self.summary_label.grid(row=1, column=0, sticky="ew")

        self.tree = make_tree(body, [
            ("story", "Story", 110, "w"),
            ("number", "No.", 60, "w"),
            ("zone", "Zone", 140, "w"),
            ("finish", "Finish", 70, "w"),
            ("surface", "Surface", 200, "w"),
            ("length", "Length (m)", 80, "e"),
            ("height", "Height (m)", 80, "e"),
            ("openings", "Openings (m²)", 95, "e"),
            ("area", "Area (m²)", 80, "e"),
        ], height=14)
        self.tree.container.grid(row=2, column=0, sticky="nsew")

    def calculate(self) -> None:
        problems: list[str] = []
        with self.window.busy("Reading the zones..."):
            zones = get_elements(self.scope.get(), ["Zone"])
            if not zones:
                self._show([], 0)
                self.window.set_status("No zones found.", "warning")
                return
            story_names = get_story_names()
            zone_details = get_details_of_elements(zones, ["floorIndex", "details"])

            self.window.set_status("Calculating the zone quantities...")
            zone_quantities = per_element("GetQuantitiesOfElements", "quantitiesOfElements", zones)
            problems += item_errors(zone_quantities)

            self.window.set_status("Reading the walls around the zones...")
            try:
                zone_relations = per_element("GetRelationsOfElements", "relations", zones)
            except ArchicadError as e:
                print(f"Zone relations are not available, walls are not grouped by surface: {e}")
                zone_relations = [{} for _ in zones]
            problems += item_errors(zone_relations)

            walls_of_zone: list[dict[str, float]] = []
            for relation in zone_relations:
                lengths: dict[str, float] = defaultdict(float)
                for part in (relation.get("zoneRelations") or {}).get("wallParts", []):
                    wall = guid_of(part.get("elementId"))
                    if wall:
                        lengths[wall] += abs(part.get("endDistance", 0.0) - part.get("begDistance", 0.0))
                walls_of_zone.append(lengths)

            wall_surfaces = self._wall_surface_labels(sorted({w for lengths in walls_of_zone for w in lengths}), problems)

            self.window.set_status("Reading the doors and windows...")
            openings_of_zone = self._openings_by_zone(problems)

        rows = []
        for zone, detail, quantity, wall_lengths in zip(zones, zone_details, zone_quantities, walls_of_zone):
            if "error" in detail or "error" in quantity:
                continue
            zone_guid = guid_of(zone)
            info = detail.get("details", {})
            q = quantity.get("quantities", {})
            floor_index = detail.get("floorIndex")
            story = story_names.get(floor_index, str(floor_index if floor_index is not None else ""))
            head = [story, info.get("numberStr", ""), info.get("name", "")]
            height = q.get("height", 0.0)
            floor_area = q.get("netArea", q.get("area", 0.0))
            rows.append([floor_index, *head, "Floor", "", "", "", "", round(floor_area, 2)])

            # Archicad's length of the walls around the zone, split by surface in the proportion of the
            # wall parts the zone relations report.
            parts_total = sum(wall_lengths.values())
            walls_length = q.get("wallsPerimeter") or parts_total
            lengths: dict[str, float] = defaultdict(float)
            if parts_total > 0:
                for wall, length in wall_lengths.items():
                    lengths[wall_surfaces.get(wall, NOT_REPORTED)] += walls_length * length / parts_total
            if not lengths and walls_length > 0:
                lengths[NOT_REPORTED] = walls_length

            # Each opening comes off the surface of its own wall; one in a wall that is not listed
            # around the zone comes off every surface in proportion.
            openings: dict[str, float] = defaultdict(float)
            for owner, area in openings_of_zone.get(zone_guid, []):
                label = wall_surfaces.get(owner, NOT_REPORTED) if owner in wall_lengths else None
                if label in lengths:
                    openings[label] += area
                elif walls_length > 0:
                    for other, length in lengths.items():
                        openings[other] += area * length / walls_length

            for label in sorted(lengths, key=lambda s: (s == NOT_REPORTED, s.lower())):
                gross = lengths[label] * height
                rows.append([floor_index, *head, "Wall", label, round(lengths[label], 2), round(height, 2),
                             round(openings[label], 2), round(max(0.0, gross - openings[label]), 2)])

            rows.append([floor_index, *head, "Ceiling", "", "", "", "", round(floor_area, 2)])

        rows.sort(key=lambda r: (-(r[0] if isinstance(r[0], int) else 0), str(r[2]), str(r[3])))
        self._show([r[1:] for r in rows], len(zones))
        if problems:
            for problem in problems:
                print(f"Skipped: {problem}")
            self.window.set_status(f"Done, {len(problems)} item(s) could not be read (see the palette output): "
                                   f"{problems[0]}", "warning")
        else:
            self.window.set_status("Done.", "ok")

    def _surface_names(self) -> dict[str, str]:
        attributes = run_tapir("GetAttributesByType", {"attributeType": "Surface"}).get("attributes", [])
        return {guid_of(a.get("attributeId")): a.get("name", "") for a in attributes}

    def _wall_surface_labels(self, walls: list[str], problems: list[str]) -> dict[str, str]:
        """ The surface label of each wall: the one surface it shows, or its surfaces with
        "(side not reported)" when they differ. """
        if not walls:
            return {}
        names = self._surface_names()

        def name(guid: str) -> str:
            return names.get(guid, "(unnamed surface)")

        def several(guids: set[str]) -> str:
            return " / ".join(sorted({name(g) for g in guids}, key=str.lower)) + " (side not reported)"

        wall_ids = [{"elementId": {"guid": w}} for w in walls]
        details = get_details_of_elements(wall_ids, ["details"])
        problems += item_errors(details)
        # coverElements [] makes Archicad list every surface of the wall without the costly overlap check.
        try:
            surfaces = per_element("GetSurfaceQuantitiesOfElements", "surfaceQuantitiesOfElements", wall_ids,
                                   coverElements=[])
        except ArchicadError as e:
            print(f"Surface quantities are not available, only surface overrides are used: {e}")
            surfaces = [{} for _ in walls]
        problems += item_errors(surfaces)

        labels = {}
        for wall, detail, surface in zip(walls, details, surfaces):
            info = detail.get("details", {}) if "error" not in detail else {}
            reference = overridden_surface(info.get("referenceMaterial"))
            opposite = overridden_surface(info.get("oppositeMaterial"))
            if reference and opposite:
                labels[wall] = name(reference) if reference == opposite else several({reference, opposite})
                continue
            shown = {guid_of(s.get("surfaceId")) for s in surface.get("surfaces", []) if s.get("surfaceId")}
            shown.discard(None)
            if len(shown) == 1:
                labels[wall] = name(next(iter(shown)))
            elif shown:
                labels[wall] = several(shown)
            else:
                labels[wall] = NOT_REPORTED
        return labels

    def _openings_by_zone(self, problems: list[str]) -> dict[str, list[tuple[str | None, float]]]:
        """ For each zone guid, the (owner wall guid, area) of the doors and windows opening into it. """
        openings = get_elements("project", ["Door", "Window"])
        if not openings:
            return {}
        details = get_details_of_elements(openings, ["details"])
        try:
            relations = per_element("GetRelationsOfElements", "relations", openings)
        except ArchicadError as e:
            print(f"Opening relations are not available, doors and windows are not subtracted: {e}")
            return {}
        problems += item_errors(details) + item_errors(relations)

        by_zone: dict[str, list[tuple[str | None, float]]] = defaultdict(list)
        for detail, relation in zip(details, relations):
            if "error" in detail or "error" in relation:
                continue
            info = detail.get("details", {})
            area = info.get("width", 0.0) * info.get("height", 0.0)
            owner = guid_of(info.get("ownerElementId"))
            sides = relation.get("openingRelations") or {}
            for zone in {guid_of(sides.get("fromRoom")), guid_of(sides.get("toRoom"))} - {None}:
                by_zone[zone].append((owner, area))
        return by_zone

    def _show(self, rows: list[list], zone_count: int) -> None:
        self.rows = rows
        self.tree.delete(*self.tree.get_children())
        for row in rows:
            self.tree.insert("", "end", values=[f"{v:.2f}" if isinstance(v, float) else v for v in row])
        totals = defaultdict(float)
        for row in rows:
            totals[row[3]] += row[8]
        self.summary_label.configure(
            text=f"{zone_count} zone(s): floors {totals['Floor']:.2f} m², walls {totals['Wall']:.2f} m², "
                 f"ceilings {totals['Ceiling']:.2f} m²")

    def save(self) -> None:
        if not self.rows:
            self.window.set_status("Nothing to save yet. Press Calculate first.", "warning")
            return
        path = filedialog.asksaveasfilename(
            parent=self.window.root, title="Save the room finishes schedule",
            defaultextension=".csv", initialfile="Room finishes.csv",
            filetypes=[("CSV (comma separated)", "*.csv"), ("Excel workbook", "*.xlsx")])
        if not path:
            return
        write_table(path, HEADERS, self.rows, sheet_name="Room finishes")
        self.window.set_status(f"Saved {len(self.rows)} rows to {path}", "ok")

    def run(self) -> None:
        self.window.run(on_start=self.calculate)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    RoomFinishesSchedule().run()
