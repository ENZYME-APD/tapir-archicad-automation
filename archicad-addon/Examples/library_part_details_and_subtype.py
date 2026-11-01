import aclib

objects = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Object'})['libraryParts'][:10]
libraryParts = [{'index': libraryPart['index']} for libraryPart in objects]

details = aclib.RunTapirCommand ('GetLibraryPartDetails', {'libraryParts': libraryParts})

# Is each object a subtype of the direct parent of the first one?
first = details['libraryParts'][0] if details['libraryParts'] else None
if first is not None and 'parentUnId' in first:
    aclib.RunTapirCommand ('IsLibraryPartSubtypeOf', {
        'libraryParts': libraryParts,
        'ancestor': {'unId': first['parentUnId']},
        'matchAncestorByMainId': True
    })
