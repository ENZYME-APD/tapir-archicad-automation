import aclib

# SetMarkerLibraryPartOfElements changes which library part the marker of a window or a door
# uses (the stamp drawn next to the opening on the floor plan), where
# SetMarkerParametersOfElements only changes values within the one already assigned. The marker
# gets the new library part's default parameters.

openings = []
for elementType in ['Window', 'Door']:
    openings += aclib.RunTapirCommand ('GetElementsByType', {'elementType': elementType})['elements']

if len (openings) == 0:
    print ('No windows or doors in the project.')
else:
    markerParametersOfElements = aclib.RunTapirCommand (
        'GetMarkerParametersOfElements', {'elements': openings})['markerParametersOfElements']

    # Sets each marker to the library part it already uses, which leaves the project unchanged.
    # A real script would give the name of another marker library part here, e.g. one listed by
    # GetAvailableLibraryParts.
    elementsWithMarkerLibraryPart = []
    for opening, markerParameters in zip (openings, markerParametersOfElements):
        if 'error' in markerParameters or 'libPart' not in markerParameters:
            continue
        elementsWithMarkerLibraryPart.append ({
            'elementId': opening['elementId'],
            'libraryPartName': markerParameters['libPart']['name']
        })

    if len (elementsWithMarkerLibraryPart) > 0:
        aclib.RunTapirCommand (
            'SetMarkerLibraryPartOfElements', {'elementsWithMarkerLibraryPart': elementsWithMarkerLibraryPart})
