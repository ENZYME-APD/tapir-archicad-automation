"""Clones the first story of the Project Map into a new View Map folder with CloneProjectMapItemToViewMap.

Deletes the clone and the folder at the end.
"""
import aclib

def AllItems (item):
    for child in item.get ('children', []):
        yield child['navigatorItem']
        yield from AllItems (child['navigatorItem'])

projectMap = aclib.RunTapirCommand ('GetNavigatorItemTree', {'navigatorMapId': 'ProjectMap'})['navigatorItemTree']
story = next (item for item in AllItems (projectMap) if item['type'] == 'StoryItem')

# A clone must go into a View Map folder, not to the View Map root.
folder = aclib.RunTapirCommand ('CreateViewMapFolder', {'folderName': 'Tapir Example Folder'})
clones = aclib.RunTapirCommand ('CloneProjectMapItemToViewMap', {
    'viewsData': [{'navigatorItemId': story['navigatorItemId'], 'parentNavigatorItemId': folder['navigatorItemId']}]
})['navigatorItems']
aclib.PrintJson (clones)

aclib.RunTapirCommand ('DeleteNavigatorItems', {'navigatorItemIds': [c for c in clones if 'navigatorItemId' in c]})
aclib.RunTapirCommand ('DeleteNavigatorItems', {'navigatorItemIds': [{'navigatorItemId': folder['navigatorItemId']}]})
