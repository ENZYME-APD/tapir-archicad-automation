# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# category = "modeling-geometry"
# archicad-versions = [25, 26, 27, 28, 29, 30]
# min-tapir-version = "1.8.0"
# changes = "model"
# ///

"""
Assign Home Story

Gives every selected element the home story that matches where it sits in 3D,
from its 3D bounding box. Each story owns the height band from its level up to
the level of the next story (the lowest story's band reaches down without end,
the highest story's band up). Three modes pick the story:

- Centre: the story whose band holds the centre of the bounding box.
- Bottom: the story whose band holds the bottom of the bounding box.
- Majority: the story whose band holds the largest part of the box's height
  (a tie goes to the lower story).

A bottom or top within the snap tolerance of a story level counts as on that
level, so an element starting at 2.995 m goes to the story at 3.00 m.

The elements stay where they are in 3D: after the home story has changed, the
bounding boxes are read again, and an element its offset has shifted is moved
back. Doors, windows, skylights, openings, the parts of curtain walls, stairs,
railings, beams and columns, and 2D elements are left alone (their story
follows their host, or they have no height).

Run it from the Tapir palette for its window, or from the command line:

    AssignHomeStory.py --mode center|bottom|majority [--dry-run] [--tolerance "1 cm"]
"""

import argparse
import math
import re
import sys
import tkinter as tk
from collections import Counter
from tkinter import ttk

try:
    from utilities.archicad import ArchicadError, batched, get_details_of_elements, is_alive, item_errors, run_tapir
    from utilities.ui import ScriptWindow, make_tree, require_archicad
except ModuleNotFoundError as error:
    # The palette of a Tapir newer than 2.0.0 provides the helpers; an older one needs copies next to the script.
    if (error.name or "").split(".")[0] not in ("utilities", "aclib"):
        raise
    sys.exit("Error: the Tapir helpers utilities and aclib were not found. Update the Tapir Add-On, or copy the "
             "utilities and aclib folders of builtin-scripts "
             "(https://github.com/ENZYME-APD/tapir-archicad-automation/tree/main/builtin-scripts) next to this script.")

SCRIPT_NAME = "Assign Home Story"
DEFAULT_TOLERANCE = "1 cm"

CENTER, BOTTOM, MAJORITY = "center", "bottom", "majority"
MODES = [
    (CENTER, "Centre", "the story whose band holds the centre of the bounding box"),
    (BOTTOM, "Bottom", "the story whose band holds the bottom of the bounding box"),
    (MAJORITY, "Majority", "the story whose band holds the largest part of the height (a tie goes to the lower story)"),
]

# The model element types whose home story is changed. Everything else is left alone:
# doors, windows, skylights and openings follow their host; the segments and parts of
# curtain walls, stairs, railings, beams and columns follow their owner; 2D elements,
# views and drawings have no height to place.
MODEL_TYPES = {
    "Wall", "Column", "Beam", "Object", "Lamp", "Slab", "Roof", "Mesh", "Zone",
    "CurtainWall", "Shell", "Morph", "Stair", "Railing",
}

MOVE_THRESHOLD = 1e-5     # m: a smaller shift after the story change is not moved back
IN_PLACE_TOLERANCE = 1e-3  # m: a box that differs more than this afterwards is reported
TIE_EPSILON = 1e-6        # m: overlaps closer than this are a tie

UNIT_FACTORS = {
    "mm": 0.001, "cm": 0.01, "m": 1.0,
    "in": 0.0254, "\"": 0.0254, "inch": 0.0254, "inches": 0.0254,
    "ft": 0.3048, "'": 0.3048, "foot": 0.3048, "feet": 0.3048,
}


def parse_length(text: str) -> float:
    """ A length in metres from text such as "1 cm", "10mm", "0.01 m", "0.5 in", "3/8\"" or
    "0.05 ft". A bare number is in metres. Raises ValueError for anything else. """
    match = re.fullmatch(r"\s*(\d+(?:[.,]\d*)?|\.\d+|\d+\s*/\s*\d+)\s*([a-zA-Z\"']*)\s*", text or "")
    if not match:
        raise ValueError(f"'{text}' is not a length. Write it like 1 cm, 10 mm, 0.5 in or 3/8\".")
    number, unit = match.group(1).replace(",", "."), match.group(2).lower()
    if "/" in number:
        numerator, denominator = (float(part) for part in number.split("/"))
        if denominator == 0:
            raise ValueError(f"'{text}' is not a length.")
        value = numerator / denominator
    else:
        value = float(number)
    if unit and unit not in UNIT_FACTORS:
        raise ValueError(f"Unknown unit '{unit}' in '{text}'. Use mm, cm, m, in or ft.")
    return value * UNIT_FACTORS.get(unit, 1.0)


class Stories:
    """ The stories sorted by level, with the band each one owns. """

    def __init__(self, stories: list[dict]):
        self.stories = sorted(stories, key=lambda s: s["level"])
        self.levels = [s["level"] for s in self.stories]
        self.names = {s["index"]: (s.get("name") or "").strip() for s in self.stories}

    def label(self, index: int) -> str:
        name = self.names.get(index)
        return f"{index}. {name}" if name else f"Story {index}"

    def band(self, position: int) -> tuple[float, float]:
        low = -math.inf if position == 0 else self.levels[position]
        high = math.inf if position == len(self.levels) - 1 else self.levels[position + 1]
        return low, high

    def snap(self, z: float, tolerance: float) -> float:
        """ z, or the story level nearest to it when that is within the tolerance. """
        nearest = min(self.levels, key=lambda level: abs(level - z))
        return nearest if abs(nearest - z) <= tolerance else z

    def story_at(self, z: float) -> int:
        """ The index of the story whose band holds z. A z exactly on a level is on the story of that level. """
        position = 0
        for i, level in enumerate(self.levels):
            if level <= z:
                position = i
        return self.stories[position]["index"]

    def target(self, box: dict, mode: str, tolerance: float) -> int:
        bottom = self.snap(box["zMin"], tolerance)
        top = max(bottom, self.snap(box["zMax"], tolerance))
        if mode == BOTTOM:
            return self.story_at(bottom)
        if mode == CENTER:
            return self.story_at((bottom + top) / 2)
        if top - bottom <= TIE_EPSILON:
            return self.story_at(bottom)
        best, best_overlap = None, -1.0
        for position, story in enumerate(self.stories):
            low, high = self.band(position)
            overlap = max(0.0, min(high, top) - max(low, bottom))
            if overlap > best_overlap + TIE_EPSILON:  # ascending, so a tie keeps the lower story
                best, best_overlap = story["index"], overlap
        return best


def read_boxes(elements: list[dict]) -> list[dict | None]:
    """ The 3D bounding box of each element, None for one without. """
    boxes = []
    for chunk in batched(elements):
        result = run_tapir("Get3DBoundingBoxes", {"elements": chunk}).get("boundingBoxes3D", [])
        boxes += [item.get("boundingBox3D") if isinstance(item, dict) else None for item in result]
    return boxes + [None] * (len(elements) - len(boxes))


def make_plan(mode: str, tolerance: float) -> dict:
    """ Reads the selection and works out which element goes to which story. """
    stories_result = run_tapir("GetStories")
    stories = Stories(stories_result.get("stories", []))
    if not stories.stories:
        raise ArchicadError("The project has no stories.")

    selected = run_tapir("GetSelectedElements").get("elements", [])
    plan = {"stories": stories, "selected": len(selected), "changes": [], "unchanged": 0, "skipped": Counter()}
    if not selected:
        return plan

    details = get_details_of_elements(selected, ["type", "id", "floorIndex"])
    candidates = []
    for element, detail in zip(selected, details):
        element_type = detail.get("type", "Unknown") if isinstance(detail, dict) else "Unknown"
        if not isinstance(detail, dict) or "error" in detail or detail.get("fromOtherDatabase"):
            plan["skipped"]["not on the floor plan"] += 1
        elif element_type not in MODEL_TYPES:
            plan["skipped"][element_type] += 1
        else:
            candidates.append((element, detail))

    boxes = read_boxes([{"elementId": element["elementId"]} for element, _ in candidates])
    for (element, detail), box in zip(candidates, boxes):
        if box is None:
            plan["skipped"]["no 3D bounding box"] += 1
            continue
        current = detail.get("floorIndex")
        target = stories.target(box, mode, tolerance)
        if target == current:
            plan["unchanged"] += 1
            continue
        plan["changes"].append({
            "elementId": element["elementId"],
            "type": detail.get("type", ""),
            "id": detail.get("id") or element["elementId"]["guid"][:8],
            "from": current,
            "to": target,
            "box": box,
        })
    return plan


def summary_lines(plan: dict) -> list[str]:
    """ How many elements go from which story to which. """
    stories = plan["stories"]
    moves = Counter((change["from"], change["to"]) for change in plan["changes"])
    lines = [f"{count} from {stories.label(source)} to {stories.label(target)}"
             for (source, target), count in sorted(moves.items())]
    return lines


def skipped_text(plan: dict) -> str:
    if not plan["skipped"]:
        return ""
    parts = [f"{count} {reason}" for reason, count in sorted(plan["skipped"].items())]
    return "Left alone: " + ", ".join(parts) + "."


def apply_plan(plan: dict, progress=print) -> dict:
    """ Changes the home stories, then moves back the elements the change has shifted.
    Returns the counts and the IDs of the elements that failed or did not stay in place. """
    changes = plan["changes"]
    failed: list[str] = []

    progress(f"Changing the home story of {len(changes)} elements...")
    changed = []
    for chunk in batched(changes):
        result = run_tapir("SetDetailsOfElements", {"elementsWithDetails": [
            {"elementId": change["elementId"], "details": {"floorIndex": change["to"]}} for change in chunk
        ]}).get("executionResults", [])
        for change, item in zip(chunk, result):
            errors = item_errors([item])
            if errors or not item.get("success", True):
                failed.append(f"{change['id']}: {errors[0] if errors else 'not changed'}")
            else:
                changed.append(change)

    progress(f"Keeping {len(changed)} elements in place...")
    after = read_boxes([{"elementId": change["elementId"]} for change in changed])
    moves = []
    for change, box in zip(changed, after):
        if box is not None:
            shift = change["box"]["zMin"] - box["zMin"]
            if abs(shift) > MOVE_THRESHOLD:
                moves.append((change, shift))
    for chunk in batched(moves):
        result = run_tapir("MoveElements", {"elementsWithMoveVectors": [
            {"elementId": change["elementId"], "moveVector": {"x": 0.0, "y": 0.0, "z": shift}} for change, shift in chunk
        ]}).get("executionResults", [])
        for (change, _), item in zip(chunk, result):
            errors = item_errors([item])
            if errors:
                failed.append(f"{change['id']}: changed story, but could not be moved back: {errors[0]}")

    # The elements whose box is not the same as before: a top linked to a story, for example,
    # follows the new home story and changes the height.
    not_in_place = []
    final = read_boxes([{"elementId": change["elementId"]} for change in changed])
    for change, box in zip(changed, final):
        if box is None:
            continue
        if (abs(box["zMin"] - change["box"]["zMin"]) > IN_PLACE_TOLERANCE
                or abs(box["zMax"] - change["box"]["zMax"]) > IN_PLACE_TOLERANCE):
            not_in_place.append(change["id"])

    return {"changed": len(changed), "moved": len(moves), "failed": failed, "notInPlace": not_in_place}


def result_text(result: dict) -> str:
    text = f"{result['changed']} elements got a new home story ({result['moved']} moved back into place)."
    if result["failed"]:
        text += f" {len(result['failed'])} failed, see the output."
    if result["notInPlace"]:
        text += f" {len(result['notInPlace'])} changed in height or position, see the output."
    return text


def print_result(result: dict) -> None:
    for line in result["failed"]:
        print(f"Failed: {line}")
    if result["notInPlace"]:
        print("Not in the same place or height as before (a top linked to a story follows the new home story; "
              "check these): " + ", ".join(result["notInPlace"]))


class AssignHomeStory:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Select elements in Archicad, choose how the story is picked and press Preview. Apply gives every "
            "listed element the story it sits on in 3D and keeps it where it is. Doors, windows, openings and "
            "the parts of curtain walls, stairs and railings follow their host and are left alone.",
            min_width=600, min_height=500)
        self._build_body()
        self.window.add_button("Preview", self.preview)
        self.apply_button = self.window.add_button("Apply...", self.apply, primary=True)

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(2, weight=1)

        options = ttk.LabelFrame(body, text="Home story by", padding=(10, 6))
        options.grid(row=0, column=0, sticky="ew")
        options.columnconfigure(1, weight=1)
        self.mode = tk.StringVar(value=CENTER)
        for row, (value, label, description) in enumerate(MODES):
            ttk.Radiobutton(options, text=label, value=value, variable=self.mode).grid(row=row, column=0, sticky="w")
            ttk.Label(options, text=description, style="Muted.TLabel").grid(row=row, column=1, sticky="w", padx=(8, 0))

        tolerance = ttk.Frame(body)
        tolerance.grid(row=1, column=0, sticky="ew", pady=(8, 4))
        ttk.Label(tolerance, text="Snap tolerance:").pack(side="left")
        self.tolerance = tk.StringVar(value=DEFAULT_TOLERANCE)
        ttk.Entry(tolerance, textvariable=self.tolerance, width=10).pack(side="left", padx=(6, 0))
        ttk.Label(tolerance, text="a bottom or top this close to a story level counts as on it (mm, cm, m, in, ft)",
                  style="Muted.TLabel").pack(side="left", padx=(8, 0))

        self.tree = make_tree(body, [
            ("id", "Element ID", 160, "w"),
            ("type", "Type", 100, "w"),
            ("from", "Home story now", 140, "w"),
            ("to", "New home story", 140, "w"),
        ], height=12)
        self.tree.container.grid(row=2, column=0, sticky="nsew", pady=(4, 0))
        self.summary = ttk.Label(body, text="Press Preview to see what would change.", justify="left")
        self.summary.grid(row=3, column=0, sticky="ew", pady=(6, 0))
        body.bind("<Configure>", lambda e: self.summary.configure(wraplength=max(200, e.width - 28)))

    def _make_plan(self) -> dict | None:
        try:
            tolerance = parse_length(self.tolerance.get())
        except ValueError as e:
            self.window.set_status(str(e), "error")
            return None
        with self.window.busy("Reading the selection..."):
            plan = make_plan(self.mode.get(), tolerance)
        self._show(plan)
        return plan

    def _show(self, plan: dict) -> None:
        self.tree.delete(*self.tree.get_children())
        stories = plan["stories"]
        for change in plan["changes"]:
            self.tree.insert("", "end", values=(change["id"], change["type"],
                                                stories.label(change["from"]), stories.label(change["to"])))
        if not plan["selected"]:
            self.summary.configure(text="Nothing is selected. Select the elements in Archicad first.")
            return
        lines = summary_lines(plan) or ["No element changes story."]
        lines.append(f"{plan['unchanged']} already on the right story.")
        skipped = skipped_text(plan)
        if skipped:
            lines.append(skipped)
        self.summary.configure(text="\n".join(lines))
        self.window.set_status(f"{len(plan['changes'])} of {plan['selected']} selected elements would change story.")

    def preview(self) -> None:
        self._make_plan()

    def apply(self) -> None:
        # The selection is read again, so the change is made to what is selected now.
        plan = self._make_plan()
        if plan is None:
            return
        if not plan["changes"]:
            self.window.set_status("Nothing to change: every selected element is on its story already.")
            return
        message = (f"Change the home story of {len(plan['changes'])} elements?\n\n" + "\n".join(summary_lines(plan)) +
                   "\n\nThe elements are kept in place in 3D. The story change and the move back are separate "
                   "Undo steps.")
        if not self.window.ask_ok_cancel(SCRIPT_NAME, message):
            return
        with self.window.busy(f"Changing the home story of {len(plan['changes'])} elements..."):
            result = apply_plan(plan, progress=self.window.set_status)
        print_result(result)
        self.window.set_status(result_text(result), "error" if result["failed"] or result["notInPlace"] else "info")

    def run(self) -> None:
        self.window.run(on_start=self.preview)


def run_command_line(args: argparse.Namespace) -> int:
    if not is_alive():
        print("Error: cannot connect to Archicad. Start Archicad with the Tapir Add-On, or pass its --port.")
        return 1
    try:
        tolerance = parse_length(args.tolerance)
    except ValueError as e:
        print(f"Error: {e}")
        return 1
    try:
        plan = make_plan(args.mode, tolerance)
        if not plan["selected"]:
            print("Nothing is selected. Select the elements in Archicad first.")
            return 0
        stories = plan["stories"]
        for change in plan["changes"]:
            print(f"{change['id']} ({change['type']}): {stories.label(change['from'])} -> {stories.label(change['to'])}")
        for line in summary_lines(plan):
            print(line)
        print(f"{plan['unchanged']} already on the right story.")
        skipped = skipped_text(plan)
        if skipped:
            print(skipped)
        if not plan["changes"]:
            print("Nothing to change.")
            return 0
        if args.dry_run:
            print("Dry run: nothing was changed.")
            return 0
        result = apply_plan(plan)
    except ArchicadError as e:
        print(f"Error: {e}")
        return 1
    print_result(result)
    print(result_text(result))
    return 1 if result["failed"] else 0


def main() -> None:
    parser = argparse.ArgumentParser(description="Gives the selected elements the home story they sit on in 3D.")
    parser.add_argument("--mode", choices=[CENTER, BOTTOM, MAJORITY],
                        help="Run without the window: how the story is picked.")
    parser.add_argument("--dry-run", action="store_true", help="Only report what would change.")
    parser.add_argument("--tolerance", default=DEFAULT_TOLERANCE,
                        help="Snap tolerance, e.g. \"1 cm\", \"10 mm\", \"0.5 in\" (default: 1 cm).")
    # --port and --host, passed by the palette, are read by aclib.
    args, _ = parser.parse_known_args()
    if args.mode:
        sys.exit(run_command_line(args))
    require_archicad(SCRIPT_NAME)
    AssignHomeStory().run()


if __name__ == "__main__":
    main()
