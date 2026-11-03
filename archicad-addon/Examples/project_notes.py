import aclib

# Store the original Project Notes (File > Info > Project Notes) to restore them at the end
originalProjectNotes = aclib.RunTapirCommand ('GetProjectNotes', debug = False)

# Only the given fields are changed, the others keep their value
aclib.RunTapirCommand (
    'SetProjectNotes', {
        'client': 'Tapir Client',
        'projectStatus': 'Design Development',
        'notes': 'Changed by the Tapir SetProjectNotes command.'
    })

projectNotes = aclib.RunTapirCommand ('GetProjectNotes', debug = False)
print ('Project Notes after SetProjectNotes:')
print (aclib.JsonDumpDictionary ({
    'client': projectNotes['client'],
    'projectStatus': projectNotes['projectStatus'],
    'notes': projectNotes['notes']
}))

# architect and draftsmen were not given, so they are unchanged. Archicad keeps them as name parts (see the
# CONTACT_* and CAD_TECHNICIAN_* fields of GetProjectInfoFields), which SetProjectNotes leaves as they are.
print ('Architect and draftsmen unchanged: {}'.format (
    projectNotes['architect'] == originalProjectNotes['architect'] and projectNotes['draftsmen'] == originalProjectNotes['draftsmen']))

aclib.RunTapirCommand ('SetProjectNotes', originalProjectNotes, debug = False)
print ('Restored: {}'.format (aclib.RunTapirCommand ('GetProjectNotes', debug = False) == originalProjectNotes))
