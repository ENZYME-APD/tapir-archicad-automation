"""
Example for issue #1390: CreateAssociativeDimensions, CreateAssociativeDimensionsOnSection and
CreateWallThicknessDimensions take an optional 'favoriteName', so a dimension is created with
the settings of a saved Dimension favorite - its layer, line pen, markers and so on. The
favorite's text size, text pen and text frame are not applied (tested in Archicad 28), so the
same commands take an optional 'textHeight' (mm) and 'textPen' for the dimension texts.

The script places two walls and a dimension between them, saves that dimension as a favorite,
then dimensions the walls again from the favorite, once with a larger text in pen 20, tries a
favorite that does not exist, and cleans up after itself. In Archicad, save a favorite with
other markers or pens and pass its name instead to see its settings on the new dimensions.
The texts are not read back (GetDetailsOfElements does not support dimensions), so check
them in the floor plan.
"""
import aclib

def run (command, parameters = None):
    return aclib.RunTapirCommand (command, parameters or {}, debug = False)

passes = fails = 0

def check (label, expected, got):
    global passes, fails
    ok = (got == expected)
    if ok:
        passes += 1
    else:
        fails += 1
    print ('  [{}] {}  (expected={!r}, got={!r})'.format ('PASS' if ok else 'FAIL', label, expected, got))

favoriteName = 'DimensionFromPython'

walls = run ('CreateWalls', {
    'wallsData': [
        {
            'begCoordinate': {'x': 0.0, 'y': -80.0},
            'endCoordinate': {'x': 4.0, 'y': -80.0},
            'height': 3.0,
            'thickness': 0.3
        },
        {
            'begCoordinate': {'x': 7.0, 'y': -80.0},
            'endCoordinate': {'x': 11.0, 'y': -80.0},
            'height': 3.0,
            'thickness': 0.3
        }
    ]
})['elements']
wallIds = [w['elementId'] for w in walls]

def dimensionData (referenceY, favorite = None):
    # inIndex 1 and 2 are the two end points of a wall's reference line.
    data = {
        'referencePoint': {'x': 0.0, 'y': referenceY},
        'direction': {'x': 1.0, 'y': 0.0},
        'witnessPoints': [
            {'elementId': wallIds[0], 'inIndex': 2},
            {'elementId': wallIds[1], 'inIndex': 1}
        ]
    }
    if favorite is not None:
        data['favoriteName'] = favorite
    return data

created = []

# --- the template dimension the favorite is saved from --------------------------------
template = run ('CreateAssociativeDimensions', {'dimensionsData': [dimensionData (-78.0)]})['elements'][0]
created.append (template)
run ('CreateFavoritesFromElements', {'favoritesFromElements': [{
    'elementId': template['elementId'],
    'favorite': favoriteName
}]})

# --- 1. a dimension from the favorite, next to one without it --------------------------
print ('CreateAssociativeDimensions with and without favoriteName')
response = run ('CreateAssociativeDimensions', {'dimensionsData': [
    dimensionData (-77.0, favoriteName),
    dimensionData (-76.0)
]})
check ('dimension from the favorite created', True, 'elementId' in response['elements'][0])
check ('dimension without favorite created', True, 'elementId' in response['elements'][1])
created += [e for e in response['elements'] if 'elementId' in e]

# --- 2. a wall thickness dimension from the favorite ----------------------------------
print ('CreateWallThicknessDimensions with favoriteName, textHeight and textPen')
response = run ('CreateWallThicknessDimensions', {'dimensionsData': [{
    'wallId': wallIds[0],
    'referencePoint': {'x': 2.0, 'y': -79.0},
    'direction': {'x': 0.0, 'y': 1.0},
    'favoriteName': favoriteName,
    'textHeight': 5.0,
    'textPen': 20
}]})
check ('wall thickness dimension created', True, 'elementId' in response['elements'][0])
created += [e for e in response['elements'] if 'elementId' in e]

# --- 3. an unknown favorite is reported by name ---------------------------------------
print ('CreateAssociativeDimensions with a favoriteName that does not exist')
response = run ('CreateAssociativeDimensions', {'dimensionsData': [dimensionData (-75.0, favoriteName + ' (no such favorite)')]})
error = response['elements'][0].get ('error', {})
check ('error names the favorite', True, favoriteName in error.get ('message', ''))
created += [e for e in response['elements'] if 'elementId' in e]

# --- cleanup ---------------------------------------------------------------------------
run ('DeleteFavorites', {'favorites': [favoriteName]})
run ('DeleteElements', {'elements': created + walls})

print ('{} passed, {} failed'.format (passes, fails))
