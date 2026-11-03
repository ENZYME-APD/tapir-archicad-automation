import aclib

curtainWalls = aclib.RunCommand (
    'API.GetElementsByType', {
        'elementType': 'CurtainWall'
    })['elements']
print('Project contains {} Curtain Wall(s)'.format(len(curtainWalls)))

if curtainWalls:
    # Open the first Curtain Wall in hierarchical 3D editing mode
    aclib.RunTapirCommand (
        'EditHierarchicalElement', {
            'elementId': curtainWalls[0]['elementId']
        })

    # Leave the editing mode: accept True keeps the changes (OK),
    # accept False discards them (Cancel)
    aclib.RunTapirCommand (
        'FinishHierarchicalEditing', {
            'accept': False
        })

# Finishing again fails, there is no editing mode to finish anymore
aclib.RunTapirCommand (
    'FinishHierarchicalEditing', {
        'accept': True
    })
