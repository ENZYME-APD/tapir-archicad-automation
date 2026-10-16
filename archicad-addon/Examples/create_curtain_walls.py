"""
Creates Curtain Wall elements with CreateCurtainWalls.

A curtain wall is placed along a reference polyline on the floor plan; every pair of
consecutive coordinates becomes one segment. Its frame and panel classes, surfaces,
boundary and corner frames come from the Curtain Wall tool defaults of the open
project (or from 'favoriteName'), so the examples only give what differs: the height,
the column and row pattern and which class fills which cell.

Three curtain walls are created on a 10 m grid so nothing overlaps.
"""

import aclib

# --- 1. the simplest call: only the reference line ---------------------------
# Everything else - height, pattern, classes - is the tool default.

aclib.RunTapirCommand ('CreateCurtainWalls', {
    'curtainWallsData': [
        {
            'coordinates': [
                { 'x': 0.0, 'y': 0.0 },
                { 'x': 8.0, 'y': 0.0 }
            ]
        }
    ]
})

# --- 2. a mullion-transom grid with fixed sizes -------------------------------
# The column pattern unit is one 1.25 m column, repeated along the segment; the row
# pattern unit is a 0.9 m parapet row and a 2.1 m window row, repeated up to the
# height. The partial unit at the end of the segment is what 'FixedSizes' leaves.

aclib.RunTapirCommand ('CreateCurtainWalls', {
    'curtainWallsData': [
        {
            'coordinates': [
                { 'x': 0.0, 'y': 10.0 },
                { 'x': 8.0, 'y': 10.0 }
            ],
            'zCoordinate': 0.0,
            'height': 6.0,
            'primaryPattern': {
                'logic': 'FixedSizes',
                'sizes': [1.25]
            },
            'secondaryPattern': {
                'logic': 'FixedSizes',
                'sizes': [0.9, 2.1]
            }
        }
    ]
})

# --- 3. two segments around a corner, best division, cells assigned -------------
# 'BestDivision' stretches the columns so that whole pattern units fill each segment.
# The pattern unit has two columns and two rows and repeats along the segment, so a
# cell assignment applies to every repetition: the first cell of the bottom row gets
# the second panel class (the tool defaults of a new project have a glass and a blind
# panel class), the second cell of the bottom row is left without a panel and without
# a frame below it (an open cell, e.g. an entrance). Cells not listed keep the
# assignment of the first default cell, because the pattern changed.

response = aclib.RunTapirCommand ('CreateCurtainWalls', {
    'curtainWallsData': [
        {
            'coordinates': [
                { 'x': 0.0, 'y': 20.0 },
                { 'x': 8.0, 'y': 20.0 },
                { 'x': 8.0, 'y': 26.0 }
            ],
            'height': 7.0,
            'primaryPattern': {
                'logic': 'BestDivision',
                'sizes': [1.5, 1.5],
                'origin': 'StartWithPattern'
            },
            'secondaryPattern': {
                'logic': 'FixedSizes',
                'sizes': [1.0, 2.5]
            },
            'cells': [
                { 'columnIndex': 0, 'rowIndex': 0, 'panelClassIndex': 2 },
                { 'columnIndex': 1, 'rowIndex': 0, 'panelClassIndex': 0, 'bottomFrameClassIndex': 0 }
            ]
        }
    ]
})

# The created curtain wall and its frames and panels can be read back like any other
# element; the panels and frames report the name of their class.
curtainWallId = response['elements'][0].get ('elementId')
if curtainWallId is not None:
    subelements = aclib.RunTapirCommand ('GetSubelementsOfHierarchicalElements', {
        'elements': [ { 'elementId': curtainWallId } ]
    })
    panels = subelements['subelements'][0]['cWallPanels']
    print ('Curtain wall {} has {} panels'.format (curtainWallId['guid'], len (panels)))
