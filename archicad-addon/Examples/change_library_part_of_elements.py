import aclib

# ChangeLibraryPartOfElements switches placed library part based elements (Object, Lamp,
# Window, Door, Skylight, Zone and symbol Label) to another library part of the same type,
# keeping the elements themselves, e.g. to replace missing library parts with the matching
# parts of the loaded libraries. The new part is given by 'name', 'index' or 'guid', as
# GetAvailableLibraryParts returns them. keepParameters (default true) copies the values of
# the parameters with the same name and type, keepSize (default true) keeps A and B.

objects = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Object'})['elements']

if len (objects) == 0:
    print ('No objects in the project.')
else:
    detailsOfElements = aclib.RunTapirCommand (
        'GetDetailsOfElements', {'elements': objects})['detailsOfElements']

    # Sets every object to the library part it already uses, which leaves the project unchanged.
    # A real script would give the name of the part to switch to here, e.g. 'Chair 01'.
    elementsWithLibraryPart = []
    for obj, details in zip (objects, detailsOfElements):
        libPart = details.get ('details', {}).get ('libPart')
        if libPart is None or not libPart['name']:
            continue
        elementsWithLibraryPart.append ({
            'elementId': obj['elementId'],
            'libraryPart': {'name': libPart['name']},
            'keepParameters': True,
            'keepSize': True
        })

    if len (elementsWithLibraryPart) > 0:
        aclib.RunTapirCommand (
            'ChangeLibraryPartOfElements', {'elementsWithLibraryPart': elementsWithLibraryPart})
