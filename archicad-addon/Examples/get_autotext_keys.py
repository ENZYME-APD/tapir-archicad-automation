import aclib

# Discover the autotext keys a Text or Label can embed as '<KEY>', and resolve keys back to the
# names shown in Archicad's Insert Autotext dialog. The key lists and names depend on the project,
# the Archicad version and the UI language, so the script prints what it checked rather than the
# raw lists.

failed = 0
def check (label, ok):
    # A False here is a real defect (e.g. names and keys swapped), so it is printed loudly
    # rather than recorded quietly into the golden.
    global failed
    print ('{}: {}'.format (label, 'OK' if ok else 'FAILED'))
    failed += 0 if ok else 1

# Without an element: the keys common to every element type (e.g. Element ID, Area).
genericKeys = aclib.RunTapirCommand ('GetAutoTextKeys', {}, debug = False)['autoTextKeys']
check ('generic keys found', len (genericKeys) > 0)
propertyKeys = [k for k in genericKeys if k['key'].startswith ('PROPERTY-')]
check ('property-based keys found', len (propertyKeys) > 0)
check ('names are not keys', all (not k['name'].startswith ('PROPERTY-') for k in genericKeys))

# With an element: its own type's keys come on top of the generic ones.
walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements']
if walls:
    wallKeys = aclib.RunTapirCommand ('GetAutoTextKeys', {'elementId': walls[0]['elementId']}, debug = False)['autoTextKeys']
    check ('wall has more keys than generic', len (wallKeys) > len (genericKeys))

# Name lookup: a project info key, a property key, and an unknown key (reported as an error item).
projectInfoFields = aclib.RunTapirCommand ('GetProjectInfoFields', {}, debug = False)['fields']
projectNameField = next (f for f in projectInfoFields if f['projectInfoId'] == 'PROJECTNAME')
keys = ['PROJECTNAME', 'NOT-A-REAL-KEY'] + ([propertyKeys[0]['key']] if propertyKeys else [])
names = aclib.RunTapirCommand ('GetAutoTextName', {'keys': keys}, debug = False)['autoTextNames']
check ('PROJECTNAME resolves to its project info name', names[0].get ('name') == projectNameField['projectInfoName'])
check ('unknown key is an error', 'error' in names[1])
if propertyKeys:
    check ('property key resolves to its own name', names[2].get ('name') == propertyKeys[0]['name'])

# Embedding a key: the content is resolved by Archicad, so reading the text back gives the value.
elements = aclib.RunTapirCommand ('CreateTexts', {
    'textsData': [
        {
            'coordinate': {'x': 0.0, 'y': -40.0, 'z': 0.0},
            'text': 'Project: <PROJECTNAME>',
            'height': 2.5
        }
    ]
}, debug = False)['elements']
details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': elements}, debug = False)
check ('embedded key resolved', details['detailsOfElements'][0]['details']['text'] == 'Project: ' + projectNameField['projectInfoValue'])
aclib.RunTapirCommand ('DeleteElements', {'elements': elements}, debug = False)

print ('get_autotext_keys: {} check(s) failed'.format (failed))
