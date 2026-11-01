import aclib

parts = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Object'}) ['libraryParts'][:3]

response = aclib.RunTapirCommand ('GetLibrariesOfLibParts', {
    'libraryParts': [{'guid': part['guid']} for part in parts]
})
