# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# category = "documentation-layouts"
# archicad-versions = [27, 28, 29, 30]
# min-tapir-version = "2.0.0"
# changes = "model"
# ///

"""
Relink Drawing, Keep Placement

Relinks the Drawing selected on a layout to another view, and keeps it exactly where it was on
the sheet: same displayed scale, same orientation, same frame. Example: show a plan from a 1:200
view instead of a 1:500 view, without moving or re-cropping the Drawing afterwards.

How it works. A placed Drawing maps a model point m to the sheet as

    sheet = pos + s * R(alpha) * (A * m + T - modelOffset)

    A, T   rotation and translation of the source view (GetViewSettings transformationMatrix:
           A = [[t0 t1] [t4 t5]], T = (t3, t7))
    s      displayed scale = drawingScale of the Drawing * ratio (magnification)
    alpha  angle of the Drawing on the layout

To keep the Drawing identical when it shows another view, the model -> sheet mapping must not
change, which gives the new Drawing's settings directly (nothing is measured):

    ratio2 = s1 / drawingScale2
    alpha2 = alpha1 + theta1 - theta2                 (theta = rotation of the view, read from A)
    pos2   = pos1 + s1*R(alpha1)*(T1 - mo1) - s1*R(alpha2)*(T2 - mo2)

and the frame (clip polygon) is reused as it is, in sheet coordinates, so the zoom of the two
views never matters. A Drawing that is not cropped gets its previous full frame as its clip.

A Drawing can only be relinked to a view of the same family, i.e. with the same 2D coordinates:
plans, worksheets and details on one side; sections and elevations on the other.

Before each relink the exact state of the Drawing (view, magnification, angle, position, frame) is
written to Documents/Tapir/relink-drawing-history.json, so that "Back to ..." can relink it to
its previous view, exactly as it was, in one click (a relink is several Tapir commands, so
Archicad's Undo would revert it one step at a time).

The window follows the selection in Archicad: select a Drawing on a layout, pick the target view
in the View Map tree, press Relink (or double-click the view).
"""

import argparse
import json
import math
import os
import sys
import time
import traceback
import urllib.request
from pathlib import Path

SCRIPT_ID = os.environ.get("TAPIR_SCRIPT_ID") or "relink-drawing-keep-placement"
HISTORY_FILE = Path.home() / "Documents" / "Tapir" / "relink-drawing-history.json"
POLL = 0.3            # seconds between two reads of the window's result
SELECTION_EVERY = 3   # the selection is read every SELECTION_EVERY loops (about 1 s)
EPS_ORTHO = 1e-6


# ---------------------------------------------------------------- Archicad / Tapir
class Api:
    def __init__(self, port, busy_retries=6):
        self.port = port
        self.busy_retries = busy_retries      # about 10 s in all while Archicad stays busy

    def _post(self, body, timeout):
        req = urllib.request.Request("http://127.0.0.1:%d" % self.port, json.dumps(body).encode(),
                                     {"Content-Type": "application/json"})
        with urllib.request.urlopen(req, timeout=timeout) as r:
            return json.loads(r.read())

    def ac(self, cmd, params=None, timeout=30):
        body = {"command": cmd}
        if params is not None:
            body["parameters"] = params
        # "Invalid program status" (4001): Archicad is busy for a moment (right after a relink, or
        # while the user is drawing); the command was not run, so it can be sent again.
        for attempt in range(self.busy_retries + 1):
            r = self._post(body, timeout)
            if r.get("succeeded"):
                return r.get("result") or {}
            msg = (r.get("error") or {}).get("message") or str(r)
            if "Invalid program status" not in msg or attempt == self.busy_retries:
                raise RuntimeError("%s: %s" % (cmd, msg))
            time.sleep(0.5 + 0.5 * attempt)

    def t(self, cmd, params=None, timeout=60):
        p = {"addOnCommandId": {"commandNamespace": "TapirCommand", "commandName": cmd}}
        if params is not None:
            p["addOnCommandParameters"] = params
        res = self.ac("API.ExecuteAddOnCommand", p, timeout).get("addOnCommandResponse") or {}
        if isinstance(res, dict) and "error" in res and len(res) == 1:
            raise RuntimeError("%s: %s" % (cmd, res["error"].get("message")))
        return res


def find_port():
    """The port the Tapir palette passes (--port), else the first Archicad found."""
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int)
    args, _ = ap.parse_known_args()
    if args.port:
        return args.port
    for port in range(19723, 19743):
        try:
            Api(port, busy_retries=0).t("GetProjectInfo", timeout=3)
            return port
        except Exception:
            pass
    return None


def ok(res):
    return all(x.get("success") for x in (res.get("executionResults") or [{}]))


def friendly(e):
    m = str(e)
    if "open modal dialog" in m or "ongoing user input" in m or "Invalid program status" in m:
        return "Archicad is busy (a dialog is open or an input is in progress): press Esc, then try again."
    if "timed out" in m:
        return "Archicad does not answer (timeout)."
    return m


# ---------------------------------------------------------------- geometry
def rot(a, v):
    c, s = math.cos(a), math.sin(a)
    return (v[0] * c - v[1] * s, v[0] * s + v[1] * c)


def view_transform(vs):
    """(theta, T) of a view; refuses a transformation that is not a pure rotation."""
    t = vs.get("transformationMatrix") or []
    if len(t) < 8:
        return 0.0, (0.0, 0.0)
    a, b, d, e = t[0], t[1], t[4], t[5]
    if abs(a - e) > EPS_ORTHO or abs(b + d) > EPS_ORTHO or abs(a * a + d * d - 1) > EPS_ORTHO:
        raise RuntimeError("View transformation not supported (mirror or scale): %s" % t[:8])
    return math.atan2(d, a), (t[3], t[7])


def to_sheet(m, pos, s, alpha, theta, T, mo):
    """Model point -> sheet, with the mapping given at the top of this file."""
    v = rot(theta, m)
    v = (v[0] + T[0] - mo[0], v[1] + T[1] - mo[1])
    w = rot(alpha, (v[0] * s, v[1] * s))
    return (pos[0] + w[0], pos[1] + w[1])


def view_to_sheet(v, pos, s, alpha, mo):
    w = rot(alpha, ((v[0] - mo[0]) * s, (v[1] - mo[1]) * s))
    return (pos[0] + w[0], pos[1] + w[1])


def norm_angle(a):
    return math.atan2(math.sin(a), math.cos(a))


def xy(p):
    return (p["x"], p["y"])


# ---------------------------------------------------------------- reading the project
def walk(node, parents=()):
    it = node.get("navigatorItem", node)
    yield it, parents
    for c in node.get("children") or it.get("children") or []:
        yield from walk(c, parents + (it,))


def item_label(it):
    return ("%s %s" % (it.get("prefix") or "", it.get("name") or "")).strip()


class Notice(Exception):
    """A message for the user (nothing to do, or nothing fits), not an error."""


def selected_drawing(api):
    sel = api.t("GetSelectedElements").get("elements") or []
    if not sel:
        raise Notice("Select a Drawing on a layout.")
    det = api.t("GetDetailsOfElements", {"elements": sel})["detailsOfElements"]
    drawings = [(e, d) for e, d in zip(sel, det) if d.get("type") == "Drawing"]
    if len(drawings) != 1:
        raise Notice("Select a single Drawing." if drawings else
                     "The selection holds no Drawing: select a Drawing.")
    return drawings[0]


def current_layout(api):
    w = api.t("GetCurrentWindowType")
    if w.get("currentWindowType") != "Layout":
        raise Notice("Open a layout and select a Drawing on it.")
    return w["databaseId"]


def layout_drawing_items(api, layout_db):
    """The active layout's item and its DrawingItems, from the Layout Book tree."""
    tree = api.ac("API.GetNavigatorItemTree", {"navigatorTreeId": {"type": "LayoutBook"}})
    layouts = [it for it, _ in walk(tree["navigatorTree"]["rootItem"]) if it.get("type") == "LayoutItem"]
    dbs = api.t("GetDatabaseIdFromNavigatorItemId", {
        "navigatorItemIds": [{"navigatorItemId": it["navigatorItemId"]} for it in layouts]})["databases"]
    for it, db in zip(layouts, dbs):
        if (db.get("databaseId") or {}).get("guid") == layout_db["guid"]:
            return it, [c.get("navigatorItem", c) for c in (it.get("children") or [])
                        if c.get("navigatorItem", c).get("type") == "DrawingItem"]
    raise Notice("The active layout was not found in the Layout Book.")


def view_settings(api, guids):
    if not guids:
        return []
    return api.t("GetViewSettings", {"navigatorItemIds": [{"navigatorItemId": {"guid": g}} for g in guids]})[
        "viewSettings"]


# View families: a Drawing is relinked only to a view with the same 2D coordinates.
GROUPS = {"StoryItem": "plan", "WorksheetItem": "plan", "DetailItem": "plan",
          "WorksheetDrawingItem": "plan", "DetailDrawingItem": "plan",
          "SectionItem": "section", "ElevationItem": "section", "InteriorElevationItem": "section"}
GROUP_LABEL = {"plan": "plans, worksheets and details", "section": "sections and elevations"}
TYPE_LABEL = {"StoryItem": "plan", "WorksheetItem": "worksheet", "DetailItem": "detail",
              "WorksheetDrawingItem": "worksheet", "DetailDrawingItem": "detail",
              "SectionItem": "section", "ElevationItem": "elevation", "InteriorElevationItem": "interior elevation"}


def nav_types(api):
    """guid -> navigator item type (Project Map and View Map)."""
    out = {}
    for tid in ("ProjectMap", "ViewMap"):
        try:
            tree = api.ac("API.GetNavigatorItemTree", {"navigatorTreeId": {"type": tid}})
        except RuntimeError:
            continue
        for it, _ in walk(tree["navigatorTree"]["rootItem"]):
            g = (it.get("navigatorItemId") or {}).get("guid")
            if g:
                out[g.upper()] = it.get("type")
    return out


def view_tree(api):
    """The View Map in its order, folders included (even empty ones), as a list of rows.

    Folder: {"folder": True, "name", "depth", "key", "anc"}; view: guid, name, prefix, path, type,
    group, vs (its view settings), depth, anc (keys of its parent folders). The root and the
    project's own folder (depths 0 and 1) are left out."""
    tree = api.ac("API.GetNavigatorItemTree", {"navigatorTreeId": {"type": "ViewMap"}})
    nodes = list(walk(tree["navigatorTree"]["rootItem"]))
    views = [(it, par) for it, par in nodes if it.get("navigatorItemId") and it.get("type") in GROUPS]
    vs = dict(zip((id(it) for it, _ in views),
                  view_settings(api, [it["navigatorItemId"]["guid"] for it, _ in views])))
    rows = []
    for it, par in nodes:
        depth = len(par) - 2
        anc = [(p.get("navigatorItemId") or {}).get("guid") or str(id(p)) for p in par[2:]]
        if it.get("type") == "FolderItem" or (it.get("type") not in GROUPS and it.get("children")):
            if depth >= 0:
                rows.append({"folder": True, "name": it.get("name") or "", "depth": depth, "anc": anc,
                             "key": (it.get("navigatorItemId") or {}).get("guid") or str(id(it))})
            continue
        v = vs.get(id(it))
        if v is None or "error" in v or not v.get("transformationMatrix"):
            continue
        path = " / ".join(p.get("name") or "" for p in par[2:] if p.get("name"))
        rows.append({"guid": it["navigatorItemId"]["guid"], "name": it.get("name") or "",
                     "prefix": it.get("prefix") or "", "path": path, "type": it.get("type"),
                     "group": GROUPS[it["type"]], "vs": v, "depth": max(depth, 0), "anc": anc})
    return rows


def title_params(api, title_id):
    """GDL parameters of a Drawing title worth copying, and the drawing name it shows."""
    if not title_id:
        return [], None
    ps = (api.t("GetGDLParametersOfElements", {"elements": [{"elementId": title_id}]})
          .get("gdlParametersOfElements") or [{}])[0].get("parameters") or []
    name = next((p.get("value") for p in ps if p["name"] == "AC_DrawingName"), None)
    # Computed by Archicad for this very title (its drawing, name, number, scale, frames): not copied.
    skip = {"AC_DrawingGUID", "AC_DrawingName", "AC_DrawingNumber", "AC_DrawingScale",
            "AC_OriginalDrawingScale", "AC_Magnification", "AC_SourceFileName", "AC_SourceFilePath",
            "AC_PreviewType", "AC_ConvertedTitle", "AC_LayoutRect", "AC_GridRect", "AC_DrawingRect"}
    keep = [{"name": p["name"], "value": p["value"]} for p in ps
            if "value" in p and not isinstance(p["value"], list) and not p["name"].startswith("AC_BackReference")
            and p["name"] not in skip and "GLOB_" not in p["name"]]
    return keep, name


def element_details(api, elem_id):
    return api.t("GetDetailsOfElements", {"elements": [{"elementId": elem_id}]})["detailsOfElements"][0]


# ---------------------------------------------------------------- source view of the Drawing
class AmbiguousSource(Exception):
    """Several possible source views with different rotation or translation: the user picks one."""

    def __init__(self, candidates):
        super().__init__("Ambiguous source view: %s" % ", ".join(c["name"] for c in candidates))
        self.candidates = candidates          # [{"guid", "name"}]


def resolve_source(api, d, items, forced):
    """(theta1, T1, name, family, guid) of the view the Drawing shows.

    Tapir Add-Ons that return sourceNavigatorItemId on Drawings give it directly. Older ones do
    not (navigatorItemId is the Drawing's own link item, refused by GetViewSettings): the
    DrawingItems of the layout are then filtered by scale, then by the title's drawing name. Only
    theta and T matter, so candidates that share them are as good as one another."""
    forced = forced or (d["details"].get("sourceNavigatorItemId") or {}).get("guid")
    if forced:
        v = view_settings(api, [forced])[0]
        if "error" in v:
            raise Notice("The source view cannot be read: %s" % v["error"].get("message"))
        th, T = view_transform(v)
        vm = api.ac("API.GetNavigatorItemTree", {"navigatorTreeId": {"type": "ViewMap"}})
        name = next((item_label(it) for it, _ in walk(vm["navigatorTree"]["rootItem"])
                     if (it.get("navigatorItemId") or {}).get("guid", "").upper() == forced.upper()), None) \
            or next((item_label(it) for it in items
                     if (it.get("sourceNavigatorItemId") or {}).get("guid", "").upper() == forced.upper()),
                    "source view")
        return th, T, name, GROUPS.get(nav_types(api).get(forced.upper())), forced
    dets = d["details"]
    denom = round(1.0 / dets["drawingScale"], 6)
    types = nav_types(api)
    group_of = lambda it: GROUPS.get(types.get(it["sourceNavigatorItemId"]["guid"].upper()))
    srcs = [it for it in items if it.get("sourceNavigatorItemId") and group_of(it)]
    vs = view_settings(api, [it["sourceNavigatorItemId"]["guid"] for it in srcs])
    cands = [(it, v) for it, v in zip(srcs, vs) if "error" not in v and v.get("transformationMatrix")
             and abs((v.get("drawingScale") or 0) - denom) < 1e-6]
    _, tname = title_params(api, dets.get("titleElementId"))
    if tname and len(cands) > 1:
        named = [(it, v) for it, v in cands if it.get("name") == tname]
        cands = named or cands
    if not cands:
        raise Notice("Source view not found: no plan, section or elevation view at 1:%g is placed on "
                     "this layout (a Drawing from an external file, a schedule or a 3D view?)." % denom)
    transforms = {}
    for it, v in cands:
        th, T = view_transform(v)
        transforms.setdefault((round(th % (2 * math.pi), 9), round(T[0], 6), round(T[1], 6)), []).append(it)
    groups = {group_of(it) for it, _ in cands}
    if len(transforms) > 1 or len(groups) > 1:
        seen = {}
        for it, _ in cands:
            seen.setdefault(it["sourceNavigatorItemId"]["guid"], item_label(it))
        raise AmbiguousSource([{"guid": g, "name": n} for g, n in seen.items()])
    _, its = next(iter(transforms.items()))
    th, T = view_transform(next(v for it, v in cands if it is its[0]))
    return (th, T, " | ".join(dict.fromkeys(item_label(it) for it in its)), groups.pop(),
            its[0]["sourceNavigatorItemId"]["guid"])


# ---------------------------------------------------------------- computing the relink
def old_frame(api, d):
    """Frame of the Drawing in sheet coordinates: its clip polygon, or its full frame."""
    dets = d["details"]
    if dets.get("isCutWithFrame") and dets.get("clipPolygon"):
        return [xy(p) for p in dets["clipPolygon"]]
    cb = api.t("GetDrawingContentBoxes", {"elements": [{"elementId": d["_id"]}]})["contentBoxes"][0]
    if "error" in cb:
        raise Notice("The frame of the Drawing cannot be read: %s" % cb["error"].get("message"))
    b = cb["contentBox"]
    s = dets["drawingScale"] * dets["ratio"]
    pos, al, mo = xy(dets["pos"]), dets.get("angle") or 0.0, xy(dets["modelOffset"])
    return [view_to_sheet(v, pos, s, al, mo) for v in
            ((b["xMin"], b["yMin"]), (b["xMax"], b["yMin"]), (b["xMax"], b["yMax"]), (b["xMin"], b["yMax"]))]


def plan(d, th1, T1, target_vs):
    dets = d["details"]
    s1 = dets["drawingScale"] * dets["ratio"]
    a1 = dets.get("angle") or 0.0
    th2, T2 = view_transform(target_vs)
    ds2 = 1.0 / target_vs["drawingScale"]
    return {"s": s1, "ratio": s1 / ds2, "ds2": ds2, "angle": norm_angle(a1 + th1 - th2),
            "th1": th1, "T1": T1, "th2": th2, "T2": T2}


def pos_for(d, p, mo2):
    dets = d["details"]
    pos1, a1, mo1, s = xy(dets["pos"]), dets.get("angle") or 0.0, xy(dets["modelOffset"]), p["s"]
    u = rot(a1, ((p["T1"][0] - mo1[0]) * s, (p["T1"][1] - mo1[1]) * s))
    w = rot(p["angle"], ((p["T2"][0] - mo2[0]) * s, (p["T2"][1] - mo2[1]) * s))
    return (pos1[0] + u[0] - w[0], pos1[1] + u[1] - w[1])


def mapping_gap(d, p, pos2, mo2, s2, angle2):
    """Largest distance on the sheet (m) between where the old and the new Drawing put the same
    model points; s2 and angle2 are read back from the new Drawing."""
    dets = d["details"]
    s1 = dets["drawingScale"] * dets["ratio"]
    old = (xy(dets["pos"]), s1, dets.get("angle") or 0.0, p["th1"], p["T1"], xy(dets["modelOffset"]))
    new = (pos2, s2, angle2, p["th2"], p["T2"], mo2)
    pts = [(0, 0), (100, 0), (0, 100), (-250, 75), (1000, -500)]
    return max(math.dist(to_sheet(m, *old), to_sheet(m, *new)) for m in pts)


# ---------------------------------------------------------------- relinking
# Drawing settings the relink resets (it recreates the Drawing) and that are copied back.
COPY_FIELDS = ("nameType", "customName", "numberingType", "customNumber", "isInNumbering",
               "isScaleIncludedInAutotextAndIndex", "hasBorderLine", "borderLineTypeIndex", "borderPenIndex",
               "borderSize", "penTableUsageMode", "penTableIndex", "colorMode", "isTransparentBackground",
               "rasterizeDPI", "manualUpdate", "titleLibraryPartIndex", "anchorPoint", "useOwnOriginAsAnchor")


def relink(api, d, layout_db, target_guid, ratio, angle, expected_ds, pos_from_mo, frame):
    """Relinks Drawing d to target_guid, then gives it ratio, angle, pos (pos_from_mo(modelOffset of
    the new Drawing)) and the frame (sheet coordinates; None: not cropped). Copies the settings and
    the title back. Returns (new element id, warnings, details read back)."""
    dets = d["details"]
    tparams, _ = title_params(api, dets.get("titleElementId"))
    told = element_details(api, dets["titleElementId"])["details"] if dets.get("titleElementId") else None
    warnings = []

    rl = api.t("ChangeDrawingLink", {"drawingsWithNewLinks": [{
        "elementId": d["_id"], "navigatorItemId": {"guid": target_guid}, "layoutDatabaseId": layout_db}]})
    new_id = (((rl.get("elements") or [{}])[0]).get("elementId"))
    if not new_id:
        raise RuntimeError("ChangeDrawingLink: %s" % rl)
    elem = {"elementId": new_id}

    def set_details(td):
        r = api.t("SetDetailsOfElements", {"elementsWithDetails": [{**elem, "details": {"typeSpecificDetails": td}}]})
        if not ok(r):
            raise RuntimeError("SetDetailsOfElements %s: %s" % (list(td), r))

    set_details({k: dets[k] for k in COPY_FIELDS if k in dets})
    # Scale and angle first; uncrop and update before setting pos, then pos, then the clip polygon,
    # each in its own call.
    set_details({"ratio": ratio, "angle": angle})
    set_details({"isCutWithFrame": False, "clipPolygon": []})
    api.t("UpdateDrawings", {"elements": [elem]})
    nd = element_details(api, new_id)["details"]
    if abs(nd["drawingScale"] - expected_ds) > 1e-9:
        raise RuntimeError("drawingScale read back %.6g instead of %.6g (1:%g): stopped before setting pos"
                           % (nd["drawingScale"], expected_ds, 1 / expected_ds))
    pos2 = pos_from_mo(xy(nd["modelOffset"]))
    set_details({"pos": {"x": pos2[0], "y": pos2[1]}})
    if frame:
        set_details({"isCutWithFrame": True, "clipPolygon": [{"x": x, "y": y} for x, y in frame]})

    # Title: its parameters, then its previous position (a relink does not keep it).
    nd = element_details(api, new_id)["details"]
    tid = nd.get("titleElementId")
    if tid and tparams:
        r = api.t("SetGDLParametersOfElements", {"elementsWithGDLParameters": [
            {"elementId": tid, "gdlParameters": tparams}]})
        if not ok(r):
            warnings.append("Some parameters of the title could not be copied.")
    if told and tid:
        tn = element_details(api, tid)["details"]
        o1, o2 = told.get("origin") or {}, tn.get("origin") or {}
        dx, dy = o1.get("x", 0) - o2.get("x", 0), o1.get("y", 0) - o2.get("y", 0)
        if math.hypot(dx, dy) > 1e-7:
            api.t("MoveElements", {"elementsWithMoveVectors": [
                {"elementId": tid, "moveVector": {"x": dx, "y": dy, "z": 0}}]})
    elif told and not tid:
        warnings.append("The relinked Drawing has no title any more.")

    nd = element_details(api, new_id)["details"]
    if abs(nd["ratio"] - ratio) > 1e-9:
        warnings.append("Archicad refused the magnification (%.4g read back instead of %.4g)." % (nd["ratio"], ratio))
    return new_id, warnings, nd


def clip_gap(nd, frame):
    clip = [xy(q) for q in nd.get("clipPolygon") or []]
    if not frame:
        return 0.0 if not nd.get("isCutWithFrame") else float("inf")
    if len(clip) != len(frame):
        return float("inf")
    return max(math.dist(a, b) for a, b in zip(clip, frame))


def snapshot(d, source_guid):
    """Exact state of the Drawing before a relink, to go back to it."""
    x = d["details"]
    return {"source": source_guid, "drawingScale": x["drawingScale"], "ratio": x["ratio"],
            "angle": x.get("angle") or 0.0, "pos": xy(x["pos"]),
            "clip": [xy(q) for q in x.get("clipPolygon") or []] if x.get("isCutWithFrame") else None}


# ---------------------------------------------------------------- the window's session
def add_hints(rows, with_path=False):
    """Tells apart views of the same name by their place in the View Map: their rank among the
    views of that name in the same folder ("2nd of 3"), and the folder path when with_path."""
    by_key = {}
    for r in rows:
        if not r.get("folder"):
            by_key.setdefault((r["path"], r["label"]), []).append(r)
    for (path, _), same in by_key.items():
        for i, r in enumerate(same, 1):
            parts = [path or "top level"] if with_path else []
            if len(same) > 1:
                parts.append("%s of %d in this folder" % (ordinal(i), len(same)))
            r["hint"] = " - ".join(parts)


def ordinal(n):
    return "%d%s" % (n, "th" if 10 <= n % 100 <= 20 else {1: "st", 2: "nd", 3: "rd"}.get(n % 10, "th"))


class Session:
    def __init__(self, api):
        self.api = api
        self.views = None
        self.sources = {}        # Drawing guid -> guid of its source view (picked or known)
        info = api.t("GetProjectInfo")
        self.project = info.get("projectLocation") or info.get("projectName") or "?"

    # History of the state before each relink, per project then per relinked Drawing; "prev" keeps
    # the older history, so that "Back to ..." can be used several times in a row.
    def _load(self):
        try:
            return json.loads(HISTORY_FILE.read_text(encoding="utf-8"))
        except (OSError, ValueError):
            return {}

    def history(self):
        return self._load().get(self.project, {})

    def save_history(self, hist):
        allh = self._load()
        allh[self.project] = hist
        HISTORY_FILE.parent.mkdir(parents=True, exist_ok=True)
        tmp = HISTORY_FILE.with_suffix(".tmp")
        tmp.write_text(json.dumps(allh, ensure_ascii=False, indent=1), encoding="utf-8")
        os.replace(tmp, HISTORY_FILE)

    def select(self, elem_id):
        try:
            self.api.t("ChangeSelectionOfElements", {"addElementsToSelection": [{"elementId": elem_id}],
                                                     "removeElementsFromSelection": []})
        except Exception:
            pass

    def all_rows(self, refresh=False):
        if refresh or self.views is None:
            self.views = view_tree(self.api)
        return self.views

    def all_views(self):
        return [r for r in self.all_rows() if not r.get("folder")]

    def selection_key(self):
        sel = self.api.t("GetSelectedElements", timeout=5).get("elements") or []
        return tuple(sorted(e["elementId"]["guid"] for e in sel))

    def read(self):
        api = self.api
        layout_db = current_layout(api)
        sel_id, d = selected_drawing(api)
        d["_id"] = sel_id["elementId"]
        if d["details"].get("linkType") not in (None, "InternalView"):
            raise Notice("This Drawing does not come from a view of the project (external file): not supported.")
        lay, items = layout_drawing_items(api, layout_db)
        st = {"layout_db": layout_db, "lay": lay, "d": d, "candidates": None}
        try:
            th1, T1, src, group, src_guid = resolve_source(api, d, items, self.sources.get(d["_id"]["guid"]))
            st.update(th1=th1, T1=T1, src=src, group=group, src_guid=src_guid)
        except AmbiguousSource as e:
            st["candidates"] = e.candidates
        return st

    def summary(self, st):
        dets = st["d"]["details"]
        out = {
            "drawingGuid": st["d"]["_id"]["guid"],
            "layout": item_label(st["lay"]),
            "drawing": {"scale": 1 / (dets["drawingScale"] * dets["ratio"]), "title": bool(dets.get("titleElementId"))},
            "candidates": None, "source": None, "views": [], "undo": None,
        }
        h = self.history().get(st["d"]["_id"]["guid"])
        if h:
            out["undo"] = {"name": h.get("name") or "previous view"}
        rows = [dict(r, label=item_label(r)) for r in self.all_views()]
        if st["candidates"]:
            order = {r["guid"].upper(): i for i, r in enumerate(rows)}
            add_hints(rows, with_path=True)
            hints = {r["guid"].upper(): r["hint"] for r in rows}
            cands = sorted(st["candidates"], key=lambda c: order.get(c["guid"].upper(), 1e9))
            out["candidates"] = [{"guid": c["guid"], "name": c["name"],
                                  "hint": hints.get(c["guid"].upper(), "not in the View Map")} for c in cands]
            return out
        out["source"] = {"name": st["src"], "group": GROUP_LABEL.get(st["group"], "all families")}
        fam = [r for r in rows if st["group"] is None or r["group"] == st["group"]]
        add_hints(fam)
        hints = {r["guid"]: r.get("hint") or "" for r in fam}
        for v in self.all_rows():
            if v.get("folder"):
                out["views"].append({"folder": True, "name": v["name"], "depth": v["depth"],
                                     "key": v["key"], "anc": v["anc"]})
                continue
            if st["group"] is not None and v["group"] != st["group"]:
                continue
            th, _ = view_transform(v["vs"])
            out["views"].append({"guid": v["guid"], "name": item_label(v), "path": v["path"],
                                 "kind": TYPE_LABEL.get(v["type"], v["type"]),
                                 "scale": v["vs"]["drawingScale"], "rot": round(math.degrees(th) % 360, 2),
                                 "depth": v["depth"], "anc": v["anc"], "hint": hints.get(v["guid"], "")})
        return out

    def link(self, st, target_guid):
        if st["candidates"]:
            raise Notice("Pick the source view of the Drawing first.")
        tg = next((v for v in self.all_views() if v["guid"] == target_guid), None)
        if not tg:
            raise Notice("Target view not found: press Refresh views.")
        if st["group"] is not None and tg["group"] != st["group"]:
            raise Notice("The target view is not of the same family as the Drawing.")
        d = st["d"]
        p = plan(d, st["th1"], st["T1"], tg["vs"])
        frame = old_frame(self.api, d)
        old_guid = d["_id"]["guid"]
        snap = snapshot(d, st["src_guid"])
        new_id, warnings, nd = relink(self.api, d, st["layout_db"], tg["guid"], p["ratio"], p["angle"], p["ds2"],
                                      lambda mo2: pos_for(d, p, mo2), frame)
        self.sources[new_id["guid"]] = tg["guid"]
        hist = self.history()
        prev = hist.pop(old_guid, None)
        if snap:
            hist[new_id["guid"]] = {"snap": snap, "name": st["src"], "prev": prev}
        self.save_history(hist)
        self.select(new_id)
        gap = max(mapping_gap(d, p, xy(nd["pos"]), xy(nd["modelOffset"]),
                              nd["drawingScale"] * nd["ratio"], nd.get("angle") or 0.0), clip_gap(nd, frame))
        name = item_label(tg)
        if gap < 1e-5:
            return {"kind": "ok", "text": "Relinked to “%s”. The Drawing has not moved." % name, "log": warnings}
        # The gap is on the sheet (paper), so it is given in millimeters and in inches.
        return {"kind": "warn", "text": "Relinked to “%s”, but the check finds a gap of %.2f mm (%.3f in): "
                "check the Drawing." % (name, gap * 1000, gap / 0.0254), "log": warnings}

    def revert(self, st):
        guid = st["d"]["_id"]["guid"]
        hist = self.history()
        h = hist.get(guid)
        if not h:
            raise Notice("No previous view is known for this Drawing.")
        snap = h["snap"]
        new_id, warnings, nd = relink(self.api, st["d"], st["layout_db"], snap["source"], snap["ratio"],
                                      snap["angle"], snap["drawingScale"], lambda mo2: tuple(snap["pos"]),
                                      snap["clip"])
        hist.pop(guid, None)
        if h.get("prev"):
            hist[new_id["guid"]] = h["prev"]
        self.save_history(hist)
        self.sources[new_id["guid"]] = snap["source"]
        self.select(new_id)
        gap = max(math.dist(xy(nd["pos"]), snap["pos"]), abs(nd["ratio"] - snap["ratio"]),
                  abs(norm_angle((nd.get("angle") or 0) - snap["angle"])), clip_gap(nd, snap["clip"]))
        if gap < 1e-5:
            return {"kind": "ok", "text": "Back to “%s”, exactly as before." % h.get("name"), "log": warnings}
        return {"kind": "warn", "text": "Back to “%s”, but the check finds a gap: check the Drawing."
                % h.get("name"), "log": warnings}


# ---------------------------------------------------------------- the window
PAGE = r"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>Relink Drawing</title>
<style>
  :root { --bg:#f6f6f4; --card:#fff; --ink:#1d1d1b; --muted:#707070; --line:#e2e2de; --acc:#2563eb; --ok:#2e8b57; --err:#c0392b; --warn:#a86a00; --sel:#eaf1fc; }
  * { box-sizing:border-box; }
  html, body { height:100%; }
  body { margin:0; background:var(--bg); color:var(--ink); font:13px/1.4 -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif; display:flex; flex-direction:column; }
  main { flex:1; display:flex; flex-direction:column; padding:12px; gap:10px; min-height:0; }
  .card { background:var(--card); border:1px solid var(--line); border-radius:8px; padding:10px 12px; }
  .row { display:flex; gap:8px; align-items:center; }
  button { font:inherit; padding:6px 12px; border-radius:6px; border:1px solid var(--line); background:var(--card); color:var(--ink); cursor:pointer; white-space:nowrap; }
  button.primary { background:var(--acc); border-color:var(--acc); color:#fff; font-weight:600; }
  button:disabled { opacity:.45; cursor:default; }
  input[type=search] { font:inherit; flex:1; min-width:0; padding:6px 9px; border-radius:6px; border:1px solid var(--line); background:var(--card); color:var(--ink); }
  dl { display:grid; grid-template-columns:max-content 1fr; gap:3px 12px; margin:0; }
  dt { color:var(--muted); }
  dd { margin:0; }
  .msg { padding:8px 10px; border-radius:6px; white-space:pre-wrap; }
  .msg.err { background:#fbeceb; color:var(--err); }
  .msg.ok { background:#e9f5ee; color:var(--ok); }
  .msg.warn { background:#fbf3e3; color:var(--warn); }
  .msg.info { background:#eef2f8; color:#3b4a63; }
  .cands { display:flex; flex-direction:column; gap:5px; margin-top:6px; }
  .cands button { text-align:left; }
  .log { font-size:12px; margin:6px 0 0; padding-left:16px; }
  #views { flex:1; min-height:120px; overflow:auto; border:1px solid var(--line); border-radius:8px; background:var(--card); }
  .fold { padding:4px 9px; font-weight:600; color:var(--muted); border-bottom:1px solid var(--line); background:var(--bg); cursor:pointer; user-select:none; }
  .fold:hover { color:var(--ink); }
  .fold .cnt { font-weight:400; font-size:11px; margin-left:6px; }
  .fold::before { content:"\25BE  "; font-weight:400; display:inline-block; width:14px; }
  .fold.closed::before { content:"\25B8  "; }
  .hint { font-size:11px; color:var(--muted); font-weight:400; }
  .v { display:grid; grid-template-columns:1fr auto auto; gap:10px; padding:5px 9px; cursor:pointer; border-bottom:1px solid var(--line); }
  .v:hover { background:#f2f6fd; }
  .v.on { background:var(--sel); outline:2px solid var(--acc); outline-offset:-2px; }
  .m { color:var(--muted); font-variant-numeric:tabular-nums; white-space:nowrap; }
  .tag { font-size:10px; padding:0 5px; border-radius:8px; border:1px solid var(--line); color:var(--muted); margin-left:5px; }
  footer { display:flex; gap:8px; align-items:center; padding:10px 12px; border-top:1px solid var(--line); background:var(--card); }
  #choice { flex:1; color:var(--muted); overflow:hidden; text-overflow:ellipsis; white-space:nowrap; }
  .busy main, .busy footer { opacity:.5; pointer-events:none; }
</style>
</head>
<body>
<main>
  <div id="top"></div>
  <div id="info" class="card" hidden>
    <dl>
      <dt>Layout</dt><dd id="iLayout"></dd>
      <dt>Drawing</dt><dd id="iDrawing"></dd>
      <dt>Source view</dt><dd id="iSource"></dd>
      <dt id="tGroup">Views offered</dt><dd id="iGroup"></dd>
    </dl>
  </div>
  <div id="pick" class="row" hidden>
    <input id="filter" type="search" placeholder="Filter (name, folder, type, 1:200...)">
    <button id="btnFold" title="Collapse or expand all folders">Collapse all</button>
    <button id="btnViews" title="Read the views of the project again">Refresh views</button>
  </div>
  <div id="views" hidden></div>
</main>
<footer>
  <span id="choice"></span>
  <button id="btnUndo" hidden>Back</button>
  <button id="btnLink" class="primary" disabled>Relink</button>
  <button id="btnClose">Close</button>
</footer>
<script>
const DATA = /*DATA*/null;
const VIEWS = (DATA.sel && DATA.sel.views) || [];
const $ = id => document.getElementById(id);
window.onerror = (m, src, line) => { msg("err", "Error in the window: " + m + " (line " + line + ")"); document.body.classList.remove("busy"); };
let chosen = DATA.chosen || null;
// Collapsed folders, kept from one display to the next (the window is shown again at each selection).
let closed = new Set();
try { closed = new Set(JSON.parse(localStorage.getItem("relink_collapsed") || "[]")); } catch (e) {}
function saveClosed() {
  try { localStorage.setItem("relink_collapsed", JSON.stringify([...closed])); } catch (e) {}
  const keys = VIEWS.filter(v => v.folder).map(v => v.key);
  $("btnFold").textContent = keys.length && keys.every(k => closed.has(k)) ? "Expand all" : "Collapse all";
}
function submit(action, extra) {
  document.body.classList.add("busy");
  window.ACAPI.SubmitResult(JSON.stringify(Object.assign({ action, filter: $("filter").value, chosen }, extra || {})));
  if (action === "close") window.ACAPI.ClosePalette();
}
function msg(kind, text, log) {
  const top = $("top"); top.innerHTML = "";
  if (!text) return;
  const d = document.createElement("div"); d.className = "msg " + kind; d.textContent = text;
  if (log && log.length) {
    const ul = document.createElement("ul"); ul.className = "log";
    log.forEach(l => { const li = document.createElement("li"); li.textContent = l; ul.appendChild(li); });
    d.appendChild(ul);
  }
  top.appendChild(d);
}
const deg = r => r.toFixed(2) + "°";
const num = x => (+x.toFixed(2)).toString();

function drawViews() {
  const f = $("filter").value.trim().toLowerCase().split(/\s+/).filter(Boolean);
  const match = v => f.every(w => `${v.name} ${v.path} ${v.kind} ${v.hint || ""} 1:${v.scale}`.toLowerCase().includes(w));
  // With a filter: the matching views and their parent folders; without: everything, empty folders too.
  const keep = new Set();
  if (f.length) VIEWS.forEach(v => { if (!v.folder && match(v)) v.anc.forEach(k => keep.add(k)); });
  const count = {};
  VIEWS.forEach(v => { if (!v.folder) v.anc.forEach(k => count[k] = (count[k] || 0) + 1); });
  const hiddenByFold = v => !f.length && v.anc.some(k => closed.has(k));
  const box = $("views"); box.innerHTML = "";
  let n = 0;
  VIEWS.forEach(v => {
    const row = document.createElement("div");
    row.style.paddingLeft = (9 + 16 * v.depth) + "px";
    if (v.folder) {
      if ((f.length && !keep.has(v.key)) || hiddenByFold(v)) return;
      row.className = "fold" + (!f.length && closed.has(v.key) ? " closed" : ""); row.textContent = v.name || "-";
      const c = document.createElement("span"); c.className = "cnt"; c.textContent = count[v.key] ? `(${count[v.key]})` : "(empty)";
      row.appendChild(c);
      row.onclick = () => { if (f.length) return; closed.has(v.key) ? closed.delete(v.key) : closed.add(v.key); saveClosed(); drawViews(); };
      box.appendChild(row); return;
    }
    if ((f.length && !match(v)) || hiddenByFold(v)) return;
    row.className = "v" + (v.guid === chosen ? " on" : "");
    row.innerHTML = '<span></span><span class="m"></span><span class="m"></span>';
    row.children[0].textContent = v.name;
    if (v.kind !== "plan") { const k = document.createElement("span"); k.className = "tag"; k.textContent = v.kind; row.children[0].appendChild(k); }
    if (v.hint) { const h = document.createElement("div"); h.className = "hint"; h.textContent = v.hint; row.children[0].appendChild(h); }
    row.children[1].textContent = "1:" + v.scale;
    row.children[2].textContent = deg(v.rot);
    row.onclick = () => { chosen = v.guid; drawViews(); };
    row.ondblclick = () => { chosen = v.guid; submit("relink"); };
    box.appendChild(row); n++;
  });
  if (!n) box.innerHTML = '<div class="fold">No view matches the filter.</div>';
  const c = VIEWS.find(v => !v.folder && v.guid === chosen);
  $("btnLink").disabled = !c;
  $("choice").textContent = c ? `→ ${c.name} (1:${c.scale})` : "Pick the target view.";
}

$("filter").oninput = () => { try { localStorage.setItem("relink_filter", $("filter").value); } catch (e) {} drawViews(); };
$("btnFold").onclick = () => {
  const keys = VIEWS.filter(v => v.folder).map(v => v.key);
  const allClosed = keys.length && keys.every(k => closed.has(k));
  closed.clear(); if (!allClosed) keys.forEach(k => closed.add(k));
  saveClosed(); drawViews();
};
$("btnViews").onclick = () => submit("refresh", { refreshViews: true });
$("btnLink").onclick = () => submit("relink");
$("btnUndo").onclick = () => submit("back");
$("btnClose").onclick = () => submit("close");

if (DATA.message) msg(DATA.message.kind, DATA.message.text, DATA.message.log);
if (DATA.sel) {
  const s = DATA.sel, d = s.drawing;
  $("iLayout").textContent = s.layout;
  $("iDrawing").textContent = `1:${num(d.scale)}` + (d.title ? ", with title" : "");
  $("info").hidden = false;
  if (s.undo) {
    $("btnUndo").hidden = false;
    $("btnUndo").textContent = `↶ Back to “${s.undo.name}”`;
    $("btnUndo").title = `Relink to “${s.undo.name}” again, exactly as before`;
  }
  if (s.candidates) {
    $("iSource").textContent = "several views are possible on this layout - which one?";
    const box = document.createElement("div"); box.className = "cands";
    s.candidates.forEach(c => {
      const b = document.createElement("button"); b.textContent = c.name;
      if (c.hint) { const h = document.createElement("div"); h.className = "hint"; h.textContent = c.hint; b.appendChild(h); }
      b.onclick = () => submit("source", { drawing: s.drawingGuid, source: c.guid });
      box.appendChild(b);
    });
    $("iSource").appendChild(box);
    $("tGroup").hidden = $("iGroup").hidden = true;
  } else {
    $("iSource").textContent = s.source.name;
    $("iGroup").textContent = s.source.group;
    $("pick").hidden = $("views").hidden = false;
    let saved = ""; try { saved = localStorage.getItem("relink_filter") || ""; } catch (e) {}
    $("filter").value = DATA.filter || saved;
    saveClosed();
    drawViews();
    const on = document.querySelector(".v.on"); if (on) on.scrollIntoView({ block: "center" });
  }
} else if (!DATA.message) {
  msg("info", "Select a Drawing on a layout.");
}
</script>
</body>
</html>
"""


def show(api, data, first=False):
    html = PAGE.replace("/*DATA*/null", json.dumps(data, ensure_ascii=False).replace("</", "<\\/"))
    p = {"htmlContent": html, "scriptId": SCRIPT_ID}
    if first:
        p.update({"title": "Relink Drawing, Keep Placement", "width": 560, "height": 640})
    api.t("ShowScriptUI", p)


def main():
    port = find_port()
    if not port:
        sys.exit("No Archicad with the Tapir Add-On was found.")
    api = Api(port)
    ses = Session(api)
    ui = {"filter": "", "chosen": None}
    pending = []                               # a display to do again (Archicad was busy)

    def render(message=None, refresh=False, first=False):
        data = {"message": message, "filter": ui["filter"], "chosen": ui["chosen"], "sel": None}
        try:
            if refresh:
                ses.all_rows(refresh=True)
            data["sel"] = ses.summary(ses.read())
        except Notice as e:
            if not message:
                data["message"] = {"kind": "info", "text": str(e)}
        except Exception as e:
            traceback.print_exc()
            data["message"] = {"kind": "err", "text": friendly(e)}
        try:
            show(api, data, first)
        except Exception as e:                 # still busy: show it again at the next loop
            print("Cannot show the window yet: %s" % friendly(e), flush=True)
            pending.append((message, refresh, first))

    api.t("GetScriptUIResult", {"scriptId": SCRIPT_ID})      # drop a result left from a previous run
    try:
        last_sel = ses.selection_key()
    except Exception:
        last_sel = None
    render(first=True)

    n = 0
    while True:
        time.sleep(POLL)
        n += 1
        try:
            r = api.t("GetScriptUIResult", {"scriptId": SCRIPT_ID}, timeout=5)
        except Exception:
            continue                                          # Archicad busy for a moment
        if pending and not r.get("hasResult"):
            args = pending.pop()
            pending.clear()
            render(*args)
            continue
        if not r.get("hasResult"):
            if r.get("isOpen") is False:
                return                                        # the window was closed
            if n % SELECTION_EVERY == 0:                      # follow the selection in Archicad
                try:
                    key = ses.selection_key()
                except Exception:
                    continue
                if key != last_sel:
                    last_sel = key
                    render()
            continue
        res = json.loads(r["result"])
        act = res.get("action")
        if act == "close":
            return
        ui["filter"], ui["chosen"] = res.get("filter") or "", res.get("chosen")
        if act == "source":
            ses.sources[res["drawing"]] = res["source"]
            render()
        elif (act == "relink" and ui["chosen"]) or act == "back":
            try:
                st = ses.read()
                message = ses.link(st, ui["chosen"]) if act == "relink" else ses.revert(st)
            except Notice as e:
                message = {"kind": "err", "text": str(e)}
            except Exception as e:
                traceback.print_exc()
                message = {"kind": "err", "text": friendly(e)}
            try:
                last_sel = ses.selection_key()
            except Exception:
                pass
            render(message)
        else:
            render(refresh=bool(res.get("refreshViews")))


if __name__ == "__main__":
    main()
