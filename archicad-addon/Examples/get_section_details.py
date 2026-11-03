import aclib

# Reads the geometry of the section markers (CutPlane elements) and elevation markers of the
# floor plan: the section or elevation line, the depth line, the depth and the direction the
# view looks in. Archicad stores elevations the same way as sections.
for elementType in ('CutPlane', 'Elevation'):
    markers = aclib.RunTapirCommand ('GetElementsByType', {
        'elementType': elementType
    })['elements']

    if not markers:
        print ('No {} found on the floor plan.'.format (elementType))
        continue

    detailsOfElements = aclib.RunTapirCommand ('GetDetailsOfElements', {
        'elements': markers
    })['detailsOfElements']

    for marker, detailsOfElement in zip (markers, detailsOfElements):
        if 'error' in detailsOfElement:
            print (marker['elementId']['guid'], detailsOfElement['error'])
            continue
        details = detailsOfElement['details']
        print (elementType, details['idStr'], details['name'])
        print ('  line:', details['mainCoordinates'])
        print ('  depth line:', details['depthCoordinates'])
        if 'depth' in details:
            print ('  depth:', details['depth'], 'looking towards', details['viewDirection'])
