import aclib

availableLibraryParts = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Object'})

aclib.RunTapirCommand ('GetLibraryPartMetadata', {
    'libraryParts': [{'name': libraryPart['fileName']} for libraryPart in availableLibraryParts['libraryParts'][:5]]
})
