import aclib

# Tells whether Archicad is in a hierarchical editing mode (e.g. curtain wall editing),
# so a script can ask the user to leave it before modifying the project.
result = aclib.RunTapirCommand('GetEditModeState', {})
if 'inEditMode' in result:
    print('In any hierarchical editing mode:', result['inEditMode'])
for mode in ['curtainWall', 'stair', 'beam', 'column']:
    state = result[mode]
    if state['inEditMode'] and 'elementId' in state:
        print('{}: editing {}'.format(mode, state['elementId']['guid']))
    else:
        print('{}: {}'.format(mode, state['inEditMode']))
