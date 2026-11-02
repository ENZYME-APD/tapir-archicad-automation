import base64
import aclib

# Create an add-on viewpoint root, a group in it and a leaf node in the group,
# rename the leaf node, then read all three back. Roots and groups can't be
# created in a Teamwork project.

def createViewpoint(viewpoint):
    result = aclib.RunTapirCommand(
        'CreateAddOnViewpoints', {'viewpoints': [viewpoint]})['viewpoints'][0]
    print(result)
    return result.get('viewpointId')


rootId = createViewpoint({
    'itemType': 'Root',
    'displayId': 'TAPIR',
    'displayName': 'Tapir example root'})

groupId = None
if rootId:
    groupId = createViewpoint({
        'itemType': 'Group',
        'parentViewpointId': rootId,
        'displayId': 'G1',
        'displayName': 'Tapir example group'})

leafId = None
if groupId:
    leafId = createViewpoint({
        'itemType': 'Leaf',
        'parentViewpointId': groupId,
        'displayId': 'L1',
        'displayName': 'Tapir example leaf',
        'data': base64.b64encode(b'created by Tapir').decode('ascii')})

if leafId:
    changed = aclib.RunTapirCommand('ChangeAddOnViewpoints', {
        'viewpoints': [{
            'viewpointId': leafId,
            'displayName': 'Tapir example leaf (renamed)'}]})
    print(changed)

    viewpoints = aclib.RunTapirCommand('GetAddOnViewpoints', {
        'viewpointIds': [{'viewpointId': id} for id in (rootId, groupId, leafId)]})
    for viewpoint in viewpoints['viewpoints']:
        print(viewpoint)
        if viewpoint.get('data'):
            print(base64.b64decode(viewpoint['data']))
