"""Prints the zoom and the rotation of every story of the project with GetView2DTransformations."""
import aclib

def AllItems (item):
    for child in item.get ('children', []):
        yield child['navigatorItem']
        yield from AllItems (child['navigatorItem'])

projectMap = aclib.RunTapirCommand ('GetNavigatorItemTree', {'navigatorMapId': 'ProjectMap'})['navigatorItemTree']
stories = [item for item in AllItems (projectMap) if item['type'] == 'StoryItem']

aclib.PrintJson (aclib.RunTapirCommand ('GetView2DTransformations', {
    'navigatorItemIds': [{'navigatorItemId': story['navigatorItemId']} for story in stories]
}))
