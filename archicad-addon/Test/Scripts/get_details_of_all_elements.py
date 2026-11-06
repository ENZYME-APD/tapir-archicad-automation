import aclib

# Sweep every element of the project through GetDetailsOfElements.
#
# The command has to survive every element the project contains, including element types it
# has no branch for and types it cannot even name - those are expected to come back as
# 'Not yet supported element type' inside 'details', never to take Archicad down with them.
elements = aclib.RunTapirCommand ('GetAllElements', {}, debug = False)['elements']

# In chunks, so a project with tens of thousands of elements stays within the request size
# the connection is comfortable with.
chunkSize = 500
detailsOfElements = []
for chunkStart in range (0, len (elements), chunkSize):
    response = aclib.RunTapirCommand ('GetDetailsOfElements', {
        'elements': elements[chunkStart:chunkStart + chunkSize]
    }, debug = False)
    detailsOfElements.extend (response['detailsOfElements'])

supportedTypes = set ()
unsupportedTypes = set ()
failedElementCount = 0
withFloorPlanPolygons = 0
for detailsOfElement in detailsOfElements:
    if 'error' in detailsOfElement:
        failedElementCount += 1
        continue
    elementType = detailsOfElement['type']
    if 'error' in detailsOfElement['details']:
        unsupportedTypes.add (elementType)
    else:
        supportedTypes.add (elementType)
    if detailsOfElement.get ('floorPlanPolygons'):
        withFloorPlanPolygons += 1

print ('Asked for the details of {} elements, got {} answers back'.format (
    len (elements), len (detailsOfElements)))
print ('Element types with details: ' + ', '.join (sorted (supportedTypes)))
print ('Element types not yet supported: ' + ', '.join (sorted (unsupportedTypes)))
print ('Elements the command could not read at all: {}'.format (failedElementCount))
print ('Elements with floor plan polygons: {}'.format (withFloorPlanPolygons))
