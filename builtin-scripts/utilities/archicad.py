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
    return bool(result and result.get("isAlive"))


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


_details_fields_supported = True


def get_details_of_elements(elements: list[ElementId], fields: list[str]) -> list[dict]:
    """ GetDetailsOfElements limited to the given fields, which skips the costly
    ones (floorPlanPolygons) the script does not need. Add-Ons before 1.5.9 do
    not know the fields parameter and reject the call; they get the plain one. """
    global _details_fields_supported
    if not elements:
        return []
    if _details_fields_supported:
        try:
            return run_tapir("GetDetailsOfElements", {"elements": elements, "fields": fields}).get("detailsOfElements", [])
        except ArchicadError:
            _details_fields_supported = False
    return run_tapir("GetDetailsOfElements", {"elements": elements}).get("detailsOfElements", [])
