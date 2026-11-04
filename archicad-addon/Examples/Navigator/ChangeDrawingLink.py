"""Places a Drawing of the first story on a layout, then links it to the second story with ChangeDrawingLink.

Needs at least two stories and a layout. ChangeDrawingLink recreates the Drawing, so it returns
a new element id. Deletes the Drawing at the end.
"""
import aclib

def AllItems (item):
    for child in item.get ('children', []):
        yield child['navigatorItem']
        yield from AllItems (child['navigatorItem'])

projectMap = aclib.RunTapirCommand ('GetNavigatorItemTree', {'navigatorMapId': 'ProjectMap'})['navigatorItemTree']
stories = [item for item in AllItems (projectMap) if item['type'] == 'StoryItem']
layouts = aclib.RunTapirCommand ('GetDatabases', {'types': ['Layout']})['databases']
if len (stories) < 2 or not layouts:
    raise SystemExit ('The project needs two stories and a layout.')

layoutDatabaseId = layouts[0]['databaseId']
drawing = aclib.RunTapirCommand ('CreateDrawings', {
    'drawingsData': [{
        'navigatorItemId': stories[0]['navigatorItemId'],
        'layoutDatabaseId': layoutDatabaseId,
        'position': {'x': 0.1, 'y': 0.1}
    }]
})['elements'][0]

relinked = aclib.RunTapirCommand ('ChangeDrawingLink', {
    'drawingsWithNewLinks': [{
        'elementId': drawing['elementId'],
        'navigatorItemId': stories[1]['navigatorItemId'],
        'layoutDatabaseId': layoutDatabaseId
    }]
})['elements']
aclib.PrintJson (relinked)

aclib.RunTapirCommand ('DeleteElements', {'elements': [d for d in relinked if 'elementId' in d]})
