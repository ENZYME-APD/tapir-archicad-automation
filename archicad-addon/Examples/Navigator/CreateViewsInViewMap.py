"""Creates an independent View Map view of the first story in a new folder with CreateViewsInViewMap.

Deletes the view and the folder at the end.
"""
import aclib

def AllItems (item):
    for child in item.get ('children', []):
        yield child['navigatorItem']
        yield from AllItems (child['navigatorItem'])

projectMap = aclib.RunTapirCommand ('GetNavigatorItemTree', {'navigatorMapId': 'ProjectMap'})['navigatorItemTree']
story = next (item for item in AllItems (projectMap) if item['type'] == 'StoryItem')

# A new view must go into a View Map folder, not to the View Map root.
folder = aclib.RunTapirCommand ('CreateViewMapFolder', {'folderName': 'Tapir Example Folder'})
views = aclib.RunTapirCommand ('CreateViewsInViewMap', {
    'viewsData': [{
        'navigatorItemId': story['navigatorItemId'],
        'parentNavigatorItemId': folder['navigatorItemId'],
        'name': 'Tapir Example View'
    }]
})['navigatorItems']
aclib.PrintJson (views)

aclib.RunTapirCommand ('DeleteNavigatorItems', {'navigatorItemIds': [v for v in views if 'navigatorItemId' in v]})
aclib.RunTapirCommand ('DeleteNavigatorItems', {'navigatorItemIds': [{'navigatorItemId': folder['navigatorItemId']}]})
