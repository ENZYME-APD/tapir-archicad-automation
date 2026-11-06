# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

"""
Find and Replace in Texts

Searches the content of every Text and text Label (the selection, the current
story or the whole project), with match case and whole word options. Every
match is listed with its story and a preview of the changed line, with a
checkbox per match; only the checked matches are replaced. The replacement is
written with ChangeTextContent, so the styles of the rest of the content and
the autotexts in it stay as they are.

A match is never written inside an autotext, nor after one whose value differs
from its key, because it is not known whether Archicad counts the positions
there in the key or in the value. Such matches are listed but cannot be
checked; change them in Archicad.
"""

import re
import tkinter as tk
from tkinter import ttk

from utilities.archicad import (batched, get_details_of_elements, get_elements, get_story_names, item_errors,
                                run_tapir, select_elements)
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Find and Replace in Texts"
CHECKED, UNCHECKED, LOCKED = "☑", "☐", "–"
TEXT_TYPES = ["Text", "Label"]
LINE_ENDS = "\r\n"
PREVIEW_CONTEXT = 40  # characters shown on each side of a match
MAX_LISTED = 5000


def utf16_length(text: str) -> int:
    """ The length in UTF-16 code units: Archicad counts positions in its UTF-16 strings,
    Python in code points, which differ for characters outside the BMP (emoji). """
    return len(text.encode("utf-16-le")) // 2


def find_spans(text: str, term: str, match_case: bool, whole_word: bool) -> list[tuple[int, int]]:
    pattern = re.escape(term)
    if whole_word:
        pattern = rf"(?<!\w){pattern}(?!\w)"
    return [(m.start(), m.end()) for m in re.finditer(pattern, text, 0 if match_case else re.IGNORECASE)]


def protected_spans(runs: list[dict]) -> list[tuple[int, int]]:
    """ The autotext runs' spans in the content; the runs' texts concatenate to the content. """
    spans, position = [], 0
    for run in runs:
        length = len(run.get("text", ""))
        if run.get("effectProtected"):
            spans.append((position, position + length))
        position += length
    return spans


def common_prefix_length(a: str, b: str) -> int:
    length = 0
    for x, y in zip(a, b):
        if x != y:
            break
        length += 1
    return length


def line_around(text: str, start: int, end: int) -> tuple[int, int]:
    """ The start and end of the line that holds text[start:end]. """
    line_start = max(text.rfind(c, 0, start) for c in LINE_ENDS) + 1
    ends = [i for i in (text.find(c, end) for c in LINE_ENDS) if i >= 0]
    return line_start, min(ends) if ends else len(text)


def preview(text: str, start: int, end: int, middle: str) -> str:
    line_start, line_end = line_around(text, start, end)
    before = text[max(line_start, start - PREVIEW_CONTEXT):start]
    after = text[end:min(line_end, end + PREVIEW_CONTEXT)]
    if start - PREVIEW_CONTEXT > line_start:
        before = "…" + before
    if end + PREVIEW_CONTEXT < line_end:
        after += "…"
    return f"{before}[{middle}]{after}"


def unsafe_reason(text: str, raw_text: str | None, runs: list[dict], start: int, end: int) -> str | None:
    """ Why the match cannot be replaced by position, or None when it can. """
    autotexts = protected_spans(runs)
    if any(start < span_end and span_start < end for span_start, span_end in autotexts):
        return "Inside an autotext"
    if raw_text is not None:
        # With autotexts the content reads with their values, the raw content with their keys;
        # the position is the same in both only before the first autotext.
        if raw_text != text and common_prefix_length(text, raw_text) < end:
            return "After an autotext"
    elif autotexts and autotexts[0][0] < end:
        return "After an autotext"
    return None


class TextFindReplace:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Finds a text in the content of Texts and text Labels and replaces it in the checked matches. "
            "The styles and the autotexts of the content stay as they are. Nothing changes until you press Replace; "
            "each Replace can be undone in Archicad.",
            min_width=820, min_height=560)
        self.matches: list[dict] = []
        self.checked: dict[int, bool] = {}
        # the content each element had when it was searched, to refuse a replace after it changed
        self.searched_texts: dict[str, str] = {}
        self._build_body()
        self.window.add_button("Select in Archicad", self.select_in_archicad)
        self.replace_button = self.window.add_button("Replace checked", self.replace)
        self.replace_button.state(["disabled"])
        self.window.add_button("Find", self.find, primary=True)

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(2, weight=1)

        search = ttk.LabelFrame(body, text="Search", padding=(10, 6))
        search.grid(row=0, column=0, sticky="ew")
        search.columnconfigure(1, weight=1)
        self.find_text = tk.StringVar()
        self.replace_text = tk.StringVar()
        ttk.Label(search, text="Find:").grid(row=0, column=0, sticky="w")
        find_entry = ttk.Entry(search, textvariable=self.find_text)
        find_entry.grid(row=0, column=1, columnspan=3, sticky="ew", padx=(6, 0))
        ttk.Label(search, text="Replace with:").grid(row=1, column=0, sticky="w", pady=(6, 0))
        ttk.Entry(search, textvariable=self.replace_text).grid(row=1, column=1, columnspan=3, sticky="ew", padx=(6, 0), pady=(6, 0))
        self.replace_text.trace_add("write", lambda *_: self._refresh_previews())

        self.match_case = tk.BooleanVar(value=False)
        self.whole_word = tk.BooleanVar(value=False)
        ttk.Checkbutton(search, text="Match case", variable=self.match_case).grid(row=2, column=1, sticky="w", padx=(6, 0), pady=(6, 0))
        ttk.Checkbutton(search, text="Whole word", variable=self.whole_word).grid(row=2, column=2, sticky="w", padx=(10, 0), pady=(6, 0))

        scope = ttk.Frame(search)
        scope.grid(row=3, column=1, columnspan=3, sticky="w", padx=(6, 0), pady=(6, 0))
        self.scope = tk.StringVar(value="project")
        for column, (value, text) in enumerate([("selection", "Selection"), ("story", "Current story"),
                                                ("project", "All stories")]):
            ttk.Radiobutton(scope, text=text, value=value, variable=self.scope).grid(
                row=0, column=column, sticky="w", padx=(0 if column == 0 else 10, 0))
        find_entry.focus_set()

        self.summary = ttk.Label(body, text="", style="Muted.TLabel")
        self.summary.grid(row=1, column=0, sticky="w", pady=(8, 4))

        self.tree = make_tree(body, [
            ("check", "Replace", 64, "center"),
            ("story", "Story", 110, "w"),
            ("element", "Element", 120, "w"),
            ("before", "Found", 250, "w"),
            ("after", "After the replace", 250, "w"),
        ], height=14)
        for column in ("check", "story", "element"):
            self.tree.column(column, stretch=False)
        self.tree.column("after", stretch=True)
        self.tree.container.grid(row=2, column=0, sticky="nsew")
        self.tree.tag_configure("locked", foreground="#999999")
        self.tree.bind("<ButtonRelease-1>", self._on_click)
        self.tree.bind("<space>", lambda _e: self._toggle(self.tree.focus()))

        buttons = ttk.Frame(body, padding=(0, 8, 0, 0))
        buttons.grid(row=3, column=0, sticky="w")
        ttk.Button(buttons, text="Check all", command=lambda: self._set_all(True)).grid(row=0, column=0)
        ttk.Button(buttons, text="Uncheck all", command=lambda: self._set_all(False)).grid(row=0, column=1, padx=(6, 0))

    # ---- searching ----

    def _term(self) -> str | None:
        term = self.find_text.get()
        if not term:
            self.window.set_status("Type the text to find first.", "warning")
            return None
        if any(c in term for c in LINE_ENDS):
            self.window.set_status("The text to find must be on one line.", "warning")
            return None
        return term

    def find(self) -> None:
        term = self._term()
        if term is None:
            return
        with self.window.busy("Reading the texts and labels..."):
            elements = get_elements(self.scope.get(), TEXT_TYPES)
            details = get_details_of_elements(elements, ["type", "id", "floorIndex", "details"])
            stories = get_story_names()

        self.matches, self.checked, self.searched_texts = [], {}, {}
        searched = 0
        for element, detail in zip(elements, details):
            if not detail or "error" in detail:
                continue
            specific = detail.get("details", {})
            if detail.get("type") == "Label" and specific.get("labelClass") != "Text":
                continue
            text = specific.get("text", "")
            searched += 1
            spans = find_spans(text, term, self.match_case.get(), self.whole_word.get())
            if not spans:
                continue
            guid = element["elementId"]["guid"]
            self.searched_texts[guid] = text
            floor = detail.get("floorIndex")
            for start, end in spans:
                self.matches.append({
                    "elementId": {"guid": guid},
                    "text": text,
                    "start": start,
                    "end": end,
                    "story": stories.get(floor, "") if floor is not None else "",
                    "element": " ".join(part for part in (detail.get("type", ""), detail.get("id", "")) if part),
                    "locked": unsafe_reason(text, specific.get("rawText"), specific.get("runs", []), start, end),
                })

        found = len(self.matches)
        shown = self.matches[:MAX_LISTED]
        self.tree.delete(*self.tree.get_children())
        for index, match in enumerate(shown):
            self.checked[index] = match["locked"] is None
            self.tree.insert("", "end", iid=str(index), tags=("locked",) if match["locked"] else ())
        self.matches = shown
        self._refresh_previews()

        elements_with_matches = len({m["elementId"]["guid"] for m in self.matches})
        locked = sum(1 for m in self.matches if m["locked"])
        self.summary.configure(text=f"{searched} text(s) searched, {found} match(es) "
                                    + (f"(the first {MAX_LISTED} listed; search again after replacing them) " if found > len(self.matches) else "")
                                    + f"in {elements_with_matches} element(s)"
                                    + (f", {locked} inside or after an autotext (cannot be replaced here)" if locked else ""))
        if not self.matches:
            self.window.set_status(f"“{term}” was not found.", "info")
        else:
            self.window.set_status("Uncheck the matches to keep, then press Replace checked.", "ok")

    def _refresh_previews(self) -> None:
        new = self.replace_text.get()
        for index, match in enumerate(self.matches):
            text, start, end = match["text"], match["start"], match["end"]
            after = match["locked"] or preview(text, start, end, new)
            self.tree.item(str(index), values=(
                LOCKED if match["locked"] else (CHECKED if self.checked.get(index) else UNCHECKED),
                match["story"], match["element"], preview(text, start, end, text[start:end]), after))
        self._update_replace_button()

    # ---- checking ----

    def _on_click(self, event) -> None:
        if self.tree.identify_region(event.x, event.y) == "cell" and self.tree.identify_column(event.x) == "#1":
            self._toggle(self.tree.identify_row(event.y))

    def _toggle(self, row: str) -> None:
        if not row or self.matches[int(row)]["locked"]:
            return
        index = int(row)
        self.checked[index] = not self.checked.get(index)
        self.tree.set(row, "check", CHECKED if self.checked[index] else UNCHECKED)
        self._update_replace_button()

    def _set_all(self, state: bool) -> None:
        for index, match in enumerate(self.matches):
            if not match["locked"]:
                self.checked[index] = state
        self._refresh_previews()

    def _checked_matches(self) -> list[dict]:
        return [m for i, m in enumerate(self.matches) if self.checked.get(i) and not m["locked"]]

    def _update_replace_button(self) -> None:
        self.replace_button.state(["!disabled"] if self._checked_matches() else ["disabled"])

    # ---- actions ----

    def select_in_archicad(self) -> None:
        rows = self.tree.selection()
        matches = [self.matches[int(r)] for r in rows] if rows else self._checked_matches()
        guids = list(dict.fromkeys(m["elementId"]["guid"] for m in matches))
        if not guids:
            self.window.set_status("There is nothing to select.", "warning")
            return
        select_elements([{"elementId": {"guid": guid}} for guid in guids])
        self.window.set_status(f"Selected {len(guids)} element(s) in Archicad.", "ok")

    def replace(self) -> None:
        new = self.replace_text.get()
        if any(c in new for c in LINE_ENDS):
            self.window.set_status("The replacement must be on one line.", "warning")
            return
        matches = self._checked_matches()
        if not matches:
            self.window.set_status("Check the matches to replace first.", "warning")
            return
        guids = list(dict.fromkeys(m["elementId"]["guid"] for m in matches))
        if not self.window.ask_ok_cancel(SCRIPT_NAME, f"Replace {len(matches)} match(es) in {len(guids)} element(s) "
                                                      f"with “{new}”?\n\nYou can undo it in Archicad."):
            return

        with self.window.busy("Checking that the texts have not changed since the search..."):
            element_ids = [{"elementId": {"guid": guid}} for guid in guids]
            current = get_details_of_elements(element_ids, ["details"])
            changed = {guid for guid, detail in zip(guids, current)
                       if not detail or "error" in detail
                       or detail.get("details", {}).get("text") != self.searched_texts.get(guid)}

            # The changes of one element go from its end to its start, so each position is
            # still the searched one when Archicad applies it.
            order = {guid: index for index, guid in enumerate(guids)}
            changes = []
            for match in sorted((m for m in matches if m["elementId"]["guid"] not in changed),
                                key=lambda m: (order[m["elementId"]["guid"]], -m["start"])):
                text = match["text"]
                changes.append({
                    "elementId": match["elementId"],
                    "from": utf16_length(text[:match["start"]]),
                    "range": utf16_length(text[match["start"]:match["end"]]),
                    "content": new,
                })

            self.window.set_status(f"Replacing {len(changes)} match(es)...")
            errors = []
            for chunk in batched(changes):
                results = run_tapir("ChangeTextContent", {"contentChanges": chunk}).get("executionResults", [])
                errors += item_errors(results)

        replaced = len(changes) - len(errors)
        self.find()
        message = f"Replaced {replaced} of {len(matches)} match(es)."
        if changed:
            message += f" {len(changed)} element(s) changed since the search and were left alone."
        if errors:
            message += f" First error: {errors[0]}"
        self.window.set_status(message, "warning" if changed or errors else "ok")

    def run(self) -> None:
        self.window.run()


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    TextFindReplace().run()
