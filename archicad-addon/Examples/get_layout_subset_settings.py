import aclib

# Reads the settings of every subset in the Layout Book, e.g. to create a new
# subset or layouts that follow the same ID scheme with CreateLayoutSubset,
# which accepts the same fields.
layoutBookTree = aclib.RunTapirCommand ('GetNavigatorItemTree', {
    'navigatorMapId': 'LayoutBook'
})['navigatorItemTree']

def CollectSubsets (item, collectedSubsets):
    for child in item.get ('children', []):
        navigatorItem = child['navigatorItem']
        if navigatorItem['type'] == 'SubSetItem':
            collectedSubsets.append (navigatorItem)
        CollectSubsets (navigatorItem, collectedSubsets)
    return collectedSubsets

subsets = CollectSubsets (layoutBookTree, [])

subsetSettings = aclib.RunTapirCommand ('GetLayoutSubsetSettings', {
    'navigatorItemIds': [{'navigatorItemId': subset['navigatorItemId']} for subset in subsets]
})['subsetSettings']

print ('{} subsets in the layout book:'.format (len (subsets)))
for subset, settings in zip (subsets, subsetSettings):
    if 'error' in settings:
        print ('  {}: {}'.format (subset['name'], settings['error']['message']))
        continue
    print ('  {} (ID: {}, own prefix: {} "{}", numbering: {} from {}, continue: {}, upper prefix: {}, in ID sequence: {})'.format (
        settings['name'],
        subset['uiId'],
        settings['addOwnPrefix'],
        settings['ownPrefix'],
        settings['numberingStyle'],
        settings['startAt'],
        settings['continueNumbering'],
        settings['useUpperPrefix'],
        settings['includeToIDSequence']))
