import aclib

# CreateHatches accepts the same 'holes' array as SetDetailsOfElements and GetDetailsOfElements use for
# hatches: each hole has a polygonOutline and optional polygonArcs, whose indices count from the first
# point of that hole. Here a 4 x 3 hatch is created with a square hole and a hole with a curved edge.

created = aclib.RunTapirCommand ('CreateHatches', {
    'hatchesData': [
        {
            'coordinates': [
                { 'x': 100.0, 'y': 10.0 },
                { 'x': 104.0, 'y': 10.0 },
                { 'x': 104.0, 'y': 13.0 },
                { 'x': 100.0, 'y': 13.0 }
            ],
            'holes': [
                {
                    'polygonOutline': [
                        { 'x': 100.5, 'y': 10.5 },
                        { 'x': 101.5, 'y': 10.5 },
                        { 'x': 101.5, 'y': 11.5 },
                        { 'x': 100.5, 'y': 11.5 }
                    ]
                },
                {
                    'polygonOutline': [
                        { 'x': 102.5, 'y': 11.0 },
                        { 'x': 103.5, 'y': 11.0 },
                        { 'x': 103.5, 'y': 12.0 },
                        { 'x': 102.5, 'y': 12.0 }
                    ],
                    'polygonArcs': [
                        { 'begIndex': 0, 'endIndex': 1, 'arcAngle': 1.0 }
                    ]
                }
            ],
            'contourPenIndex': 1
        }
    ]
}, debug = False)
elements = created['elements']

details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': elements}, debug = False)
d = details['detailsOfElements'][0]['details']
print ('Outline points: {}'.format (len (d['coordinates'])))
for index, hole in enumerate (d['holes']):
    print ('Hole {}: {} points, {} arcs'.format (index, len (hole['polygonOutline']), len (hole.get ('polygonArcs', []))))

aclib.RunTapirCommand ('DeleteElements', {'elements': elements}, debug = False)
