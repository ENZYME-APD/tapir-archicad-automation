"""Creates a railing, makes it 1.2 m high and raises it by 0.15 m, then changes its second segment and the node between the two segments, hides the top rail and moves the first rail of the first segment, and changes the first baluster set and panel of the first segment and the post of the first node, with ModifyRailings.

Only the given fields change; the reference line, the other components, the first segment and the
railing's GUID stay as they are. A segment or node is given by its index in the segments and nodes
lists GetDetailsOfElements returns; a rail, handrail, inner post, panel or baluster set by its index
among the components of that kind GetSubelementsOfHierarchicalElements lists for that segment, and a
post by its index among the posts of that node (their ownerElementId; inner posts and baluster sets
may belong to the segment's railingPattern). A lateral offset only takes effect on a segment left or
right of the reference line. GetDetailsOfElements reads the new values back.
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

aclib.PrintJson (aclib.RunTapirCommand ('ModifyRailings', {'railingsWithDetails': [{
    'elementId': railing['elementId'],
    'segments': [{'index': 0, 'toprail': {'visible': False}, 'rails': [{'index': 0, 'height': 0.3, 'horizontalOffset': 0.02}]}]
}]}))

aclib.PrintJson (aclib.RunTapirCommand ('ModifyRailings', {'railingsWithDetails': [{
    'elementId': railing['elementId'],
    'segments': [{'index': 0,
                  'balusterSets': [{'index': 0, 'distribution': {'type': 'PatternLength', 'length': 0.12}}],
                  'panels': [{'index': 0, 'visible': True, 'frame': {'topOffset': 0.1}}]}],
    'nodes': [{'index': 0, 'posts': [{'index': 0, 'verticalPosition': {'bottomRailIndex': 0}}]}]
}]}))

components = aclib.RunTapirCommand ('GetSubelementsOfHierarchicalElements', {'elements': [railing]})['subelements'][0]
for name in ('railingToprails', 'railingRails', 'railingHandrails', 'railingPosts', 'railingPanels', 'railingBalusterSets'):
    componentDetails = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': components.get (name, [])})['detailsOfElements']
    aclib.PrintJson ({name: [{k: v for k, v in c['details'].items () if k not in ('elementId', 'ownerElementId')} for c in componentDetails]})

details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [railing], 'fields': ['floorIndex', 'details']})['detailsOfElements'][0]
aclib.PrintJson ({
    'floorIndex': details['floorIndex'],
    'height': details['details']['height'],
    'bottomOffset': details['details']['bottomOffset'],
    'segmentHeights': details['details']['segmentHeights'],
    'segments': [{k: v for k, v in segment.items () if k != 'elementId'} for segment in details['details']['segments']],
    'nodes': [{k: v for k, v in node.items () if k != 'elementId'} for node in details['details']['nodes']]
})
