"""Creates a View Map view of the first story, rotates it by 30 degrees with SetViewRotation and reads the rotation back.

Deletes the view and its folder at the end.
"""
import math
import aclib

def AllItems (item):
    for child in item.get ('children', []):
        yield child['navigatorItem']
        yield from AllItems (child['navigatorItem'])

projectMap = aclib.RunTapirCommand ('GetNavigatorItemTree', {'navigatorMapId': 'ProjectMap'})['navigatorItemTree']
story = next (item for item in AllItems (projectMap) if item['type'] == 'StoryItem')

folder = aclib.RunTapirCommand ('CreateViewMapFolder', {'folderName': 'Tapir Example Folder'})
view = aclib.RunTapirCommand ('CreateViewsInViewMap', {
    'viewsData': [{
        'navigatorItemId': story['navigatorItemId'],
        'parentNavigatorItemId': folder['navigatorItemId'],
        'name': 'Tapir Rotated View'
    }]
})['navigatorItems'][0]

aclib.PrintJson (aclib.RunTapirCommand ('SetViewRotation', {
    'navigatorItemIdsWithRotation': [{'navigatorItemId': view['navigatorItemId'], 'rotation': math.radians (30)}]
}))
aclib.RunTapirCommand ('GetView2DTransformations', {'navigatorItemIds': [{'navigatorItemId': view['navigatorItemId']}]})

aclib.RunTapirCommand ('DeleteNavigatorItems', {'navigatorItemIds': [{'navigatorItemId': view['navigatorItemId']}]})
aclib.RunTapirCommand ('DeleteNavigatorItems', {'navigatorItemIds': [{'navigatorItemId': folder['navigatorItemId']}]})
