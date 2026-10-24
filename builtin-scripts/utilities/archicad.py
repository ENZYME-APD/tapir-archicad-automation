""" Calls into the Archicad instance the Tapir palette started the script for.

The palette passes --port (and --host) on the command line; aclib reads them,
so every call here goes to that same Archicad, even with several running.
Unlike the aclib functions, these raise ArchicadError instead of printing and
returning None, so a script can show one clear message in its window.
"""

import urllib.error

import aclib

Guid = dict  # {"guid": "..."}
ElementId = dict  # {"elementId": {"guid": "..."}}


class ArchicadError(Exception):
    pass


def _error_text(error) -> str:
    if isinstance(error, dict):
        message = error.get("message") or "Unknown error"
        code = error.get("code")
        return f"{message} (code {code})" if code is not None else message
    return str(error)


def is_alive() -> bool:
    try:
        result = aclib.RunCommand("API.IsAlive", {})
    except (urllib.error.URLError, ConnectionError, OSError):
        return False
    # a succeeded answer means Archicad is there; only an explicit false says otherwise
    return result is not None and result.get("isAlive", True) is not False


def run_command(command: str, parameters: dict | None = None) -> dict:
    """ Runs an official Archicad JSON command ("API." is added when missing). """
    if not command.startswith("API."):
        command = "API." + command
    try:
        result = aclib.RunCommand(command, parameters or {})
    except (urllib.error.URLError, ConnectionError, OSError) as e:
        raise ArchicadError(f"Cannot reach Archicad on port {aclib.port}: {e}") from e
    if result is None:
        raise ArchicadError(f"{command} failed. See the palette output for the details.")
    return result


def run_tapir(command: str, parameters: dict | None = None) -> dict:
    """ Runs a Tapir Add-On command. """
    result = run_command("API.ExecuteAddOnCommand", {
        "addOnCommandId": {"commandNamespace": "TapirCommand", "commandName": command},
        "addOnCommandParameters": parameters or {},
    })
    response = result.get("addOnCommandResponse")
    if response is None:
        raise ArchicadError(f"{command} returned no response.")
    if "error" in response:
        raise ArchicadError(f"{command}: {_error_text(response['error'])}")
    return response


def item_errors(items: list[dict]) -> list[str]:
    """ The error messages of the failed items of a per-item command result. """
    return [_error_text(item["error"]) for item in items if isinstance(item, dict) and "error" in item]


def select_elements(element_ids: list[ElementId]) -> None:
    """ Makes the given elements the selection in Archicad (the rest is deselected). """
    current = run_tapir("GetSelectedElements").get("elements", [])
    wanted = {e["elementId"]["guid"] for e in element_ids}
    run_tapir("ChangeSelectionOfElements", {
        "removeElementsFromSelection": [e for e in current if e["elementId"]["guid"] not in wanted],
        "addElementsToSelection": element_ids,
    })


BATCH_SIZE = 1000


def batched(items: list, size: int = BATCH_SIZE):
    """ The items in slices of size, so one call does not get too large on big projects. """
    for start in range(0, len(items), size):
        yield items[start:start + size]


_details_fields_supported = True


def get_details_of_elements(elements: list[ElementId], fields: list[str]) -> list[dict]:
    """ GetDetailsOfElements limited to the given fields, which skips the costly
    ones (floorPlanPolygons) the script does not need. Add-Ons before 1.5.9 do
    not know the fields parameter and reject the call; they get the plain one. """
    global _details_fields_supported
    details = []
    for chunk in batched(elements):
        if _details_fields_supported:
            try:
                details += run_tapir("GetDetailsOfElements", {"elements": chunk, "fields": fields}).get("detailsOfElements", [])
                continue
            except ArchicadError:
                _details_fields_supported = False
        details += run_tapir("GetDetailsOfElements", {"elements": chunk}).get("detailsOfElements", [])
    return details


def get_story_names() -> dict[int, str]:
    """ The story names by floor index. """
    stories = run_tapir("GetStories").get("stories", [])
    return {s["index"]: s.get("name") or f"Story {s['index']}" for s in stories}


def get_elements(scope: str, element_types: list[str] | None = None, filters: list[str] | None = None) -> list[ElementId]:
    """ The elements of the given types (all types when None) in the scope:
    "selection", "story" (the current story) or "project". The filters are not
    applied to the selection. A type the Add-On does not know is skipped. """
    if scope == "selection":
        elements = run_tapir("GetSelectedElements").get("elements", [])
        if element_types is None:
            return elements
        details = get_details_of_elements(elements, ["type"])
        return [e for e, d in zip(elements, details) if d.get("type") in element_types]

    filters = list(filters or [])
    if scope == "story":
        filters.append("OnActualFloor")
    parameters = {"filters": filters} if filters else {}
    if element_types is None:
        return run_tapir("GetAllElements", parameters).get("elements", [])
    elements = []
    for element_type in element_types:
        try:
            elements += run_tapir("GetElementsByType", {"elementType": element_type, **parameters}).get("elements", [])
        except ArchicadError as e:
            print(f"Skipping {element_type}: {e}")
    return elements
