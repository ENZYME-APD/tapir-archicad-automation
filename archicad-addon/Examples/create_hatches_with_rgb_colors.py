import aclib

# A Hatch can be colored by exact RGB values instead of pens: foregroundColor replaces fillPenIndex and
# backgroundColor replaces fillBackgroundPenIndex. The colors use the same {red, green, blue} (0.0-1.0)
# format as GetZoneCategories and GetPenTables, so a zone category's color can be passed straight through.
# Here a swatch is drawn in the color of the first zone category, as a zone area schedule would do.

def FormatColor (color):
    return '({:.2f}, {:.2f}, {:.2f})'.format (color['red'], color['green'], color['blue'])

zoneCategories = aclib.RunTapirCommand ('GetZoneCategories', {}, debug = False)['zoneCategories']
swatchColor = zoneCategories[0]['color']
print ('Swatch color taken from zone category "{}": {}'.format (zoneCategories[0]['name'], FormatColor (swatchColor)))

# The same color for the foreground and the background makes the swatch look solid with any fill pattern.
created = aclib.RunTapirCommand ('CreateHatches', {
    'hatchesData': [
        {
            'coordinates': [
                { 'x': 90.0, 'y': 10.0 },
                { 'x': 92.0, 'y': 10.0 },
                { 'x': 92.0, 'y': 12.0 },
                { 'x': 90.0, 'y': 12.0 }
            ],
            'contourPenIndex': 1,
            'foregroundColor': swatchColor,
            'backgroundColor': swatchColor
        }
    ]
}, debug = False)
elements = created['elements']

def PrintHatchColors (label):
    details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': elements}, debug = False)
    d = details['detailsOfElements'][0]['details']
    foreground = FormatColor (d['foregroundColor']) if 'foregroundColor' in d else 'pen {}'.format (d['fillPenIndex'])
    background = FormatColor (d['backgroundColor']) if 'backgroundColor' in d else 'pen {}'.format (d['fillBackgroundPenIndex'])
    print ('{}: foreground {}, background {}'.format (label, foreground, background))

PrintHatchColors ('After create')

# Setting a pen index again drops the RGB override of that side, so the pen is in effect.
aclib.RunTapirCommand ('SetDetailsOfElements', {
    'elementsWithDetails': [
        {
            'elementId': elements[0]['elementId'],
            'details': {
                'typeSpecificDetails': {
                    'fillPenIndex': 3,
                    'backgroundColor': { 'red': 1.0, 'green': 1.0, 'blue': 1.0 }
                }
            }
        }
    ]
}, debug = False)
PrintHatchColors ('After switching the foreground to pen 3 and the background to white')

aclib.RunTapirCommand ('DeleteElements', {'elements': elements}, debug = False)
