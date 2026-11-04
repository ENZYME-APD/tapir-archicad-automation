"""CreateLibraryPartFromElements makes a new object whose 2D symbol is a copy of 2D drawing elements of the plan.
This draws a square and a circle around the project origin, turns them into an object with a simple 3D script,
places it with CreateObjects, then deletes the drawn elements and the placed object again.
"""
import aclib

libraryPartName = 'Tapir Example Symbol'

# The parent subtype by its GUID, found among the loaded library parts: the first of these
# subtype templates of the Archicad Library that is loaded.
subtypeNames = ['Model Element', 'General Object', 'General GDL Object']
availableLibraryParts = aclib.RunTapirCommand ('GetAvailableLibraryParts', {
    'filterByTypeId': 'Object'
})['libraryParts']
subtypeGuids = { part['documentName']: part['guid'] for part in availableLibraryParts }
parentGuid = next ((subtypeGuids[name] for name in subtypeNames if name in subtypeGuids), None)
print ('Parent subtype found:', parentGuid is not None)
parent = { 'parentId': parentGuid } if parentGuid is not None else { 'parentName': subtypeNames[0] }

drawnElements = []
drawnElements += aclib.RunTapirCommand ('CreatePolylines', {
    'polylinesData': [{
        'coordinates': [
            { 'x': 0.0, 'y': 0.0 },
            { 'x': 1.0, 'y': 0.0 },
            { 'x': 1.0, 'y': 1.0 },
            { 'x': 0.0, 'y': 1.0 },
            { 'x': 0.0, 'y': 0.0 }
        ]
    }]
})['elements']
drawnElements += aclib.RunTapirCommand ('CreateCircles', {
    'circlesData': [
        { 'origin': { 'x': 0.5, 'y': 0.5 }, 'radius': 0.3 }
    ]
})['elements']

response = aclib.RunTapirCommand ('CreateLibraryPartFromElements', {
    'name': libraryPartName,
    'elements': drawnElements,
    **parent,
    'folder': 'TapirExamples',
    'overwrite': True,
    'scripts': {
        '3D': 'BLOCK A, B, 0.5'
    },
    'comment': 'Created by the Tapir example CreateLibraryPartFromElements.py',
    'keywords': 'Tapir example'
})
print ('Library part created:', 'error' not in response)
if 'error' in response:
    print (response['error']['message'])
else:
    print ('A and B:', round (response['a'], 3), round (response['b'], 3))

    elementId = aclib.RunTapirCommand ('CreateObjects', {
        'objectsData': [{
            'libraryPartName': response['name'],
            'coordinates': { 'x': 5.0, 'y': 0.0, 'z': 0.0 }
        }]
    })['elements'][0]['elementId']
    aclib.RunTapirCommand ('DeleteElements', { 'elements': [{ 'elementId': elementId }] })

aclib.RunTapirCommand ('DeleteElements', { 'elements': drawnElements })
