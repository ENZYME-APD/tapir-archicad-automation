import aclib

# Property objects are the GDL library parts of the old listing system. Projects migrated from
# older Archicad versions still link them to construction elements, by criteria or individually.
# GetPropertyObjectsOfElements lists them per element, GetDescriptorsOfElements lists the
# descriptors they contribute. libraryPartIndex is the same as the index returned by
# GetAvailableLibraryParts.

walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements']

propertyObjectsOfWalls = aclib.RunTapirCommand ('GetPropertyObjectsOfElements', {
    'elements': walls
}, debug = False)['propertyObjectsOfElements']

descriptorsOfWalls = aclib.RunTapirCommand ('GetDescriptorsOfElements', {
    'elements': walls
}, debug = False)['descriptorsOfElements']

for wall, propertyObjectsOfWall, descriptorsOfWall in zip (walls, propertyObjectsOfWalls, descriptorsOfWalls):
    print ('Wall {}'.format (wall['elementId']['guid']))

    if 'error' in propertyObjectsOfWall:
        print ('  property objects: error\n{}'.format (aclib.JsonDumpDictionary (propertyObjectsOfWall['error'])))
    else:
        print ('  {} property objects'.format (len (propertyObjectsOfWall['propertyObjects'])))
        for propertyObject in propertyObjectsOfWall['propertyObjects']:
            print ('    {} ({}), {}'.format (
                propertyObject.get ('name', '?'), propertyObject['libraryPartIndex'],
                'individually assigned' if propertyObject['isIndividuallyAssigned'] else 'by criteria'))

    if 'error' in descriptorsOfWall:
        print ('  descriptors: error\n{}'.format (aclib.JsonDumpDictionary (descriptorsOfWall['error'])))
    else:
        print ('  {} descriptors'.format (len (descriptorsOfWall['descriptors'])))
        for descriptor in descriptorsOfWall['descriptors']:
            print ('    {} in {} ({}), set {}, index {}'.format (
                descriptor['status'], descriptor.get ('name', '?'), descriptor['libraryPartIndex'],
                descriptor['setIndex'], descriptor['index']))
