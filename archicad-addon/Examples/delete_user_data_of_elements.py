import aclib

# DeleteUserDataOfElements deletes the user data that this add-on stored on the elements.
# The user data of other add-ons is left as it is. All deletions are one undo step.
walls = aclib.RunTapirCommand (
    'GetElementsByType', {
        'elementType': 'Wall'
    }, debug = False)['elements']

if not walls:
    print ('The project has no Walls.')
else:
    executionResults = aclib.RunTapirCommand (
        'DeleteUserDataOfElements', {
            'elements': walls[:3]
        }, debug = False)['executionResults']

    for wall, executionResult in zip (walls, executionResults):
        print ('Wall {}'.format (wall['elementId']['guid']))
        if executionResult['success']:
            print ('  User data deleted.')
        else:
            print ('  Error:\n{}'.format (aclib.JsonDumpDictionary (executionResult['error'])))
