"""GetShapePrimitivesOfElements draws the elements as ACAPI_DrawingPrimitive_ShapePrims draws them by default, in
the format of GetDrawingPrimitivesOfElements. includeMarkers draws the objects attached to the elements
(window and door markers, ...) with them or leaves them out; Archicad's own setting is changed back
afterwards. The floor plan must be the current database.
"""
import aclib
from collections import Counter

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements']
if not walls:
    print ('No walls in the project.')
    raise SystemExit (0)

for includeMarkers in [True, False]:
    result = aclib.RunTapirCommand ('GetShapePrimitivesOfElements', {
        'elements': walls[:3],
        'includeMarkers': includeMarkers
    })
    if result is None or 'shapePrimitivesOfElements' not in result:
        print ('Failed: {}'.format (result))
        raise SystemExit (1)

    print ('=' * 30)
    print ('includeMarkers: {}'.format (includeMarkers))
    for wall, shapePrimitives in zip (walls, result['shapePrimitivesOfElements']):
        if 'error' in shapePrimitives:
            print ('Wall {}: error {}'.format (wall['elementId']['guid'], shapePrimitives['error']['message']))
            continue
        primitives = shapePrimitives['primitives']
        print ('Wall {}: {} primitives {}'.format (
            wall['elementId']['guid'], len (primitives), dict (Counter (p['type'] for p in primitives))))
