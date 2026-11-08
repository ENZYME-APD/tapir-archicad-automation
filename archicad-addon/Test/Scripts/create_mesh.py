import aclib

def alternate_hole_outline_key():
    while True:
        yield "polygonCoordinates" #legacy key
        yield "polygonOutline"

alternating_hole_outline_key = alternate_hole_outline_key()
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
                        f'{next(alternating_hole_outline_key)}': [
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

# Demonstrate the line-display fields: ridges/showLines control which edges Archicad
# draws on a mesh, contourPen/levelPen/lineTypeIndex control how those lines look.
# Pen 2 is conventionally red; line type 2 is dashed in the standard set.
aclib.RunTapirCommand(
    'CreateMeshes', {
        'meshesData': [
            {
                'floorIndex': 2,
                'level': 0.0,
                'polygonCoordinates': [
                    {'x': -20.0, 'y': -20.0, 'z': 0.0},
                    {'x':  -5.0, 'y': -20.0, 'z': 0.0},
                    {'x':  -5.0, 'y':  -5.0, 'z': 2.0},
                    {'x': -20.0, 'y':  -5.0, 'z': 2.0},
                ],
                'sublines': [
                    {'coordinates': [
                        {'x': -20.0, 'y': y, 'z': 1.0} for y in range(-19, -5, 2)
                    ]}
                ],
                'ridges':        'UserDefined',
                'showLines':     False,
                'contourPen':    2,
                'levelPen':      4,
                'lineTypeIndex': 2,
            }
        ]
    })