import aclib

# Requires Archicad 27 or later.

# Rebuild the current database, the one the API works on.
print (aclib.RunTapirCommand ('RebuildCurrentDatabase'))

# Open the first layout, go back to the floor plan, then close the layout window again.
layouts = aclib.RunTapirCommand ('GetDatabases', { 'types': ['Layout'] })['databases']
if not layouts:
    print ('The project has no layouts; nothing to close.')
else:
    layoutId = layouts[0]['databaseId']
    aclib.RunTapirCommand ('ChangeWindow', { 'databaseId': layoutId })
    aclib.RunTapirCommand ('ChangeWindow', { 'windowType': 'FloorPlan' })

    result = aclib.RunTapirCommand ('CloseWindows', {
        'windows': [
            { 'databaseId': layoutId, 'windowType': 'Layout' }
        ]
    })
    print (aclib.JsonDumpDictionary (result))
