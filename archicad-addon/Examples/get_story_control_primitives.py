import aclib
from collections import Counter

# Pick the first section from the Project Map and resolve its database.
navigatorTree = aclib.RunCommand ('API.GetNavigatorItemTree', {
    'navigatorTreeId': { 'type': 'ProjectMap' }
})

def FindSections (item, foundItems):
    for child in item.get ('children', []):
        navigatorItem = child['navigatorItem']
        if navigatorItem.get ('type') == 'SectionItem':
            foundItems.append (navigatorItem['navigatorItemId'])
        FindSections (navigatorItem, foundItems)
    return foundItems

sectionNavigatorItemIds = FindSections (navigatorTree['navigatorTree']['rootItem'], [])
if not sectionNavigatorItemIds:
    print ('No section found in the Project Map.')
    raise SystemExit (0)

databases = aclib.RunTapirCommand ('GetDatabaseIdFromNavigatorItemId', {
    'navigatorItemIds': [
        { 'navigatorItemId': sectionNavigatorItemIds[0] }
    ]
}, debug=False)['databases']

result = aclib.RunTapirCommand ('GetStoryControlPrimitives', {
    'databaseId': databases[0]['databaseId']
}, debug=False)
if result is None or 'primitives' not in result:
    print ('Failed: {}'.format (result))
    raise SystemExit (1)

primitives = result['primitives']
print ('Primitives by type: {}'.format (dict (Counter (p['type'] for p in primitives))))
for primitive in primitives:
    if primitive['type'] == 'Line':
        print ('Story line from ({0:.3f}, {1:.3f}) to ({2:.3f}, {3:.3f})'.format (
            primitive['c1']['x'], primitive['c1']['y'], primitive['c2']['x'], primitive['c2']['y']))
    elif primitive['type'] == 'Text':
        print ('Text "{}" at ({:.3f}, {:.3f})'.format (primitive.get ('text', ''), primitive['loc']['x'], primitive['loc']['y']))
