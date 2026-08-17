import aclib

def get_navigator_item_guids_from_tree(current_branch, navigator_item_guids, navigator_item_type):
    for navigator_item in current_branch:
        navigator_item = navigator_item['navigatorItem']
        if navigator_item['type'] == navigator_item_type:
            navigator_item_guids.append(navigator_item['navigatorItemId'])
        children = navigator_item.get('children')
        if children:
            get_navigator_item_guids_from_tree(children, navigator_item_guids, navigator_item_type)


# CreateDrawings must refuse a navigatorItemId that is not a view or viewpoint instead of
# handing it to Archicad's element creation. Nothing is created by any of the calls below.

# 1. A guid that exists nowhere in the project.
aclib.RunTapirCommand(command='CreateDrawings',
                      parameters={'drawingsData': [{
                          'navigatorItemId': {'guid': 'DEADBEEF-0000-0000-0000-000000000001'},
                          'name': 'InvalidSource',
                          'position': {'x': 0.05, 'y': 0.05}
                      }]}, debug=True)

# 2. A navigator item that exists but cannot be placed as a Drawing - a layout.
navigator_tree = aclib.RunCommand(command='API.GetNavigatorItemTree',
                                  parameters={'navigatorTreeId': {'type': 'LayoutBook'}})

layout_guids = []
get_navigator_item_guids_from_tree(navigator_tree['navigatorTree']['rootItem']['children'],
                                   layout_guids, navigator_item_type='LayoutItem')

if layout_guids:
    aclib.RunTapirCommand(command='CreateDrawings',
                          parameters={'drawingsData': [{
                              'navigatorItemId': layout_guids[0],
                              'name': 'InvalidSource',
                              'position': {'x': 0.05, 'y': 0.05}
                          }]}, debug=True)
