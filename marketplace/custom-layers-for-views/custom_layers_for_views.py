# /// script
# requires-python = ">=3.11"
# dependencies = [
#     "pyobjc-framework-ApplicationServices; sys_platform == 'darwin'",
#     "pyobjc-framework-Quartz; sys_platform == 'darwin'",
# ]
# ///

# /// tapir
# category = "navigator-views"
# archicad-versions = [29]
# min-tapir-version = "2.0.2"
# changes = "model"
# ///

"""
Custom Layers for Views

Stores custom layers in views of the View Map: each view's own layers as they are when it opens, with
the layers you pick shown, hidden, locked or unlocked. A view that uses a layer combination gets the
combination's states as its own custom layers.

The Archicad API cannot write a view's custom layers (see "What the Archicad API can't do" on
archicad-api.com), so the script does what you would do by hand, view by view: it opens the view, sets
the layers in the window, and presses View Settings > Get Current Window's Settings in the Navigator
through macOS Accessibility, without bringing Archicad to the front. That button stores every setting
of the window, so the script then writes the view's other settings (zoom, scale, pen set, ...) back and
reads the view again: a view that does not read back as asked is put back as it was and reported.

macOS only, and only where Archicad's interface is in English: the script finds the buttons by their
names (the constants below). Tested on Archicad 29 alone.
"""

import sys
import time
import tkinter as tk
import urllib.parse
from dataclasses import dataclass, field
from tkinter import messagebox, ttk

try:
    from utilities.archicad import ArchicadError, run_command, run_tapir
    from utilities.navigator_utils import VIEW_NAVITEM_TYPES
    from utilities.ui import ScriptWindow, make_tree, require_archicad
except ModuleNotFoundError as error:
    # The palette of a Tapir newer than 2.0.0 provides the helpers; an older one needs copies next to the script.
    if (error.name or "").split(".")[0] not in ("utilities", "aclib"):
        raise
    sys.exit("Error: the Tapir helpers utilities and aclib were not found. Update the Tapir Add-On, or copy the "
             "utilities and aclib folders of builtin-scripts "
             "(https://github.com/ENZYME-APD/tapir-archicad-automation/tree/main/builtin-scripts) next to this script.")

SCRIPT_NAME = "Custom Layers for Views"

# The names Archicad's English interface gives the controls the script presses. English is the only language
# the script was tried in.
NAVIGATOR_SETTINGS_BUTTON = "Settings…"
VIEW_SETTINGS_DIALOG = "View Settings"
GET_WINDOW_SETTINGS_BUTTON = "Get Current Window's Settings"
OK_BUTTON = "OK"

NULL_GUID = "00000000-0000-0000-0000-000000000000"
LAYER_FIELDS = ("isHidden", "isLocked", "isWireframe", "intersectionGroupNr")
# What is not written back with SetViewSettings after the redefine: the layers, which the redefine has just
# stored, and what the command does not take.
NOT_WRITTEN_BACK = ("layerCombination", "customLayers", "rotation", "transformationMatrix")
FOLDER_TYPES = ("FolderItem", "UndefinedItem")  # UndefinedItem: clone folders
ESCAPE_KEY = 53
# The changes a layer can be given, as (caption, the layer state fields it sets)
LAYER_CHANGES = [("Show", {"isHidden": False}), ("Hide", {"isHidden": True}),
                 ("Lock", {"isLocked": True}), ("Unlock", {"isLocked": False})]
UNTOUCHED, PUT_BACK, CHANGED = "untouched", "put back", "changed"

LayerStates = dict[str, dict]  # layer guid: {isHidden, isLocked, isWireframe, intersectionGroupNr}


class ViewFailed(Exception):
    """ A view could not be given its custom layers; differences says why, one text per reason. """

    def __init__(self, differences: list[str]):
        super().__init__("; ".join(differences))
        self.differences = differences
        # what the failure left of the view: UNTOUCHED, PUT_BACK or CHANGED (then still_different says what)
        self.view_is = UNTOUCHED
        self.still_different: list[str] = []


class DialogFailed(Exception):
    """ A step in Archicad's own windows did not work. """


@dataclass
class View:
    guid: str
    view_id: str
    name: str
    parent: str  # the guid of the folder that holds the view
    layers: str = ""  # "Custom", the layer combination's name, or "not stored"
    result: str = ""


@dataclass
class Layer:
    guid: str
    name: str
    change: dict = field(default_factory=dict)  # the state fields to set, e.g. {"isHidden": False}


# ---- reading the project ----

def read_view_map() -> tuple[list[View], list[tuple[str, str, str]]]:
    """ The views of the View Map and its folders as (guid, name, parent guid), both in the map's order. """
    tree = run_command("API.GetNavigatorItemTree", {"navigatorTreeId": {"type": "ViewMap"}})
    views: list[View] = []
    folders: list[tuple[str, str, str]] = []

    def walk(item: dict) -> None:
        parent = item["navigatorItemId"]["guid"]
        for wrapper in item.get("children", []):
            child = wrapper["navigatorItem"]
            guid = child["navigatorItemId"]["guid"]
            if child["type"] in VIEW_NAVITEM_TYPES:
                views.append(View(guid=guid, view_id=child.get("prefix", ""), name=child.get("name", ""),
                                  parent=parent))
            elif child["type"] in FOLDER_TYPES:
                folders.append((guid, child.get("name", "") or "(unnamed)", parent))
            walk(child)

    walk(tree["navigatorTree"]["rootItem"])
    return views, folders


def view_settings(view_guids: list[str]) -> list[dict]:
    if not view_guids:
        return []
    answer = run_tapir("GetViewSettings", {"navigatorItemIds": [{"navigatorItemId": {"guid": g}} for g in view_guids]})
    return answer["viewSettings"]


def settings_of(view_guid: str) -> dict:
    settings = view_settings([view_guid])[0]
    if "error" in settings:
        raise ViewFailed([f"GetViewSettings: {settings['error'].get('message', settings['error'])}"])
    return settings


def layers_caption(settings: dict) -> str:
    """ How a view holds its layers, for the table. """
    if "error" in settings:
        return "?"
    if not settings.get("saveFlags", {}).get("layers", True):
        return "not stored"
    return settings.get("layerCombination") or "Custom"


def design_option_combination(view_guid: str) -> dict:
    """ The view's design option combination (status and name); {} where Archicad has none (before Archicad 29). """
    try:
        answer = run_tapir("GetDesignOptionCombinationOfViews",
                           {"navigatorItemIds": [{"navigatorItemId": {"guid": view_guid}}]})
    except ArchicadError:
        return {}
    combination = answer.get("designOptionCombinationOfViews", [{}])[0]
    return {key: value for key, value in combination.items() if key != "navigatorItemId"}


def read_layers() -> list[Layer]:
    attributes = run_tapir("GetAttributesByType", {"attributeType": "Layer"})["attributes"]
    return [Layer(guid=a["attributeId"]["guid"], name=a["name"]) for a in attributes]


def window_layers(layers: list[Layer]) -> LayerStates:
    """ The state of every layer in the active window. """
    answer = run_command("API.GetLayerAttributes",
                         {"attributeIds": [{"attributeId": {"guid": layer.guid}} for layer in layers]})
    return {layer.guid: {f: item["layerAttribute"][f] for f in LAYER_FIELDS}
            for layer, item in zip(layers, answer["attributes"]) if "layerAttribute" in item}


# ---- what a view stores, and what differs ----

def stored_layers(settings: dict) -> LayerStates | None:
    """ The custom layers a view stores, without the layers deleted since (null guid); None for a view that
    uses a layer combination or stores no layers. """
    if settings.get("layerCombination") or not settings.get("saveFlags", {}).get("layers"):
        return None
    if "customLayers" not in settings:
        return None
    return {state["attributeId"]["guid"]: {f: state[f] for f in LAYER_FIELDS}
            for state in settings["customLayers"] if state["attributeId"]["guid"] != NULL_GUID}


def differences(was, now, path: str = "") -> list[str]:
    """ Where now differs from was, as "setting: was became now" texts; numbers within a millionth are equal. """
    if isinstance(was, dict) and isinstance(now, dict):
        return [d for key in sorted(was.keys() | now.keys())
                for d in differences(was.get(key), now.get(key), f"{path}.{key}")]
    if isinstance(was, list) and isinstance(now, list) and len(was) == len(now):
        return [d for index, (w, n) in enumerate(zip(was, now)) for d in differences(w, n, f"{path}[{index}]")]
    numbers = all(isinstance(value, (int, float)) and not isinstance(value, bool) for value in (was, now))
    if numbers and abs(was - now) <= 1e-6 * max(1.0, abs(was), abs(now)):
        return []
    return [] if was == now else [f"{path.lstrip('.')}: {was!r} became {now!r}"]


def without_layers(settings: dict) -> dict:
    kept = {key: value for key, value in settings.items() if key not in ("layerCombination", "customLayers")}
    kept["saveFlags"] = {key: value for key, value in settings.get("saveFlags", {}).items() if key != "layers"}
    return kept


def changes_to_make(states: LayerStates, wanted: dict[str, dict]) -> dict[str, dict]:
    """ The wanted layer changes that differ from the window's states, as {layer guid: {field: value}}. """
    return {guid: change for guid, change in wanted.items()
            if guid in states and any(states[guid][f] != value for f, value in change.items())}


def layer_changes_text(was: LayerStates, now: LayerStates, names: dict[str, str]) -> str:
    """ What changed in a view's layers, in words, e.g. "Notes: shown; Walls: locked". """
    words = {("isHidden", False): "shown", ("isHidden", True): "hidden",
             ("isLocked", True): "locked", ("isLocked", False): "unlocked"}
    texts = []
    for guid, state in now.items():
        changed = [words[(f, state[f])] for f in ("isHidden", "isLocked") if was.get(guid, {}).get(f) != state[f]]
        if changed and guid in was:
            texts.append(f"{names.get(guid, guid)}: {', '.join(changed)}")
        elif guid not in was:
            texts.append(f"{names.get(guid, guid)}: {'hidden' if state['isHidden'] else 'shown'} (new in the view)")
    return "; ".join(texts)


# ---- Archicad's own windows, through macOS Accessibility ----

class ArchicadWindows:
    """ The Navigator and the View Settings dialog of the Archicad the project is open in. Archicad is not
    brought to the front, and the pointer and the keyboard are not used. """

    def __init__(self, project_path: str):
        import AppKit
        import ApplicationServices as ax
        import Quartz
        self.ax, self.quartz = ax, Quartz
        if not ax.AXIsProcessTrusted():
            raise DialogFailed("This needs Accessibility: in System Settings > Privacy & Security > Accessibility, "
                               "allow the app the script runs in (Archicad, or your terminal).")
        found = []
        for app in AppKit.NSWorkspace.sharedWorkspace().runningApplications():
            if "archicad" not in (app.localizedName() or "").lower():
                continue
            element = ax.AXUIElementCreateApplication(app.processIdentifier())
            ax.AXUIElementSetMessagingTimeout(element, 3.0)
            documents = [self.get(w, "AXDocument") for w in self.get(element, "AXWindows") or []]
            paths = [urllib.parse.unquote(urllib.parse.urlparse(str(d)).path) for d in documents if d]
            if project_path in paths:
                found.append((app.processIdentifier(), element))
        if len(found) != 1:
            raise DialogFailed("Could not find the Archicad window of this project. Save the project first, and "
                               "close a second Archicad that has the same file open.")
        self.pid, self.app = found[0]

    def get(self, element, attribute):
        error, value = self.ax.AXUIElementCopyAttributeValue(element, attribute, None)
        return value if error == 0 else None

    def children(self, element) -> list:
        return list(self.get(element, "AXChildren") or [])

    def texts(self, element) -> list[str]:
        values = (self.get(element, a) for a in ("AXTitle", "AXDescription", "AXValue"))
        return [str(v).strip() for v in values if isinstance(v, str)]

    def walk(self, element, depth: int = 12):
        yield element
        if depth:
            for child in self.children(element):
                yield from self.walk(child, depth - 1)

    def windows(self) -> list:
        return list(self.get(self.app, "AXWindows") or [])

    def project_window(self):
        """ The project's own window, the one with a document. Its subrole reads AXDialog for some seconds after
        a view is opened through the API, so the subrole cannot tell it from a dialog. """
        for window in self.windows():
            if self.get(window, "AXDocument"):
                return window
        raise DialogFailed("Archicad's project window was not found.")

    def dialogs(self) -> list:
        return [w for w in self.windows() if not self.get(w, "AXDocument")
                and (self.get(w, "AXModal") or self.get(w, "AXSubrole") == "AXDialog")]

    def dialog_titles(self) -> list[str]:
        return [str(self.get(w, "AXTitle") or "") for w in self.dialogs()]

    def find(self, root, role: str, text: str):
        for element in self.walk(root):
            if self.get(element, "AXRole") == role and text in self.texts(element):
                return element
        raise DialogFailed(f"\"{text}\" was not found in \"{self.get(root, 'AXTitle')}\". The script finds "
                           "Archicad's buttons by their English names.")

    def press(self, element) -> None:
        # a press that opens a dialog can answer an error and still have worked
        self.ax.AXUIElementPerformAction(element, "AXPress")

    def wait_for_dialog(self, title: str, seconds: float = 8.0):
        end = time.time() + seconds
        while time.time() < end:
            for window in self.windows():
                if self.get(window, "AXTitle") == title:
                    return window
            time.sleep(0.25)
        raise DialogFailed(f"The \"{title}\" dialog did not open.")

    def close_dialogs(self) -> None:
        """ Escape, sent to Archicad alone, until no dialog is left. """
        for _ in range(4):
            if not self.dialogs():
                return
            for down in (True, False):
                event = self.quartz.CGEventCreateKeyboardEvent(None, ESCAPE_KEY, down)
                self.quartz.CGEventPostToPid(self.pid, event)
                time.sleep(0.05)
            time.sleep(0.5)
        raise DialogFailed(f"A dialog stayed open in Archicad: {', '.join(self.dialog_titles())}. Close it there.")

    def navigator(self):
        """ The Navigator docked in the project window, and its four map buttons. """
        for element in self.walk(self.project_window(), 3):
            kids = self.children(element)
            switches = [c for c in kids if self.get(c, "AXSubrole") == "AXToggle"]
            if len(switches) == 4 and any(self.get(c, "AXRole") == "AXGroup" for c in kids):
                return element, switches
        raise DialogFailed("The Navigator was not found in Archicad's project window. Show it (Window > Palettes > "
                           "Navigator), docked in the window, with its Properties panel open.")

    def redefine_open_view(self, view: View) -> None:
        """ View Settings > Get Current Window's Settings > OK, for the view just opened, which the Navigator's
        Properties must show. """
        if self.dialogs():
            raise DialogFailed(f"A dialog is open in Archicad: {', '.join(self.dialog_titles())}. Close it first.")
        try:
            navigator, switches = self.navigator()
            if not self.get(switches[1], "AXValue"):  # Project Map, View Map, Layout Book, Publisher Sets
                self.press(switches[1])
                time.sleep(1.0)
            settings = self.find(navigator, "AXButton", NAVIGATOR_SETTINGS_BUTTON)
            expected = [text for text in (view.view_id, view.name) if text]
            for attempt in range(4):
                shown = [text for c in self.children(self.get(settings, "AXParent"))
                         if self.get(c, "AXRole") == "AXTextField" for text in self.texts(c)]
                if all(text in shown for text in expected):
                    break
                if attempt == 3:
                    raise DialogFailed(f"The Navigator's Properties show {shown}, not the view "
                                       f"\"{' '.join(expected)}\". Open the Properties panel of the Navigator.")
                time.sleep(1.5)
            self.press(settings)
            dialog = self.wait_for_dialog(VIEW_SETTINGS_DIALOG)
            self.press(self.find(dialog, "AXButton", GET_WINDOW_SETTINGS_BUTTON))
            time.sleep(1.0)
            self.press(self.find(dialog, "AXButton", OK_BUTTON))
            time.sleep(1.0)
            if self.dialogs():
                raise DialogFailed(f"Left open in Archicad: {', '.join(self.dialog_titles())}.")
        except DialogFailed:
            try:
                self.close_dialogs()
            except DialogFailed as stuck:
                raise DialogFailed(str(stuck)) from None
            raise


# ---- giving one view its custom layers ----

def open_view(view: View) -> None:
    answer = run_tapir("ChangeWindow", {"navigatorItemId": {"guid": view.guid}})
    if not answer.get("success"):
        raise ViewFailed([f"The view could not be opened: {answer.get('error', {}).get('message', answer)}"])


def set_window_layers(layers: list[Layer], changes: dict[str, dict]) -> LayerStates:
    """ Sets the changes in the window's layers; returns the window's layers read back. """
    names = {layer.guid: layer.name for layer in layers}
    run_tapir("CreateLayers", {"overwriteExisting": True, "layerDataArray": [
        {"attributeId": {"guid": guid}, "name": names[guid], **change} for guid, change in changes.items()]})
    states = window_layers(layers)
    unset = [names[guid] for guid, change in changes.items() if any(states[guid][f] != v for f, v in change.items())]
    if unset:
        raise ViewFailed([f"The window's layers did not take the change: {', '.join(unset)}"])
    return states


def write_settings(view: View, settings: dict) -> list[str]:
    """ SetViewSettings; returns its failure as a list of texts, empty when it succeeded. """
    result = run_tapir("SetViewSettings", {"navigatorItemIdsWithViewSettings": [
        {"navigatorItemId": {"guid": view.guid}, "viewSettings": settings}]})["executionResults"][0]
    return [] if result.get("success") else [f"SetViewSettings: {result.get('error', {}).get('message', result)}"]


def redefine(windows: ArchicadWindows, view: View) -> None:
    try:
        windows.redefine_open_view(view)
    except DialogFailed as e:
        raise ViewFailed([str(e)]) from e


def put_back(windows: ArchicadWindows, view: View, layers: list[Layer], before: dict, before_options: dict,
             opened_with: LayerStates) -> list[str]:
    """ Gives a view that a redefine has overwritten its settings from before again; returns what still differs,
    empty when the view is as it was. A layer combination, or storing no layers, is written back with
    SetViewSettings. Custom layers take a second redefine, from the window's layers as they were when the view
    first opened: the view then also stores the layers made since it was saved, in the state they opened in. """
    was = stored_layers(before)
    back = {key: value for key, value in before.items() if key not in NOT_WRITTEN_BACK}
    try:
        if was is None:
            if before.get("saveFlags", {}).get("layers"):
                back["layerCombination"] = before["layerCombination"]
            failed = write_settings(view, back)
        else:
            open_view(view)
            current = window_layers(layers)
            changes = {guid: {f: v for f, v in state.items() if current[guid][f] != v}
                       for guid, state in opened_with.items() if guid in current and current[guid] != state}
            if changes:
                set_window_layers(layers, changes)
                redefine(windows, view)
            failed = write_settings(view, back)
        after = settings_of(view.guid)
        if was is None:
            failed += differences({"layers": layers_caption(before)}, {"layers": layers_caption(after)})
        else:
            now_stored = stored_layers(after) or {}
            failed += differences(was, {guid: now_stored.get(guid) for guid in was}, "customLayers")
        failed += differences(without_layers(before), without_layers(after))
        failed += differences(before_options, design_option_combination(view.guid), "designOptionCombination")
    except (ViewFailed, ArchicadError) as e:
        return [f"Putting the view back failed: {e}"]
    return failed


def store_custom_layers(windows: ArchicadWindows, view: View, layers: list[Layer], wanted: dict[str, dict]) -> str:
    """ Stores custom layers in the view: its layers as they are when it opens, with the wanted changes
    ({layer guid: {field: value}}). Returns what changed in the view's layers, in words. Raises ViewFailed, whose
    view_is says what the failure left of the view. """
    before, before_options = settings_of(view.guid), design_option_combination(view.guid)
    opened_with: LayerStates | None = None  # set once the redefine is about to overwrite the view
    try:
        open_view(view)
        states = window_layers(layers)
        as_opened = states
        changes = changes_to_make(states, wanted)
        if not changes and stored_layers(before) is None:
            # Archicad stores an untouched window's layers under its layer combination's name: writing one
            # layer's own state again makes them custom.
            layer = next(layer for layer in layers if layer.guid in states and layer.name.isprintable())
            changes = {layer.guid: {"isHidden": states[layer.guid]["isHidden"]}}
        if changes:
            states = set_window_layers(layers, changes)

        opened_with = as_opened
        redefine(windows, view)

        back = {key: value for key, value in before.items() if key not in NOT_WRITTEN_BACK}
        back["saveFlags"] = {**before.get("saveFlags", {}), "layers": True}
        failed = write_settings(view, back)

        after = settings_of(view.guid)
        stored = stored_layers(after)
        if stored is None:
            failed.append(f"The view does not store custom layers (it reads \"{layers_caption(after)}\").")
        else:
            failed += differences(states, stored, "layers (window, then view)")
        failed += differences(without_layers(before), without_layers(after))
        failed += differences(before_options, design_option_combination(view.guid), "designOptionCombination")
        if failed:
            raise ViewFailed(failed)
    except (ViewFailed, ArchicadError) as e:
        failure = e if isinstance(e, ViewFailed) else ViewFailed([str(e)])
        if opened_with is not None:
            failure.still_different = put_back(windows, view, layers, before, before_options, opened_with)
            failure.view_is = CHANGED if failure.still_different else PUT_BACK
        raise failure from e
    names = {layer.guid: layer.name for layer in layers}
    return layer_changes_text(stored_layers(before) or as_opened, stored, names)


# ---- the window ----

class CustomLayersForViews:
    """ The window: pick the views and what their layers should do, then store. """

    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "macOS and English Archicad only. Stores custom layers in the views you select: each view's own layers, with the layers below shown, "
            "hidden, locked or unlocked. A view that uses a layer combination gets the combination's states as "
            "custom layers. The script opens each view in Archicad and presses View Settings > Get Current "
            "Window's Settings for it, then puts the view's zoom and other settings back and checks the result. "
            "Leave Archicad alone while it runs. Nothing changes until you confirm.",
            min_width=780, min_height=640)
        self.window.root.geometry("860x700")  # the description is one long line until the window wraps it
        self.views: list[View] = []
        self.layers: list[Layer] = []
        self._build_body()
        self.window.add_button("Read again", self.read)
        self.store_button = self.window.add_button("Store custom layers...", self.store, primary=True)
        self.store_button.state(["disabled"])

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(1, weight=3)
        body.rowconfigure(3, weight=2)
        ttk.Label(body, text="Views (select the ones to give custom layers):").grid(row=0, column=0, sticky="w")
        self.view_tree = make_tree(body, [("id", "ID", 90, "w"), ("layers", "Layers", 200, "w"),
                                          ("result", "Result", 300, "w")], height=10, show_tree=True)
        self.view_tree.heading("#0", text="View", anchor="w")
        self.view_tree.column("#0", width=260, stretch=True)
        self.view_tree.container.grid(row=1, column=0, sticky="nsew", pady=(4, 10))
        self.view_tree.bind("<<TreeviewSelect>>", lambda _e: self._update_button())

        ttk.Label(body, text="Layers (select layers, then press what the views should do with them; "
                             "the other layers stay as each view has them):").grid(row=2, column=0, sticky="w")
        self.layer_tree = make_tree(body, [("name", "Layer", 380, "w"), ("change", "In the selected views", 200, "w")],
                                    height=8)
        self.layer_tree.container.grid(row=3, column=0, sticky="nsew", pady=(4, 6))
        buttons = ttk.Frame(body)
        buttons.grid(row=4, column=0, sticky="w")
        for caption, change in LAYER_CHANGES:
            ttk.Button(buttons, text=caption, command=lambda c=change: self._change_layers(c)).pack(
                side="left", padx=(0, 6))
        ttk.Button(buttons, text="No change", command=lambda: self._change_layers(None)).pack(side="left")

    # ---- reading ----

    def read(self) -> None:
        with self.window.busy("Reading the views and layers..."):
            self.views, folders = read_view_map()
            for view, settings in zip(self.views, view_settings([view.guid for view in self.views])):
                view.layers = layers_caption(settings)
            changes = {layer.guid: layer.change for layer in self.layers}
            self.layers = read_layers()
            for layer in self.layers:
                layer.change = changes.get(layer.guid, {})
        self.view_tree.delete(*self.view_tree.get_children())
        for guid, name, parent in folders:
            self.view_tree.insert(parent if self.view_tree.exists(parent) else "", "end", iid=guid, text=name,
                                  open=True, tags=("folder",))
        for view in self.views:
            self.view_tree.insert(view.parent if self.view_tree.exists(view.parent) else "", "end", iid=view.guid,
                                  text=view.name, values=(view.view_id, view.layers, view.result))
        self._fill_layers()
        self.window.set_status(f"{len(self.views)} views, {len(self.layers)} layers.")
        self._update_button()

    def _fill_layers(self) -> None:
        selected = self.layer_tree.selection()
        self.layer_tree.delete(*self.layer_tree.get_children())
        for layer in self.layers:
            if layer.name.isprintable():  # not the Archicad Layer, whose name is a control character
                self.layer_tree.insert("", "end", iid=layer.guid, values=(layer.name, change_caption(layer.change)))
        self.layer_tree.selection_set([guid for guid in selected if self.layer_tree.exists(guid)])

    def _change_layers(self, change: dict | None) -> None:
        selected = set(self.layer_tree.selection())
        for layer in self.layers:
            if layer.guid in selected:
                layer.change = {} if change is None else {**layer.change, **change}
        self._fill_layers()

    def selected_views(self) -> list[View]:
        """ The selected views; a selected folder selects the views in it. """
        chosen: set[str] = set()

        def add(item: str) -> None:
            chosen.add(item)
            for child in self.view_tree.get_children(item):
                add(child)

        for item in self.view_tree.selection():
            add(item)
        return [view for view in self.views if view.guid in chosen]

    def _update_button(self) -> None:
        self.store_button.state(["!disabled"] if self.selected_views() else ["disabled"])

    # ---- storing ----

    def store(self) -> None:
        views = self.selected_views()
        if not views:
            self.window.set_status("Select at least one view.", "warning")
            return
        wanted = {layer.guid: layer.change for layer in self.layers if layer.change}
        named = [f"  {layer.name}: {change_caption(layer.change).lower()}" for layer in self.layers if layer.change]
        message = (f"Store custom layers in {len(views)} view(s)?\n\n"
                   + ("Layers changed in each view:\n" + "\n".join(named[:12])
                      + (f"\n  ... and {len(named) - 12} more" if len(named) > 12 else "")
                      if named else "No layer is changed: each view keeps the layers it shows now, as custom layers.")
                   + "\n\nEach view is opened in Archicad in turn, which replaces the layers and zoom of the "
                     "window you have open there, and Archicad is left on the last view. Leave Archicad alone "
                     "until the script is done. This cannot be undone with Undo.")
        if not self.window.ask_ok_cancel(SCRIPT_NAME, message):
            return
        project = run_tapir("GetProjectInfo")
        try:
            windows = ArchicadWindows(project.get("projectPath", ""))
        except DialogFailed as e:
            self.window.set_status(str(e), "error")
            return
        done = 0
        with self.window.busy("Storing custom layers..."):
            for index, view in enumerate(views, start=1):
                self.window.set_status(f"View {index} of {len(views)}: {view.view_id} {view.name}".strip())
                try:
                    changed = store_custom_layers(windows, view, self.layers, wanted)
                    view.result, view.layers = f"Stored. {changed}".strip(), "Custom"
                    done += 1
                except ViewFailed as e:
                    left = {UNTOUCHED: "The view is unchanged.", PUT_BACK: "The view was put back as it was.",
                            CHANGED: "THE VIEW IS CHANGED: " + "; ".join(e.still_different)}[e.view_is]
                    view.result = f"Failed: {'; '.join(e.differences)} {left}"
                    print(f"{view.view_id} {view.name}: {view.result}")
                    self._show(view)
                    break
                self._show(view)
        failed = len(views) - done
        if failed:
            self.window.set_status(f"{done} view(s) stored. Stopped at a view that failed: see its Result, and "
                                   f"the palette's output.", "error")
        else:
            self.window.set_status(f"Custom layers stored in {done} view(s).", "ok")

    def _show(self, view: View) -> None:
        self.view_tree.item(view.guid, values=(view.view_id, view.layers, view.result))
        self.window.root.update()

    def run(self) -> None:
        self.window.run(on_start=self.read)


def change_caption(change: dict) -> str:
    """ A layer's change in words, e.g. "Show, lock"; "" for no change. """
    parts = [caption for caption, fields in LAYER_CHANGES if all(change.get(f) == v for f, v in fields.items())]
    return ", ".join(parts).capitalize() if parts else ""


def main() -> None:
    require_archicad(SCRIPT_NAME)
    if sys.platform != "darwin":
        root = tk.Tk()
        root.withdraw()
        messagebox.showerror(SCRIPT_NAME, "This script works on macOS only: it presses View Settings > Get Current "
                                          "Window's Settings through macOS Accessibility, because the Archicad API "
                                          "cannot write a view's custom layers.")
        root.destroy()
        sys.exit(1)
    CustomLayersForViews().run()


if __name__ == "__main__":
    main()
