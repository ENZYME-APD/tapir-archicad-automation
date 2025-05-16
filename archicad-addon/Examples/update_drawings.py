import aclib

def get_navigator_item_guids_from_tree(current_branch, navigator_item_guids, navigator_item_type):
    for navigator_item in current_branch:
        navigator_item = navigator_item['navigatorItem']
        if navigator_item['type'] == navigator_item_type:
            navigator_item_guids.append(navigator_item['navigatorItemId'])
        children = navigator_item.get('children')
        if children:
            get_navigator_item_guids_from_tree(children, navigator_item_guids, navigator_item_type)


navigator_tree_id = {'navigatorTreeId': {'type': 'LayoutBook'}}

navigator_tree = aclib.RunCommand(command='API.GetNavigatorItemTree',
                                  parameters=navigator_tree_id)

layout_guids = []
get_navigator_item_guids_from_tree(navigator_tree['navigatorTree']['rootItem']['children'],
                            layout_guids, navigator_item_type='LayoutItem')

layout_elements = [{'navigatorItemId': guid} for guid in layout_guids]

databases = aclib.RunTapirCommand(command='GetDatabaseIdFromNavigatorItemId',
                                  parameters={'navigatorItemIds': layout_elements}, debug=True)

drawings = aclib.RunTapirCommand(command='GetElementsByType',
                                 parameters={'elementType': 'Drawing',
                                             'databases': databases['databases']} )['elements']

aclib.RunTapirCommand(command='UpdateDrawings',
                      parameters={'elements' : drawings}, debug=True)
