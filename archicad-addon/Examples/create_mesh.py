import aclib

nX = 5
nY = 6
width = 10
height = 10
gap = 1

aclib.RunTapirCommand(
    'CreateMeshes', {
        'meshesData': [
            {
                'floorIndex': 2,
                'polygonCoordinates': [
                    {'x': 0.0 + x,   'y': 0.0 + y,    'z': 0.0 },
                    {'x': width + x, 'y': 0.0 + y,    'z': 0.0 },
                    {'x': width + x, 'y': height + y, 'z': 0.0 },
                    {'x': 0.0 + x,   'y': height + y, 'z': 0.0 }
                ],
                'sublines': [
                    {
                        'coordinates': [
                            {'x': x + i, 'y': y + j, 'z': x*0.1 + y*0.1 + i*0.1 + j*0.1 } for i in range(width+1)
                        ]
                    } for j in range(height+1)
                ],
                'holes': [
                        {
                        'polygonCoordinates': [
                        {'x': width/2 + x,   'y': height/2 + y,    'z': x*0.2 + y*0.1 },
                        {'x': width/4 + x, 'y': height/2 + y,    'z': x*0.2 + y*0.1 },
                        {'x': width/4 + x, 'y': height/4 + y, 'z': x*0.2 + y*0.1 },
                        {'x': width/2 + x,   'y': height/4 + y, 'z': x*0.2 + y*0.1 }
                        ],
                    }
                ]
            } for x in range(0, nX*(width+gap), (width+gap)) for y in range(0, nY*(height+gap), (height+gap))
        ]
    })