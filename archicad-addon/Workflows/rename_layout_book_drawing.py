import aclib

# Renaming the Layout Book item of a placed Drawing with RenameNavigatorItem also
# sets the custom name of the Drawing itself (#1479), so GetDetailsOfElements and
# the Drawing's title show the same name as the Layout Book.

ORIGINAL_NAME = 'TAPIR RENAME DRAWING'
NEW_NAME = 'TAPIR RENAMED DRAWING'


def find_by_type(branch, navigator_item_type):
    for entry in branch:
        item = entry['navigatorItem']
        if item.get('type') == navigator_item_type:
            yield item
        children = item.get('children')
        if children:
            yield from find_by_type(children, navigator_item_type)


def layout_book_drawings():
    layout_book = aclib.RunCommand('API.GetNavigatorItemTree', {'navigatorTreeId': {'type': 'LayoutBook'}})
    return list(find_by_type(layout_book['navigatorTree']['rootItem']['children'], 'DrawingItem'))


# 1. Place a Drawing of a story on the first layout
layout_book = aclib.RunCommand('API.GetNavigatorItemTree', {'navigatorTreeId': {'type': 'LayoutBook'}})
layouts = list(find_by_type(layout_book['navigatorTree']['rootItem']['children'], 'LayoutItem'))
assert layouts, 'No layout found'
layout_db = aclib.RunTapirCommand('GetDatabaseIdFromNavigatorItemId', {
    'navigatorItemIds': [{'navigatorItemId': layouts[0]['navigatorItemId']}]
}, debug=False)
layout_db_id = layout_db['databases'][0]['databaseId']

project_map = aclib.RunCommand('API.GetNavigatorItemTree', {'navigatorTreeId': {'type': 'ProjectMap'}})
stories = list(find_by_type(project_map['navigatorTree']['rootItem']['children'], 'StoryItem'))
assert stories, 'No StoryItem found in ProjectMap'

create_result = aclib.RunTapirCommand('CreateDrawings', {'drawingsData': [{
    'navigatorItemId': stories[0]['navigatorItemId'],
    'layoutDatabaseId': layout_db_id,
    'name': ORIGINAL_NAME,
    'position': {'x': 0.1, 'y': 0.1},
}]})
created = [element for element in create_result['elements'] if 'elementId' in element]
assert len(created) == 1, 'CreateDrawings failed: {}'.format(create_result)

# 2. Find its Layout Book item and rename it
drawing_items = [item for item in layout_book_drawings() if item['name'] == ORIGINAL_NAME]
assert drawing_items, 'The Layout Book item of the new Drawing was not found'
# The Layout Book item has a guid of its own, not the Drawing's. RenameNavigatorItem finds the
# Drawing on the item's layout by the view it shows.

rename_result = aclib.RunTapirCommand('RenameNavigatorItem', {
    'navigatorItemId': drawing_items[0]['navigatorItemId'],
    'newName': NEW_NAME,
})

# 3. The Layout Book and the Drawing now carry the same name
assert any(item['name'] == NEW_NAME for item in layout_book_drawings())
details = aclib.RunTapirCommand('GetDetailsOfElements', {'elements': created}, debug=False)
drawing = details['detailsOfElements'][0]['details']
print('nameType: {}, customName: {}'.format(drawing['nameType'], drawing.get('customName')))
assert rename_result.get('success'), 'RenameNavigatorItem failed: {}'.format(rename_result)
assert drawing['nameType'] == 'CustomName'
assert drawing['customName'] == NEW_NAME

# 4. Clean up
aclib.RunTapirCommand('DeleteElements', {'elements': created}, debug=False)
