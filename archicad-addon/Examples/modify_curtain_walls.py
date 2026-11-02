"""
Modifies Curtain Wall elements in place with ModifyCurtainWalls.

A curtain wall created with CreateCurtainWalls (or placed by hand) keeps its GUID,
properties and classifications when it is changed this way, instead of being deleted
and recreated. The fields are those of CreateCurtainWalls: the height, the bottom
elevation and home story, the reference line (same number of segments) and the
column (primary) and row (secondary) pattern with its cell assignments. Only the
given fields change.

The example creates a curtain wall first so that it has something to modify.
"""

import aclib

# --- 1. create a curtain wall to modify -----------------------------------------

response = aclib.RunTapirCommand ('CreateCurtainWalls', {
    'curtainWallsData': [
        {
            'coordinates': [
                { 'x': 0.0, 'y': 30.0 },
                { 'x': 8.0, 'y': 30.0 }
            ],
            'zCoordinate': 0.0,
            'height': 4.0,
            'primaryPattern': {
                'logic': 'FixedSizes',
                'sizes': [1.0]
            },
            'secondaryPattern': {
                'logic': 'FixedSizes',
                'sizes': [1.0]
            }
        }
    ]
})
curtainWallId = response['elements'][0].get ('elementId')
if curtainWallId is None:
    raise SystemExit ('The curtain wall could not be created: {}'.format (response['elements'][0]))

# --- 2. change the height and the bottom elevation ------------------------------
# The frames and panels follow the new extent; everything else stays.

aclib.RunTapirCommand ('ModifyCurtainWalls', {
    'curtainWallsWithDetails': [
        {
            'elementId': curtainWallId,
            'height': 6.0,
            'zCoordinate': 0.5
        }
    ]
})

# --- 3. change the grid ---------------------------------------------------------
# A new column and row pattern is applied to every segment; the cells of the new
# pattern unit are assigned as in CreateCurtainWalls (the second column of the bottom
# row is left open: no panel and no frame below it).

aclib.RunTapirCommand ('ModifyCurtainWalls', {
    'curtainWallsWithDetails': [
        {
            'elementId': curtainWallId,
            'primaryPattern': {
                'logic': 'BestDivision',
                'sizes': [1.5, 1.5],
                'origin': 'StartWithPattern'
            },
            'secondaryPattern': {
                'logic': 'FixedSizes',
                'sizes': [0.9, 2.1]
            },
            'cells': [
                { 'columnIndex': 1, 'rowIndex': 0, 'panelClassIndex': 0, 'bottomFrameClassIndex': 0 }
            ]
        }
    ]
})

# --- 4. move the reference line ------------------------------------------------
# The number of coordinates must stay the number of segments plus one; the segments
# are moved and resized, not added or removed.

aclib.RunTapirCommand ('ModifyCurtainWalls', {
    'curtainWallsWithDetails': [
        {
            'elementId': curtainWallId,
            'coordinates': [
                { 'x': 0.0, 'y': 32.0 },
                { 'x': 10.0, 'y': 32.0 }
            ]
        }
    ]
})

# --- 5. the modified curtain wall is still the same element ---------------------

details = aclib.RunTapirCommand ('GetDetailsOfElements', {
    'elements': [ { 'elementId': curtainWallId } ]
})
print ('Height after the modifications: {}'.format (details['detailsOfElements'][0]['details']['height']))
