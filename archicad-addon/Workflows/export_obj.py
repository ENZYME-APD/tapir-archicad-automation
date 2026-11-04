"""Exports the 3D model of the project, or of some elements, to a Wavefront OBJ file.

Reads the solid bodies of each element with Get3DGeometryOfElements and writes one OBJ object per
element, named after its type, its Element ID and its GUID. OBJ faces have no holes, so a face with
holes is triangulated (the default), or with --holes bridge written as one polygon whose holes are
joined to the outline by a seam. Faces without holes are written as they are. The camera of the 3D
window (Get3DProjectionSettings) is written as a comment at the top of the file. Changes nothing in
the project.

    python archicad-addon/Workflows/export_obj.py [<output.obj>] [--elementType Wall ...] [--selected]
        [--holes triangulate|bridge] [--zUp] [--batchSize 100] [--port 19723]

Without an output path the file goes to the temp folder. Without --elementType or --selected every
element visible in 3D is exported. Archicad is Z up; the file is written Y up (x, z, -y) for the
OBJ importers that expect it (Blender's default), unless --zUp keeps Archicad's axes. Lengths are
in meters.

The camera of a saved 3D view can be read the same way after opening the view with ChangeWindow.
For a large model the reads take a while; a native Export3DModel command (#1051) would only be
added if this is too slow.
"""
import argparse
import datetime
import os
import tempfile
import aclib

parser = argparse.ArgumentParser (description = 'Exports the 3D model of the project to an OBJ file.')
parser.add_argument ('output', nargs = '?', default = None, help = 'The OBJ file to write. Default: a new file in the temp folder.')
parser.add_argument ('--elementType', action = 'append', default = [], help = 'Export only the elements of this type (e.g. Wall). Can be given more than once.')
parser.add_argument ('--selected', action = 'store_true', help = 'Export only the elements selected in Archicad.')
parser.add_argument ('--holes', choices = ['triangulate', 'bridge'], default = 'triangulate', help = 'How faces with holes are written.')
parser.add_argument ('--zUp', action = 'store_true', help = 'Keep the Z up axes of Archicad instead of writing Y up.')
parser.add_argument ('--batchSize', type = int, default = 100, help = 'The number of elements read with one Get3DGeometryOfElements call.')
# Read by aclib; named here so that their values are not taken for the output path.
parser.add_argument ('--host', help = 'The host of Archicad (see aclib).')
parser.add_argument ('--port', type = int, help = 'The port of Archicad (see aclib).')
args, _ = parser.parse_known_args ()

EPSILON = 1e-12


def RunTapirCommand (command, parameters = None, required = True):
    # The workflows print every command by default; the geometry would flood the output.
    result = aclib.RunTapirCommand (command, parameters or {}, debug = False)
    if required and (not result or 'error' in result):
        raise SystemExit ('{} failed; is Archicad running with the Tapir Add-On?'.format (command))
    return result


def GetElementsToExport ():
    if args.selected:
        return RunTapirCommand ('GetSelectedElements')['elements']
    if args.elementType:
        elements = []
        for elementType in args.elementType:
            elements += RunTapirCommand ('GetElementsByType', {'elementType': elementType, 'filters': ['IsVisibleIn3D']})['elements']
        return elements
    return RunTapirCommand ('GetAllElements', {'filters': ['IsVisibleIn3D']})['elements']


def Batches (items, size):
    for start in range (0, len (items), size):
        yield items[start:start + size]


# --- Faces with holes --------------------------------------------------------------------------
# A face is planar. It is projected to the coordinate plane its normal is closest to, its holes
# are joined to the outline by seams (each hole to a vertex it can see), and the joined polygon is
# written as it is (--holes bridge) or cut into triangles by ear clipping. The loops carry the
# vertex indices of the body, so a seam vertex appears twice.

def Normal (points):
    # Newell's method: the normal of a planar polygon, its length twice the area.
    nx = ny = nz = 0.0
    for i in range (len (points)):
        x1, y1, z1 = points[i]
        x2, y2, z2 = points[(i + 1) % len (points)]
        nx += (y1 - y2) * (z1 + z2)
        ny += (z1 - z2) * (x1 + x2)
        nz += (x1 - x2) * (y1 + y2)
    return nx, ny, nz


def ProjectTo2D (normal):
    # Drops the dominant axis of the normal, keeping the counterclockwise order seen from the normal.
    ax, ay, az = (abs (c) for c in normal)
    if az >= ax and az >= ay:
        return (lambda p: (p[0], p[1])) if normal[2] > 0 else (lambda p: (p[1], p[0]))
    if ax >= ay:
        return (lambda p: (p[1], p[2])) if normal[0] > 0 else (lambda p: (p[2], p[1]))
    return (lambda p: (p[2], p[0])) if normal[1] > 0 else (lambda p: (p[0], p[2]))


def SignedArea (loop, coords):
    area = 0.0
    for i in range (len (loop)):
        x1, y1 = coords[loop[i]]
        x2, y2 = coords[loop[(i + 1) % len (loop)]]
        area += x1 * y2 - x2 * y1
    return area / 2.0


def Cross (o, a, b):
    return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])


def SegmentsCross (p1, p2, q1, q2):
    # True when the two segments cross at a point inside both of them (touching ends do not count).
    d1 = Cross (q1, q2, p1)
    d2 = Cross (q1, q2, p2)
    d3 = Cross (p1, p2, q1)
    d4 = Cross (p1, p2, q2)
    return ((d1 > EPSILON and d2 < -EPSILON) or (d1 < -EPSILON and d2 > EPSILON)) and \
           ((d3 > EPSILON and d4 < -EPSILON) or (d3 < -EPSILON and d4 > EPSILON))


def LoopEdges (loop, coords):
    for i in range (len (loop)):
        yield coords[loop[i]], coords[loop[(i + 1) % len (loop)]]


def BridgeHoles (outline, holes, coords):
    """Joins the holes to the outline. The outline is counterclockwise, the holes clockwise; the
    result is one loop of slots (vertex index, 2D point)."""
    merged = [(v, coords[v]) for v in outline]
    remaining = [[(v, coords[v]) for v in hole] for hole in holes]
    # The hole reaching farthest in x first, so the seams of the later ones do not cross it.
    remaining.sort (key = lambda hole: -max (p[0] for _, p in hole))
    while remaining:
        hole = remaining.pop (0)
        holeStart = max (range (len (hole)), key = lambda i: hole[i][1][0])
        m = hole[holeStart][1]
        otherEdges = []
        for other in remaining:
            otherEdges += [(other[i][1], other[(i + 1) % len (other)][1]) for i in range (len (other))]
        otherEdges += [(hole[i][1], hole[(i + 1) % len (hole)][1]) for i in range (len (hole))]
        mergedEdges = [(merged[i][1], merged[(i + 1) % len (merged)][1]) for i in range (len (merged))]
        candidates = sorted (range (len (merged)), key = lambda i: (merged[i][1][0] - m[0]) ** 2 + (merged[i][1][1] - m[1]) ** 2)
        target = candidates[0]
        for i in candidates:
            p = merged[i][1]
            if not any (SegmentsCross (m, p, a, b) for a, b in mergedEdges + otherEdges):
                target = i
                break
        holeLoop = hole[holeStart:] + hole[:holeStart + 1]
        merged = merged[:target + 1] + holeLoop + merged[target:]
    return merged


def PointInTriangle (p, a, b, c):
    return Cross (a, b, p) >= -EPSILON and Cross (b, c, p) >= -EPSILON and Cross (c, a, p) >= -EPSILON


def EarClip (slots):
    """Triangulates a counterclockwise loop of slots (vertex index, 2D point) by ear clipping.
    Returns triangles of vertex indices."""
    triangles = []
    slots = list (slots)
    guard = 0
    maxRounds = 2 * len (slots) + 10  # one ear or one flat corner is removed per round
    start = 0
    while len (slots) > 3 and guard < maxRounds:
        guard += 1
        n = len (slots)
        # Only a corner that is not convex can lie inside an ear.
        reflex = [slots[i] for i in range (n) if Cross (slots[i - 1][1], slots[i][1], slots[(i + 1) % n][1]) <= EPSILON]
        earFound = False
        for k in range (n):
            i = (start + k) % n
            prev, cur, nxt = slots[i - 1], slots[i], slots[(i + 1) % n]
            if Cross (prev[1], cur[1], nxt[1]) <= EPSILON:
                continue  # reflex or degenerate corner
            a, b, c = prev[1], cur[1], nxt[1]
            blocked = False
            for other in reflex:
                q = other[1]
                if other[0] in (prev[0], cur[0], nxt[0]) or q == a or q == b or q == c:
                    continue
                if PointInTriangle (q, a, b, c):
                    blocked = True
                    break
            if blocked:
                continue
            triangles.append ((prev[0], cur[0], nxt[0]))
            del slots[i]
            start = max (0, i - 1)
            earFound = True
            break
        if not earFound:
            # A degenerate rest (collinear points, touching seams): drop a corner that has no area.
            flat = [i for i in range (n) if abs (Cross (slots[i - 1][1], slots[i][1], slots[(i + 1) % n][1])) <= EPSILON]
            if flat:
                del slots[flat[0]]
                continue
            break
    if len (slots) == 3:
        triangles.append (tuple (s[0] for s in slots))
    elif len (slots) > 3:
        # No ear left in a malformed face: a fan keeps it closed rather than leaving a hole.
        triangles += [(slots[0][0], slots[i][0], slots[i + 1][0]) for i in range (1, len (slots) - 1)]
    return [t for t in triangles if len (set (t)) == 3]


def FacesOfPolygon (polygon, vertices):
    outline = polygon['vertexIds']
    holes = [hole['vertexIds'] for hole in polygon.get ('holes', []) if len (hole['vertexIds']) >= 3]
    if not holes:
        return [outline]
    normal = Normal ([vertices[v] for v in outline])
    if sum (c * c for c in normal) <= EPSILON:
        return [outline]
    project = ProjectTo2D (normal)
    coords = {v: project (vertices[v]) for v in set (outline).union (*holes)}
    if SignedArea (outline, coords) < 0:
        outline = list (reversed (outline))
    holes = [list (reversed (hole)) if SignedArea (hole, coords) > 0 else hole for hole in holes]
    merged = BridgeHoles (outline, holes, coords)
    if args.holes == 'bridge':
        return [[v for v, _ in merged]]
    return EarClip (merged)


# --- Writing -----------------------------------------------------------------------------------

def ToObjAxes (coordinate):
    x, y, z = coordinate['x'], coordinate['y'], coordinate['z']
    return (x, y, z) if args.zUp else (x, z, -y)


def ObjectName (element, details):
    parts = [details.get ('type', 'Element')]
    if details.get ('id'):
        parts.append (details['id'])
    parts.append (element['elementId']['guid'])
    return '_'.join (parts).replace (' ', '_')


def CameraComment ():
    settings = RunTapirCommand ('Get3DProjectionSettings', required = False)
    lines = ['# Camera of the 3D window (Archicad coordinates, Z up, meters), from Get3DProjectionSettings:']
    if not settings or 'error' in settings:
        return lines + ['#   not available']
    if settings.get ('isPersp') and 'perspective' in settings:
        persp = settings['perspective']
        for key in ('cameraPosition', 'targetPosition'):
            c = persp[key]
            lines.append ('#   {}: {:.4f} {:.4f} {:.4f}'.format (key, c['x'], c['y'], c['z']))
        for key in ('viewCone', 'rollAngle', 'azimuth', 'distance'):
            if key in persp:
                lines.append ('#   {}: {}'.format (key, persp[key]))
    elif 'axonometric' in settings:
        axo = settings['axonometric']
        lines.append ('#   axonometric {}, azimuth {}'.format (axo['projectionType'], axo['azimuth']))
    lines.append ('# A saved 3D view\'s camera: open the view with ChangeWindow, then call Get3DProjectionSettings.')
    return lines


def Main ():
    output = args.output
    if output is None:
        output = os.path.join (tempfile.gettempdir (), 'tapir_export_{}.obj'.format (datetime.datetime.now ().strftime ('%Y%m%d_%H%M%S')))

    elements = GetElementsToExport ()
    print ('Exporting {} elements to {}'.format (len (elements), output))

    counts = {'objects': 0, 'vertices': 0, 'faces': 0, 'skipped': 0}
    with open (output, 'w', encoding = 'utf-8', newline = '\n') as obj:
        obj.write ('# Exported from Archicad by Tapir (archicad-addon/Workflows/export_obj.py)\n')
        obj.write ('# Axes: {}\n'.format ('Archicad (Z up)' if args.zUp else 'Y up (x, z, -y of Archicad)'))
        obj.write ('\n'.join (CameraComment ()) + '\n')

        vertexOffset = 1  # OBJ vertex indices start at 1 and run through the whole file.
        for batch in Batches (elements, max (1, args.batchSize)):
            geometries = RunTapirCommand ('Get3DGeometryOfElements', {'elements': batch})['geometries3D']
            detailsList = RunTapirCommand ('GetDetailsOfElements', {'elements': batch, 'fields': ['type', 'id']})['detailsOfElements']
            for element, geometry, details in zip (batch, geometries, detailsList):
                if 'error' in geometry or not geometry.get ('bodies'):
                    counts['skipped'] += 1
                    continue
                obj.write ('o {}\n'.format (ObjectName (element, details if 'error' not in details else {})))
                counts['objects'] += 1
                for body in geometry['bodies']:
                    vertices = [(v['x'], v['y'], v['z']) for v in body['vertices']]
                    for v in body['vertices']:
                        obj.write ('v {:.6f} {:.6f} {:.6f}\n'.format (*ToObjAxes (v)))
                    for polygon in body['polygons']:
                        for face in FacesOfPolygon (polygon, vertices):
                            obj.write ('f {}\n'.format (' '.join (str (vertexOffset + v) for v in face)))
                            counts['faces'] += 1
                    vertexOffset += len (vertices)
                    counts['vertices'] += len (vertices)

    print ('Wrote {objects} objects, {vertices} vertices and {faces} faces; {skipped} elements had no 3D body.'.format (**counts))
    print (output)


Main ()
