"""Gets the unique ID, reference GUID and Teamwork lock status of the first five door library parts with GetLibraryPartIdentities."""
import aclib

doors = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Door'}, debug = False)['libraryParts']
if not doors:
    raise SystemExit ('No door library part is loaded.')

aclib.RunTapirCommand ('GetLibraryPartIdentities', {
    'libraryParts': [{'index': door['index']} for door in doors[:5]]
})
