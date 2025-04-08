import aclib

def get_drawing_guids_from_tree(current_branch, drawing_guids, navigator_item_type):
    for navigator_item in current_branch:
        navigator_item = navigator_item['navigatorItem']
        if navigator_item['type'] == navigator_item_type:
            drawing_guids.append(navigator_item['navigatorItemId'])
        children = navigator_item.get('children')
        if children:
            get_drawing_guids_from_tree(children, drawing_guids, navigator_item_type)


navigator_tree_id = {'navigatorTreeId': {'type': 'LayoutBook'}}

navigator_tree = aclib.RunCommand(command='API.GetNavigatorItemTree',
                                  parameters=navigator_tree_id)

drawing_gids = []
get_drawing_guids_from_tree(navigator_tree['navigatorTree']['rootItem']['children'],
                            drawing_gids, navigator_item_type='LayoutItem')

drawing_elements = [{'navigatorItemId': guid} for guid in drawing_gids]

databases = aclib.RunTapirCommand(command='GetDatabaseIdFromNavigatorItemId',
                                  parameters={'navigatorItemIds': drawing_elements}, debug=True)

drawings = aclib.RunTapirCommand(command='GetElementsByType',
                                 parameters={'elementType': 'Drawing',
                                             'databases': databases['databases']} )

aclib.RunTapirCommand(command='UpdateDrawings',
                      parameters=drawings, debug=True)
