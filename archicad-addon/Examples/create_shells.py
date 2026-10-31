import aclib
import math

circle = lambda cx, cy, r: [{'x': cx + r * math.cos (a * math.pi / 8), 'y': cy + r * math.sin (a * math.pi / 8)} for a in range (16)]

# An extruded barrel vault (profile x across, y up) with a square roof light cut through it,
# a revolved dome with an oculus, and a ruled shell between a straight and an arched edge.
aclib.RunTapirCommand (
    'CreateShells', {
        'shellsData': [
            {
                'shellClass': 'Extruded',
                'thickness': 0.2,
                'profile': {
                    'coordinates': [{'x': -3.0, 'y': 0.0}, {'x': 3.0, 'y': 0.0}],
                    'arcs': [{'begIndex': 0, 'endIndex': 1, 'arcAngle': -math.pi / 2}]
                },
                'extrusionVector': {'x': 0.0, 'y': 8.0, 'z': 0.0},
                'holes': [
                    {
                        'coordinates': [{'x': -1.0, 'y': 3.0}, {'x': 1.0, 'y': 3.0}, {'x': 1.0, 'y': 5.0}, {'x': -1.0, 'y': 5.0}],
                        'depth': 10.0
                    }
                ]
            },
            {
                'shellClass': 'Revolved',
                'thickness': 0.15,
                'basePlane': {'origin': {'x': 15.0, 'y': 4.0, 'z': 0.0}},
                'profile': {
                    'coordinates': [{'x': 4.0, 'y': 0.0}, {'x': 0.0, 'y': 4.0}],
                    'arcs': [{'begIndex': 0, 'endIndex': 1, 'arcAngle': math.pi / 2}]
                },
                # The dome's base plane stands upright, so the oculus is drawn in a horizontal plane inside it.
                'holes': [
                    {
                        'coordinates': circle (0.0, 0.0, 1.0),
                        'plane': {
                            'xAxis': {'x': 1.0, 'y': 0.0, 'z': 0.0},
                            'yAxis': {'x': 0.0, 'y': 0.0, 'z': -1.0},
                            'zAxis': {'x': 0.0, 'y': 1.0, 'z': 0.0}
                        },
                        'depth': 6.0
                    }
                ]
            },
            {
                'shellClass': 'Ruled',
                'thickness': 0.1,
                'basePlane': {'origin': {'x': 25.0, 'y': 0.0, 'z': 0.0}},
                'profile': {
                    'coordinates': [{'x': 0.0, 'y': 0.0}, {'x': 6.0, 'y': 0.0}]
                },
                'profilePlane': {
                    'xAxis': {'x': 1.0, 'y': 0.0, 'z': 0.0},
                    'yAxis': {'x': 0.0, 'y': 0.0, 'z': 1.0},
                    'zAxis': {'x': 0.0, 'y': -1.0, 'z': 0.0}
                },
                'secondProfile': {
                    'coordinates': [{'x': 0.0, 'y': 3.0}, {'x': 6.0, 'y': 3.0}],
                    'arcs': [{'begIndex': 0, 'endIndex': 1, 'arcAngle': -math.pi / 3}]
                },
                'secondProfilePlane': {
                    'origin': {'x': 0.0, 'y': 6.0, 'z': 0.0},
                    'xAxis': {'x': 1.0, 'y': 0.0, 'z': 0.0},
                    'yAxis': {'x': 0.0, 'y': 0.0, 'z': 1.0},
                    'zAxis': {'x': 0.0, 'y': -1.0, 'z': 0.0}
                }
            }
        ]
    })
