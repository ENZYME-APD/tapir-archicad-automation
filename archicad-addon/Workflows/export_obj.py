"""Exports the 3D model of the project, or of some elements, to a Wavefront OBJ or a binary glTF file.

Reads the solid bodies of each element with Get3DGeometryOfElements and writes one OBJ object per
element, named after its type, its Element ID and its GUID. OBJ faces have no holes, so a face with
holes is triangulated (the default), or with --holes bridge written as one polygon whose holes are
joined to the outline by a seam. Faces without holes are written as they are. The camera of the 3D
window (Get3DProjectionSettings) is written as a comment at the top of the file. Changes nothing in
the project.

The materials go to a .mtl file next to the OBJ (same name, .mtl extension), which the OBJ names
with mtllib. It has one material per surface the faces use, named after the surface, with the
surface's colour, transparency and specular reflection read by GetSurfaces. Each face gets the
material of its surface with usemtl; a face whose material is not a surface attribute of the
project (one defined in a GDL script) gets the grey material Tapir_NoSurface. --noMtl writes no
.mtl and no usemtl lines.

--format glb (the default for an output path ending in .glb) writes one binary glTF file instead, which Blender imports with
File > Import > glTF 2.0. It has one mesh per element (its node named like the OBJ object, with
the GUID, type and Element ID in the node's extras), every face cut into triangles with the
normal of its face, and one glTF material per surface: the base colour and the alpha from the
surface's colour and transparency (transparent ones blend), the roughness from its shine, double
sided. A perspective camera of the 3D window becomes a glTF camera node named Archicad_3D_Window;
its view cone is taken as the horizontal angle of a picture of --cameraAspect (16:9 unless given),
and Archicad's values are kept in the node's extras. An axonometric 3D window gives no camera.
glTF is always Y up, so --zUp and --holes only apply to OBJ; --noMtl writes no materials.

    python archicad-addon/Workflows/export_obj.py [<output.obj|.glb>] [--format obj|glb]
        [--elementType Wall ...] [--selected] [--holes triangulate|bridge] [--zUp] [--noMtl]
        [--cameraAspect 1.7778] [--batchSize 100] [--port 19723]

Without an output path the file goes to the temp folder. Without --elementType or --selected every
element visible in 3D is exported. Archicad is Z up; the file is written Y up (x, z, -y) for the
OBJ importers that expect it (Blender's default), unless --zUp keeps Archicad's axes. Lengths are
in meters.

The camera of a saved 3D view can be read the same way after opening the view with ChangeWindow.
For a large model the reads take a while; a native Export3DModel command (#1051) would only be
added if this is too slow.
"""
import argparse
import array
import datetime
import json
import math
import os
import struct
import sys
import tempfile
import aclib

parser = argparse.ArgumentParser (description = 'Exports the 3D model of the project to an OBJ or a binary glTF file.')
parser.add_argument ('output', nargs = '?', default = None, help = 'The file to write. Default: a new file in the temp folder.')
parser.add_argument ('--format', choices = ['obj', 'glb'], default = None, help = 'obj (Wavefront OBJ with a .mtl) or glb (binary glTF 2.0). Default: glb for an output path ending in .glb, else obj.')
parser.add_argument ('--cameraAspect', type = float, default = 16.0 / 9.0, help = 'glb: the width / height of the camera\'s picture (default 16:9).')
parser.add_argument ('--elementType', action = 'append', default = [], help = 'Export only the elements of this type (e.g. Wall). Can be given more than once.')
parser.add_argument ('--selected', action = 'store_true', help = 'Export only the elements selected in Archicad.')
parser.add_argument ('--holes', choices = ['triangulate', 'bridge'], default = 'triangulate', help = 'How faces with holes are written.')
parser.add_argument ('--zUp', action = 'store_true', help = 'Keep the Z up axes of Archicad instead of writing Y up.')
parser.add_argument ('--noMtl', action = 'store_true', help = 'Write no .mtl material file and no materials in the OBJ.')
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


def FacesOfPolygon (polygon, vertices, triangles = False):
    """The faces to write for a polygon: its outline when it has no holes, else the outline with
    the holes joined (--holes bridge) or triangles. With triangles = True (glTF) every face is cut
    into triangles, also one without holes, and a face without area gives none."""
    outline = polygon['vertexIds']
    holes = [hole['vertexIds'] for hole in polygon.get ('holes', []) if len (hole['vertexIds']) >= 3]
    if not holes and (not triangles or len (outline) <= 3):
        return [outline] if not triangles or len (set (outline)) == 3 else []
    normal = Normal ([vertices[v] for v in outline])
    if sum (c * c for c in normal) <= EPSILON:
        return [] if triangles else [outline]
    project = ProjectTo2D (normal)
    coords = {v: project (vertices[v]) for v in set (outline).union (*holes)}
    if SignedArea (outline, coords) < 0:
        outline = list (reversed (outline))
    holes = [list (reversed (hole)) if SignedArea (hole, coords) > 0 else hole for hole in holes]
    merged = BridgeHoles (outline, holes, coords) if holes else [(v, coords[v]) for v in outline]
    if args.holes == 'bridge' and not triangles:
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


# --- Materials ---------------------------------------------------------------------------------
# One material per surface attribute the faces use. The surfaces are read with GetSurfaces once per
# batch of elements, only the ones not read before.

NO_SURFACE_MATERIAL = 'Tapir_NoSurface'
SURFACE_FIELDS = ['surfaceColor', 'transparency', 'specularReflection', 'specularColor', 'shine']


class Materials:
    def __init__ (self):
        self.nameOfGuid = {}     # surface guid -> material name
        self.surfaceOfName = {}  # material name -> the surface read by GetSurfaces (None: not readable)
        self.used = []           # the material names used by a face, in the order of first use

    def ReadSurfaces (self, geometries):
        guids = []
        for geometry in geometries:
            if 'error' in geometry:
                continue
            for body in geometry.get ('bodies', []):
                for polygon in body['polygons']:
                    guid = SurfaceGuidOf (polygon)
                    if guid and guid not in self.nameOfGuid and guid not in guids:
                        guids.append (guid)
        if not guids:
            return
        result = RunTapirCommand ('GetSurfaces', {
            'attributeIds': [{'attributeId': {'guid': guid}} for guid in guids],
            'fields': SURFACE_FIELDS
        }, required = False)
        surfaces = result.get ('surfaces', []) if result and 'error' not in result else []
        for i, guid in enumerate (guids):
            surface = surfaces[i] if i < len (surfaces) and 'error' not in surfaces[i] else None
            self.Add (guid, surface)

    def Add (self, guid, surface):
        base = MaterialName (surface['name']) if surface and surface.get ('name') else 'Surface_' + guid
        name = base
        if name in self.surfaceOfName or name == NO_SURFACE_MATERIAL:
            name = '{}_{}'.format (base, guid[:8])  # two surfaces with the same name after the cleanup
        self.nameOfGuid[guid] = name
        self.surfaceOfName[name] = surface

    def NameOf (self, polygon):
        guid = SurfaceGuidOf (polygon)
        name = self.nameOfGuid.get (guid, NO_SURFACE_MATERIAL) if guid else NO_SURFACE_MATERIAL
        if name not in self.used:
            self.used.append (name)
        return name

    def WriteMtl (self, path):
        with open (path, 'w', encoding = 'utf-8', newline = '\n') as mtl:
            mtl.write ('# Materials of the Archicad surfaces, exported by Tapir (archicad-addon/Workflows/export_obj.py)\n')
            for name in self.used:
                surface = self.surfaceOfName.get (name)
                mtl.write ('\nnewmtl {}\n'.format (name))
                if surface is None:
                    if name != NO_SURFACE_MATERIAL:
                        mtl.write ('# The surface could not be read with GetSurfaces.\n')
                    mtl.write ('Kd 0.800000 0.800000 0.800000\nKs 0.000000 0.000000 0.000000\nNs 0.000000\nd 1.000000\nillum 1\n')
                    continue
                mtl.write ('# Surface "{}"\n'.format (surface.get ('name', '')))
                mtl.write ('Kd {:.6f} {:.6f} {:.6f}\n'.format (*Rgb (surface.get ('surfaceColor'), 0.8)))
                specular = Clamp (surface.get ('specularReflection', 0.0) / 100.0)
                mtl.write ('Ks {:.6f} {:.6f} {:.6f}\n'.format (*(c * specular for c in Rgb (surface.get ('specularColor'), 1.0))))
                # Archicad's shine runs from 0 to 10000, the OBJ exponent from 0 to 1000.
                mtl.write ('Ns {:.6f}\n'.format (max (0.0, min (1000.0, surface.get ('shine', 0.0) / 10.0))))
                mtl.write ('d {:.6f}\n'.format (1.0 - Clamp (surface.get ('transparency', 0.0) / 100.0)))
                mtl.write ('illum 2\n')

    def GltfMaterial (self, name):
        # glTF colours are linear; Archicad's are sRGB. The roughness follows the Blinn-Phong
        # exponent of the .mtl (shine / 10): roughness = sqrt (2 / (exponent + 2)).
        surface = self.surfaceOfName.get (name)
        material = {'name': name, 'doubleSided': True}
        if surface is None:
            material['pbrMetallicRoughness'] = {'baseColorFactor': [0.6, 0.6, 0.6, 1.0], 'metallicFactor': 0.0, 'roughnessFactor': 1.0}
            return material
        alpha = 1.0 - Clamp (surface.get ('transparency', 0.0) / 100.0)
        exponent = max (0.0, min (1000.0, surface.get ('shine', 0.0) / 10.0))
        material['pbrMetallicRoughness'] = {
            'baseColorFactor': [SrgbToLinear (c) for c in Rgb (surface.get ('surfaceColor'), 0.8)] + [alpha],
            'metallicFactor': 0.0,
            'roughnessFactor': math.sqrt (2.0 / (exponent + 2.0))
        }
        if alpha < 1.0:
            material['alphaMode'] = 'BLEND'
        material['extras'] = {'archicadSurface': surface.get ('name', '')}
        return material


def SurfaceGuidOf (polygon):
    surfaceId = polygon.get ('surfaceId')
    return surfaceId.get ('guid') if isinstance (surfaceId, dict) else None


def MaterialName (name):
    # A material name is one token in OBJ and MTL files.
    return '_'.join (name.split ()) or 'Surface'


def Clamp (value):
    return max (0.0, min (1.0, value))


def Rgb (color, default):
    if not isinstance (color, dict):
        return (default, default, default)
    return tuple (Clamp (color.get (key, default)) for key in ('red', 'green', 'blue'))


def SrgbToLinear (c):
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


# --- glTF --------------------------------------------------------------------------------------
# A binary glTF 2.0 file: a JSON chunk and one binary buffer. Each element is a node with one mesh,
# with one primitive (triangles) per material. A face's vertices are written once per face with
# the face's normal, so the faces stay flat.

ARRAY_BUFFER = 34962
ELEMENT_ARRAY_BUFFER = 34963
FLOAT = 5126
UNSIGNED_INT = 5125


def YUp (x, y, z):
    return (x, z, -y)


def Normalized (v):
    length = math.sqrt (sum (c * c for c in v))
    return tuple (c / length for c in v) if length > EPSILON else None


def CrossProduct (a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def QuaternionOfColumns (x, y, z):
    # The rotation whose matrix has the columns x, y, z, as a glTF quaternion (x, y, z, w).
    m00, m10, m20 = x
    m01, m11, m21 = y
    m02, m12, m22 = z
    trace = m00 + m11 + m22
    if trace > 0.0:
        s = 0.5 / math.sqrt (trace + 1.0)
        q = ((m21 - m12) * s, (m02 - m20) * s, (m10 - m01) * s, 0.25 / s)
    elif m00 > m11 and m00 > m22:
        s = 2.0 * math.sqrt (1.0 + m00 - m11 - m22)
        q = (0.25 * s, (m01 + m10) / s, (m02 + m20) / s, (m21 - m12) / s)
    elif m11 > m22:
        s = 2.0 * math.sqrt (1.0 + m11 - m00 - m22)
        q = ((m01 + m10) / s, 0.25 * s, (m12 + m21) / s, (m02 - m20) / s)
    else:
        s = 2.0 * math.sqrt (1.0 + m22 - m00 - m11)
        q = ((m02 + m20) / s, (m12 + m21) / s, 0.25 * s, (m10 - m01) / s)
    length = math.sqrt (sum (c * c for c in q))
    return [c / length for c in q]


def GltfCamera ():
    """The camera node and the camera of a perspective 3D window, or (None, None, reason)."""
    settings = RunTapirCommand ('Get3DProjectionSettings', required = False)
    if not settings or 'error' in settings:
        return None, None, 'Get3DProjectionSettings failed'
    if not settings.get ('isPersp') or 'perspective' not in settings:
        return None, None, 'the 3D window shows an axonometry'
    persp = settings['perspective']
    pos, target = persp['cameraPosition'], persp['targetPosition']
    # The camera frame as Graphisoft's ModelAccess_Test exporter builds it, in Archicad axes:
    # right = dir x Z, top = right x dir, both turned by the roll angle (degrees) around dir.
    direction = Normalized ((target['x'] - pos['x'], target['y'] - pos['y'], target['z'] - pos['z'])) or (0.0, 1.0, 0.0)
    right0 = Normalized ((direction[1], -direction[0], 0.0)) or (1.0, 0.0, 0.0)
    top0 = CrossProduct (right0, direction)
    roll = math.radians (persp.get ('rollAngle', 0.0))
    right = tuple (r * math.cos (roll) + t * math.sin (roll) for r, t in zip (right0, top0))
    top = tuple (t * math.cos (roll) - r * math.sin (roll) for r, t in zip (right0, top0))
    back = tuple (-c for c in direction)
    # A glTF camera looks along its -Z with +Y up; the columns go to glTF's Y up axes.
    rotation = QuaternionOfColumns (YUp (*right), YUp (*top), YUp (*back))
    # The view cone is in degrees (Tapir's schema), but a value of at most pi can only be radians.
    viewCone = persp.get ('viewCone', 60.0)
    horizontal = viewCone if viewCone <= math.pi else math.radians (viewCone)
    horizontal = max (math.radians (1.0), min (math.radians (179.0), horizontal))
    aspect = args.cameraAspect if args.cameraAspect > 0 else 16.0 / 9.0
    camera = {
        'name': 'Archicad_3D_Window',
        'type': 'perspective',
        'perspective': {'yfov': 2.0 * math.atan (math.tan (horizontal / 2.0) / aspect), 'aspectRatio': aspect, 'znear': 0.05}
    }
    node = {
        'name': 'Archicad_3D_Window',
        'translation': list (YUp (pos['x'], pos['y'], pos['z'])),
        'rotation': rotation,
        'extras': {key: persp[key] for key in ('cameraPosition', 'targetPosition', 'viewCone', 'rollAngle', 'isTwoPointPersp') if key in persp}
    }
    return camera, node, None


class GlbWriter:
    def __init__ (self, materials, withMaterials):
        self.materials = materials
        self.withMaterials = withMaterials
        self.binary = bytearray ()
        self.gltf = {'asset': {'version': '2.0', 'generator': 'Tapir (archicad-addon/Workflows/export_obj.py)'},
                     'scene': 0, 'scenes': [{'name': 'Archicad', 'nodes': []}],
                     'nodes': [], 'meshes': [], 'accessors': [], 'bufferViews': []}
        self.materialIndex = {}  # material name -> index in gltf['materials']

    def AddView (self, values, target):
        if sys.byteorder != 'little':
            values.byteswap ()
        self.gltf['bufferViews'].append ({'buffer': 0, 'byteOffset': len (self.binary), 'byteLength': len (values) * values.itemsize, 'target': target})
        self.binary += values.tobytes ()
        return len (self.gltf['bufferViews']) - 1

    def AddAccessor (self, values, target, componentType, accessorType, count, bounds = None):
        accessor = {'bufferView': self.AddView (values, target), 'componentType': componentType, 'count': count, 'type': accessorType}
        if bounds:
            accessor['min'], accessor['max'] = bounds
        self.gltf['accessors'].append (accessor)
        return len (self.gltf['accessors']) - 1

    def MaterialOf (self, polygon):
        name = self.materials.NameOf (polygon)
        if name not in self.materialIndex:
            self.materialIndex[name] = len (self.materialIndex)
        return self.materialIndex[name]

    def AddElement (self, name, extras, bodies):
        """Adds one element; returns its vertex and triangle counts."""
        primitives = {}  # material index (None without materials) -> [positions, normals, indices]
        for body in bodies:
            vertices = [(v['x'], v['y'], v['z']) for v in body['vertices']]
            for polygon in body['polygons']:
                triangles = FacesOfPolygon (polygon, vertices, triangles = True)
                normal = Normalized (Normal ([vertices[v] for v in polygon['vertexIds']]))
                if not triangles or normal is None:
                    continue
                material = self.MaterialOf (polygon) if self.withMaterials else None
                positions, normals, indices = primitives.setdefault (material, [array.array ('f'), array.array ('f'), array.array ('I')])
                local = {}  # vertex index of the body -> index in the primitive, for this face
                for triangle in triangles:
                    for v in triangle:
                        if v not in local:
                            local[v] = len (positions) // 3
                            positions.extend (YUp (*vertices[v]))
                            normals.extend (YUp (*normal))
                        indices.append (local[v])
        if not primitives:
            return 0, 0
        mesh = {'name': name, 'primitives': []}
        vertexCount = triangleCount = 0
        for material, (positions, normals, indices) in primitives.items ():
            count = len (positions) // 3
            bounds = ([min (positions[i::3]) for i in range (3)], [max (positions[i::3]) for i in range (3)])
            primitive = {
                'attributes': {
                    'POSITION': self.AddAccessor (positions, ARRAY_BUFFER, FLOAT, 'VEC3', count, bounds),
                    'NORMAL': self.AddAccessor (normals, ARRAY_BUFFER, FLOAT, 'VEC3', count)
                },
                'indices': self.AddAccessor (indices, ELEMENT_ARRAY_BUFFER, UNSIGNED_INT, 'SCALAR', len (indices)),
                'mode': 4  # triangles
            }
            if material is not None:
                primitive['material'] = material
            mesh['primitives'].append (primitive)
            vertexCount += count
            triangleCount += len (indices) // 3
        self.gltf['meshes'].append (mesh)
        self.AddNode ({'name': name, 'mesh': len (self.gltf['meshes']) - 1, 'extras': extras})
        return vertexCount, triangleCount

    def AddNode (self, node):
        self.gltf['nodes'].append (node)
        self.gltf['scenes'][0]['nodes'].append (len (self.gltf['nodes']) - 1)

    def AddCamera (self, camera, node):
        self.gltf['cameras'] = [camera]
        node['camera'] = 0
        self.AddNode (node)

    def Write (self, path):
        if self.materialIndex:
            self.gltf['materials'] = [None] * len (self.materialIndex)
            for name, index in self.materialIndex.items ():
                self.gltf['materials'][index] = self.materials.GltfMaterial (name)
        for key in ('meshes', 'accessors', 'bufferViews'):
            if not self.gltf[key]:
                del self.gltf[key]  # glTF allows no empty arrays
        if not self.gltf['nodes']:
            del self.gltf['nodes']
            del self.gltf['scenes'][0]['nodes']
        binary = bytes (self.binary)
        if binary:
            self.gltf['buffers'] = [{'byteLength': len (binary)}]
        jsonChunk = json.dumps (self.gltf, separators = (',', ':')).encode ('utf-8')
        jsonChunk += b' ' * (-len (jsonChunk) % 4)
        binary += b'\0' * (-len (binary) % 4)
        length = 12 + 8 + len (jsonChunk) + (8 + len (binary) if binary else 0)
        with open (path, 'wb') as glb:
            glb.write (struct.pack ('<III', 0x46546C67, 2, length))  # 'glTF', version 2
            glb.write (struct.pack ('<II', len (jsonChunk), 0x4E4F534A))  # 'JSON'
            glb.write (jsonChunk)
            if binary:
                glb.write (struct.pack ('<II', len (binary), 0x004E4942))  # 'BIN'
                glb.write (binary)


def ElementsWithGeometry (elements, materials, readSurfaces):
    """Yields (element, details, geometry) for each element, reading them in batches."""
    for batch in Batches (elements, max (1, args.batchSize)):
        geometries = RunTapirCommand ('Get3DGeometryOfElements', {'elements': batch})['geometries3D']
        detailsList = RunTapirCommand ('GetDetailsOfElements', {'elements': batch, 'fields': ['type', 'id']})['detailsOfElements']
        if readSurfaces:
            materials.ReadSurfaces (geometries)
        for element, geometry, details in zip (batch, geometries, detailsList):
            yield element, details if 'error' not in details else {}, geometry


def ExportGlb (output, elements):
    materials = Materials ()
    writer = GlbWriter (materials, not args.noMtl)
    if args.zUp or args.holes != 'triangulate':
        print ('--zUp and --holes apply to OBJ only; glTF is Y up and made of triangles.')
    counts = {'objects': 0, 'vertices': 0, 'faces': 0, 'skipped': 0}
    for element, details, geometry in ElementsWithGeometry (elements, materials, not args.noMtl):
        if 'error' in geometry or not geometry.get ('bodies'):
            counts['skipped'] += 1
            continue
        extras = {'guid': element['elementId']['guid'], 'type': details.get ('type', ''), 'elementId': details.get ('id', '')}
        vertexCount, triangleCount = writer.AddElement (ObjectName (element, details), extras, geometry['bodies'])
        if triangleCount == 0:
            counts['skipped'] += 1
            continue
        counts['objects'] += 1
        counts['vertices'] += vertexCount
        counts['faces'] += triangleCount
    camera, node, reason = GltfCamera ()
    if camera:
        writer.AddCamera (camera, node)
    else:
        print ('No camera written: {}.'.format (reason))
    writer.Write (output)
    print ('Wrote {objects} meshes, {vertices} vertices and {faces} triangles; {skipped} elements had no 3D body.'.format (**counts))
    if not args.noMtl:
        print ('Wrote {} materials.'.format (len (writer.materialIndex)))
    print (output)


def Main ():
    output = args.output
    exportFormat = args.format or ('glb' if output and output.lower ().endswith ('.glb') else 'obj')
    if output is None:
        output = os.path.join (tempfile.gettempdir (), 'tapir_export_{}.{}'.format (datetime.datetime.now ().strftime ('%Y%m%d_%H%M%S'), exportFormat))
    elements = GetElementsToExport ()
    print ('Exporting {} elements to {}'.format (len (elements), output))
    if exportFormat == 'glb':
        ExportGlb (output, elements)
        return

    mtlPath = None if args.noMtl else os.path.splitext (output)[0] + '.mtl'
    materials = Materials ()
    counts = {'objects': 0, 'vertices': 0, 'faces': 0, 'skipped': 0}
    with open (output, 'w', encoding = 'utf-8', newline = '\n') as obj:
        obj.write ('# Exported from Archicad by Tapir (archicad-addon/Workflows/export_obj.py)\n')
        obj.write ('# Axes: {}\n'.format ('Archicad (Z up)' if args.zUp else 'Y up (x, z, -y of Archicad)'))
        obj.write ('\n'.join (CameraComment ()) + '\n')
        if mtlPath:
            obj.write ('mtllib {}\n'.format (os.path.basename (mtlPath)))

        vertexOffset = 1  # OBJ vertex indices start at 1 and run through the whole file.
        for element, details, geometry in ElementsWithGeometry (elements, materials, mtlPath is not None):
            if 'error' in geometry or not geometry.get ('bodies'):
                counts['skipped'] += 1
                continue
            obj.write ('o {}\n'.format (ObjectName (element, details)))
            counts['objects'] += 1
            currentMaterial = None  # each object names its first material
            for body in geometry['bodies']:
                vertices = [(v['x'], v['y'], v['z']) for v in body['vertices']]
                for v in body['vertices']:
                    obj.write ('v {:.6f} {:.6f} {:.6f}\n'.format (*ToObjAxes (v)))
                for polygon in body['polygons']:
                    if mtlPath:
                        material = materials.NameOf (polygon)
                        if material != currentMaterial:
                            obj.write ('usemtl {}\n'.format (material))
                            currentMaterial = material
                    for face in FacesOfPolygon (polygon, vertices):
                        obj.write ('f {}\n'.format (' '.join (str (vertexOffset + v) for v in face)))
                        counts['faces'] += 1
                vertexOffset += len (vertices)
                counts['vertices'] += len (vertices)

    print ('Wrote {objects} objects, {vertices} vertices and {faces} faces; {skipped} elements had no 3D body.'.format (**counts))
    print (output)
    if mtlPath:
        materials.WriteMtl (mtlPath)
        print ('Wrote {} materials.'.format (len (materials.used)))
        print (mtlPath)


Main ()
