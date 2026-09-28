"""
Creates one of every element type Tapir can create.

Each call stands on its own, so any block below can be copied out as it is. The
elements are laid out on a 10 m grid so nothing overlaps.
"""

import aclib

# --- walls, and the openings that live in them -------------------------------

walls = aclib.RunTapirCommand ('CreateWalls', {
    'wallsData': [
        {
            'begCoordinate': { 'x': 0.0, 'y': 0.0 },
            'endCoordinate': { 'x': 8.0, 'y': 0.0 },
            'height': 3.0,
            'thickness': 0.3
        }
    ]
})
wallId = walls['elements'][0]['elementId']

# Windows, doors and openings need the wall they sit in. centerOffset is
# measured along the wall from its begCoordinate. Everything not given here -
# the library part and the marker (window stamp) settings included - comes from
# the Window / Door tool defaults of the open project, or from 'favoriteName'.
aclib.RunTapirCommand ('CreateWindows', {
    'windowsData': [
        { 'ownerWallId': wallId, 'centerOffset': 2.0, 'width': 1.0, 'height': 1.5 }
    ]
})

aclib.RunTapirCommand ('CreateDoors', {
    'doorsData': [
        { 'ownerWallId': wallId, 'centerOffset': 5.0, 'width': 1.0, 'height': 2.1 }
    ]
})

# An opening is a plain hole - basePoint is its bottom left corner in world space.
aclib.RunTapirCommand ('CreateOpenings', {
    'openingsData': [
        {
            'ownerElementId': wallId,
            'basePoint': { 'x': 6.5, 'y': 0.0, 'z': 1.0 },
            'width': 0.8,
            'height': 0.8
        }
    ]
})

# --- the other structural elements -------------------------------------------

aclib.RunTapirCommand ('CreateColumns', {
    'columnsData': [
        { 'coordinates': { 'x': 10.0, 'y': 0.0, 'z': 0.0 } }
    ]
})

aclib.RunTapirCommand ('CreateBeams', {
    'beamsData': [
        {
            'begCoordinate': { 'x': 20.0, 'y': 0.0 },
            'endCoordinate': { 'x': 26.0, 'y': 0.0 },
            'zCoordinate': 3.0
        }
    ]
})

# level is the Z of the slab's reference plane.
aclib.RunTapirCommand ('CreateSlabs', {
    'slabsData': [
        {
            'level': 0.0,
            'polygonCoordinates': [
                { 'x': 30.0, 'y': 0.0 },
                { 'x': 36.0, 'y': 0.0 },
                { 'x': 36.0, 'y': 5.0 },
                { 'x': 30.0, 'y': 5.0 }
            ]
        }
    ]
})

aclib.RunTapirCommand ('CreateRoofs', {
    'roofsData': [
        {
            'level': 3.0,
            'polygonCoordinates': [
                { 'x': 40.0, 'y': 0.0 },
                { 'x': 46.0, 'y': 0.0 },
                { 'x': 46.0, 'y': 5.0 },
                { 'x': 40.0, 'y': 5.0 }
            ]
        }
    ]
})

aclib.RunTapirCommand ('CreateMeshes', {
    'meshesData': [
        {
            'polygonCoordinates': [
                { 'x': 50.0, 'y': 0.0 },
                { 'x': 56.0, 'y': 0.0 },
                { 'x': 56.0, 'y': 5.0 },
                { 'x': 50.0, 'y': 5.0 }
            ]
        }
    ]
})

# Two points make a straight stair, three or more an L or U shaped one.
aclib.RunTapirCommand ('CreateStairs', {
    'stairsData': [
        {
            'baseLinePoints': [
                { 'x': 60.0, 'y': 0.0 },
                { 'x': 64.0, 'y': 0.0 }
            ],
            'zCoordinate': 0.0
        }
    ]
})

# size builds a box; pass body instead to describe arbitrary geometry.
aclib.RunTapirCommand ('CreateMorphs', {
    'morphsData': [
        {
            'basePoint': { 'x': 70.0, 'y': 0.0, 'z': 0.0 },
            'size': { 'x': 2.0, 'y': 2.0, 'z': 2.0 }
        }
    ]
})

# A zone is either automatic - it finds the room around a point - or manual,
# with the outline given. This is the manual form.
aclib.RunTapirCommand ('CreateZones', {
    'zonesData': [
        {
            'name': 'Office',
            'numberStr': '001',
            'geometry': {
                'polygonCoordinates': [
                    { 'x': 80.0, 'y': 0.0 },
                    { 'x': 86.0, 'y': 0.0 },
                    { 'x': 86.0, 'y': 5.0 },
                    { 'x': 80.0, 'y': 5.0 }
                ]
            }
        }
    ]
})

# --- library part based elements ---------------------------------------------
#
# These two need a library part that is loaded in the project. If yours are
# named differently the command answers "Not found library part with name ...",
# so change them to something your own libraries contain.

aclib.RunTapirCommand ('CreateObjects', {
    'objectsData': [
        { 'libraryPartName': 'Chair 01', 'coordinates': { 'x': 0.0, 'y': 10.0, 'z': 0.0 } }
    ]
})

aclib.RunTapirCommand ('CreateLamps', {
    'lampsData': [
        { 'libraryPartName': 'General Light', 'coordinates': { 'x': 10.0, 'y': 10.0, 'z': 0.0 } }
    ]
})

# --- 2D drafting elements ----------------------------------------------------

aclib.RunTapirCommand ('CreateLineElements', {
    'linesData': [
        {
            'begCoordinate': { 'x': 20.0, 'y': 10.0 },
            'endCoordinate': { 'x': 26.0, 'y': 10.0 }
        }
    ]
})

aclib.RunTapirCommand ('CreatePolylines', {
    'polylinesData': [
        {
            'coordinates': [
                { 'x': 30.0, 'y': 10.0 },
                { 'x': 33.0, 'y': 13.0 },
                { 'x': 36.0, 'y': 10.0 }
            ]
        }
    ]
})

# Angles are in radians, measured counterclockwise from the X axis.
aclib.RunTapirCommand ('CreateArcs', {
    'arcsData': [
        {
            'origin': { 'x': 40.0, 'y': 10.0 },
            'radius': 2.0,
            'begAngle': 0.0,
            'endAngle': 3.14159
        }
    ]
})

aclib.RunTapirCommand ('CreateCircles', {
    'circlesData': [
        { 'origin': { 'x': 50.0, 'y': 10.0 }, 'radius': 2.0 }
    ]
})

aclib.RunTapirCommand ('CreateSplines', {
    'splinesData': [
        {
            'coordinates': [
                { 'x': 60.0, 'y': 10.0 },
                { 'x': 62.0, 'y': 13.0 },
                { 'x': 64.0, 'y': 10.0 }
            ]
        }
    ]
})

aclib.RunTapirCommand ('CreateHatches', {
    'hatchesData': [
        {
            'coordinates': [
                { 'x': 70.0, 'y': 10.0 },
                { 'x': 74.0, 'y': 10.0 },
                { 'x': 74.0, 'y': 14.0 },
                { 'x': 70.0, 'y': 14.0 }
            ]
        }
    ]
})

aclib.RunTapirCommand ('CreateHotspots', {
    'hotspotsData': [
        { 'position': { 'x': 80.0, 'y': 10.0 } }
    ]
})

aclib.RunTapirCommand ('CreateTexts', {
    'textsData': [
        { 'coordinate': { 'x': 0.0, 'y': 20.0, 'z': 0.0 }, 'text': 'Created by Tapir' }
    ]
})

# A label either points at an element or stands on its own with a leader line.
aclib.RunTapirCommand ('CreateLabels', {
    'labelsData': [
        { 'parentElementId': wallId, 'text': 'Exterior wall' }
    ]
})

# --- dimensions --------------------------------------------------------------
#
# An associative dimension follows the elements it measures. Each witness point
# names an element, and direction is the direction the dimension line runs in.

aclib.RunTapirCommand ('CreateAssociativeDimensions', {
    'dimensionsData': [
        {
            'referencePoint': { 'x': 0.0, 'y': -2.0 },
            'direction': { 'x': 1.0, 'y': 0.0 },
            'witnessPoints': [
                { 'elementId': wallId, 'nodeId': 1 },
                { 'elementId': wallId, 'nodeId': 2 }
            ]
        }
    ]
})

aclib.RunTapirCommand ('CreateWallThicknessDimensions', {
    'dimensionsData': [
        {
            'wallId': wallId,
            'referencePoint': { 'x': 4.0, 'y': -1.0 },
            'direction': { 'x': 0.0, 'y': 1.0 }
        }
    ]
})
