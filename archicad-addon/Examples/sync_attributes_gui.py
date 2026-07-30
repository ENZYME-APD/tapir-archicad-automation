"""
sync_attributes_gui.py - Compare and sync Tapir attributes between two open Archicad instances.

Scans a range of local ports for running Archicad instances (each Tapir add-on listens on its
own port, 19723 + N), lets the user pick two of them, fetches all attributes of the 11 types
covered by this session's exhaustive test suite (Layer, LayerCombination, Line, Fill,
ZoneCategory, MEPSystem, PenTable, Surface, Composite, BuildingMaterial, Profile), and shows a
tkinter GUI where the user picks a sync direction (A->B / B<-A / skip) per difference before
applying.

Design notes (see project memory for the full reasoning):
- Attributes are matched by NAME across the two projects, never by guid/index - those are
  project-local and never comparable across separate files.
- Fields that reference another attribute (e.g. Surface.fillId, BuildingMaterial.cutFillIndex)
  are resolved to the REFERENCED ATTRIBUTE'S NAME for comparison, then re-resolved to the
  target project's own attributeId/index when applying a sync. A reference that doesn't resolve
  in the target project (no attribute with that name) is flagged and skipped, never
  auto-created - cascading dependency creation is out of scope for v1.
- Profile and LayerCombination are structurally awkward to diff field-by-field across projects
  (Profile skins carry an internal skinId that's never stable across separate copies;
  LayerCombination always lists the status of every project layer, so two different projects
  produce enormous unrelated noise). Both are shown as "identical" / "different" only for their
  complex part (Profile's skins, LayerCombination's layers) - not offered for field-level sync -
  while Profile's scalar fields (wallType, width, height, ...) still get full field-level sync.
"""

import base64
import json
import tkinter as tk
import urllib.error
import urllib.request
from tkinter import messagebox, ttk

PORT_RANGE = range(19723, 19733)
HOST = "127.0.0.1"
PROBE_TIMEOUT = 3       # fast, used only for port discovery - a dead port should fail quickly
COMMAND_TIMEOUT = 30    # generous, used for actual Get/Create/Delete calls - a complex Profile
                        # geometry change can genuinely take a while (Archicad has to rebuild every
                        # element using that Profile), and a hard 5s cutoff was firing on those.


class ArchicadCommError(Exception):
    """Raised when a request to an Archicad instance fails at the network/transport level (timeout,
    connection reset, malformed response) - distinct from a well-formed JSON-RPC error response,
    which callers handle by inspecting the returned dict's "error" key instead."""

# ---------------------------------------------------------------------------------------------
# Attribute type definitions
# ---------------------------------------------------------------------------------------------
# refs: field -> ("id" | "index", targetType) for fields that reference another attribute.
# opaque_fields: fields compared only as "identical"/"different" (never offered for field-level
# sync) because their internal identifiers (skinId, per-layer guids) are never stable across
# separate project files.
ATTRIBUTE_TYPES = {
    "Layer": {
        "get_command": "GetLayers", "response_key": "layers",
        "create_command": "CreateLayers", "array_field": "layerDataArray",
        "refs": {}, "opaque_fields": [],
        # isHidden/isLocked/isWireframe reflect the CURRENTLY ACTIVE layer combination's view state,
        # not a durable property of the layer itself - two projects can have the same layers defined
        # but a different combination active, which would otherwise show up as a false-positive diff.
        # Excluded from both diff and sync; LayerCombination is the type that actually owns per-layer
        # visibility/lock state per combination.
        "volatile_fields": ["isHidden", "isLocked", "isWireframe"],
    },
    "Line": {
        "get_command": "GetLines", "response_key": "lines",
        "create_command": "CreateLines", "array_field": "lineDataArray",
        "refs": {}, "opaque_fields": [],
    },
    "Fill": {
        "get_command": "GetFills", "response_key": "fills",
        "create_command": "CreateFills", "array_field": "fillDataArray",
        "refs": {}, "opaque_fields": [],
        "texture_fields": ["texture"],
    },
    "ZoneCategory": {
        "get_command": "GetZoneCategories", "response_key": "zoneCategories",
        "create_command": "CreateZoneCategories", "array_field": "zoneCategoryDataArray",
        "refs": {}, "opaque_fields": [],
    },
    "MEPSystem": {
        "get_command": "GetMEPSystems", "response_key": "mepSystems",
        "create_command": "CreateMEPSystems", "array_field": "mepSystemDataArray",
        "refs": {"fillId": ("id", "Fill"), "centerLineTypeId": ("id", "Line")},
        "opaque_fields": [],
        "domain_list_to_scalar": True,  # Get returns ["Piping"], Create wants "Piping"
    },
    "PenTable": {
        "get_command": "GetPenTables", "response_key": "penTables",
        "create_command": "CreatePenTables", "array_field": "penTableDataArray",
        "refs": {}, "opaque_fields": [],
        "pens_field": "pens",  # summarized specially (255 entries)
    },
    "Surface": {
        "get_command": "GetSurfaces", "response_key": "surfaces",
        "create_command": "CreateSurfaces", "array_field": "surfaceDataArray",
        "refs": {"fillId": ("id", "Fill")}, "opaque_fields": [],
        "texture_fields": ["texture"],
    },
    "Composite": {
        "get_command": "GetComposites", "response_key": "composites",
        "create_command": "CreateComposites", "array_field": "compositeDataArray",
        "refs": {}, "opaque_fields": [],
        "nested_refs": {  # listField -> {itemField: ("id", targetType)}
            "skins": {"buildingMaterialId": ("id", "BuildingMaterial")},
            "separators": {"lineTypeId": ("id", "Line")},
        },
    },
    "BuildingMaterial": {
        "get_command": "GetBuildingMaterials", "response_key": "buildingMaterials",
        "create_command": "CreateBuildingMaterials", "array_field": "buildingMaterialDataArray",
        "refs": {"cutFillIndex": ("index", "Fill"), "cutSurfaceIndex": ("index", "Surface")},
        "opaque_fields": [],
    },
    "Profile": {
        "get_command": "GetProfiles", "response_key": "profiles",
        "create_command": "CreateProfiles", "array_field": "profileDataArray",
        "refs": {}, "opaque_fields": ["skins"],
        # useWith/width/height/minimumWidth/minimumHeight/widthStretchable/heightStretchable/hasCoreSkin are
        # all computed/derived from the geometry (GetProfiles exposes them for convenience) - NOT valid
        # CreateProfiles input fields at all. Sending them back verbatim gets the whole call rejected by the
        # JSON schema's additionalProperties:false ("Validation failed... path '#/profileDataArray/0/useWith'"),
        # confirmed live. Still fetched (useful to SEE in the diff) but excluded from the sync payload below.
        "readonly_fields": ["useWith", "width", "height", "minimumWidth", "minimumHeight",
                             "widthStretchable", "heightStretchable", "hasCoreSkin"],
        # "skinOutlines" must be requested separately from "skins" - GetProfiles only includes
        # outlineCoords/outlineSubPolyEnds/outlineArcs (the actual polygon geometry) when it's asked for
        # explicitly; without it every skin looks geometry-less and replaceSkins sync would have nothing to
        # rebuild from.
        "fields_filter": ["wallType", "beamType", "coluType", "handrailType", "otherGDLObjectType",
                           "useWith", "width", "height", "minimumWidth", "minimumHeight",
                           "widthStretchable", "heightStretchable", "hasCoreSkin", "skins", "skinOutlines"],
    },
    "LayerCombination": {
        "get_command": "GetLayerCombinations", "response_key": "layerCombinations",
        "create_command": "CreateLayerCombinations", "array_field": "layerCombinationDataArray",
        "refs": {}, "opaque_fields": ["layers"],
        "wrapped_response": "layerCombination",  # each item is {"layerCombination": {...}}
        "attribute_ids_param": "attributes",  # GetLayerCombinations uses "attributes" not "attributeIds"
    },
}

IGNORED_KEYS = {"attributeId", "index", "name", "attributeIndex"}  # LayerCombination uses "attributeIndex" instead of "index"


# ---------------------------------------------------------------------------------------------
# JSON-RPC client (parameterized by port - aclib.py's module-level globals only support one
# target at a time, which doesn't work for comparing two instances in the same process)
# ---------------------------------------------------------------------------------------------
class ArchicadInstance:
    def __init__(self, port, host=HOST):
        self.port = port
        self.host = host
        self.label = f"{host}:{port}"
        self.project_name = None

    def run_command(self, command, parameters=None, timeout=COMMAND_TIMEOUT):
        request_data = {"command": command, "parameters": parameters or {}}
        req = urllib.request.Request(
            f"http://{self.host}:{self.port}",
            data=json.dumps(request_data).encode("utf8"),
            headers={"Content-Type": "application/json"},
        )
        try:
            with urllib.request.urlopen(req, timeout=timeout) as resp:
                response_json = json.loads(resp.read())
        except (urllib.error.URLError, OSError, TimeoutError, ConnectionError, ValueError, json.JSONDecodeError) as exc:
            raise ArchicadCommError(f"{self.label}: {exc}") from exc
        if not response_json.get("succeeded"):
            return None
        return response_json.get("result")

    def run_tapir_command(self, command, parameters=None, timeout=COMMAND_TIMEOUT):
        result = self.run_command("API.ExecuteAddOnCommand", {
            "addOnCommandId": {"commandNamespace": "TapirCommand", "commandName": command},
            "addOnCommandParameters": parameters or {},
        }, timeout=timeout)
        if result is None:
            return None
        return result.get("addOnCommandResponse")


def probe_port(port):
    instance = ArchicadInstance(port)
    try:
        info = instance.run_tapir_command("GetProjectInfo", {}, timeout=PROBE_TIMEOUT)
    except ArchicadCommError:
        # Nothing listening on this port, or something unrelated to Archicad is.
        return None
    if info is None or "error" in (info or {}):
        return None
    instance.project_name = info.get("projectName") or f"(untitled, port {port})"
    return instance


def discover_instances(port_range=PORT_RANGE):
    found = []
    for port in port_range:
        instance = probe_port(port)
        if instance is not None:
            found.append(instance)
    return found


# ---------------------------------------------------------------------------------------------
# Fetch + normalize
# ---------------------------------------------------------------------------------------------
class ProjectAttributeData:
    """All attributes of one Archicad instance, name-keyed per type, plus the guid/index -> name
    lookup maps needed to resolve and re-resolve cross-attribute references."""

    def __init__(self, instance):
        self.instance = instance
        self.by_type = {}      # type -> {name: raw_field_dict}
        self.id_map = {}       # type -> {guid: name}
        self.index_map = {}    # type -> {index: name}
        self.attr_id_by_name = {}  # type -> {name: attributeId dict, e.g. {"guid": ...}}
        self.texture_resolution = {}  # raw texture name variant -> canonical fileName (see fetch_texture_resolution)


def fetch_headers(instance, attr_type):
    result = instance.run_tapir_command("GetAttributesByType", {"attributeType": attr_type})
    if result is None:
        # A None result means the JSON-RPC call itself did not succeed (as opposed to succeeding
        # with zero attributes) - most commonly because Archicad has a modal dialog open (e.g. the
        # Profile Editor / "complex profile" window) and is refusing API calls while it's up. Silently
        # treating this as "zero attributes exist" previously caused every reference to this type to
        # falsely report "not found in target project" instead of surfacing the real cause.
        raise ArchicadCommError(f"{instance.label}: GetAttributesByType({attr_type}) did not succeed - "
                                 f"Archicad may have a modal dialog open (e.g. Profile Editor) blocking API calls. Close it and retry.")
    return result.get("attributes", [])


def fetch_texture_resolution(instance):
    """Build a lookup resolving any texture name variant to a stable canonical form, using
    GetAvailableLibraryParts(filterByTypeId="Picture"). Needed because a Fill/Surface's stored
    texture name is a plain string, not a live-resolved reference - confirmed live that it can drift
    from a picture's current CATALOG display name (documentName) while still resolving to the exact
    same underlying image via filename fallback, causing a guaranteed false-positive "different" diff
    (e.g. a picture catalogued as documentName "Béton - 10 c -opt" / fileName "Concrete - 10 c
    -opt.jpg": one project's Surface stored the French documentName, another project's Surface stored
    something matching the English fileName - same picture, different stored string). Canonicalizing
    on fileName-without-extension (the more physically stable identifier) so both variants map to the
    same value. Returns {} if the call fails - callers must treat that as "leave names unresolved",
    not an error, since this is a best-effort false-positive reducer, not required functionality."""
    try:
        result = instance.run_tapir_command("GetAvailableLibraryParts", {"filterByTypeId": "Picture"})
    except ArchicadCommError:
        return {}
    if result is None:
        return {}
    resolution = {}
    for part in result.get("libraryParts", []):
        file_name = part.get("fileName", "")
        canonical = file_name.rsplit(".", 1)[0] if "." in file_name else file_name
        if not canonical:
            continue
        doc_name = part.get("documentName", "")
        if doc_name:
            resolution[doc_name] = canonical
        resolution[canonical] = canonical
    return resolution


def resolve_texture_name(raw_name, texture_resolution):
    if not raw_name:
        return raw_name
    return texture_resolution.get(raw_name, raw_name)


def fetch_project_data(instance, progress_callback=None):
    data = ProjectAttributeData(instance)
    if progress_callback:
        progress_callback("Fetching texture library index...")
    data.texture_resolution = fetch_texture_resolution(instance)

    # Pass 1: headers for every type (needed both as the data to compare and as the id/index
    # maps used to resolve references in every OTHER type).
    headers_by_type = {}
    for attr_type in ATTRIBUTE_TYPES:
        if progress_callback:
            progress_callback(f"Listing {attr_type}...")
        headers = fetch_headers(instance, attr_type)
        headers_by_type[attr_type] = headers
        data.id_map[attr_type] = {h["attributeId"]["guid"]: h["name"] for h in headers}
        data.index_map[attr_type] = {h["index"]: h["name"] for h in headers}
        data.attr_id_by_name[attr_type] = {h["name"]: h["attributeId"] for h in headers}

    # Pass 2: detailed fields for every type.
    for attr_type, type_def in ATTRIBUTE_TYPES.items():
        headers = headers_by_type[attr_type]
        if not headers:
            data.by_type[attr_type] = {}
            continue
        if progress_callback:
            progress_callback(f"Fetching {attr_type} details ({len(headers)})...")

        attribute_ids_param = type_def.get("attribute_ids_param", "attributeIds")
        params = {attribute_ids_param: [{"attributeId": h["attributeId"]} for h in headers]}
        if "fields_filter" in type_def:
            params["fields"] = type_def["fields_filter"]

        result = instance.run_tapir_command(type_def["get_command"], params)
        if result is None:
            raise ArchicadCommError(f"{instance.label}: {type_def['get_command']} did not succeed - "
                                     f"Archicad may have a modal dialog open (e.g. Profile Editor) blocking API calls. Close it and retry.")
        items = result.get(type_def["response_key"], [])

        wrapped_key = type_def.get("wrapped_response")
        by_name = {}
        for item in items:
            if wrapped_key:
                item = item.get(wrapped_key, item)
            if "error" in item:
                continue
            name = item.get("name")
            if name is None:
                continue
            volatile = set(type_def.get("volatile_fields", []))
            by_name[name] = {k: v for k, v in item.items() if k not in IGNORED_KEYS and k not in volatile}
        data.by_type[attr_type] = by_name

    return data


def resolve_ref_value(raw_value, ref_kind, target_type, data):
    """Turn a raw attributeId dict ({"attributeId": {"guid": ...}}) or raw index into the
    referenced attribute's name, for cross-project comparison. Returns None if unresolvable."""
    if raw_value is None:
        return None
    if ref_kind == "id":
        guid = raw_value.get("attributeId", {}).get("guid") if isinstance(raw_value, dict) else None
        if guid is None:
            return None
        return data.id_map.get(target_type, {}).get(guid)
    else:  # "index"
        return data.index_map.get(target_type, {}).get(raw_value)


def normalize_fields(attr_type, raw_fields, data):
    """Replace reference fields (attributeId / index pointing at another attribute) with the
    referenced attribute's NAME, so the result is meaningful to compare across two different
    project files. Returns (normalized_dict, opaque_dict) - opaque_dict holds the fields this
    type marks as compare-only (never offered for field-level sync)."""
    type_def = ATTRIBUTE_TYPES[attr_type]
    normalized = {}
    opaque = {}
    opaque_field_names = set(type_def.get("opaque_fields", []))

    for field, value in raw_fields.items():
        if field in opaque_field_names:
            opaque[field] = value
            continue
        if field in type_def.get("refs", {}):
            ref_kind, target_type = type_def["refs"][field]
            normalized[field] = ("__ref__", resolve_ref_value(value, ref_kind, target_type, data))
            continue
        if field in type_def.get("nested_refs", {}):
            item_refs = type_def["nested_refs"][field]
            resolved_list = []
            for item in value or []:
                resolved_item = dict(item)
                for item_field, (ref_kind, target_type) in item_refs.items():
                    if item_field in resolved_item:
                        resolved_item[item_field] = ("__ref__", resolve_ref_value(resolved_item[item_field], ref_kind, target_type, data))
                resolved_list.append(resolved_item)
            normalized[field] = resolved_list
            continue
        normalized[field] = value

    return normalized, opaque


FLOAT_TOLERANCE = 0.00002  # just above one 16-bit color-channel step (1/65535 ~= 0.0000153,
# confirmed live by writing known values and reading them back: readback*65535 always lands on an
# exact integer - Archicad's pen colors are quantized to 16 bits per channel internally) - see
# round_floats()/fuzzy_equal()


def round_floats(value):
    """Round every float leaf to FLOAT_TOLERANCE's precision before fingerprinting, so Archicad's own
    write/read quantization noise (confirmed live: a color channel sent back exactly as read came
    back ~1.5e-5 off) doesn't make a synced opaque field (e.g. Profile skins) perpetually fingerprint
    as "different" no matter how many times it's re-applied."""
    if isinstance(value, float):
        return round(value / FLOAT_TOLERANCE) * FLOAT_TOLERANCE
    if isinstance(value, dict):
        return {k: round_floats(v) for k, v in value.items()}
    if isinstance(value, list):
        return [round_floats(v) for v in value]
    return value


def opaque_fingerprint(attr_type, opaque_value, data=None):
    """Content fingerprint ignoring internal identifiers that are never stable across separate
    project files (Profile skins' skinId; LayerCombination layers' per-entry attributeId, resolved
    to the referenced Layer's NAME via `data` - comparing raw guids across two different project
    files would always differ even for identically-configured layers, since guids are project-local,
    causing every LayerCombination to false-positive as "different")."""
    opaque_value = round_floats(opaque_value)
    if attr_type == "Profile":
        # opaque_value is the raw "skins" list; strip skinId, sort for order-independence. Also drop
        # edges[0] and edges[-1] - the anchor-bridge slots (see AttributeCommands.cpp's
        # BuildHatchFromSkinDefinition crash investigation note) that newSkins can never write to
        # (skipped there to avoid a demonstrated Archicad-side crash) - comparing them would make a
        # Profile sync forever show as "different" even when every real, renderable edge matches.
        # Also drop isInnerLine/isCutEndLine per remaining edge - these are Archicad-computed
        # topology flags (confirmed live: neither is accepted by either edgeOverrides write path,
        # ApplyProfileSkinOverrides or BuildHatchFromSkinDefinition), derived from how the profile's
        # geometry happens to be internally triangulated/segmented rather than a settable property,
        # so a rebuilt skin can legitimately get a different isInnerLine classification than the
        # original even when every real, syncable property (material/pen/line type/visibility)
        # matches exactly.
        # Skin-level and edge-level reference fields must be resolved to the referenced attribute's
        # NAME before comparing, exactly like LayerCombination's layers below - comparing raw guids
        # would always differ across two separate project files even when e.g. the SAME-NAMED
        # building material is referenced on both sides (confirmed live: "Poteau générique" false-
        # positived as "different" solely because its skin's buildingMaterialId guid differed between
        # projects, despite both resolving to the identical material name "GÉNÉRIQUE - TÔLES
        # INTÉRIEURES" - the exact same class of bug as the LayerCombination one below, just found
        # later since it required a skin with a distinct-per-skin material reference to surface).
        skin_ref_fields = {"buildingMaterialId": "BuildingMaterial", "surfaceId": "Surface",
                            "fillId": "Fill", "contourLineTypeId": "Line", "cutEndLineTypeId": "Line"}
        edge_ref_fields = {"buildingMaterialId": "BuildingMaterial", "lineTypeId": "Line"}

        def resolved_ref_or_guid(value, target_type):
            name = resolve_ref_value(value, "id", target_type, data) if data is not None else None
            return name if name is not None else value.get("attributeId", {}).get("guid")

        stripped = []
        for skin in (opaque_value or []):
            entry = {k: v for k, v in skin.items() if k != "skinId"}
            for field, target_type in skin_ref_fields.items():
                if field in entry:
                    entry[field] = resolved_ref_or_guid(entry[field], target_type)
            edges = entry.get("edges")
            if isinstance(edges, list) and len(edges) >= 2:
                # Bridge slots aren't just edges[0]/edges[-1] (anchor -> first contour, last contour ->
                # anchor) - a skin with multiple contours (e.g. a hollow profile with an inner hole) has
                # one more inert bridge edge AT EACH contour boundary, connecting one contour's own
                # closing-duplicate coordinate to the next contour's first real coordinate. Its start
                # coordinate index is exactly outlineSubPolyEnds[i] for every contour i except the very
                # last (whose own bridge is already the standard edges[-1]) - confirmed live: a 2-contour
                # skin ("Acier creux rectangulaire", outlineSubPolyEnds=[0, 5, 10]) kept showing
                # "different" after every sync solely because of edge index 5, the only bridge not
                # already excluded by the edges[0]/edges[-1] rule.
                subpoly_ends = entry.get("outlineSubPolyEnds") or []
                bridge_indices = {0, len(edges) - 1} | set(subpoly_ends[1:])
                resolved_edges = []
                for i, e in enumerate(edges):
                    if i in bridge_indices:
                        continue
                    edge_entry = {k: v for k, v in e.items() if k not in ("isInnerLine", "isCutEndLine")}
                    for field, target_type in edge_ref_fields.items():
                        if field in edge_entry:
                            edge_entry[field] = resolved_ref_or_guid(edge_entry[field], target_type)
                    resolved_edges.append(edge_entry)
                entry["edges"] = resolved_edges
            stripped.append(entry)
        return json.dumps(sorted(stripped, key=lambda s: json.dumps(s, sort_keys=True)), sort_keys=True)
    if attr_type == "LayerCombination":
        # opaque_value is the raw "layers" list; resolve each entry's attributeId to the Layer's name
        # (unresolvable entries keep the raw guid as a fallback key, still sortable/comparable).
        resolved = []
        for layer in (opaque_value or []):
            name = resolve_ref_value(layer, "id", "Layer", data) if data is not None else None
            entry = {k: v for k, v in layer.items() if k != "attributeId"}
            entry["layerName"] = name if name is not None else layer.get("attributeId", {}).get("guid")
            resolved.append(entry)
        return json.dumps(sorted(resolved, key=lambda s: json.dumps(s, sort_keys=True)), sort_keys=True)
    return json.dumps(opaque_value, sort_keys=True)


# ---------------------------------------------------------------------------------------------
# Diff engine
# ---------------------------------------------------------------------------------------------
class FieldDiff:
    def __init__(self, field, value_a, value_b):
        self.field = field
        self.value_a = value_a
        self.value_b = value_b


def fuzzy_equal(a, b):
    """Structural equality that tolerates small floating-point noise in numeric leaves. Archicad's
    own pen-color storage re-quantizes on write/read (confirmed live: a PenTable color sent back
    exactly as read came back as e.g. 0.007843137 -> 0.007827878, a ~1.5e-5 difference) - comparing
    with strict `==` made synced attributes perpetually show as "different" no matter how many times
    they were re-applied, since there's no value that survives Archicad's own round-trip unchanged."""
    if isinstance(a, bool) or isinstance(b, bool):
        return a is b
    if isinstance(a, (int, float)) and isinstance(b, (int, float)):
        return abs(a - b) <= FLOAT_TOLERANCE
    if isinstance(a, dict) and isinstance(b, dict):
        return a.keys() == b.keys() and all(fuzzy_equal(a[k], b[k]) for k in a)
    if isinstance(a, list) and isinstance(b, list):
        return len(a) == len(b) and all(fuzzy_equal(x, y) for x, y in zip(a, b))
    return a == b


def texture_field_equal(va, vb, resolution_a, resolution_b):
    """Compare a Fill/Surface "texture" field, resolving the "name" sub-field through each side's
    OWN texture library index first (see fetch_texture_resolution) - a stored texture name can drift
    from a picture's current catalog display name while still resolving to the exact same underlying
    image, which strict comparison would false-positive as "different" forever (confirmed live:
    "Béton - 10 c -opt" vs "Concrete - 10 c -opt" - same picture, different stored string). Only the
    comparison is affected; the raw field value used for sync payloads is untouched elsewhere."""
    if va is None or vb is None:
        return va == vb
    name_a = resolve_texture_name(va.get("name"), resolution_a)
    name_b = resolve_texture_name(vb.get("name"), resolution_b)
    if name_a != name_b:
        return False
    rest_a = {k: v for k, v in va.items() if k != "name"}
    rest_b = {k: v for k, v in vb.items() if k != "name"}
    return fuzzy_equal(rest_a, rest_b)


class AttributeDiff:
    def __init__(self, attr_type, name, status):
        self.attr_type = attr_type
        self.name = name
        self.status = status  # "only_a" | "only_b" | "different" | "identical"
        self.field_diffs = []       # list[FieldDiff] - normal, syncable fields
        self.readonly_field_diffs = []  # list[FieldDiff] - informational only, never drives "different"
        self.opaque_differs = False  # True if this type's opaque part (skins/layers) differs
        self.unresolved_refs = []   # human-readable notes about refs that didn't resolve


class IndexMismatch:
    """A same-named attribute that sits at a different index in A vs B - see
    find_index_mismatches(). One of these becomes one actionable GUI row, exactly like an
    AttributeDiff, so index fixes get the same per-row L/●/R choice as content syncs instead of
    being bulk-only (per explicit user feedback: the earlier bulk-only "All A -> B fixes everything"
    design gave no way to fix just one mismatch, or skip one, individually)."""
    def __init__(self, attr_type, name, index_a, index_b):
        self.attr_type = attr_type
        self.name = name
        self.index_a = index_a
        self.index_b = index_b


def diff_attribute_type(attr_type, data_a, data_b):
    names_a = set(data_a.by_type.get(attr_type, {}).keys())
    names_b = set(data_b.by_type.get(attr_type, {}).keys())
    results = []

    for name in sorted(names_a | names_b):
        if name not in names_b:
            results.append(AttributeDiff(attr_type, name, "only_a"))
            continue
        if name not in names_a:
            results.append(AttributeDiff(attr_type, name, "only_b"))
            continue

        raw_a = data_a.by_type[attr_type][name]
        raw_b = data_b.by_type[attr_type][name]
        norm_a, opaque_a = normalize_fields(attr_type, raw_a, data_a)
        norm_b, opaque_b = normalize_fields(attr_type, raw_b, data_b)

        diff = AttributeDiff(attr_type, name, "identical")
        readonly_field_names = set(ATTRIBUTE_TYPES[attr_type].get("readonly_fields", []))
        texture_field_names = set(ATTRIBUTE_TYPES[attr_type].get("texture_fields", []))
        all_fields = set(norm_a.keys()) | set(norm_b.keys())
        for field in sorted(all_fields):
            va, vb = norm_a.get(field), norm_b.get(field)
            if field in texture_field_names:
                if texture_field_equal(va, vb, data_a.texture_resolution, data_b.texture_resolution):
                    continue
            elif fuzzy_equal(va, vb):
                continue
            if isinstance(va, tuple) and va[0] == "__ref__" and va[1] is None:
                diff.unresolved_refs.append(f"{field} (A side): reference did not resolve to a name")
            if isinstance(vb, tuple) and vb[0] == "__ref__" and vb[1] is None:
                diff.unresolved_refs.append(f"{field} (B side): reference did not resolve to a name")
            # Read-only/computed fields (e.g. Profile's minimumWidth) can never be synced (Archicad
            # derives them from geometry) - showing them as a "difference" would make the attribute
            # perpetually look unsynced even right after a successful sync. Keep them visible for
            # context but never let them drive status="different" on their own.
            target_list = diff.readonly_field_diffs if field in readonly_field_names else diff.field_diffs
            target_list.append(FieldDiff(field, va, vb))

        opaque_names = set(opaque_a.keys()) | set(opaque_b.keys())
        for field in opaque_names:
            fp_a = opaque_fingerprint(attr_type, opaque_a.get(field), data_a)
            fp_b = opaque_fingerprint(attr_type, opaque_b.get(field), data_b)
            if fp_a != fp_b:
                diff.opaque_differs = True

        if diff.field_diffs or diff.opaque_differs:
            diff.status = "different"
        results.append(diff)

    return results


def find_index_mismatches(attr_type, data_a, data_b):
    """Cross-check, for one attribute type, whether same-named attributes share the same index
    between A and B. Index numbering restarts at 1 for EACH attribute type independently, so this
    must be checked type-by-type, never across types. This matters because some references are
    INDEX-based, not name-based (e.g. BuildingMaterial.cutFillIndex points at a Fill by index) - if
    two projects assign different indices to the same-named attribute, an index-based reference that
    looks fine in each project individually can silently point at the WRONG attribute once
    compared/ported across projects (confirmed live: reindexing two BuildingMaterials in one project
    changed which material a Profile skin's buildingMaterialId resolved to there, purely as a side
    effect of the index shift). Returns a list of IndexMismatch, one per same-named attribute whose
    index differs; empty if fully consistent. (The "same index, different name" angle is the exact
    same underlying data viewed from the other side - not surfaced separately, it would just
    duplicate every entry here.)"""
    name_to_index_a = {name: idx for idx, name in data_a.index_map.get(attr_type, {}).items()}
    name_to_index_b = {name: idx for idx, name in data_b.index_map.get(attr_type, {}).items()}
    mismatches = []
    for name in sorted(set(name_to_index_a) & set(name_to_index_b)):
        ia, ib = name_to_index_a[name], name_to_index_b[name]
        if ia != ib:
            mismatches.append(IndexMismatch(attr_type, name, ia, ib))
    return mismatches


def _cleanup_placeholders(instance, attr_type, placeholder_ids):
    """Delete the disposable gap-filling placeholders reindex_attributes_bulk created. Best-effort -
    always attempts every one even if an earlier one fails, since leaving a couple of junk
    placeholders behind is a much smaller problem than leaving the whole reindex half-done. Returns
    (all_ok, note)."""
    if not placeholder_ids:
        return True, None
    failures = []
    for attr_id in placeholder_ids:
        try:
            r = instance.run_tapir_command("DeleteAttributes", {"attributesToDelete": [
                {"attributeType": attr_type, "attributeId": {"attributeId": attr_id}}
            ]})
        except ArchicadCommError as exc:
            failures.append(str(exc))
            continue
        exec_results = (r or {}).get("executionResults", [])
        if not (exec_results and exec_results[0].get("success")):
            failures.append("delete rejected")
    return (not failures), "; ".join(failures) if failures else None


def reindex_attributes_bulk(instance, data, attr_type, moves):
    """Move multiple attributes to new target indices in one coordinated batch, on a single
    instance. `moves` is a list of (name, target_index) pairs.

    There is NO direct "move while preserving identity" operation exposed by the public Archicad
    API (confirmed via extensive live testing - every ACAPI_Attribute_Modify attempt with a changed
    header.index failed), and CreateX's "index" field is ONLY honored when OVERWRITING an attribute
    already sitting at that index - for a genuinely FREE index, Create's "index" field is silently
    ignored and Archicad always fills the LOWEST free gap instead (confirmed live: requesting
    index=999 landed at index 2). This matches how Archicad's own Attribute Manager "Reindex" works
    under the hood per the Graphisoft community forum (Barry Kelly): "Append by Name" fills
    sequentially from the lowest gap, and to skip a slot you "duplicate the last attribute to
    increase the last number" - i.e. deliberately waste the lower gap with a throwaway.

    Calling a single-move reindex in a naive per-name loop is UNSAFE for interdependent moves: if A
    -> 5 while B (currently sitting AT index 5) is ALSO being moved elsewhere in the same batch,
    processing A first overwrites B's content at index 5 before B's own move ever runs, permanently
    losing it (a first, broken version of this function did exactly that live before this was
    caught). Avoided by snapshotting every batch attribute's content BEFORE touching anything, then:
      1. Delete every batch attribute (after snapshotting). This frees every name AND every index
         slot the batch touches in one step, so no later phase can ever find "an in-the-way batch
         attribute" - only non-batch attributes (untouched) or genuine gaps remain.
      2. Place moves in ASCENDING target_index order, under a UNIQUE TEMPORARY name (never the real
         final name yet - two batch targets could transiently collide with each other's real names
         otherwise). Fills any gap below a target with a disposable placeholder first, UNLESS that
         gap is itself an upcoming batch target (ascending order guarantees every target_index IS
         the lowest remaining gap by its own turn, so no placeholder is ever wasted on a slot the
         batch was going to fill anyway).
      3. Rename every placed attribute from its temp name to its real final name - safe now, since
         phase 1 freed every real name this batch uses and no non-batch attribute would coincidentally
         hold one of the temp names.

    Real gotcha found live: CreateX's "index" field's JSON schema type is STRING, not integer -
    sending a plain number fails with a generic schema validation error that gives no hint the type
    is wrong.

    If a step fails partway, the affected attribute's content is only recoverable from this
    function's own error report (or by re-running it) - Archicad has no cross-command transaction
    boundary, so a partial batch failure leaves exactly what actually happened, not an automatic
    rollback.

    Returns a list of (name, target_index, ok, error_message), one entry per requested move, in the
    same order as `moves`."""
    type_def = ATTRIBUTE_TYPES[attr_type]
    if type_def.get("opaque_fields"):
        return [(n, ti, False, "bulk reindex does not support types with opaque geometry fields (Profile, LayerCombination)") for n, ti in moves]

    index_to_name = dict(data.index_map.get(attr_type, {}))
    name_to_index = {n: i for i, n in index_to_name.items()}
    name_to_attr_id = dict(data.attr_id_by_name.get(attr_type, {}))

    def strip_readonly(fields):
        result = dict(fields)
        for readonly_field in type_def.get("readonly_fields", []):
            result.pop(readonly_field, None)
        return result

    real_moves = [(n, ti) for n, ti in moves if name_to_index.get(n) != ti]
    results = {n: (True, None) for n, ti in moves if name_to_index.get(n) == ti}
    if not real_moves:
        return [(n, ti, *results[n]) for n, ti in moves]

    # Some types (e.g. MEPSystem's predefined domain systems) use a reserved, deliberately sparse
    # index range far above ordinary user-created attributes (seen live: 2000000001+) - gap-filling
    # up to a target that high would mean creating billions of disposable placeholders (previously
    # crashed with MemoryError just building the `range()`). These aren't reachable via ordinary
    # Create-at-lowest-free-gap anyway (Archicad never allocates a fresh attribute into that reserved
    # range), so treat any target far past the highest currently-known index as out of reach rather
    # than attempting it.
    max_known_index = max(index_to_name.keys(), default=0)
    gap_fill_ceiling = max_known_index + 1000
    out_of_range = {n for n, ti in real_moves if ti > gap_fill_ceiling}
    if out_of_range:
        msg = (f"Target index is far beyond the highest known index ({max_known_index}) for {attr_type} - "
               f"likely a reserved/predefined index range that can't be reached by creating gap-filling "
               f"placeholders. Aborted before making any change (no attribute in this batch was touched).")
        return [(n, ti, False, msg) for n, ti in moves]

    batch_names = {n for n, _ in real_moves}
    snapshots = {}
    for name, _ in real_moves:
        raw = data.by_type.get(attr_type, {}).get(name)
        if raw is None or name not in name_to_attr_id:
            snapshots[name] = None
        else:
            snapshots[name] = (strip_readonly(raw), name_to_attr_id[name])

    # Phase 1: delete every batch attribute that actually exists.
    to_delete = [(n, snapshots[n][1]) for n, _ in real_moves if snapshots.get(n)]
    if to_delete:
        try:
            del_result = instance.run_tapir_command("DeleteAttributes", {"attributesToDelete": [
                {"attributeType": attr_type, "attributeId": {"attributeId": aid}} for _, aid in to_delete
            ]})
        except ArchicadCommError as exc:
            return [(n, ti, False, f"Bulk delete failed before any moves were made: {exc}") for n, ti in moves]
        exec_results = (del_result or {}).get("executionResults", [])
        for (name, _aid), exec_res in zip(to_delete, exec_results):
            if not exec_res.get("success"):
                results[name] = (False, f"Failed to delete source before reindexing: {exec_res.get('error', {}).get('message', 'unknown')}")

    remaining_occupied = {idx: nm for idx, nm in index_to_name.items() if nm not in batch_names}
    # Every batch attribute's OWN original slot is now a genuine gap (freed by phase 1's delete) -
    # this is exactly why a swap (A -> B's old index, B -> A's old index) needs gap-filling: both
    # targets are gaps at this point, not "occupied by the other batch member" (that member is gone).

    # Phase 2: place moves in ASCENDING target_index order, filling any gap below a target with a
    # disposable placeholder first (unless that gap is itself an upcoming batch target, in which
    # case processing it in order will fill it for real when its own turn comes - ascending order
    # guarantees every target_index IS the lowest remaining gap by the time its own turn arrives, so
    # no placeholder is ever wasted on a slot the batch was going to fill anyway).
    ordered_moves = sorted((m for m in real_moves if m[0] not in results), key=lambda m: m[1])
    upcoming_targets = {ti for _, ti in ordered_moves}
    placeholder_ids = []
    temp_names = {}
    placeholder_template = None
    for name, target_index in ordered_moves:
        if name in results:
            continue
        snap = snapshots.get(name)
        if snap is None:
            results[name] = (False, "Source attribute not found (already missing).")
            continue
        raw, _ = snap
        if placeholder_template is None:
            placeholder_template = raw

        occupant_name = remaining_occupied.get(target_index)
        if occupant_name is None:
            gaps_below = [i for i in range(1, target_index)
                          if i not in remaining_occupied and i not in upcoming_targets]
            failed = False
            for gap_idx in gaps_below:
                ph_name = f"__bulk_reindex_gapfill__{len(placeholder_ids)}"
                ph_payload = dict(placeholder_template)
                ph_payload["name"] = ph_name
                try:
                    r = instance.run_tapir_command(type_def["create_command"], {
                        "overwriteExisting": True, type_def["array_field"]: [ph_payload]})
                except ArchicadCommError as exc:
                    results[name] = (False, f"Failed to fill gap at index {gap_idx} en route to {target_index}: {exc}")
                    failed = True
                    break
                p_items = (r or {}).get("attributeIds", [])
                if not p_items or "error" in p_items[0]:
                    err = p_items[0]["error"].get("message") if p_items else "Archicad rejected the request"
                    results[name] = (False, f"Failed to fill gap at index {gap_idx} en route to {target_index}: {err}")
                    failed = True
                    break
                placeholder_ids.append(p_items[0]["attributeId"])
                remaining_occupied[gap_idx] = ph_name
            if failed:
                continue

        temp_name = f"__bulk_reindex_tmp__{name}"
        temp_names[name] = temp_name
        payload = dict(raw)
        payload["name"] = temp_name
        if occupant_name is not None:
            occupant_id = name_to_attr_id.get(occupant_name)
            if occupant_id is None:
                results[name] = (False, f"Target index {target_index} is occupied by '{occupant_name}', which could not be resolved to an id.")
                continue
            payload["attributeId"] = occupant_id
        try:
            r = instance.run_tapir_command(type_def["create_command"], {
                "overwriteExisting": True, type_def["array_field"]: [payload]})
        except ArchicadCommError as exc:
            results[name] = (False, f"Failed to place at index {target_index}: {exc}")
            continue
        items = (r or {}).get("attributeIds", [])
        if not items or "error" in items[0]:
            err = items[0]["error"].get("message") if items else "Archicad rejected the request"
            results[name] = (False, f"Failed to place at index {target_index}: {err}")
            continue
        remaining_occupied[target_index] = temp_name
        name_to_attr_id[temp_name] = items[0]["attributeId"]

    ph_ok, ph_err = _cleanup_placeholders (instance, attr_type, placeholder_ids)
    placeholder_note = None if ph_ok else f"{len(placeholder_ids)} gap-filling placeholder(s) could not be cleaned up: {ph_err}"

    # Phase 3: rename every successfully-placed temp attribute to its real name.
    for name, target_index in real_moves:
        if name in results:
            continue
        temp_name = temp_names.get(name)
        rename_payload = dict(snapshots[name][0])
        rename_payload["name"] = name
        rename_payload["attributeId"] = name_to_attr_id[temp_name]
        try:
            r2 = instance.run_tapir_command(type_def["create_command"], {
                "overwriteExisting": True, type_def["array_field"]: [rename_payload]})
        except ArchicadCommError as exc:
            results[name] = (False, f"Placed at index {target_index} but failed to rename from '{temp_name}': {exc}")
            continue
        items2 = (r2 or {}).get("attributeIds", [])
        if not items2 or "error" in items2[0]:
            err = items2[0]["error"].get("message") if items2 else "Archicad rejected the request"
            results[name] = (False, f"Placed at index {target_index} but failed to rename from '{temp_name}': {err}")
            continue
        results[name] = (True, placeholder_note if placeholder_note else None)

    return [(n, ti, *results.get(n, (False, "Not processed."))) for n, ti in moves]


def diff_all(data_a, data_b):
    all_diffs = {}
    for attr_type in ATTRIBUTE_TYPES:
        all_diffs[attr_type] = diff_attribute_type(attr_type, data_a, data_b)
    return all_diffs


# ---------------------------------------------------------------------------------------------
# Sync application
# ---------------------------------------------------------------------------------------------
def unwrap_ref_for_sync(value):
    """Undo the ("__ref__", name) marker normalize_fields() produced, returning the plain name
    (or None) so apply_sync can re-resolve it against the target project."""
    if isinstance(value, tuple) and len(value) == 2 and value[0] == "__ref__":
        return value[1]
    return value


def build_field_payload(attr_type, field, value, target_data):
    """Convert one normalized field value back into the raw JSON shape CreateX expects,
    re-resolving any reference by name in the TARGET project. Returns (ok, payload_value)."""
    type_def = ATTRIBUTE_TYPES[attr_type]

    if field in type_def.get("refs", {}):
        _, target_type = type_def["refs"][field]
        name = unwrap_ref_for_sync(value)
        if name is None:
            return False, None
        attr_id = target_data.attr_id_by_name.get(target_type, {}).get(name)
        if attr_id is None:
            return False, None
        ref_kind = type_def["refs"][field][0]
        if ref_kind == "id":
            return True, {"attributeId": attr_id}
        else:  # index - CreateBuildingMaterials takes a plain integer index in the TARGET project
            target_index = next((idx for idx, n in target_data.index_map.get(target_type, {}).items() if n == name), None)
            if target_index is None:
                return False, None
            return True, target_index

    if field in type_def.get("nested_refs", {}):
        item_refs = type_def["nested_refs"][field]
        payload_list = []
        for item in value:
            payload_item = dict(item)
            for item_field, (ref_kind, target_type) in item_refs.items():
                name = unwrap_ref_for_sync(payload_item.get(item_field))
                if name is None:
                    return False, None
                attr_id = target_data.attr_id_by_name.get(target_type, {}).get(name)
                if attr_id is None:
                    return False, None
                payload_item[item_field] = {"attributeId": attr_id}
            payload_list.append(payload_item)
        return True, payload_list

    if attr_type == "MEPSystem" and field == "domain" and type_def.get("domain_list_to_scalar"):
        return True, (value[0] if value else "Ventilation")

    return True, value


def apply_field_syncs(target_instance, target_data, attr_type, name, field_values):
    """field_values: dict of {field: normalized_value} to push into `name` on target_instance.
    Returns (success: bool, error_message: str | None)."""
    type_def = ATTRIBUTE_TYPES[attr_type]
    payload = {"name": name}
    existing_attr_id = target_data.attr_id_by_name.get(attr_type, {}).get(name)
    if existing_attr_id is not None:
        payload["attributeId"] = existing_attr_id

    skipped = []
    for field, value in field_values.items():
        ok, converted = build_field_payload(attr_type, field, value, target_data)
        if not ok:
            skipped.append(field)
            continue
        payload[field] = converted

    call_params = {"overwriteExisting": True, type_def["array_field"]: [payload]}
    try:
        result = target_instance.run_tapir_command(type_def["create_command"], call_params)
    except ArchicadCommError as exc:
        return False, f"Communication error: {exc}", skipped
    if result is None:
        return False, "Archicad rejected the request (JSON-RPC call did not succeed) - the target instance may be busy (e.g. a modal dialog open) or in a state that can't accept commands right now.", skipped
    items = result.get("attributeIds", [])
    if items and "error" in items[0]:
        return False, items[0]["error"].get("message", "Unknown error"), skipped
    # Keep target_data's id-by-name map current within the same on_apply batch: without this, a later
    # action in the same batch that references THIS attribute by name (e.g. a Profile skin's
    # buildingMaterialId) would still resolve against the pre-Compare snapshot and find nothing, even
    # though the attribute now genuinely exists in the target project - forcing the user to Compare and
    # Apply a second time. Confirmed live as the actual cause of "some Profiles need two passes".
    if items and "attributeId" in items[0] and existing_attr_id is None:
        target_data.attr_id_by_name.setdefault(attr_type, {})[name] = items[0]["attributeId"]
    return True, None, skipped


def build_contours_from_outline(outline_coords, outline_subpoly_ends, outline_arcs):
    """Inverse of the C++ BuildHatchPolygonGeometry: turn GetProfiles' flat outlineCoords /
    outlineSubPolyEnds / outlineArcs (coords[0] is a reserved anchor at (0,0); each contour is
    closed by repeating its own first vertex) back into the "contours" shape newSkins expects -
    0-based real vertices per contour, arcs 0-based within their own contour. outline_subpoly_ends[0]
    is always 0 (closing the anchor's own degenerate subpoly); each subsequent entry closes one real
    contour, so contour i spans outline_coords[outline_subpoly_ends[i-1]+1 : outline_subpoly_ends[i]]
    (the slice already excludes the closing duplicate at the end index)."""
    contours = []
    for i in range(1, len(outline_subpoly_ends)):
        start = outline_subpoly_ends[i - 1] + 1
        end = outline_subpoly_ends[i]
        real_coords = outline_coords[start:end]
        contour = {"polygonCoordinates": [{"x": c["x"], "y": c["y"]} for c in real_coords]}
        contour_arcs = []
        for arc in outline_arcs:
            # An arc's begIndex/endIndex may land exactly on `end` (the closing-duplicate coordinate,
            # same position as `start`, closing the contour's last segment back to its own start) -
            # the C++ side (BuildHatchPolygonGeometry) auto-appends this same closing duplicate right
            # after the n_real real coordinates it's given, at local index n_real, so `arc_index -
            # start` already lands on the right slot for this case with NO special-casing needed - the
            # old code's strict `< end` bound wrongly excluded it entirely (confirmed live: "Lisse bois
            # complexe"'s last arc, begIndex 24/endIndex 25 on a 26-coord skin, vanished after
            # replaceSkins because endIndex==end failed that check).
            beg = arc["begIndex"]
            end_ = arc["endIndex"]
            if not (start <= beg <= end and start <= end_ <= end):
                continue
            local_beg = beg - start
            local_end = end_ - start
            if local_beg == local_end:
                continue  # degenerate (both ends on the same vertex) - not a real arc
            contour_arcs.append({
                "begIndex": local_beg,
                "endIndex": local_end,
                "arcAngle": arc["arcAngle"],
            })
        if contour_arcs:
            contour["polygonArcs"] = contour_arcs
        contours.append(contour)
    return contours


def build_new_skins_payload(skins, source_data, target_data):
    """Convert GetProfiles' "skins" (fetched with skinOutlines) into a newSkins array ready to send
    to the TARGET instance: rebuilds each skin's polygon geometry via build_contours_from_outline
    and resolves every attribute reference (buildingMaterialId, surfaceId, fillId,
    contourLineTypeId, cutEndLineTypeId, per-edge lineTypeId) by NAME against the target project.
    A skin with an unresolvable reference is dropped entirely rather than partially applied,
    consistent with this tool's "flag and skip" policy for missing dependencies elsewhere. Returns
    (any_skin_built: bool, new_skins_list, notes) - notes explain any dropped/adjusted skin."""
    ref_fields = {
        "buildingMaterialId": "BuildingMaterial",
        "surfaceId": "Surface",
        "fillId": "Fill",
        "contourLineTypeId": "Line",
        "cutEndLineTypeId": "Line",
    }
    new_skins = []
    notes = []
    for skin in skins:
        skin_label = skin.get("skinId", "?")
        outline_coords = skin.get("outlineCoords") or []
        outline_subpoly_ends = skin.get("outlineSubPolyEnds") or []
        if len(outline_coords) < 4 or len(outline_subpoly_ends) < 2:
            notes.append(f"skin {skin_label}: no usable geometry (was skinOutlines requested?), skipped")
            continue
        contours = build_contours_from_outline(outline_coords, outline_subpoly_ends, skin.get("outlineArcs") or [])

        payload = {"contours": contours}
        unresolved = False
        for field, target_type in ref_fields.items():
            if field not in skin:
                continue
            ref_name = resolve_ref_value(skin[field], "id", target_type, source_data)
            if ref_name is None:
                continue  # source side itself doesn't resolve (unusual) - just omit, not fatal
            attr_id = target_data.attr_id_by_name.get(target_type, {}).get(ref_name)
            if attr_id is None:
                notes.append(f"skin {skin_label}: {field} '{ref_name}' not found in target project, skin skipped")
                unresolved = True
                break
            payload[field] = {"attributeId": attr_id}
        if unresolved:
            continue

        for scalar_field in ("contourPen", "isCore", "isFinish", "visibleCutEndLines", "cutEndLinePen"):
            if scalar_field in skin:
                payload[scalar_field] = skin[scalar_field]

        edge_overrides = []
        for i, edge in enumerate(skin.get("edges", [])):
            override = {"edgeIndex": i}
            if "pen" in edge:
                override["pen"] = edge["pen"]
            if "isVisibleLine" in edge:
                override["isVisibleLine"] = edge["isVisibleLine"]
            for edge_ref_field, edge_ref_type in (("lineTypeId", "Line"), ("buildingMaterialId", "BuildingMaterial")):
                if edge_ref_field not in edge:
                    continue
                ref_name = resolve_ref_value(edge[edge_ref_field], "id", edge_ref_type, source_data)
                if ref_name is None:
                    notes.append(f"skin {skin_label}: edge {i} {edge_ref_field} did not resolve on the source side, skipped for this edge")
                    continue
                attr_id = target_data.attr_id_by_name.get(edge_ref_type, {}).get(ref_name)
                if attr_id is None:
                    notes.append(f"skin {skin_label}: edge {i} {edge_ref_field} '{ref_name}' not found in target project, skipped for this edge")
                    continue
                override[edge_ref_field] = {"attributeId": attr_id}
            edge_overrides.append(override)
        if edge_overrides:
            payload["edgeOverrides"] = edge_overrides

        new_skins.append(payload)

    return (len(new_skins) > 0), new_skins, notes


def build_layers_payload(layers, source_data, target_data):
    """Remap a LayerCombination's "layers" list from the SOURCE project's Layer guids to the TARGET
    project's own guids, matched by Layer name (guids are project-local and never match across two
    separate project files). A layer with no same-named counterpart in the target project is
    dropped (flagged in notes), not auto-created - consistent with this tool's general "flag and
    skip unresolvable dependencies" policy."""
    remapped = []
    notes = []
    for layer in layers or []:
        layer_name = resolve_ref_value(layer, "id", "Layer", source_data)
        if layer_name is None:
            notes.append("layers: a source layer reference did not resolve to a name, skipped")
            continue
        target_attr_id = target_data.attr_id_by_name.get("Layer", {}).get(layer_name)
        if target_attr_id is None:
            notes.append(f"layers: layer '{layer_name}' not found in target project, skipped")
            continue
        entry = {k: v for k, v in layer.items() if k != "attributeId"}
        entry["attributeId"] = target_attr_id
        remapped.append(entry)
    return remapped, notes


def copy_whole_attribute(target_instance, target_data, attr_type, name, raw_fields, source_data):
    """Used for 'only in A -> create in B' (or vice versa) AND for a "different" Profile whose skins
    differ: copies every field, resolving references against the target project (skipping
    unresolvable ones), and for Profile rebuilds its geometry via newSkins+replaceSkins since
    sourceAttributeId only resolves within a single project."""
    normalized, opaque = normalize_fields(attr_type, raw_fields, source_data)
    type_def = ATTRIBUTE_TYPES[attr_type]
    for readonly_field in type_def.get("readonly_fields", []):
        normalized.pop(readonly_field, None)
    all_fields = dict(normalized)
    extra_notes = []
    for field, value in opaque.items():
        if attr_type == "Profile" and field == "skins":
            built_any, new_skins, notes = build_new_skins_payload(value, source_data, target_data)
            extra_notes.extend(notes)
            if built_any:
                all_fields["newSkins"] = new_skins
                all_fields["replaceSkins"] = True
            continue
        if attr_type == "LayerCombination" and field == "layers":
            remapped, notes = build_layers_payload(value, source_data, target_data)
            extra_notes.extend(notes)
            all_fields["layers"] = remapped
            continue
        all_fields[field] = value
    ok, err, skipped = apply_field_syncs(target_instance, target_data, attr_type, name, all_fields)
    return ok, err, skipped + extra_notes


# ---------------------------------------------------------------------------------------------
# GUI
# ---------------------------------------------------------------------------------------------
class SyncApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Tapir Attribute Sync")
        self.geometry("1100x700")
        self._set_app_icon()

        self.instances = []
        self.instance_a = None
        self.instance_b = None
        self.data_a = None
        self.data_b = None
        self.all_diffs = {}
        self.index_mismatches = {}
        self.row_choice = {}  # tree item id -> tk.StringVar ("skip" | "a_to_b" | "b_to_a")

        self._build_header()
        self._build_toolbar()
        self._build_tree()
        self._build_statusbar()

        self.after(100, self.on_discover)

    def _set_app_icon(self):
        # iconphoto (unlike iconbitmap) accepts an in-memory PhotoImage directly, so the embedded
        # PNG data can be used as-is - no temp file needed. Wrapped since it's cosmetic, never
        # worth crashing the whole tool over an icon failing to apply.
        try:
            self._icon_image = tk.PhotoImage(data=TAPIR_LOGO_PNG_B64)
            self.iconphoto(True, self._icon_image)
        except Exception:
            pass

    def _build_header(self):
        header = ttk.Frame(self)
        header.pack(side=tk.TOP, fill=tk.X, padx=6, pady=(6, 0))
        try:
            self._logo_image = tk.PhotoImage(data=TAPIR_LOGO_PNG_B64).subsample(4, 4)  # 128x128 -> 32x32
            ttk.Label(header, image=self._logo_image).pack(side=tk.LEFT, padx=(0, 8))
        except Exception:
            pass
        ttk.Label(header, text="Tapir Attribute Sync", font=("", 14, "bold")).pack(side=tk.LEFT)

    def _build_toolbar(self):
        bar = ttk.Frame(self)
        bar.pack(side=tk.TOP, fill=tk.X, padx=6, pady=6)

        ttk.Button(bar, text="Rescan Instances", command=self.on_discover).pack(side=tk.LEFT)

        ttk.Label(bar, text="  A:").pack(side=tk.LEFT)
        self.combo_a = ttk.Combobox(bar, state="readonly", width=30)
        self.combo_a.pack(side=tk.LEFT, padx=(0, 8))

        ttk.Label(bar, text="B:").pack(side=tk.LEFT)
        self.combo_b = ttk.Combobox(bar, state="readonly", width=30)
        self.combo_b.pack(side=tk.LEFT, padx=(0, 8))

        ttk.Button(bar, text="Compare", command=self.on_compare).pack(side=tk.LEFT, padx=(8, 0))

        ttk.Button(bar, text="All A → B", command=lambda: self._apply_direction_to_all("a_to_b")).pack(side=tk.LEFT, padx=(16, 0))
        ttk.Button(bar, text="Keep All", command=lambda: self._apply_direction_to_all("skip")).pack(side=tk.LEFT, padx=(4, 0))
        ttk.Button(bar, text="All A ← B", command=lambda: self._apply_direction_to_all("b_to_a")).pack(side=tk.LEFT, padx=(4, 0))

        ttk.Button(bar, text="Apply Selected Syncs", command=self.on_apply).pack(side=tk.RIGHT)

    def _build_tree(self):
        # Column order/behavior mirrors GoodSync: [tree: type/attribute/field] [L value] [<-] [o] [->] [R value]
        # [status]. L = A, R = B. Clicking <- marks "copy R into L" (B -> A); clicking -> marks "copy L into R"
        # (A -> B); clicking o resets to no action. Every row starts at "o" (no action) by default.
        columns = ("value_l", "sync_l", "sync_mid", "sync_r", "value_r", "status")
        self.tree = ttk.Treeview(self, columns=columns, show="tree headings", selectmode="browse")
        self.tree.heading("#0", text="Type / Attribute / Field")
        self.tree.heading("value_l", text="L (A)")
        self.tree.heading("sync_l", text="")
        self.tree.heading("sync_mid", text="")
        self.tree.heading("sync_r", text="")
        self.tree.heading("value_r", text="R (B)")
        self.tree.heading("status", text="Status")
        self.tree.column("#0", width=260)
        self.tree.column("value_l", width=260)
        self.tree.column("sync_l", width=28, anchor="center")
        self.tree.column("sync_mid", width=28, anchor="center")
        self.tree.column("sync_r", width=28, anchor="center")
        self.tree.column("value_r", width=260)
        self.tree.column("status", width=90)
        self.tree.pack(fill=tk.BOTH, expand=True, padx=6, pady=(0, 6))
        self.tree.bind("<Button-1>", self.on_tree_click)

        # Row background tint reflects the pending action - the strong at-a-glance cue ttk.Treeview can't give
        # per-cell (tag_configure colors the whole row, not one column), so lean into that instead of fighting it.
        self.tree.tag_configure("pending_a_to_b", background="#dff5df")   # light green: A will overwrite B
        self.tree.tag_configure("pending_b_to_a", background="#dde8fb")   # light blue: B will overwrite A

    SYNC_ICONS = {
        "skip":    ("◁", "●", "▷"),  # hollow-left, filled-dot, hollow-right
        "b_to_a":  ("◀", "○", "▷"),  # filled-left, hollow-dot, hollow-right
        "a_to_b":  ("◁", "○", "▶"),  # hollow-left, hollow-dot, filled-right
    }
    ROW_TAGS = {"skip": (), "b_to_a": ("pending_b_to_a",), "a_to_b": ("pending_a_to_b",)}

    def _set_row_icons(self, item, direction):
        icons = self.SYNC_ICONS[direction]
        self.tree.set(item, "sync_l", icons[0])
        self.tree.set(item, "sync_mid", icons[1])
        self.tree.set(item, "sync_r", icons[2])
        self.tree.item(item, tags=self.ROW_TAGS[direction])

    @staticmethod
    def _status_text_for_direction(kind, d, direction):
        """What clicking a direction will actually DO to this row, in plain terms. For "whole"
        (content sync) rows: not just "different"/"only A", but the concrete outcome - create it on
        the empty side, copy the attribute's content across, or delete it (when the side chosen as
        the winner doesn't have the attribute at all - "not present" wins by deleting the other
        side's copy). For "index" (reindex) rows: which side's index number will move to match the
        other."""
        if kind == "index":
            if direction == "skip":
                return f"index differs (A={d.index_a}, B={d.index_b})"
            if direction == "a_to_b":
                return f"Move to index {d.index_a} in B"
            return f"Move to index {d.index_b} in A"
        if direction == "skip":
            return {"only_a": "only A", "only_b": "only B", "different": "different"}[d.status]
        if direction == "a_to_b":
            if d.status == "only_a":
                return "Create in B"
            if d.status == "only_b":
                return "Delete attribute"
            return "Copy A → B"
        else:  # b_to_a
            if d.status == "only_b":
                return "Create in A"
            if d.status == "only_a":
                return "Delete attribute"
            return "Copy B → A"

    def _apply_direction(self, item, direction):
        kind, d, var = self.row_choice[item]
        var.set(direction)
        self._set_row_icons(item, direction)
        self.tree.set(item, "status", self._status_text_for_direction(kind, d, direction))

    def _apply_direction_to_all(self, direction):
        # Covers every actionable row uniformly - content diffs AND index mismatches, since both
        # live in the same self.row_choice now (per explicit user feedback: index mismatches need
        # their own per-row L/●/R choice like everything else, not a separate bulk-only mechanism).
        for item in self.row_choice:
            self._apply_direction(item, direction)

    def _build_statusbar(self):
        self.status_var = tk.StringVar(value="Ready.")
        ttk.Label(self, textvariable=self.status_var, anchor="w").pack(side=tk.BOTTOM, fill=tk.X, padx=6, pady=(0, 4))

    def set_status(self, text):
        self.status_var.set(text)
        self.update_idletasks()

    # -- discovery --------------------------------------------------------------------------
    def on_discover(self):
        self.set_status("Scanning ports 19723-19732 for open Archicad instances...")
        self.instances = discover_instances()
        labels = [f"{inst.project_name} ({inst.label})" for inst in self.instances]
        self.combo_a["values"] = labels
        self.combo_b["values"] = labels
        if len(self.instances) >= 2:
            self.combo_a.current(0)
            self.combo_b.current(1)
        elif len(self.instances) == 1:
            self.combo_a.current(0)
        self.set_status(f"Found {len(self.instances)} instance(s).")

    # -- compare ------------------------------------------------------------------------------
    def on_compare(self):
        ia, ib = self.combo_a.current(), self.combo_b.current()
        if ia < 0 or ib < 0 or ia == ib:
            messagebox.showwarning("Select two instances", "Pick two different Archicad instances for A and B.")
            return
        self.instance_a = self.instances[ia]
        self.instance_b = self.instances[ib]

        try:
            self.set_status(f"Fetching attributes from A ({self.instance_a.label})...")
            self.data_a = fetch_project_data(self.instance_a, self.set_status)
            self.set_status(f"Fetching attributes from B ({self.instance_b.label})...")
            self.data_b = fetch_project_data(self.instance_b, self.set_status)
        except ArchicadCommError as exc:
            self.set_status("Fetch failed.")
            messagebox.showerror("Fetch failed", f"Could not fetch attributes: {exc}\n\nThe instance may have become busy or unresponsive. Try Rescan Instances, then Compare again.")
            return

        self.set_status("Comparing...")
        self.all_diffs = diff_all(self.data_a, self.data_b)
        self.index_mismatches = {attr_type: find_index_mismatches(attr_type, self.data_a, self.data_b)
                                  for attr_type in ATTRIBUTE_TYPES}
        self._populate_tree()
        n_diff = sum(1 for diffs in self.all_diffs.values() for d in diffs if d.status != "identical")
        n_index_mismatch = sum(len(m) for m in self.index_mismatches.values())
        msg = f"Done. {n_diff} attribute(s) differ or exist on only one side."
        if n_index_mismatch:
            msg += f" {n_index_mismatch} index mismatch(es) found (see 'Index Mismatches')."
        self.set_status(msg)

    def _insert_row(self, parent, text, value_l, value_r, status, entry_without_var):
        item = self.tree.insert(parent, "end", text=text, values=(value_l, "", "", "", value_r, status))
        var = tk.StringVar(value="skip")
        self._set_row_icons(item, "skip")
        self.row_choice[item] = entry_without_var + (var,)
        return item

    def _populate_tree(self):
        self.tree.delete(*self.tree.get_children())
        self.row_choice = {}

        # Index mismatches: each same-named attribute sitting at a different index in A vs B gets
        # its own actionable row (L/●/R, exactly like a content diff) - shown up front since a
        # mismatch here can silently break an index-based reference (e.g.
        # BuildingMaterial.cutFillIndex) even when every name-based diff below looks clean.
        total_mismatches = sum(len(m) for m in self.index_mismatches.values())
        if total_mismatches:
            warn_node = self.tree.insert("", "end", text=f"Index Mismatches ({total_mismatches})", open=True)
            for attr_type, mismatches in self.index_mismatches.items():
                if not mismatches:
                    continue
                type_node = self.tree.insert(warn_node, "end", text=attr_type, open=True)
                for m in mismatches:
                    self._insert_row(type_node, m.name, str(m.index_a), str(m.index_b),
                                      f"index differs (A={m.index_a}, B={m.index_b})", ("index", m))

        for attr_type, diffs in self.all_diffs.items():
            interesting = [d for d in diffs if d.status != "identical"]
            if not interesting:
                continue
            type_node = self.tree.insert("", "end", text=f"{attr_type} ({len(interesting)})", open=True)

            for d in interesting:
                if d.status in ("only_a", "only_b"):
                    value_l = "(exists)" if d.status == "only_a" else "(not present)"
                    value_r = "(not present)" if d.status == "only_a" else "(exists)"
                    self._insert_row(type_node, d.name, value_l, value_r, "only A" if d.status == "only_a" else "only B",
                                      ("whole", d))
                    continue

                # Sync acts on the WHOLE attribute (a -> b or b -> a copies every field at once) - never on a
                # single field in isolation. The field rows below are read-only detail, shown to help decide
                # which direction makes sense, not individually actionable.
                attr_node = self._insert_row(type_node, d.name, "(differs)", "(differs)", "different", ("whole", d))
                if d.opaque_differs:
                    detail_field = "skins" if attr_type == "Profile" else "layers"
                    detail_note = "will be rebuilt via newSkins+replaceSkins" if attr_type == "Profile" else "not auto-synced"
                    self.tree.insert(attr_node, "end", text=detail_field, values=("(differs)", "", "", "", "(differs)", detail_note))
                for fd in d.field_diffs:
                    self.tree.insert(attr_node, "end", text=fd.field, values=(_fmt(fd.value_a), "", "", "", _fmt(fd.value_b), ""))
                for fd in d.readonly_field_diffs:
                    self.tree.insert(attr_node, "end", text=fd.field, values=(_fmt(fd.value_a), "", "", "", _fmt(fd.value_b), "read-only, not synced"))
                if d.unresolved_refs:
                    for note in d.unresolved_refs:
                        self.tree.insert(attr_node, "end", text="!", values=(note, "", "", "", "", ""))

    def on_tree_click(self, event):
        region = self.tree.identify_region(event.x, event.y)
        if region != "cell":
            return
        col = self.tree.identify_column(event.x)
        direction = {"#2": "b_to_a", "#3": "skip", "#4": "a_to_b"}.get(col)
        if direction is None:
            return
        item = self.tree.identify_row(event.y)
        if item not in self.row_choice:
            return
        self._apply_direction(item, direction)

    def _resolve_action(self, entry):
        """Resolves one row_choice entry into what actually needs to happen. Returns a tuple whose
        first element is always the action kind:
          - ("copy", d, source_data, target_data, target_instance) - "whole" row, source has the
            attribute, create/overwrite it on target.
          - ("delete", d, source_data, target_data, target_instance) - "whole" row, source does NOT
            have it ("not present" was chosen as the winning side), delete target's copy to match.
          - ("reindex", m, target_data, target_instance, target_index) - "index" row, move the
            attribute to target_index on target_instance (see reindex_attribute)."""
        row_kind, obj, var = entry
        direction = var.get()
        if row_kind == "index":
            if direction == "a_to_b":
                return "reindex", obj, self.data_b, self.instance_b, obj.index_a
            else:
                return "reindex", obj, self.data_a, self.instance_a, obj.index_b

        if direction == "a_to_b":
            source_data, target_data, target_instance = self.data_a, self.data_b, self.instance_b
        else:
            source_data, target_data, target_instance = self.data_b, self.data_a, self.instance_a
        source_has = obj.name in source_data.by_type.get(obj.attr_type, {})
        kind = "copy" if source_has else "delete"
        return kind, obj, source_data, target_data, target_instance

    def _build_reindex_groups(self, resolved):
        """Group every "reindex" action by (attr_type, direction) and close each group over its
        dependencies - if a selected move's target index is currently occupied, in the target
        project, by ANOTHER attribute that ALSO has a pending index mismatch (same type, same
        direction) but wasn't itself selected, that occupant is pulled into the SAME group using ITS
        OWN correct target index. Without this, fixing only one side of a swap would overwrite the
        other's identity in the target project with no way back (confirmed live) - the user should
        only have to click one side of a swap/cycle, not know to click every member of it. Occupants
        that aren't a pending mismatch are left alone (a normal occupied-target move, not a
        dependency). Returns (groups, auto_included) where groups maps
        (attr_type, direction) -> (target_instance, target_data, [(name, target_index), ...]) and
        auto_included is a list of "attr_type/name" strings pulled in this way, for the confirm
        dialog."""
        reindex_groups = {}
        for action in resolved:
            if action[0] != "reindex":
                continue
            _, m, target_data, target_instance, target_index = action
            direction = "a_to_b" if target_instance is self.instance_b else "b_to_a"
            key = (m.attr_type, direction)
            if key not in reindex_groups:
                reindex_groups[key] = (target_instance, target_data, {})
            reindex_groups[key][2][m.name] = target_index

        auto_included = []
        for (attr_type, direction), (target_instance, target_data, moves_by_name) in reindex_groups.items():
            mismatches_by_name = {m.name: m for m in self.index_mismatches.get(attr_type, [])}
            target_index_map = target_data.index_map.get(attr_type, {})
            frontier = list(moves_by_name.keys())
            while frontier:
                name = frontier.pop()
                displaced_name = target_index_map.get(moves_by_name[name])
                if displaced_name is None or displaced_name in moves_by_name:
                    continue
                displaced_mismatch = mismatches_by_name.get(displaced_name)
                if displaced_mismatch is None:
                    continue  # occupant isn't a pending mismatch - a normal occupied-target overwrite, not a dependency
                displaced_target = displaced_mismatch.index_a if direction == "a_to_b" else displaced_mismatch.index_b
                moves_by_name[displaced_name] = displaced_target
                auto_included.append(f"{attr_type}/{displaced_name}")
                frontier.append(displaced_name)

        groups = {key: (ti, td, list(moves.items())) for key, (ti, td, moves) in reindex_groups.items()}
        return groups, auto_included

    # -- apply --------------------------------------------------------------------------------
    def on_apply(self):
        if not self.all_diffs:
            return
        actions = [entry for entry in self.row_choice.values() if entry[-1].get() != "skip"]
        if not actions:
            messagebox.showinfo("Nothing to do", "No sync direction was chosen. Click All A → B / All A ← B, or an individual row's arrow.")
            return

        resolved = [self._resolve_action(entry) for entry in actions]
        reindex_groups, auto_included = self._build_reindex_groups(resolved)
        n_delete = sum(1 for r in resolved if r[0] == "delete")
        n_reindex = sum(len(moves) for _, _, moves in reindex_groups.values())
        n_content = sum(1 for r in resolved if r[0] != "reindex")
        msg = f"Apply {n_content} content change(s) and {n_reindex} index move(s) now? This writes directly into the target Archicad project(s)."
        if n_delete:
            msg += f"\n\n{n_delete} of these will DELETE an attribute (the side you picked as the winner doesn't have it - deleting the other side's copy is how 'not present' syncs)."
        if auto_included:
            msg += (f"\n\n{len(auto_included)} more attribute(s) will be auto-included in the index moves "
                     f"(they'd otherwise be overwritten by a move you selected): " + ", ".join(auto_included[:10]))
            if len(auto_included) > 10:
                msg += f", and {len(auto_included) - 10} more."
        if not messagebox.askyesno("Confirm sync", msg):
            return

        errors = []
        applied = 0

        for (attr_type, direction), (target_instance, target_data, moves) in reindex_groups.items():
            results = reindex_attributes_bulk(target_instance, target_data, attr_type, moves)
            for name, target_index, ok, err in results:
                if ok:
                    applied += 1
                    if err:  # non-fatal note (e.g. leftover placeholder cleanup issue)
                        errors.append(f"{attr_type}/{name} -> index {target_index}: {err}")
                else:
                    errors.append(f"{attr_type}/{name} -> index {target_index}: {err}")

        for action in resolved:
            kind = action[0]
            if kind == "reindex":
                continue  # handled above, grouped

            _, d, source_data, target_data, target_instance = action
            if kind == "delete":
                target_attr_id = target_data.attr_id_by_name.get(d.attr_type, {}).get(d.name)
                if target_attr_id is None:
                    applied += 1  # already absent on both sides - nothing to do
                    continue
                try:
                    result = target_instance.run_tapir_command("DeleteAttributes", {"attributesToDelete": [
                        {"attributeType": d.attr_type, "attributeId": {"attributeId": target_attr_id}}
                    ]})
                except ArchicadCommError as exc:
                    errors.append(f"{d.attr_type}/{d.name}: delete failed: communication error: {exc}")
                    continue
                exec_results = (result or {}).get("executionResults", [])
                if exec_results and exec_results[0].get("success"):
                    applied += 1
                else:
                    err_msg = exec_results[0].get("error", {}).get("message", "Unknown error") if exec_results else "Archicad rejected the request (e.g. the attribute is still in use by an element - delete refused)."
                    errors.append(f"{d.attr_type}/{d.name}: delete failed: {err_msg}")
                continue

            raw = source_data.by_type[d.attr_type][d.name]
            ok, err, skipped = copy_whole_attribute(target_instance, target_data, d.attr_type, d.name, raw, source_data)
            if not ok:
                errors.append(f"{d.attr_type}/{d.name}: {err}")
            else:
                applied += 1
                if skipped:
                    errors.append(f"{d.attr_type}/{d.name}: skipped unresolvable field(s) {skipped}")

        summary = f"Applied {applied}/{n_content + n_reindex} change(s)."
        if errors:
            summary += f"\n\n{len(errors)} issue(s):\n" + "\n".join(errors[:20])
            if len(errors) > 20:
                summary += f"\n... and {len(errors) - 20} more."
        messagebox.showinfo("Sync complete", summary)
        self.on_compare()  # refresh diff view against the new state


def _fmt(value):
    value = unwrap_ref_for_sync(value)
    if isinstance(value, (dict, list)):
        text = json.dumps(value, ensure_ascii=False)
        return text if len(text) <= 80 else text[:77] + "..."
    return str(value)


# ---------------------------------------------------------------------------------------------
# Embedded Tapir logo (128x128 PNG, base64) - kept as a single self-contained script instead of
# shipping companion .png/.ico files alongside it. Only used cosmetically by _set_app_icon() /
# _build_header() above; safe to delete this constant and those two call sites if ever unwanted.
# ---------------------------------------------------------------------------------------------
TAPIR_LOGO_PNG_B64 = (
    "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"
)


if __name__ == "__main__":
    app = SyncApp()
    app.mainloop()
