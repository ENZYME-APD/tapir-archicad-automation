import aclib

LAYOUT_NAME = 'Tapir master layout test'
HELPER_LAYOUT_NAME = 'Tapir master layout test helper'
HELPER_MASTER_NAME = 'Tapir test master'


def find_by_type (branch, navigator_item_type):
    for entry in branch:
        item = entry['navigatorItem']
        if item.get ('type') == navigator_item_type:
            yield item
        children = item.get ('children')
        if children:
            yield from find_by_type (children, navigator_item_type)


def get_layout_book_items (navigator_item_type):
    layout_book = aclib.RunCommand ('API.GetNavigatorItemTree', {'navigatorTreeId': {'type': 'LayoutBook'}})
    return list (find_by_type (layout_book['navigatorTree']['rootItem']['children'], navigator_item_type))


def get_layout_settings (layout_db_id):
    return aclib.RunTapirCommand ('GetLayoutSettings', {'layoutDatabaseIds': [{'databaseId': layout_db_id}]})['layoutSettings'][0]


def set_layout_master (layout_db_id, master_fields):
    result = aclib.RunTapirCommand ('SetLayoutSettings', {'layoutsData': [dict ({'layoutDatabaseId': layout_db_id}, **master_fields)]})
    assert result['executionResults'][0]['success'], result


masters = get_layout_book_items ('MasterLayoutItem')
assert masters, 'No master layout found in the Layout Book'
first_master = masters[0]

# 1. Create a layout on the first master layout, so that the project's own layouts are left alone
created = aclib.RunTapirCommand ('CreateLayout', {'layoutsData': [{
    'layoutName': LAYOUT_NAME,
    'masterNavigatorItemId': first_master['navigatorItemId'],
}]}, debug = False)
layout_db_id = created['databases'][0]['databaseId']

# 2. GetLayoutSettings reports the master layout of a layout
settings = get_layout_settings (layout_db_id)
assert settings['masterNavigatorItemId']['guid'] == first_master['navigatorItemId']['guid'], settings
assert settings['masterLayoutName'] == first_master['name'], settings
first_master_db_id = settings['masterLayoutDatabaseId']

# 3. Another master layout to switch to: an existing one, or a new one created together with a helper layout
helper_layout_created = False
if len (masters) > 1:
    other_master = masters[1]
else:
    aclib.RunTapirCommand ('CreateLayout', {'layoutsData': [{
        'layoutName': HELPER_LAYOUT_NAME,
        'masterLayoutName': HELPER_MASTER_NAME,
    }]}, debug = False)
    helper_layout_created = True
    other_master = next (master for master in get_layout_book_items ('MasterLayoutItem') if master['name'] == HELPER_MASTER_NAME)

# 4. Change the master layout by navigator item
set_layout_master (layout_db_id, {'masterNavigatorItemId': other_master['navigatorItemId']})
settings = get_layout_settings (layout_db_id)
assert settings['masterNavigatorItemId']['guid'] == other_master['navigatorItemId']['guid'], settings
assert settings['masterLayoutName'] == other_master['name'], settings

# 5. Change it back by database id
set_layout_master (layout_db_id, {'masterLayoutDatabaseId': first_master_db_id})
settings = get_layout_settings (layout_db_id)
assert settings['masterNavigatorItemId']['guid'] == first_master['navigatorItemId']['guid'], settings

# 6. And by name
set_layout_master (layout_db_id, {'masterLayoutName': other_master['name']})
settings = get_layout_settings (layout_db_id)
assert settings['masterLayoutName'] == other_master['name'], settings

# 7. A master layout cannot be given a master layout
result = aclib.RunTapirCommand ('SetLayoutSettings', {'layoutsData': [{
    'layoutNavigatorItemId': first_master['navigatorItemId'],
    'masterNavigatorItemId': other_master['navigatorItemId'],
}]})
assert not result['executionResults'][0]['success'], result

# 8. Clean up the layouts created above (the helper master layout, if any, has to be deleted by hand)
layouts_to_delete = [LAYOUT_NAME, HELPER_LAYOUT_NAME] if helper_layout_created else [LAYOUT_NAME]
navigator_item_ids = [{'navigatorItemId': layout['navigatorItemId']}
                      for layout in get_layout_book_items ('LayoutItem') if layout['name'] in layouts_to_delete]
aclib.RunTapirCommand ('DeleteNavigatorItems', {'navigatorItemIds': navigator_item_ids})
