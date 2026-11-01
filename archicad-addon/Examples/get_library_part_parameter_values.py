import aclib

# Reads the parameters of a loaded library part with their default values and descriptions
# (Archicad 28 or newer). The name is the fileName (with extension) returned by
# GetAvailableLibraryParts; here the first Object of the loaded libraries is read.

libraryParts = aclib.RunTapirCommand ('GetAvailableLibraryParts', {
    'filterByTypeId': 'Object'
}, debug=False).get ('libraryParts', [])

if len (libraryParts) == 0:
    print ('No Object library part is loaded.')
else:
    aclib.RunTapirCommand ('GetLibraryPartParameterValues', {
        'libraryParts': [{'name': libraryParts[0]['fileName']}]
    })
