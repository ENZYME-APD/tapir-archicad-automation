# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# category = "import-export"
# archicad-versions = [25, 26, 27, 28, 29, 30]
# min-tapir-version = "1.8.0"
# changes = "model"
# ///

"""
Property Value Importer

Reads a CSV or Excel (.xlsx) file in the layout of the Property Value Exporter and writes
its values into the properties of the elements, after showing what would change.

The file needs a header row. The columns are:
  GUID        identifies the element (optional when there is an Element ID column)
  Element ID  identifies the element when the row has no GUID
  Type, Story ignored (the exporter writes them for reading)
  then one column per property, headed "<property group>: <property name>"
An empty cell leaves the value as it is. Only cells that differ from the current value are
listed. Nothing is written until the preview has been confirmed; then one
SetPropertyValuesOfElements call writes all values in one Undo step.
"""

import csv
import io
import os
import re
import zipfile
import xml.etree.ElementTree as ET
from tkinter import filedialog

from utilities.archicad import ArchicadError, batched, get_details_of_elements, get_elements, run_tapir
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Property Value Importer"
FIXED_HEADERS = {"guid", "element id", "type", "story"}
MAX_PREVIEW_ROWS = 5000
SPREADSHEET_NS = "{http://schemas.openxmlformats.org/spreadsheetml/2006/main}"
RELATIONSHIP_NS = "{http://schemas.openxmlformats.org/officeDocument/2006/relationships}"
PACKAGE_RELATIONSHIP_NS = "{http://schemas.openxmlformats.org/package/2006/relationships}"


def property_header(prop: dict) -> str:
    """ The column heading the Property Value Exporter writes. """
    return f"{prop.get('propertyGroupName', '')}: {prop.get('propertyName', '')}"


# ---- reading the file ----

def read_csv(path: str) -> list[list[str]]:
    with open(path, newline="", encoding="utf-8-sig") as file:
        text = file.read()
    first_line = text.split("\n", 1)[0]
    delimiter = ";" if first_line.count(";") > first_line.count(",") else ","
    return [row for row in csv.reader(io.StringIO(text, newline=""), delimiter=delimiter)]


def _column_index(reference: str) -> int:
    letters = re.match(r"[A-Za-z]+", reference)
    index = 0
    for char in (letters.group(0).upper() if letters else "A"):
        index = index * 26 + ord(char) - 64
    return index - 1


def _text_of(element: ET.Element | None) -> str:
    if element is None:
        return ""
    return "".join(t.text or "" for t in element.iter(f"{SPREADSHEET_NS}t"))


def read_xlsx(path: str) -> list[list[str]]:
    """ The first sheet of the workbook as rows of text. """
    with zipfile.ZipFile(path) as archive:
        names = set(archive.namelist())
        shared: list[str] = []
        if "xl/sharedStrings.xml" in names:
            root = ET.fromstring(archive.read("xl/sharedStrings.xml"))
            shared = [_text_of(si) for si in root.iter(f"{SPREADSHEET_NS}si")]
        sheet_path = None
        if "xl/workbook.xml" in names and "xl/_rels/workbook.xml.rels" in names:
            workbook = ET.fromstring(archive.read("xl/workbook.xml"))
            first = next(workbook.iter(f"{SPREADSHEET_NS}sheet"), None)
            if first is not None:
                relationship_id = first.get(f"{RELATIONSHIP_NS}id")
                for rel in ET.fromstring(archive.read("xl/_rels/workbook.xml.rels")).iter(f"{PACKAGE_RELATIONSHIP_NS}Relationship"):
                    if rel.get("Id") == relationship_id:
                        target = rel.get("Target", "").lstrip("/")
                        sheet_path = target if target.startswith("xl/") else f"xl/{target}"
        if sheet_path not in names:
            sheets = sorted(n for n in names if n.startswith("xl/worksheets/") and n.endswith(".xml"))
            if not sheets:
                raise ValueError("The workbook has no sheet.")
            sheet_path = sheets[0]
        rows: list[list[str]] = []
        for row in ET.fromstring(archive.read(sheet_path)).iter(f"{SPREADSHEET_NS}row"):
            cells: dict[int, str] = {}
            for position, cell in enumerate(row.iter(f"{SPREADSHEET_NS}c")):
                column = _column_index(cell.get("r", "")) if cell.get("r") else position
                kind = cell.get("t")
                if kind == "inlineStr":
                    value = _text_of(cell.find(f"{SPREADSHEET_NS}is"))
                else:
                    v = cell.find(f"{SPREADSHEET_NS}v")
                    value = v.text or "" if v is not None else ""
                    if kind == "s":
                        value = shared[int(value)] if value.isdigit() and int(value) < len(shared) else ""
                    elif kind == "b":
                        value = "True" if value == "1" else "False"
                cells[column] = value
            width = max(cells, default=-1) + 1
            rows.append([cells.get(i, "") for i in range(width)])
        return rows


def read_table(path: str) -> list[list[str]]:
    return read_xlsx(path) if path.lower().endswith(".xlsx") else read_csv(path)


# ---- comparing values ----

def _number(text: str) -> float | None:
    """ The number in text, with '.' or ',' as the decimal separator, or None. """
    cleaned = text.strip().replace(" ", "")
    if not re.fullmatch(r"[-+]?(\d+([.,]\d*)?|[.,]\d+)", cleaned):
        return None
    return float(cleaned.replace(",", "."))


def same_value(current: str, new: str) -> bool:
    if current.strip() == new.strip():
        return True
    a, b = _number(current), _number(new)
    return a is not None and b is not None and abs(a - b) <= 1e-9 * max(1.0, abs(a), abs(b))


class PropertyValueImporter:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Reads a CSV or Excel file written by the Property Value Exporter and writes its values into "
            "the properties of the elements. Open the file to see what would change; nothing is written "
            "until you press Write to Archicad and confirm.",
            min_width=860, min_height=560)
        self.properties: list[dict] = []
        self.changes: list[dict] = []
        self.file_name = ""
        self._build_body()
        self.window.add_button("Open file...", self.open_file)
        self.write_button = self.window.add_button("Write to Archicad", self.write, primary=True)
        self.write_button.state(["disabled"])

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(0, weight=1)
        self.tree = make_tree(body, [("element", "Element ID", 110, "w"), ("property", "Property", 220, "w"),
                                     ("current", "Current", 130, "w"), ("new", "New", 130, "w"),
                                     ("result", "Result", 260, "w")], height=16)
        self.tree.container.grid(row=0, column=0, sticky="nsew")
        self.tree.tag_configure("problem", foreground="#b00020")
        self.tree.tag_configure("done", foreground="#1b7f3b")

    def load_properties(self) -> None:
        with self.window.busy("Reading the properties..."):
            properties = run_tapir("GetAllProperties").get("properties", [])
        self.properties = [p for p in properties if "propertyId" in p]
        self.window.set_status("Press Open file... and choose the CSV or Excel file.")

    # ---- reading and previewing ----

    def _show(self, rows: list[tuple]) -> None:
        self.tree.delete(*self.tree.get_children())
        for index, (values, tag) in enumerate(rows[:MAX_PREVIEW_ROWS]):
            self.tree.insert("", "end", iid=str(index), values=values, tags=(tag,) if tag else ())

    def _read_current(self, guids: list[str], property_ids: list[dict]) -> dict[str, list[dict | None]]:
        """ By element GUID, one item per property: {"value": text, "available": bool}, or None when unreadable. """
        current: dict[str, list[dict | None]] = {}
        with_availability = True
        for chunk in batched([{"elementId": {"guid": g}} for g in guids], 500):
            parameters = {"elements": chunk, "properties": property_ids}
            if with_availability:
                try:
                    result = run_tapir("GetPropertyValuesOfElements", {**parameters, "includeAvailability": True})
                except ArchicadError:
                    # Add-Ons before includeAvailability reject the parameter
                    with_availability = False
            if not with_availability:
                result = run_tapir("GetPropertyValuesOfElements", parameters)
            for element, entry in zip(chunk, result.get("propertyValuesForElements", [])):
                items = entry.get("propertyValues") or []
                row: list[dict | None] = []
                for item in items:
                    available = item.get("isAvailable", True)
                    if "propertyValue" in item:
                        value = item["propertyValue"].get("value", "")
                        row.append({"value": value if isinstance(value, str) else str(value), "available": available})
                    else:
                        row.append({"value": "", "available": available and "error" not in item})
                row += [None] * (len(property_ids) - len(row))
                current[element["elementId"]["guid"]] = row
        return current

    def open_file(self) -> None:
        path = filedialog.askopenfilename(parent=self.window.root, title="Open the property values",
                                          filetypes=[("Excel or CSV", "*.xlsx *.csv"), ("All files", "*.*")])
        if not path:
            return
        self.write_button.state(["disabled"])
        self.changes = []
        try:
            table = read_table(path)
        except (OSError, ValueError, zipfile.BadZipFile, ET.ParseError, csv.Error, UnicodeDecodeError) as e:
            self.window.set_status(f"Cannot read {os.path.basename(path)}: {e}", "error")
            return
        self.file_name = os.path.basename(path)
        table = [row for row in table if any(cell.strip() for cell in row)]
        if len(table) < 2:
            self.window.set_status(f"{self.file_name} has no data rows.", "warning")
            return

        # the columns
        by_header = {}
        for prop in self.properties:
            by_header.setdefault(property_header(prop).strip().lower(), prop)
        header = [cell.strip() for cell in table[0]]
        guid_column = next((i for i, h in enumerate(header) if h.lower() == "guid"), None)
        id_column = next((i for i, h in enumerate(header) if h.lower() == "element id"), None)
        if guid_column is None and id_column is None:
            self.window.set_status(f"{self.file_name} needs a GUID or an Element ID column.", "error")
            return
        property_columns: list[tuple[int, dict]] = []
        unknown: list[str] = []
        seen: set[str] = set()
        for index, name in enumerate(header):
            if not name or name.lower() in FIXED_HEADERS:
                continue
            prop = by_header.get(name.lower())
            if prop is None or prop["propertyId"]["guid"] in seen:
                unknown.append(name)
                continue
            seen.add(prop["propertyId"]["guid"])
            property_columns.append((index, prop))
        for name in unknown:
            print(f"Column skipped (no such property, or the column is repeated): {name}")
        if not property_columns:
            self.window.set_status(f"No column of {self.file_name} is a property of this project "
                                   f"(headed like \"Group: Name\").", "error")
            return

        with self.window.busy("Reading the elements..."):
            elements = get_elements("project")
            details = get_details_of_elements(elements, ["id"])
            guid_ids = {e["elementId"]["guid"]: d.get("id", "") for e, d in zip(elements, details)}
            by_element_id: dict[str, list[str]] = {}
            for guid, element_id in guid_ids.items():
                if element_id:
                    by_element_id.setdefault(element_id, []).append(guid)

            def cell(row: list[str], column: int | None) -> str:
                return row[column].strip() if column is not None and column < len(row) else ""

            rows_out: list[tuple] = []  # (values, tag) problems that need no property
            matched: list[tuple[str, str, list[str]]] = []  # (guid, label, row)
            used: set[str] = set()
            for number, row in enumerate(table[1:], start=2):
                guid, element_id = cell(row, guid_column), cell(row, id_column)
                label = element_id or guid
                if guid and guid in guid_ids:
                    target = guid
                elif guid and not element_id:
                    rows_out.append(((label, "", "", "", f"Row {number}: no element with this GUID"), "problem"))
                    continue
                elif element_id:
                    candidates = by_element_id.get(element_id, [])
                    if not candidates:
                        rows_out.append(((label, "", "", "", f"Row {number}: no element with this Element ID"), "problem"))
                        continue
                    if len(candidates) > 1:
                        rows_out.append(((label, "", "", "", f"Row {number}: Element ID used by {len(candidates)} elements"), "problem"))
                        continue
                    target = candidates[0]
                else:
                    rows_out.append((("", "", "", "", f"Row {number}: neither GUID nor Element ID"), "problem"))
                    continue
                if target in used:
                    rows_out.append(((guid_ids[target] or label, "", "", "", f"Row {number}: element already in an earlier row, ignored"), "problem"))
                    continue
                used.add(target)
                matched.append((target, guid_ids[target] or label, row))

            property_ids = [{"propertyId": prop["propertyId"]} for _, prop in property_columns]
            current = self._read_current([m[0] for m in matched], property_ids) if matched else {}

            changes: list[dict] = []
            change_rows: list[tuple] = []
            unchanged = 0
            for guid, label, row in matched:
                values = current.get(guid, [None] * len(property_columns))
                for (column, prop), now in zip(property_columns, values):
                    new = cell(row, column)
                    if not new:
                        continue
                    name = property_header(prop)
                    if now is None or not now["available"]:
                        rows_out.append(((label, name, "", new, "Property not available for this element"), "problem"))
                    elif same_value(now["value"], new):
                        unchanged += 1
                    else:
                        changes.append({"guid": guid, "property": prop, "new": new, "row": len(change_rows)})
                        change_rows.append(((label, name, now["value"], new, "Will change"), ""))

        self.changes = changes
        self._show(change_rows + rows_out)
        problems = len(rows_out)
        parts = [f"{len(changes)} value(s) would change", f"{unchanged} already the same"]
        if problems:
            parts.append(f"{problems} problem(s), not written")
        if unknown:
            parts.append(f"{len(unknown)} unknown column(s) skipped")
        shown = "" if len(change_rows) + problems <= MAX_PREVIEW_ROWS else f" Only the first {MAX_PREVIEW_ROWS} are listed."
        self.window.set_status(f"{self.file_name}: " + ", ".join(parts) + "." + shown,
                               "warning" if problems or unknown else "info")
        if changes:
            self.write_button.state(["!disabled"])

    # ---- writing ----

    def write(self) -> None:
        if not self.changes:
            self.window.set_status("Nothing to write. Open a file first.", "warning")
            return
        elements = len({c["guid"] for c in self.changes})
        if not self.window.ask_ok_cancel(
                SCRIPT_NAME, f"Write {len(self.changes)} value(s) into {elements} element(s) of the open project?\n\n"
                             "It is one Undo step in Archicad."):
            return
        parameters = {"elementPropertyValues": [
            {"elementId": {"guid": c["guid"]}, "propertyId": c["property"]["propertyId"],
             "propertyValue": {"value": c["new"]}} for c in self.changes]}
        with self.window.busy("Writing the values..."):
            results = run_tapir("SetPropertyValuesOfElements", parameters).get("executionResults", [])
        succeeded = failed = 0
        for change, result in zip(self.changes, results):
            iid = str(change["row"])
            if not self.tree.exists(iid):
                if result.get("success"):
                    succeeded += 1
                else:
                    failed += 1
                continue
            values = list(self.tree.item(iid, "values"))
            if result.get("success"):
                succeeded += 1
                values[4] = "Written"
                self.tree.item(iid, values=values, tags=("done",))
            else:
                failed += 1
                error = result.get("error") or {}
                values[4] = error.get("message", "Failed") if isinstance(error, dict) else str(error)
                self.tree.item(iid, values=values, tags=("problem",))
        self.changes = []
        self.write_button.state(["disabled"])
        self.window.set_status(f"Wrote {succeeded} value(s)" + (f"; {failed} could not be written (see Result)." if failed else "."),
                               "warning" if failed else "ok")

    def run(self) -> None:
        self.window.run(on_start=self.load_properties)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    PropertyValueImporter().run()
