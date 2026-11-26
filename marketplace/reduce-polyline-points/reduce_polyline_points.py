# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# category = "modeling-geometry"
# archicad-versions = [25, 26, 27, 28, 29, 30]
# min-tapir-version = "1.5.9"
# changes = "model"
# ///

"""Reduces (and optionally smooths) the number of points of the selected polylines
in Archicad, through the Tapir add-on, with an optional conversion to splines.

Archicad equivalent of the Rhino script CombinedFixContourCrvs.py (Mitch Heynick):
- Polyline.ReduceSegments(tol)  ->  Douglas-Peucker with the given tolerance
- Polyline.Smooth(factor)       ->  iterative Laplacian smoothing (optional)
- Objects.Replace(id, pl)       ->  SetDetailsOfElements (same GUID, same
                                    attributes: layer, pen, line type...)
- "OutputSplines" option        ->  CreateSplines (new elements with the
                                    same attributes) + deletion of the originals

The window shows a live preview: the number of points before / after, the
maximum deviation, and a reduction target in % that adjusts the tolerance
automatically (and the other way round).

Arc segments are kept as they are: their end points are fixed and only the
straight parts between them are simplified (for spline output the arcs are
turned into points before the conversion).

Usage: select polylines in Archicad, then run the script from the Tapir
palette. From the command line, without the window:
    python reduce_polyline_points.py --tol 0.02 --smooth 0.3 --iter 5
    python reduce_polyline_points.py --target 80 --output spline
--tol is in meters (Archicad's internal unit); the window lets you pick the unit.
"""

import argparse
import json
import math
import os
import sys
import time
import urllib.error
import urllib.request
from urllib.parse import urlparse

# Default values (they can also be changed in the window)
DEFAULT_TOLERANCE = 0.01        # m, the largest allowed distance from the original polyline
DEFAULT_SMOOTH_FACTOR = 0.0     # 0..1, 0 = no smoothing
DEFAULT_SMOOTH_ITERATIONS = 5   # 0..25
MAX_SMOOTH_ITERATIONS = 25
MIN_TOLERANCE = 1e-5            # m

EPS = 1e-9

# Only Archicad on this computer is contacted.
LOCAL_HOSTS = ('127.0.0.1', 'localhost', '::1')


# ---------------------------------------------------------------------------
# Talking to Archicad / Tapir
# ---------------------------------------------------------------------------

class Archicad:
    def __init__(self, host, port):
        self.url = '{}:{}'.format(host, port)
        # The Tapir palette gives each script its own window id (Tapir 2.0.0 and later).
        self.script_id = os.environ.get('TAPIR_SCRIPT_ID')

    def run(self, command, parameters=None):
        request = urllib.request.Request(self.url)
        request.add_header('Content-Type', 'application/json')
        data = json.dumps({'command': command, 'parameters': parameters or {}}).encode('utf8')
        response = json.loads(urllib.request.urlopen(request, data).read())
        if not response.get('succeeded'):
            raise RuntimeError('{}: {}'.format(command, error_message(response.get('error'))))
        return response.get('result')

    def tapir(self, command, parameters=None):
        parameters = dict(parameters or {})
        if self.script_id and command in ('ShowScriptUI', 'GetScriptUIResult'):
            parameters['scriptId'] = self.script_id
        result = self.run('API.ExecuteAddOnCommand', {
            'addOnCommandId': {'commandNamespace': 'TapirCommand', 'commandName': command},
            'addOnCommandParameters': parameters,
        })
        response = result['addOnCommandResponse']
        if isinstance(response, dict) and 'error' in response:
            raise RuntimeError('{}: {}'.format(command, error_message(response['error'])))
        return response


def error_message(error):
    if isinstance(error, dict):
        return error.get('message') or json.dumps(error)
    return str(error)


# ---------------------------------------------------------------------------
# Geometry (the same algorithm is ported to JavaScript in the window)
# ---------------------------------------------------------------------------

def _dist_point_segment(p, a, b):
    ax, ay = a
    dx, dy = b[0] - ax, b[1] - ay
    len2 = dx * dx + dy * dy
    if len2 < EPS * EPS:
        return math.hypot(p[0] - ax, p[1] - ay)
    t = max(0.0, min(1.0, ((p[0] - ax) * dx + (p[1] - ay) * dy) / len2))
    return math.hypot(p[0] - (ax + t * dx), p[1] - (ay + t * dy))


def douglas_peucker(points, first, last, tol, keep):
    """Adds to `keep` the indices kept between first and last (exclusive)."""
    stack = [(first, last)]
    while stack:
        a, b = stack.pop()
        max_d, index = -1.0, None
        for i in range(a + 1, b):
            d = _dist_point_segment(points[i], points[a], points[b])
            if d > max_d:
                max_d, index = d, i
        if index is not None and max_d > tol:
            keep.add(index)
            stack.append((a, index))
            stack.append((index, b))


def is_closed(coords):
    return len(coords) > 3 and math.hypot(coords[0][0] - coords[-1][0], coords[0][1] - coords[-1][1]) < EPS


def remove_duplicates(coords, arcs):
    """Removes consecutive points that coincide, and renumbers the arcs."""
    new_index, result = {}, []
    for i, p in enumerate(coords):
        if result and math.hypot(p[0] - result[-1][0], p[1] - result[-1][1]) < EPS:
            new_index[i] = len(result) - 1
        else:
            new_index[i] = len(result)
            result.append(p)
    new_arcs = []
    for beg, end, angle in arcs:
        b, e = new_index[beg], new_index[end]
        if b != e:
            new_arcs.append((b, e, angle))
    return result, new_arcs


class Poly:
    """A prepared polyline: without duplicate points, with its fixed points."""

    def __init__(self, coords, arcs):
        self.coords, self.arcs = remove_duplicates(coords, arcs)
        self.closed = is_closed(self.coords)
        n = len(self.coords)
        fixed = {0, n - 1}
        for beg, end, _ in self.arcs:
            fixed.update((beg, end))
        if self.closed and len(fixed) == 2:
            # Closed loop without arcs: the point farthest from the start becomes a second
            # fixed point, otherwise Douglas-Peucker has no chord to work with.
            c0 = self.coords[0]
            fixed.add(max(range(1, n - 1), key=lambda i: math.hypot(self.coords[i][0] - c0[0], self.coords[i][1] - c0[1])))
        self.fixed = sorted(fixed)

    def keep_indices(self, tol):
        keep = set(self.fixed)
        for a, b in zip(self.fixed, self.fixed[1:]):
            if b - a >= 2:
                douglas_peucker(self.coords, a, b, tol, keep)
        return sorted(keep)


def smooth(coords, arcs, factor, iterations):
    """Laplacian smoothing (the equivalent of Rhino's Polyline.Smooth).

    Each point moves towards the middle of its neighbours by `factor`.
    The ends (of an open polyline) and the ends of arcs do not move.
    """
    if factor <= 0 or iterations <= 0 or len(coords) < 3:
        return coords
    closed = is_closed(coords)
    pts = [list(p) for p in coords]
    n = len(pts)
    fixed = set()
    for beg, end, _ in arcs:
        fixed.update((beg, end))
    if not closed:
        fixed.update((0, n - 1))
    if closed and (0 in fixed or n - 1 in fixed):
        fixed.update((0, n - 1))

    m = n - 1 if closed else n  # when closed, the last point repeats the first one
    for _ in range(iterations):
        new = [p[:] for p in pts]
        for i in range(m):
            if i in fixed:
                continue
            prev = pts[(i - 1) % m] if closed else pts[i - 1]
            nxt = pts[(i + 1) % m] if closed else pts[i + 1]
            for k in (0, 1):
                mid = 0.5 * (prev[k] + nxt[k])
                new[i][k] = pts[i][k] + factor * (mid - pts[i][k])
        if closed:
            new[n - 1] = new[0][:]
        pts = new
    return [tuple(p) for p in pts]


def max_deviation(poly, keep, new_coords):
    """The largest distance between the original points and the new segment that replaces them."""
    dev, k = 0.0, 0
    for i, p in enumerate(poly.coords):
        while k < len(keep) - 2 and keep[k + 1] <= i:
            k += 1
        if k + 1 < len(new_coords):
            dev = max(dev, _dist_point_segment(p, new_coords[k], new_coords[k + 1]))
    return dev


def process(poly, tol, smooth_factor, smooth_iterations):
    """Returns (coords, arcs, max deviation), or None when the result is not valid."""
    if len(poly.coords) < 2:
        return None
    keep = poly.keep_indices(tol)
    old_to_new = {old: new for new, old in enumerate(keep)}
    coords = [poly.coords[i] for i in keep]
    arcs = [(old_to_new[b], old_to_new[e], ang) for b, e, ang in poly.arcs]
    coords = smooth(coords, arcs, smooth_factor, smooth_iterations)
    if len(coords) < 2 or (poly.closed and len(coords) < 4):
        return None
    return coords, arcs, max_deviation(poly, keep, coords)


def count_points(polys, tol):
    return sum(len(p.keep_indices(tol)) for p in polys)


def tolerance_for_target(polys, target_count):
    """The smallest tolerance (binary search on a log scale) that gives at most target_count points."""
    xs = [c[0] for p in polys for c in p.coords]
    ys = [c[1] for p in polys for c in p.coords]
    lo = math.log10(MIN_TOLERANCE)
    hi = math.log10(max(math.hypot(max(xs) - min(xs), max(ys) - min(ys)), MIN_TOLERANCE * 10))
    if count_points(polys, 10 ** hi) > target_count:
        return 10 ** hi  # the target cannot be reached: do the best possible
    for _ in range(40):
        mid = 0.5 * (lo + hi)
        if count_points(polys, 10 ** mid) > target_count:
            lo = mid
        else:
            hi = mid
    return 10 ** hi


def arc_points(p1, p2, angle, tol):
    """The points between p1 and p2 on an arc (angle > 0: counterclockwise)."""
    d = math.hypot(p2[0] - p1[0], p2[1] - p1[1])
    if abs(angle) < 1e-9 or d < EPS:
        return []
    r = d / (2 * abs(math.sin(angle / 2)))
    h = (d / 2) / math.tan(angle / 2)
    mx, my = (p1[0] + p2[0]) / 2, (p1[1] + p2[1]) / 2
    cx, cy = mx - h * (p2[1] - p1[1]) / d, my + h * (p2[0] - p1[0]) / d
    a0 = math.atan2(p1[1] - cy, p1[0] - cx)
    step = 2 * math.acos(max(-1.0, 1 - min(tol, r) / r)) if r > tol else math.pi / 2
    n = min(max(2, int(math.ceil(abs(angle) / max(step, 1e-3)))), 128)
    return [(cx + r * math.cos(a0 + angle * k / n), cy + r * math.sin(a0 + angle * k / n)) for k in range(1, n)]


def densify_arcs(coords, arcs, tol):
    """Replaces the arcs by points (for the conversion to a spline)."""
    arc_of = {}
    for b, e, ang in arcs:
        arc_of[(b, e)] = ang
        arc_of[(e, b)] = -ang
    result = [coords[0]]
    for i in range(1, len(coords)):
        ang = arc_of.get((i - 1, i))
        if ang:
            result.extend(arc_points(coords[i - 1], coords[i], ang, tol))
        result.append(coords[i])
    return result


def spline_points(coords, arcs, tol, closed):
    """Coordinates in the format of CreateSplines (a closed spline does not repeat its first point)."""
    pts = densify_arcs(coords, arcs, tol)
    if closed:
        pts = pts[:-1]
        if len(pts) < 3:
            return None
    elif len(pts) == 2:
        pts = [pts[0], ((pts[0][0] + pts[1][0]) / 2, (pts[0][1] + pts[1][1]) / 2), pts[1]]
    return pts


# ---------------------------------------------------------------------------
# Tapir Script UI window (live preview)
# ---------------------------------------------------------------------------

DIALOG_HTML = r'''<!DOCTYPE html>
<html><head><meta charset="utf-8">
<style>
  * { box-sizing: border-box; }
  html, body { height: 100%; margin: 0; }
  body { font-family: "Segoe UI", -apple-system, sans-serif; font-size: 12px; background: #f4f5f7; color: #1c1e21; display: flex; }
  .side { width: 290px; padding: 12px 14px; overflow-y: auto; border-right: 1px solid #dde0e5; background: #fafbfc; }
  .view { flex: 1; position: relative; background: white; }
  canvas { position: absolute; inset: 0; width: 100%; height: 100%; cursor: grab; }
  h1 { font-size: 14px; margin: 0 0 10px 0; }
  .stats { display: grid; grid-template-columns: 1fr auto; gap: 3px 8px; padding: 8px 10px; background: white; border: 1px solid #dde0e5; border-radius: 6px; margin-bottom: 10px; }
  .stats b { text-align: right; font-variant-numeric: tabular-nums; }
  .stats .big { font-size: 14px; }
  fieldset { border: 1px solid #dde0e5; border-radius: 6px; margin: 0 0 10px 0; padding: 6px 10px 8px 10px; background: white; }
  legend { font-weight: 600; padding: 0 4px; }
  .row { display: flex; justify-content: space-between; align-items: center; margin-top: 6px; gap: 6px; }
  .row input[type=number] { width: 90px; padding: 3px 5px; border: 1px solid #ccd; border-radius: 4px; font-size: 12px; text-align: right; }
  .row select { padding: 2px 4px; border: 1px solid #ccd; border-radius: 4px; font-size: 12px; }
  input[type=range] { width: 100%; margin: 4px 0 2px 0; }
  output { font-weight: 600; font-variant-numeric: tabular-nums; }
  .opt { display: flex; align-items: center; gap: 6px; margin-top: 5px; }
  .note { color: #777; font-size: 11px; margin-top: 3px; }
  .buttons { display: flex; justify-content: flex-end; gap: 8px; margin-top: 6px; }
  button { padding: 6px 14px; border-radius: 4px; border: 1px solid #ccd; background: white; cursor: pointer; font-size: 12px; }
  button.primary { background: #2f6fed; border-color: #2f6fed; color: white; }
  .legend { position: absolute; left: 10px; top: 8px; background: rgba(255,255,255,.85); padding: 4px 8px; border-radius: 4px; color: #555; pointer-events: none; }
  .legend i { display: inline-block; width: 16px; height: 3px; vertical-align: middle; margin: 0 4px 0 8px; }
  .tools { position: absolute; right: 10px; top: 8px; background: rgba(255,255,255,.85); padding: 3px 8px; border-radius: 4px; }
  .hint { position: absolute; left: 10px; bottom: 8px; color: #888; pointer-events: none; }
  .warn { color: #b25b00; }
</style></head>
<body>
<div class="side">
  <h1>Reduce polyline points</h1>
  <div class="stats">
    <span>Polylines</span><b id="nPoly"></b>
    <span>Points before</span><b id="nBefore"></b>
    <span>Points after</span><b id="nAfter" class="big"></b>
    <span>Reduction</span><b id="pct" class="big"></b>
    <span>Max deviation</span><b id="dev"></b>
  </div>
  <fieldset><legend>Simplification</legend>
    <div class="row"><span>Reduction target</span><output id="targetOut"></output></div>
    <input type="range" id="target" min="0" max="99" step="1">
    <div class="note" id="targetNote"></div>
    <div class="row"><span>Tolerance</span><span><input type="number" id="tolNum" step="any" min="0">
      <select id="unit"><option value="m">m</option><option value="cm">cm</option><option value="mm">mm</option><option value="ft">ft</option><option value="in">in</option></select></span></div>
    <input type="range" id="tolLog" step="0.005">
  </fieldset>
  <fieldset><legend>Smoothing</legend>
    <div class="row"><span>Factor</span><output id="smoothOut"></output></div>
    <input type="range" id="smooth" min="0" max="1" step="0.05">
    <div class="row"><span>Iterations</span><output id="iterOut"></output></div>
    <input type="range" id="iter" min="0" step="1">
    <div class="note">Smoothing moves the points: keep an eye on the max deviation.</div>
  </fieldset>
  <fieldset><legend>Output</legend>
    <label class="opt"><input type="radio" name="out" value="polyline" checked> Polylines (changed in place)</label>
    <label class="opt"><input type="radio" name="out" value="spline"> Splines (new elements)</label>
    <label class="opt" id="delRow"><input type="checkbox" id="del" checked> Delete the original polylines</label>
    <div class="note" id="splineNote" hidden>Archicad smooths the spline itself: the preview is approximate.</div>
  </fieldset>
  <div class="note" id="applyNote"></div>
  <div class="buttons">
    <button onclick="ACAPI.ClosePalette()">Cancel</button>
    <button class="primary" id="ok">Apply</button>
  </div>
</div>
<div class="view">
  <canvas id="cv"></canvas>
  <div class="legend"><i style="background:#c3c7cf"></i>original<i style="background:#2f6fed"></i>result</div>
  <label class="tools"><input type="checkbox" id="showPts" checked> points</label>
  <div class="hint">Wheel: zoom · drag: pan · double-click: fit all</div>
</div>

<script id="core">
// --- Algorithm (ported from the Python) ---
var EPS = 1e-9, MIN_TOL = 1e-5;
function distPS(p, a, b) {
  var dx = b[0] - a[0], dy = b[1] - a[1], l2 = dx * dx + dy * dy;
  if (l2 < EPS * EPS) return Math.hypot(p[0] - a[0], p[1] - a[1]);
  var t = Math.max(0, Math.min(1, ((p[0] - a[0]) * dx + (p[1] - a[1]) * dy) / l2));
  return Math.hypot(p[0] - (a[0] + t * dx), p[1] - (a[1] + t * dy));
}
function isClosed(c) { return c.length > 3 && Math.hypot(c[0][0] - c[c.length - 1][0], c[0][1] - c[c.length - 1][1]) < EPS; }
function preparePoly(coords, arcs) {
  var idx = [], pts = [];
  coords.forEach(function (p, i) {
    var q = pts[pts.length - 1];
    if (q && Math.hypot(p[0] - q[0], p[1] - q[1]) < EPS) idx.push(pts.length - 1);
    else { idx.push(pts.length); pts.push(p); }
  });
  var newArcs = [];
  arcs.forEach(function (a) { var b = idx[a[0]], e = idx[a[1]]; if (b !== e) newArcs.push([b, e, a[2]]); });
  var n = pts.length, closed = isClosed(pts), f = {};
  f[0] = 1; f[n - 1] = 1;
  newArcs.forEach(function (a) { f[a[0]] = 1; f[a[1]] = 1; });
  if (closed && Object.keys(f).length === 2) {
    var best = 1, bd = -1;
    for (var i = 1; i < n - 1; i++) { var d = Math.hypot(pts[i][0] - pts[0][0], pts[i][1] - pts[0][1]); if (d > bd) { bd = d; best = i; } }
    f[best] = 1;
  }
  var fixed = Object.keys(f).map(Number).sort(function (a, b) { return a - b; });
  return { coords: pts, arcs: newArcs, closed: closed, fixed: fixed };
}
function keepIndices(poly, tol) {
  var pts = poly.coords, keep = {}, fx = poly.fixed;
  fx.forEach(function (i) { keep[i] = 1; });
  for (var s = 0; s + 1 < fx.length; s++) {
    if (fx[s + 1] - fx[s] < 2) continue;
    var stack = [[fx[s], fx[s + 1]]];
    while (stack.length) {
      var ab = stack.pop(), a = ab[0], b = ab[1], md = -1, mi = -1;
      for (var i = a + 1; i < b; i++) { var d = distPS(pts[i], pts[a], pts[b]); if (d > md) { md = d; mi = i; } }
      if (mi >= 0 && md > tol) { keep[mi] = 1; stack.push([a, mi]); stack.push([mi, b]); }
    }
  }
  return Object.keys(keep).map(Number).sort(function (a, b) { return a - b; });
}
function smoothPts(coords, arcs, factor, iterations) {
  if (factor <= 0 || iterations <= 0 || coords.length < 3) return coords;
  var closed = isClosed(coords), n = coords.length, fixed = {};
  var pts = coords.map(function (p) { return [p[0], p[1]]; });
  arcs.forEach(function (a) { fixed[a[0]] = 1; fixed[a[1]] = 1; });
  if (!closed) { fixed[0] = 1; fixed[n - 1] = 1; }
  if (closed && (fixed[0] || fixed[n - 1])) { fixed[0] = 1; fixed[n - 1] = 1; }
  var m = closed ? n - 1 : n;
  for (var it = 0; it < iterations; it++) {
    var nw = pts.map(function (p) { return [p[0], p[1]]; });
    for (var i = 0; i < m; i++) {
      if (fixed[i]) continue;
      var pv = closed ? pts[(i - 1 + m) % m] : pts[i - 1], nx = closed ? pts[(i + 1) % m] : pts[i + 1];
      for (var k = 0; k < 2; k++) nw[i][k] = pts[i][k] + factor * (0.5 * (pv[k] + nx[k]) - pts[i][k]);
    }
    if (closed) nw[n - 1] = [nw[0][0], nw[0][1]];
    pts = nw;
  }
  return pts;
}
function processPoly(poly, tol, sf, si) {
  var keep = keepIndices(poly, tol), map = {};
  keep.forEach(function (o, i) { map[o] = i; });
  var coords = keep.map(function (i) { return poly.coords[i]; });
  var arcs = poly.arcs.map(function (a) { return [map[a[0]], map[a[1]], a[2]]; });
  coords = smoothPts(coords, arcs, sf, si);
  var dev = 0, k = 0;
  poly.coords.forEach(function (p, i) {
    while (k < keep.length - 2 && keep[k + 1] <= i) k++;
    if (k + 1 < coords.length) dev = Math.max(dev, distPS(p, coords[k], coords[k + 1]));
  });
  return { coords: coords, arcs: arcs, dev: dev };
}
function countPoints(polys, tol) { var s = 0; polys.forEach(function (p) { s += keepIndices(p, tol).length; }); return s; }
function toleranceForTarget(polys, target, logMax) {
  var lo = Math.log10(MIN_TOL), hi = logMax;
  if (countPoints(polys, Math.pow(10, hi)) > target) return Math.pow(10, hi);
  for (var i = 0; i < 30; i++) {
    var mid = 0.5 * (lo + hi);
    if (countPoints(polys, Math.pow(10, mid)) > target) lo = mid; else hi = mid;
  }
  return Math.pow(10, hi);
}
function arcCenter(p1, p2, ang) {
  var d = Math.hypot(p2[0] - p1[0], p2[1] - p1[1]), h = (d / 2) / Math.tan(ang / 2);
  return { cx: (p1[0] + p2[0]) / 2 - h * (p2[1] - p1[1]) / d, cy: (p1[1] + p2[1]) / 2 + h * (p2[0] - p1[0]) / d, r: d / (2 * Math.abs(Math.sin(ang / 2))) };
}
function densify(coords, arcs, nSeg) {
  var arcOf = {};
  arcs.forEach(function (a) { arcOf[a[0] + '_' + a[1]] = a[2]; arcOf[a[1] + '_' + a[0]] = -a[2]; });
  var out = [coords[0]];
  for (var i = 1; i < coords.length; i++) {
    var ang = arcOf[(i - 1) + '_' + i];
    if (ang) {
      var c = arcCenter(coords[i - 1], coords[i], ang), a0 = Math.atan2(coords[i - 1][1] - c.cy, coords[i - 1][0] - c.cx);
      for (var k = 1; k < nSeg; k++) out.push([c.cx + c.r * Math.cos(a0 + ang * k / nSeg), c.cy + c.r * Math.sin(a0 + ang * k / nSeg)]);
    }
    out.push(coords[i]);
  }
  return out;
}
</script>

<script>
var DATA = __DATA__;
var UNITS = { m: 1, cm: 0.01, mm: 0.001, ft: 0.3048, 'in': 0.0254 };
var polys = DATA.polys.map(function (p) { return preparePoly(p.c, p.a); });
var originals = polys.map(function (p) { return densify(p.coords, p.arcs, 24); });
var nBefore = DATA.polys.reduce(function (s, p) { return s + p.c.length; }, 0);
var minCount = polys.reduce(function (s, p) { return s + p.fixed.length; }, 0);
var maxPct = Math.max(0, Math.floor(100 * (1 - minCount / nBefore)));

var xmin = Infinity, ymin = Infinity, xmax = -Infinity, ymax = -Infinity;
polys.forEach(function (p) { p.coords.forEach(function (c) { xmin = Math.min(xmin, c[0]); xmax = Math.max(xmax, c[0]); ymin = Math.min(ymin, c[1]); ymax = Math.max(ymax, c[1]); }); });
var diag = Math.max(Math.hypot(xmax - xmin, ymax - ymin), MIN_TOL * 10);
var logMax = Math.log10(diag);

var $ = function (id) { return document.getElementById(id); };
var state = { tol: DATA.tol, smooth: DATA.smooth, iter: DATA.iter, unit: 'm', results: [] };
$('nPoly').textContent = polys.length;
$('nBefore').textContent = nBefore.toLocaleString();
$('tolLog').min = Math.log10(MIN_TOL); $('tolLog').max = logMax;
$('target').max = maxPct;
$('targetNote').textContent = 'Largest possible reduction: ' + maxPct + ' % (ends and arcs are kept)';
$('iter').max = DATA.maxIter;
$('smooth').value = state.smooth; $('iter').value = state.iter;
$('del').disabled = true; $('delRow').style.opacity = 0.5;

// Lengths are in meters inside; m keeps the original display (mm below 1 m), the other units show 4 digits.
function fmtLen(m) {
  if (state.unit === 'm') return m < 0.001 ? (m * 1000).toFixed(3) + ' mm' : m < 1 ? (m * 1000).toFixed(1) + ' mm' : m.toFixed(3) + ' m';
  return Number((m / UNITS[state.unit]).toPrecision(4)) + ' ' + state.unit;
}

function applyNote() {
  var spline = output() === 'spline', n = polys.length;
  $('applyNote').textContent = spline
    ? 'Apply creates ' + n + ' spline(s)' + ($('del').checked ? ' and deletes the ' + n + ' original polyline(s).' : '; the polylines are kept.')
    : 'Apply changes the ' + n + ' selected polyline(s) in place.';
}

function compute(fromTarget) {
  state.results = polys.map(function (p) { return processPoly(p, state.tol, state.smooth, state.iter); });
  var after = 0, dev = 0;
  state.results.forEach(function (r) { after += r.coords.length; dev = Math.max(dev, r.dev); });
  var pct = 100 * (1 - after / nBefore);
  $('nAfter').textContent = after.toLocaleString();
  $('pct').textContent = '-' + pct.toFixed(1) + ' %';
  $('dev').textContent = fmtLen(dev);
  $('dev').className = dev > state.tol * 1.0001 ? 'warn' : '';
  $('tolNum').value = Number((state.tol / UNITS[state.unit]).toPrecision(4));
  $('tolLog').value = Math.log10(state.tol);
  if (!fromTarget) $('target').value = Math.round(pct);  // the target follows the tolerance
  $('targetOut').textContent = $('target').value + ' %';
  $('smoothOut').textContent = Number(state.smooth).toFixed(2);
  $('iterOut').textContent = state.iter;
  applyNote();
  draw();
}

var pending = null;
function schedule(fn) { if (pending) cancelAnimationFrame(pending); pending = requestAnimationFrame(function () { pending = null; fn(); }); }

$('target').addEventListener('input', function () {
  $('targetOut').textContent = this.value + ' %';
  var target = Math.max(minCount, Math.round(nBefore * (1 - this.value / 100)));
  schedule(function () { state.tol = toleranceForTarget(polys, target, logMax); compute(true); });
});
$('tolLog').addEventListener('input', function () { var v = Math.pow(10, +this.value); schedule(function () { state.tol = v; compute(false); }); });
$('tolNum').addEventListener('change', function () { var v = parseFloat(this.value); if (v > 0) { state.tol = Math.max(v * UNITS[state.unit], MIN_TOL); compute(false); } });
$('unit').addEventListener('change', function () { state.unit = this.value; compute(true); });
$('smooth').addEventListener('input', function () { state.smooth = +this.value; schedule(function () { compute(false); }); });
$('iter').addEventListener('input', function () { state.iter = +this.value; schedule(function () { compute(false); }); });
$('showPts').addEventListener('change', draw);
$('del').addEventListener('change', applyNote);
Array.prototype.forEach.call(document.querySelectorAll('input[name=out]'), function (r) {
  r.addEventListener('change', function () {
    var spline = output() === 'spline';
    $('splineNote').hidden = !spline; $('del').disabled = !spline; $('delRow').style.opacity = spline ? 1 : 0.5;
    applyNote();
    draw();
  });
});
function output() { return document.querySelector('input[name=out]:checked').value; }

$('ok').addEventListener('click', function () {
  var r = { tol: state.tol, smooth: state.smooth, iter: state.iter, output: output(), deleteOriginals: $('del').checked };
  ACAPI.SubmitResult(JSON.stringify(r)).then(function () { ACAPI.ClosePalette(); });
});

// --- Preview ---
var cv = $('cv'), ctx = cv.getContext('2d'), view = null;
function fit() {
  var w = cv.clientWidth, h = cv.clientHeight, m = 20;
  var s = Math.min((w - 2 * m) / Math.max(xmax - xmin, 1e-9), (h - 2 * m) / Math.max(ymax - ymin, 1e-9));
  view = { s: s, ox: xmin - (w / s - (xmax - xmin)) / 2, oy: ymin - (h / s - (ymax - ymin)) / 2 };
}
function sx(x) { return (x - view.ox) * view.s; }
function sy(y) { return cv.clientHeight - (y - view.oy) * view.s; }
function pathLine(pts) { ctx.moveTo(sx(pts[0][0]), sy(pts[0][1])); for (var i = 1; i < pts.length; i++) ctx.lineTo(sx(pts[i][0]), sy(pts[i][1])); }
function pathSpline(pts, closed) {
  // Catmull-Rom (an approximation of Archicad's auto-smoothed spline)
  if (closed) pts = pts.slice(0, -1);
  var n = pts.length;
  if (n < 3) { pathLine(pts); return; }
  var P = function (i) { return closed ? pts[(i + n) % n] : pts[Math.max(0, Math.min(n - 1, i))]; };
  ctx.moveTo(sx(pts[0][0]), sy(pts[0][1]));
  for (var i = 0; i < (closed ? n : n - 1); i++) {
    var p0 = P(i - 1), p1 = P(i), p2 = P(i + 1), p3 = P(i + 2);
    ctx.bezierCurveTo(sx(p1[0] + (p2[0] - p0[0]) / 6), sy(p1[1] + (p2[1] - p0[1]) / 6),
                      sx(p2[0] - (p3[0] - p1[0]) / 6), sy(p2[1] - (p3[1] - p1[1]) / 6), sx(p2[0]), sy(p2[1]));
  }
}
function draw() {
  var dpr = window.devicePixelRatio || 1, w = cv.clientWidth, h = cv.clientHeight;
  if (cv.width !== w * dpr || cv.height !== h * dpr) { cv.width = w * dpr; cv.height = h * dpr; }
  if (!view) fit();
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  ctx.clearRect(0, 0, w, h);
  ctx.lineJoin = 'round';
  ctx.strokeStyle = '#c3c7cf'; ctx.lineWidth = 3; ctx.beginPath();
  originals.forEach(pathLine); ctx.stroke();
  var spline = output() === 'spline';
  ctx.strokeStyle = '#2f6fed'; ctx.lineWidth = 1.3; ctx.beginPath();
  state.results.forEach(function (r, i) {
    var pts = densify(r.coords, r.arcs, 24);
    if (spline) pathSpline(pts, polys[i].closed); else pathLine(pts);
  });
  ctx.stroke();
  if ($('showPts').checked) {
    ctx.fillStyle = '#1b3f99';
    state.results.forEach(function (r) {
      r.coords.forEach(function (c) { var x = sx(c[0]), y = sy(c[1]); if (x > -5 && y > -5 && x < w + 5 && y < h + 5) ctx.fillRect(x - 1.5, y - 1.5, 3, 3); });
    });
  }
}
cv.addEventListener('wheel', function (e) {
  e.preventDefault();
  var r = cv.getBoundingClientRect(), mx = e.clientX - r.left, my = e.clientY - r.top;
  var wx = mx / view.s + view.ox, wy = (cv.clientHeight - my) / view.s + view.oy;
  view.s *= Math.exp(-e.deltaY * 0.0015);
  view.ox = wx - mx / view.s; view.oy = wy - (cv.clientHeight - my) / view.s;
  draw();
}, { passive: false });
var drag = null;
cv.addEventListener('mousedown', function (e) { drag = { x: e.clientX, y: e.clientY }; cv.style.cursor = 'grabbing'; });
window.addEventListener('mouseup', function () { drag = null; cv.style.cursor = 'grab'; });
window.addEventListener('mousemove', function (e) {
  if (!drag) return;
  view.ox -= (e.clientX - drag.x) / view.s; view.oy += (e.clientY - drag.y) / view.s;
  drag = { x: e.clientX, y: e.clientY }; draw();
});
cv.addEventListener('dblclick', function () { fit(); draw(); });
window.addEventListener('resize', function () { fit(); draw(); });

compute(false);
</script>
</body></html>'''


def ask_options(ac, polylines, tol, smooth_factor, smooth_iterations):
    """Shows the window; returns the chosen options as a dict, or None when cancelled."""
    data = {
        'polys': [{'c': [[round(c['x'], 6), round(c['y'], 6)] for c in d['coordinates']],
                   'a': [[a['begIndex'], a['endIndex'], a['arcAngle']] for a in d.get('arcs', [])]}
                  for _, _, d in polylines],
        'tol': tol, 'smooth': smooth_factor, 'iter': smooth_iterations, 'maxIter': MAX_SMOOTH_ITERATIONS,
    }
    html = DIALOG_HTML.replace('__DATA__', json.dumps(data, separators=(',', ':')))
    ac.tapir('ShowScriptUI', {'htmlContent': html, 'title': 'Reduce polyline points',
                              'width': 980, 'height': 720, 'resizable': True, 'zoomEnabled': False})
    while True:
        time.sleep(0.3)
        resp = ac.tapir('GetScriptUIResult')
        if resp.get('hasResult'):
            return json.loads(resp['result'])
        if not resp.get('isOpen', True):
            return None


# ---------------------------------------------------------------------------
# Main program
# ---------------------------------------------------------------------------

def to_tuples(details):
    coords = [(c['x'], c['y']) for c in details['coordinates']]
    arcs = [(a['begIndex'], a['endIndex'], a['arcAngle']) for a in details.get('arcs', [])]
    return coords, arcs


def xy_list(coords):
    return [{'x': x, 'y': y} for x, y in coords]


def polyline_settings(coords, arcs):
    settings = {'coordinates': xy_list(coords)}
    if arcs:
        settings['arcs'] = [{'begIndex': b, 'endIndex': e, 'arcAngle': a} for b, e, a in arcs]
    return settings


def spline_data(info, details, coords, closed):
    data = {'coordinates': xy_list(coords), 'closed': closed}
    if 'floorIndex' in info:
        data['floorInd'] = info['floorIndex']
    if 'layerIndex' in info:
        data['layerIndex'] = int(info['layerIndex'])
    for key in ('linePenIndex', 'lineTypeId', 'roomSeparator'):
        if key in details:
            data[key] = details[key]
    return data


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('--host', default='http://127.0.0.1')
    parser.add_argument('--port', type=int, default=19723)
    parser.add_argument('--tol', type=float, default=None, help='tolerance in meters (no window)')
    parser.add_argument('--target', type=float, default=None, help='reduction target in %% (no window)')
    parser.add_argument('--smooth', type=float, default=DEFAULT_SMOOTH_FACTOR, help='smoothing factor 0..1')
    parser.add_argument('--iter', type=int, default=DEFAULT_SMOOTH_ITERATIONS, help='smoothing iterations')
    parser.add_argument('--output', choices=('polyline', 'spline'), default='polyline')
    parser.add_argument('--keep-originals', action='store_true', help='with spline output, keep the polylines')
    parser.add_argument('--no-ui', action='store_true', help='do not show the window')
    args, _ = parser.parse_known_args()

    if urlparse(args.host).hostname not in LOCAL_HOSTS:
        print('Error: --host must be this computer (http://127.0.0.1 or http://localhost), not {}.'.format(args.host))
        return 1

    ac = Archicad(args.host, args.port)
    try:
        run(ac, args)
    except urllib.error.URLError as error:
        print('Error: cannot reach Archicad on port {} ({}). Is Archicad running with the Tapir Add-On?'.format(
            args.port, error.reason))
        return 1
    except RuntimeError as error:
        print('Error: {}'.format(error))
        return 1
    return 0


def run(ac, args):
    selected = ac.tapir('GetSelectedElements')['elements']
    if not selected:
        print('Nothing is selected. Select the polylines to simplify, then run the script again.')
        return
    details = ac.tapir('GetDetailsOfElements', {
        'elements': selected, 'fields': ['type', 'floorIndex', 'layerIndex', 'details']})['detailsOfElements']
    polylines = [(elem, info, info['details']) for elem, info in zip(selected, details)
                 if info.get('type') == 'PolyLine' and 'details' in info and info['details'].get('coordinates')]
    if not polylines:
        print('There is no polyline in the selection. Nothing was changed.')
        return
    total_before = sum(len(d['coordinates']) for _, _, d in polylines)
    print('{} polyline(s) selected, {} points.'.format(len(polylines), total_before))

    polys = [Poly(*to_tuples(d)) for _, _, d in polylines]

    options = {'tol': args.tol or DEFAULT_TOLERANCE, 'smooth': args.smooth, 'iter': args.iter,
               'output': args.output, 'deleteOriginals': not args.keep_originals}
    if args.target is not None:
        options['tol'] = tolerance_for_target(polys, round(total_before * (1 - args.target / 100.0)))
    elif not args.no_ui and args.tol is None:
        options = ask_options(ac, polylines, options['tol'], options['smooth'], options['iter'])
        if options is None:
            print('Cancelled. Nothing was changed.')
            return
    tol = max(float(options['tol']), MIN_TOLERANCE)
    smooth_factor = min(max(float(options['smooth']), 0.0), 1.0)
    smooth_iterations = min(max(int(options['iter']), 0), MAX_SMOOTH_ITERATIONS)
    to_spline = options.get('output') == 'spline'

    to_update, to_create, total_after, errors, deviation = [], [], 0, 0, 0.0
    for (elem, info, d), poly in zip(polylines, polys):
        result = process(poly, tol, smooth_factor, smooth_iterations)
        if result is None:
            errors += 1
            total_after += len(d['coordinates'])
            continue
        coords, arcs, dev = result
        deviation = max(deviation, dev)
        if to_spline:
            pts = spline_points(coords, arcs, tol, poly.closed)
            if pts is None:
                errors += 1
                total_after += len(d['coordinates'])
                continue
            total_after += len(pts)
            to_create.append((elem, spline_data(info, d, pts, poly.closed)))
        else:
            total_after += len(coords)
            if len(coords) == len(d['coordinates']) and smooth_factor == 0:
                continue  # nothing to change
            to_update.append({'elementId': elem['elementId'],
                              'details': {'typeSpecificDetails': polyline_settings(coords, arcs)}})

    changed = 0
    if to_update:
        print('Changing {} polyline(s) in place...'.format(len(to_update)))
        results = ac.tapir('SetDetailsOfElements', {'elementsWithDetails': to_update})['executionResults']
        for item, res in zip(to_update, results):
            if res.get('success'):
                changed += 1
            else:
                errors += 1
                print('Failed for {}: {}'.format(item['elementId']['guid'], (res.get('error') or {}).get('message')))

    if to_create:
        print('Creating {} spline(s)...'.format(len(to_create)))
        created = ac.tapir('CreateSplines', {'splinesData': [data for _, data in to_create]})['elements']
        new_splines, to_delete = [], []
        for (elem, _), res in zip(to_create, created):
            if 'elementId' in res:
                new_splines.append(res)
                to_delete.append(elem)
            else:
                errors += 1
                print('Could not create the spline for {}: {}'.format(
                    elem['elementId']['guid'], (res.get('error') or {}).get('message')))
        changed = len(new_splines)
        if to_delete and options.get('deleteOriginals', True):
            print('Deleting the {} original polyline(s)...'.format(len(to_delete)))
            results = ac.tapir('DeleteElements', {'elements': to_delete})['executionResults']
            failed = sum(1 for r in results if not r.get('success'))
            if failed:
                print('{} original polyline(s) not deleted (locked layer?).'.format(failed))
        if new_splines:
            ac.tapir('ChangeSelectionOfElements', {'addElementsToSelection': new_splines})

    print('{} polyline(s) processed, {} {}.'.format(
        len(polylines), changed, 'spline(s) created' if to_spline else 'changed'))
    print('Points: {} -> {} (reduced by {:.1f} %), tolerance {:.4g} m, max deviation {:.4g} m'.format(
        total_before, total_after, 100.0 * (1 - total_after / float(total_before)), tol, deviation))
    if errors:
        print('{} polyline(s) with errors (not changed).'.format(errors))


if __name__ == '__main__':
    sys.exit(main())
