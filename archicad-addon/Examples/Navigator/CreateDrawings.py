"""Places the first story of the project as a Drawing on the first layout with CreateDrawings.

Needs at least one layout. Deletes the new Drawing at the end.
"""
import aclib

def AllItems (item):
    for child in item.get ('children', []):
        yield child['navigatorItem']
        yield from AllItems (child['navigatorItem'])

projectMap = aclib.RunTapirCommand ('GetNavigatorItemTree', {'navigatorMapId': 'ProjectMap'})['navigatorItemTree']
stories = [item for item in AllItems (projectMap) if item['type'] == 'StoryItem']
layouts = aclib.RunTapirCommand ('GetDatabases', {'types': ['Layout']})['databases']
if not stories or not layouts:
    raise SystemExit ('The project needs a story and a layout.')

drawings = aclib.RunTapirCommand ('CreateDrawings', {
    'drawingsData': [{
        'navigatorItemId': stories[0]['navigatorItemId'],
        'layoutDatabaseId': layouts[0]['databaseId'],
        'position': {'x': 0.1, 'y': 0.1},
        'name': 'Tapir Example Drawing'
    }]
})['elements']
aclib.PrintJson (drawings)

aclib.RunTapirCommand ('DeleteElements', {'elements': [d for d in drawings if 'elementId' in d]})
