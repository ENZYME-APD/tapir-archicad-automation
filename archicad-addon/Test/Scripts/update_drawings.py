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

aclib.RunTapirCommand(command='GetDetailsOfElements',
                      parameters={'elements': drawings}, debug=True)

aclib.RunTapirCommand(command='UpdateDrawings',
                      parameters={'elements' : drawings}, debug=True)

# ChangeDrawingLink: relink one Drawing to a different source view. Scoped to a single,
# known database (the first layout that actually has a Drawing on it) so the returned
# layoutDatabaseId is unambiguous.
drawing_to_relink = None
drawing_database = None
for database in databases['databases']:
    database_drawings = aclib.RunTapirCommand(command='GetElementsByType',
                                              parameters={'elementType': 'Drawing',
                                                          'databases': [database]}, debug=False)['elements']
    if database_drawings:
        drawing_to_relink = database_drawings[0]
        drawing_database = database
        break

if drawing_to_relink is not None:
    aclib.RunTapirCommand(command='GetElementsByType',
                          parameters={'elementType': 'Drawing', 'databases': [drawing_database]}, debug=True)

    project_tree = aclib.RunCommand(command='API.GetNavigatorItemTree',
                                    parameters={'navigatorTreeId': {'type': 'ProjectMap'}})
    story_guids = []
    get_navigator_item_guids_from_tree(project_tree['navigatorTree']['rootItem']['children'],
                                story_guids, navigator_item_type='StoryItem')

    if len(story_guids) > 1:
        aclib.RunTapirCommand(command='ChangeDrawingLink',
                              parameters={'drawingsWithNewLinks': [{
                                  'elementId': drawing_to_relink['elementId'],
                                  'navigatorItemId': story_guids[1],
                                  'layoutDatabaseId': drawing_database['databaseId']
                              }]}, debug=True)
