import aclib

# Property objects are the GDL library parts of the old listing system. Projects migrated from
# older Archicad versions still link them to construction elements, by criteria or individually.
# GetPropertyObjectsOfElements lists them per element, GetDescriptorsOfElements lists the
# descriptors they contribute, GetLinkedPropertyObjectsOfElements tells how they are linked, to
# elements or to the element defaults of a type. libraryPartIndex is the same as the index
# returned by GetAvailableLibraryParts.

walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements']

propertyObjectsOfWalls = aclib.RunTapirCommand ('GetPropertyObjectsOfElements', {
    'elements': walls
}, debug = False)['propertyObjectsOfElements']

descriptorsOfWalls = aclib.RunTapirCommand ('GetDescriptorsOfElements', {
    'elements': walls
}, debug = False)['descriptorsOfElements']

# The last item reads the links of the wall defaults.
linkedPropertyObjectsOfWalls = aclib.RunTapirCommand ('GetLinkedPropertyObjectsOfElements', {
    'elements': walls + [{'elementType': 'Wall'}]
}, debug = False)['linkedPropertyObjectsOfElements']

def PrintLinkedPropertyObjects (linkedPropertyObjects):
    if 'error' in linkedPropertyObjects:
        print ('  linked property objects: error\n{}'.format (aclib.JsonDumpDictionary (linkedPropertyObjects['error'])))
        return
    individualLibraryPartIndex = linkedPropertyObjects['individualLibraryPartIndex']
    if individualLibraryPartIndex == 0:
        print ('  no individually assigned property object')
    else:
        print ('  individually assigned: {} ({})'.format (
            linkedPropertyObjects.get ('individualLibraryPartName', '?'), individualLibraryPartIndex))
    print ('  link by criteria: {}, {} matching property objects'.format (
        linkedPropertyObjects['linkByCriteria'], len (linkedPropertyObjects['criteriaPropertyObjects'])))
    for propertyObject in linkedPropertyObjects['criteriaPropertyObjects']:
        print ('    {} ({})'.format (propertyObject.get ('name', '?'), propertyObject['libraryPartIndex']))

for wall, propertyObjectsOfWall, descriptorsOfWall, linkedPropertyObjectsOfWall in zip (walls, propertyObjectsOfWalls, descriptorsOfWalls, linkedPropertyObjectsOfWalls):
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

    PrintLinkedPropertyObjects (linkedPropertyObjectsOfWall)

print ('Wall defaults')
PrintLinkedPropertyObjects (linkedPropertyObjectsOfWalls[-1])
