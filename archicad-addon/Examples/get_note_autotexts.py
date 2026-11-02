import aclib

# List the autotexts available in the notes of each dimension type, with the element types each
# autotext is available for. The autotexts are identified by the numeric value of their
# API_NoteAutotextID in the Archicad API.

for noteOwnerType in ['Dimension', 'RadialDimension', 'LevelDimension', 'AngleDimension']:
    print (noteOwnerType)
    response = aclib.RunTapirCommand ('GetNoteAutotexts', {'noteOwnerType': noteOwnerType}, debug = False)
    if not response or 'error' in response:
        continue
    for noteAutotext in response['noteAutotexts']:
        print ('  {}: {}'.format (noteAutotext['autotextId'], ', '.join (noteAutotext['elementTypes'])))
