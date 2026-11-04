import aclib

# Lists every Drawing of the layout book with the columns of Archicad's Drawing Manager:
# Status, Update Type, Stored in Project, Source and Path come from GetDetailsOfElements.
# The renovation filter is a setting of a view, not of the drawing: GetViewSettings returns it
# (renovationFilterName) for the views of the View Map.

def get_navigator_item_guids_from_tree(current_branch, navigator_item_guids, navigator_item_type):
    for navigator_item in current_branch:
        navigator_item = navigator_item['navigatorItem']
        if navigator_item['type'] == navigator_item_type:
            navigator_item_guids.append(navigator_item['navigatorItemId'])
        children = navigator_item.get('children')
        if children:
            get_navigator_item_guids_from_tree(children, navigator_item_guids, navigator_item_type)


navigator_tree = aclib.RunCommand(command='API.GetNavigatorItemTree',
                                  parameters={'navigatorTreeId': {'type': 'LayoutBook'}})

layout_guids = []
get_navigator_item_guids_from_tree(navigator_tree['navigatorTree']['rootItem']['children'],
                                   layout_guids, navigator_item_type='LayoutItem')

databases = aclib.RunTapirCommand(command='GetDatabaseIdFromNavigatorItemId',
                                  parameters={'navigatorItemIds': [{'navigatorItemId': guid} for guid in layout_guids]},
                                  debug=False)

drawings = aclib.RunTapirCommand(command='GetElementsByType',
                                 parameters={'elementType': 'Drawing',
                                             'databases': databases['databases']},
                                 debug=False)['elements']

details = aclib.RunTapirCommand(command='GetDetailsOfElements',
                                parameters={'elements': drawings}, debug=True)['detailsOfElements']

for drawing, drawing_details in zip(drawings, details):
    d = drawing_details.get('details', {})
    print('{}: status={} updateType={} storedInProject={} source={} path={}'.format(
        drawing['elementId']['guid'],
        d.get('status', '-'),
        'Manual' if d.get('manualUpdate') else 'Automatic',
        d.get('storedInProject', '-'),
        d.get('linkType', '-'),
        d.get('linkPath', '-')))
