"""Creates a railing, then makes it 1.2 m high and raises it by 0.15 m with ModifyRailings.

Only the given fields change; the reference line, posts, rails and panels stay as they are.
GetDetailsOfElements reads the new height, bottomOffset and segmentHeights back.
"""
import aclib

railing = aclib.RunTapirCommand ('CreateRailings', {'railingsData': [{
    'coordinates': [
        {'x': 0.0, 'y': 0.0},
        {'x': 0.0, 'y': 2.0},
        {'x': 5.0, 'y': 2.0}
    ],
    'floorIndex': 0,
    'height': 1.0
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyRailings', {'railingsWithDetails': [{
    'elementId': railing['elementId'],
    'height': 1.2,
    'bottomOffset': 0.15
}]}))

details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [railing], 'fields': ['floorIndex', 'details']})['detailsOfElements'][0]
aclib.PrintJson ({
    'floorIndex': details['floorIndex'],
    'height': details['details']['height'],
    'bottomOffset': details['details']['bottomOffset'],
    'segmentHeights': details['details']['segmentHeights']
})
