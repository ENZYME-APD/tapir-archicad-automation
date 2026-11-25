"""Reads the polygon vertex IDs of a slab with GetPolygonVertexIdsOfElements and dimensions its first edge.

The inIndex and vertexId of each point are what a witness point of CreateAssociativeDimensions needs
(as inIndex and nodeId) to attach to a vertex of the slab; for a point on the edge that starts at the
point (line true) give its edgeId as nodeId instead. Needs at least one Slab.
"""
import aclib

slabs = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Slab'})['elements'][:1]
if not slabs:
    raise SystemExit ('No slab in the project.')

result = aclib.RunTapirCommand ('GetPolygonVertexIdsOfElements', {'elements': slabs})
aclib.PrintJson (result)

outline = result['polygonVertexIdsOfElements'][0]['contours'][0]['points']
first, second = outline[0], outline[1]
dx = second['coordinate']['x'] - first['coordinate']['x']
dy = second['coordinate']['y'] - first['coordinate']['y']

aclib.PrintJson (aclib.RunTapirCommand ('CreateAssociativeDimensions', {
    'dimensionsData': [{
        'referencePoint': {'x': first['coordinate']['x'] + dy, 'y': first['coordinate']['y'] - dx},
        'direction': {'x': dx, 'y': dy},
        'witnessPoints': [
            {'elementId': slabs[0]['elementId'], 'inIndex': point['inIndex'], 'nodeId': point['vertexId']}
            for point in (first, second)
        ]
    }, {
        'referencePoint': {'x': first['coordinate']['x'] + 2 * dy, 'y': first['coordinate']['y'] - 2 * dx},
        'direction': {'x': -dy, 'y': dx},
        'witnessPoints': [
            {'elementId': slabs[0]['elementId'], 'line': True, 'inIndex': first['inIndex'], 'nodeId': first['edgeId'],
             'coordinate': {'x': first['coordinate']['x'] + dx / 2, 'y': first['coordinate']['y'] + dy / 2}},
            {'elementId': slabs[0]['elementId'], 'inIndex': outline[2]['inIndex'], 'nodeId': outline[2]['vertexId']}
        ]
    }]
}))
