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

# Every item that was created from another navigator item carries the source
# item's id. On a placed Drawing's Layout Book entry it identifies the view or
# schedule the Drawing was made from - the drawing element's own
# navigatorItemId (elem.drawing.drawingGuid) is an autotext id and does not
# resolve to a navigator item, so this is the only way back to the source. The
# source id can be fed to the official GetNavigatorItemsType command to tell
# e.g. schedule-based Drawings from story-based ones.
layoutBookTree = aclib.RunTapirCommand ('GetNavigatorItemTree', {
    'navigatorMapId': 'LayoutBook'
})['navigatorItemTree']

drawings = [
    item for item in CollectItems (layoutBookTree, [])
    if item['type'] == 'DrawingItem'
]

print ('{} drawings in the layout book:'.format (len (drawings)))
for drawing in drawings:
    sourceId = drawing.get ('sourceNavigatorItemId')
    print ('  {} (source: {})'.format (
        drawing['name'],
        sourceId['guid'] if sourceId is not None else 'none'))
