import aclib

# List every autotext key Archicad knows (not only the PROPERTY- ones of GetAutoTextKeys) and
# resolve autotexts to their values without creating test texts. The keys and values depend on
# the project, the Archicad version and the UI language, so the script prints what it checked
# rather than the raw lists.

failed = 0
def check (label, ok):
    global failed
    print ('{}: {}'.format (label, 'OK' if ok else 'FAILED'))
    failed += 0 if ok else 1

allKeys = aclib.RunTapirCommand ('GetAllAutoTextKeys', {}, debug = False)['autoTexts']
check ('autotext keys found', len (allKeys) > 0)

projectInfoFields = aclib.RunTapirCommand ('GetProjectInfoFields', {}, debug = False)['fields']
projectNameField = next (f for f in projectInfoFields if f['projectInfoId'] == 'PROJECTNAME')

# Without an element context.
values = aclib.RunTapirCommand ('InterpretAutoTexts', {'autoTexts': ['<PROJECTNAME>']}, debug = False)['values']
check ('one result per autotext', len (values) == 1)
check ('<PROJECTNAME> resolves to the project name', values[0].get ('value') == projectNameField['projectInfoValue'])

# In the context of an element: a Text placed on the floor plan.
elements = aclib.RunTapirCommand ('CreateTexts', {
    'textsData': [
        {
            'coordinate': {'x': 0.0, 'y': -40.0, 'z': 0.0},
            'text': 'Autotext context',
            'height': 2.5
        }
    ]
}, debug = False)['elements']
values = aclib.RunTapirCommand ('InterpretAutoTexts', {
    'autoTexts': ['<PROJECTNAME>'],
    'elementId': elements[0]['elementId']
}, debug = False)['values']
check ('<PROJECTNAME> resolves in an element context', values[0].get ('value') == projectNameField['projectInfoValue'])
aclib.RunTapirCommand ('DeleteElements', {'elements': elements}, debug = False)

print ('interpret_autotexts: {} check(s) failed'.format (failed))
