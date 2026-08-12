import aclib

# Tapir's own GetNavigatorItemTree takes a map id and reports the ID shown on
# the navigator next to the name, plus the flags behind the View Settings
# "Custom" / "By Project Map" radio buttons.
navigatorItemTree = aclib.RunTapirCommand ('GetNavigatorItemTree', {
    'navigatorMapId': 'PublicViewMap'
})['navigatorItemTree']

def CollectItems (item, collectedItems):
    for child in item.get ('children', []):
        navigatorItem = child['navigatorItem']
        collectedItems.append (navigatorItem)
        CollectItems (navigatorItem, collectedItems)
    return collectedItems

views = [
    item for item in CollectItems (navigatorItemTree, [])
    if item['type'] != 'FolderItem'
]

# The views whose ID or name was hand-written instead of following the Project
# Map source. Comparing the strings to the source's cannot tell these apart: a
# custom value typed identical to the source's looks inherited.
overridden = [
    view for view in views if view['customUiId'] or view['customName']
]

print ('{} of {} views have a custom ID or name:'.format (len (overridden), len (views)))
for view in overridden:
    print ('  {} {} (custom ID: {}, custom name: {}, independent: {})'.format (
        view['uiId'],
        view['name'],
        view['customUiId'],
        view['customName'],
        view['isIndependent']))
