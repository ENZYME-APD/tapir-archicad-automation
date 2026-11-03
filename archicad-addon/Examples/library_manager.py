import os
import tempfile

import aclib

# Finds a few GDL objects with the Library Manager and reads their details, then adds an
# empty temporary folder as a library and removes it again by its path, leaving the
# library list as it was. The library list is this machine's, so the example prints
# only counts and results, not names or paths. Available from Archicad 28.

found = aclib.RunTapirCommand ('FindLibraryParts', {'kind': 'Object'}, debug = False)
libraryParts = found['libraryParts']
print ('Found GDL objects: {}'.format (len (libraryParts) > 0))

details = aclib.RunTapirCommand ('GetLibraryPartDetails', {
    'libraryParts': [{'name': part['name']} for part in libraryParts[:3] if 'name' in part],
    'includeScripts': True
}, debug = False)
for part in details['libraryParts']:
    if 'error' in part:
        print ('Error: {}'.format (part['error']))
    else:
        print ('{}: has GDL object data: {}'.format (part.get ('name'), 'gsmObject' in part))

with tempfile.TemporaryDirectory () as libraryFolder:
    added = aclib.RunTapirCommand ('ModifyLibraryList', {
        'add': [{'path': libraryFolder}]
    }, debug = False)
    print ('Added: {}'.format (added['addResults']))

    removed = aclib.RunTapirCommand ('ModifyLibraryList', {
        'remove': [{'path': libraryFolder}]
    }, debug = False)
    print ('Removed: {}'.format (removed['removeResults']))
