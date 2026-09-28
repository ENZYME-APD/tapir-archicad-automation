import math

import aclib


def find_by_type(branch, navigator_item_type):
    for entry in branch:
        item = entry['navigatorItem']
        if item.get('type') == navigator_item_type:
            yield item
        children = item.get('children')
        if children:
            yield from find_by_type(children, navigator_item_type)


# 1. Find a layout and its database
layout_book = aclib.RunCommand('API.GetNavigatorItemTree', {'navigatorTreeId': {'type': 'LayoutBook'}})
layouts = list(find_by_type(layout_book['navigatorTree']['rootItem']['children'], 'LayoutItem'))
assert layouts, 'No layout found'
layout_db = aclib.RunTapirCommand('GetDatabaseIdFromNavigatorItemId', {
    'navigatorItemIds': [{'navigatorItemId': layouts[0]['navigatorItemId']}]
}, debug=False)
layout_db_id = layout_db['databases'][0]['databaseId']

# 2. Find a StoryItem (floor plan viewpoint) to place as Drawing
project_map = aclib.RunCommand('API.GetNavigatorItemTree', {'navigatorTreeId': {'type': 'ProjectMap'}})
views = list(find_by_type(project_map['navigatorTree']['rootItem']['children'], 'StoryItem'))
assert views, 'No StoryItem found in ProjectMap'
nav_item_id = views[0]['navigatorItemId']

# 3. Create two Drawings on the layout:
#    - one without a name, whose title follows the source view's own name (nameType),
#    - one with a custom name and an explicit transform (angle/drawingScale/modelOffset).
create_result = aclib.RunTapirCommand('CreateDrawings', {'drawingsData': [
    {
        'navigatorItemId': nav_item_id,
        'layoutDatabaseId': layout_db_id,
        'nameType': 'ViewOrSourceFileName',
        'position': {'x': 0.1, 'y': 0.1},
    },
    {
        'navigatorItemId': nav_item_id,
        'layoutDatabaseId': layout_db_id,
        'name': 'TransformedDrawing',
        'position': {'x': 0.3, 'y': 0.1},
        'angle': math.radians(30),
        'drawingScale': 200.0,
        'modelOffset': {'x': 1.0, 'y': 2.0},
    },
]})
created = [element for element in create_result['elements'] if 'elementId' in element]
assert len(created) == 2, 'CreateDrawings failed: {}'.format(create_result)

# 4. Verify via GetDetailsOfElements
details = aclib.RunTapirCommand('GetDetailsOfElements', {'elements': created}, debug=False)
auto_named, transformed = [d['details'] for d in details['detailsOfElements']]
print('nameType of the unnamed Drawing: {}'.format(auto_named['nameType']))
assert auto_named['nameType'] == 'ViewOrSourceFileName'
print('nameType of the named Drawing: {}'.format(transformed['nameType']))
assert transformed['nameType'] == 'CustomName'
assert transformed['customName'] == 'TransformedDrawing'
print('angle: {:.6f}, drawingScale: {:.1f}, modelOffset: ({:.1f}, {:.1f})'.format(
    transformed['angle'], transformed['drawingScale'],
    transformed['modelOffset']['x'], transformed['modelOffset']['y']))
assert abs(transformed['angle'] - math.radians(30)) < 1e-9
assert abs(transformed['drawingScale'] - 200.0) < 1e-9
assert abs(transformed['modelOffset']['x'] - 1.0) < 1e-9
assert abs(transformed['modelOffset']['y'] - 2.0) < 1e-9

# 5. Clean up
aclib.RunTapirCommand('DeleteElements', {'elements': created}, debug=False)
