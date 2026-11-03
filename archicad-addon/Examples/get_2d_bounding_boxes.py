import aclib

# Get2DBoundingBoxes reads the drawn extent of 2D elements, also of elements on a layout while
# another window is open, e.g. to check that a text placed on a layout stays inside its field.

def CreatedElements (result):
    return [] if result is None else [
        {'elementId': e['elementId']} for e in result['elements'] if 'elementId' in e]

def FindLayout (items):
    for item in items:
        navigatorItem = item['navigatorItem']
        if navigatorItem.get ('type') == 'LayoutItem':
            return navigatorItem
        layout = FindLayout (navigatorItem.get ('children') or [])
        if layout is not None:
            return layout
    return None

aclib.RunTapirCommand ('ChangeWindow', {'windowType': 'FloorPlan', 'storyIndex': 0}, debug = False)
polylines = CreatedElements (aclib.RunTapirCommand (
    'CreatePolylines', {
        'polylinesData': [
            {
                'floorInd': 0,
                'coordinates': [
                    {'x': 100.0, 'y': 100.0},
                    {'x': 110.0, 'y': 100.0},
                    {'x': 110.0, 'y': 105.0}
                ]
            }
        ]
    }, debug = False))
texts = CreatedElements (aclib.RunTapirCommand (
    'CreateTexts', {
        'textsData': [
            {
                'coordinate': {'x': 100.0, 'y': 120.0, 'z': 0.0},
                'text': 'A text on the floor plan',
                'height': 5.0,
                'angle': 0.0,
                'justification': 'Left'
            }
        ]
    }, debug = False))

# On a layout, if the project has one: the text is placed there, then the floor plan is opened again.
layoutTexts = []
layoutBook = aclib.RunCommand ('API.GetNavigatorItemTree', {'navigatorTreeId': {'type': 'LayoutBook'}})
layout = FindLayout (layoutBook['navigatorTree']['rootItem'].get ('children') or [])
if layout is not None:
    databases = aclib.RunTapirCommand (
        'GetDatabaseIdFromNavigatorItemId', {
            'navigatorItemIds': [{'navigatorItemId': layout['navigatorItemId']}]
        }, debug = False)
    layoutDatabaseId = databases['databases'][0]['databaseId']
    aclib.RunTapirCommand ('ChangeWindow', {'databaseId': layoutDatabaseId}, debug = False)
    layoutTexts = CreatedElements (aclib.RunTapirCommand (
        'CreateTexts', {
            'textsData': [
                {
                    'coordinate': {'x': 0.1, 'y': 0.3, 'z': 0.0},
                    'text': 'A text on a layout',
                    'height': 5.0,
                    'angle': 0.0,
                    'justification': 'Left'
                }
            ]
        }, debug = False))
    aclib.RunTapirCommand ('ChangeWindow', {'windowType': 'FloorPlan', 'storyIndex': 0}, debug = False)

result = aclib.RunTapirCommand ('Get2DBoundingBoxes', {'elements': polylines + texts + layoutTexts})
boxes = [item['boundingBox2D'] for item in result['boundingBoxes2D']]

polylineBox = boxes[0]
print ('polyline: ({xMin}, {yMin}) - ({xMax}, {yMax})'.format (**polylineBox))

# The text widths depend on the font metrics of the machine, so only their relations are printed.
textBox = boxes[1]
print ('floor plan text starts at its anchor: {}'.format (abs (textBox['xMin'] - 100.0) < 1e-6))
print ('floor plan text is wider than tall: {}'.format (textBox['xMax'] - textBox['xMin'] > textBox['yMax'] - textBox['yMin']))
if layoutTexts:
    layoutTextBox = boxes[2]
    print ('layout text in layout coordinates: {}'.format (abs (layoutTextBox['xMin'] - 0.1) < 1e-6 and layoutTextBox['yMax'] <= 0.3 + 1e-6))

aclib.RunTapirCommand ('DeleteElements', {'elements': polylines + texts}, debug = False)
if layoutTexts:
    aclib.RunTapirCommand ('ChangeWindow', {'databaseId': layoutDatabaseId}, debug = False)
    aclib.RunTapirCommand ('DeleteElements', {'elements': layoutTexts}, debug = False)
    aclib.RunTapirCommand ('ChangeWindow', {'windowType': 'FloorPlan', 'storyIndex': 0}, debug = False)
