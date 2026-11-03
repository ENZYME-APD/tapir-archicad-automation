import aclib

response = aclib.RunTapirCommand ('GetAvailableLibraryParts', {
    'filterByTypeId': 'Door'
}, debug = False)
libraryParts = response['libraryParts'][:5]

# The index is the one GetAvailableLibraryParts returns (Archicad 27 or newer).
response = aclib.RunTapirCommand ('GetLibraryPartOriginalLocations', {
    'libraryParts': [{'index': libraryPart['index']} for libraryPart in libraryParts]
}, debug = False)

for libraryPart, location in zip (libraryParts, response.get ('locations', [])):
    print (libraryPart['fileName'], '->', location.get ('path', location.get ('error')))
