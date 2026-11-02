import aclib

# Reads the geometry of the section markers (CutPlane elements) of the floor plan:
# the section line, the depth line, the depth and the direction the section looks in.
sections = aclib.RunTapirCommand ('GetElementsByType', {
    'elementType': 'CutPlane'
})['elements']

if not sections:
    print ('No section found on the floor plan.')
else:
    detailsOfElements = aclib.RunTapirCommand ('GetDetailsOfElements', {
        'elements': sections
    })['detailsOfElements']

    for section, detailsOfElement in zip (sections, detailsOfElements):
        if 'error' in detailsOfElement:
            print (section['elementId']['guid'], detailsOfElement['error'])
            continue
        details = detailsOfElement['details']
        print (details['idStr'], details['name'])
        print ('  section line:', details['mainCoordinates'])
        print ('  depth line:', details['depthCoordinates'])
        if 'depth' in details:
            print ('  depth:', details['depth'], 'looking towards', details['viewDirection'])
