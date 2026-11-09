"""Creates a railing, makes it 1.2 m high and raises it by 0.15 m, then changes its second segment and the node between the two segments with ModifyRailings.

Only the given fields change; the reference line, posts, rails and panels, the first segment and the
railing's GUID stay as they are. A segment or node is given by its index in the segments and nodes
lists GetDetailsOfElements returns. A lateral offset only takes effect on a segment left or right of the
reference line. GetDetailsOfElements reads the new values back.
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

aclib.PrintJson (aclib.RunTapirCommand ('ModifyRailings', {'railingsWithDetails': [{
    'elementId': railing['elementId'],
    'segments': [{'index': 1, 'height': 0.9, 'referenceLineLocation': 'Left', 'lateralOffset': 0.05}],
    'nodes': [{'index': 1, 'postOffset': 0.1, 'unifyPatterns': False}]
}]}))

details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [railing], 'fields': ['floorIndex', 'details']})['detailsOfElements'][0]
aclib.PrintJson ({
    'floorIndex': details['floorIndex'],
    'height': details['details']['height'],
    'bottomOffset': details['details']['bottomOffset'],
    'segmentHeights': details['details']['segmentHeights'],
    'segments': [{k: v for k, v in segment.items () if k != 'elementId'} for segment in details['details']['segments']],
    'nodes': [{k: v for k, v in node.items () if k != 'elementId'} for node in details['details']['nodes']]
})
