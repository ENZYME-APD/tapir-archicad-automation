import aclib

# Add-on viewpoints are the Project Map items that add-ons create through the
# Archicad API. Walk them from the roots down and print the details of each.

def collectViewpointIds(viewpointIds):
    collected = []
    for viewpointId in viewpointIds:
        collected.append(viewpointId)
        children = aclib.RunTapirCommand(
            'GetAddOnViewpointChildren', viewpointId)
        if 'viewpointIds' in children:
            collected.extend(collectViewpointIds(children['viewpointIds']))
    return collected


roots = aclib.RunTapirCommand('GetAddOnViewpointRoots')
if 'viewpointIds' in roots:
    viewpointIds = collectViewpointIds(roots['viewpointIds'])
    print(f'{len(viewpointIds)} add-on viewpoint items')
    if viewpointIds:
        viewpoints = aclib.RunTapirCommand(
            'GetAddOnViewpoints', {'viewpointIds': viewpointIds})
        for viewpoint in viewpoints['viewpoints']:
            print(viewpoint)
else:
    print(roots)

# Draw the master layout of the current window in the context of the first
# layout: its autotexts then show that layout's values. The current window
# must be a master layout, otherwise the command returns an error.
layouts = aclib.RunTapirCommand('GetDatabases', {'types': ['Layout']})['databases']
if layouts:
    result = aclib.RunTapirCommand(
        'SetMasterLayoutContext', {'layoutDatabaseId': layouts[0]['databaseId']})
    print(result)
else:
    print('The project has no layouts.')
