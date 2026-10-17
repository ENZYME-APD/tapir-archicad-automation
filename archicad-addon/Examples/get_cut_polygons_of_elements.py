import aclib

# GetCutPolygonsOfElements cuts the solid bodies of the 3D model of each element with a
# horizontal plane and returns the cut polygons (outline and holes, in project X/Y) and
# their total area - the shape the floor plan shows of the element. Without a cutHeight
# the plane is the floor plan cut plane of the element's home story.
walls = aclib.RunTapirCommand (
    'GetElementsByType', {
        'elementType': 'Wall'
    }, debug = False)['elements']

# The first few walls are enough to show the shape of the response.
walls = walls[:3]

def PrintCuts (cuts):
    for cut in cuts:
        if 'error' in cut:
            print ('Error:\n{}'.format (aclib.JsonDumpDictionary (cut['error'])))
            continue

        print ('Cut at {:.2f} m: {} polygons, {:.3f} m2'.format (cut['cutHeight'], len (cut['polygons']), cut['area']))
        for polygon in cut['polygons']:
            outline = polygon['polygonOutline']
            holes = polygon.get ('holes', [])
            # The extents are printed rounded, because the exact coordinates depend on the
            # wall junctions and openings Archicad resolves in the 3D model.
            xs = [coord['x'] for coord in outline]
            ys = [coord['y'] for coord in outline]
            print ('  outline of {} points ({} holes): x {:.2f} .. {:.2f}, y {:.2f} .. {:.2f}'.format (
                len (outline), len (holes), min (xs), max (xs), min (ys), max (ys)))

print ('At the floor plan cut plane of each wall\'s story:')
PrintCuts (aclib.RunTapirCommand (
    'GetCutPolygonsOfElements', {
        'elements': walls
    }, debug = False)['cutPolygonsOfElements'])

print ('At 1.0 m above the project zero:')
PrintCuts (aclib.RunTapirCommand (
    'GetCutPolygonsOfElements', {
        'elements': walls,
        'cutHeight': 1.0
    }, debug = False)['cutPolygonsOfElements'])
