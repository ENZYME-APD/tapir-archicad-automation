import aclib

# GetSurfaceQuantitiesOfElements returns the exposed area of every surface of an element:
# the area where a covering element overlaps the surface is subtracted. This example creates
# a wall with a slab on top of it, far from the rest of the project, and reads the surfaces
# of the wall twice: once with nothing covering it and once with the slab as the covering
# element. The difference is the wall area hidden by the slab. Finish and cladding take-offs,
# thermal envelope checks and formwork estimates need exactly this.

wall = aclib.RunTapirCommand ('CreateWalls', {
    'wallsData': [
        {
            'begCoordinate': { 'x': 100.0, 'y': 0.0 },
            'endCoordinate': { 'x': 104.0, 'y': 0.0 },
            'height': 3.0,
            'thickness': 0.3
        }
    ]
}, debug = False)['elements'][0]

# The slab sits on the top of the wall and overhangs it on both sides.
slab = aclib.RunTapirCommand ('CreateSlabs', {
    'slabsData': [
        {
            'level': 3.0,
            'thickness': 0.2,
            'polygonCoordinates': [
                { 'x': 99.0, 'y': -1.0 },
                { 'x': 105.0, 'y': -1.0 },
                { 'x': 105.0, 'y': 1.0 },
                { 'x': 99.0, 'y': 1.0 }
            ]
        }
    ]
}, debug = False)['elements'][0]

def GetSurfacesOfWall (coverElements):
    response = aclib.RunTapirCommand (
        'GetSurfaceQuantitiesOfElements', {
            'elements': [wall],
            'coverElements': coverElements
        }, debug = False)
    surfaceQuantitiesOfElement = response['surfaceQuantitiesOfElements'][0]
    if 'error' in surfaceQuantitiesOfElement:
        print ('Error:\n{}'.format (aclib.JsonDumpDictionary (surfaceQuantitiesOfElement['error'])))
        raise SystemExit (1)
    return surfaceQuantitiesOfElement['surfaces']

# Omitting coverElements would make every construction element of the project a covering
# element; here the covering elements are chosen explicitly so the comparison is exact.
wholeSurfaces = GetSurfacesOfWall ([])
exposedSurfaces = GetSurfacesOfWall ([slab])

wholeArea = sum (surface['exposedArea'] for surface in wholeSurfaces)
exposedArea = sum (surface['exposedArea'] for surface in exposedSurfaces)

print ('The wall has {} surface(s)'.format (len (wholeSurfaces)))
print ('Whole surface area: {:.2f} m2'.format (wholeArea))
print ('Surface area exposed next to the slab: {:.2f} m2'.format (exposedArea))
print ('Wall area covered by the slab: {:.2f} m2'.format (wholeArea - exposedArea))

# The surface attribute (the material of the Surface painter) names each surface. A surface
# defined by a GDL script has no surfaceId, as it is not a project attribute.
surfaceGuids = sorted ({surface['surfaceId']['attributeId']['guid'] for surface in wholeSurfaces if 'surfaceId' in surface})
surfaceNames = {}
if surfaceGuids:
    attributes = aclib.RunCommand ('API.GetSurfaceAttributes', {
        'attributeIds': [{'attributeId': {'guid': guid}} for guid in surfaceGuids]
    })['attributes']
    for guid, attribute in zip (surfaceGuids, attributes):
        if 'surfaceAttribute' in attribute:
            surfaceNames[guid] = attribute['surfaceAttribute']['name']

for whole, exposed in zip (wholeSurfaces, exposedSurfaces):
    guid = whole['surfaceId']['attributeId']['guid'] if 'surfaceId' in whole else None
    print ('  {0}: {1:.2f} m2, {2:.2f} m2 exposed'.format (
        surfaceNames.get (guid, guid or 'surface without attribute'), whole['exposedArea'], exposed['exposedArea']))

# The created elements are not needed any more.
aclib.RunTapirCommand ('DeleteElements', {'elements': [wall, slab]}, debug = False)
