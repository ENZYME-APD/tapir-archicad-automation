import sys
import aclib

# Requires Archicad 28 or newer.

# A pipe route with one turn, then ModifyMEPRoutingElements changes its branch preference table and,
# on every segment, the anchor, the reference line offsets and the wall thickness. Each value is
# applied to all segments of the route.
created = aclib.RunTapirCommand ('CreateMEPRoutingElements', {
        'routingElementsData': [
            {
                'domain': 'Piping',
                'nodeCoordinates': [
                    {'x': 0.0, 'y': -8.0, 'z': 3.0},
                    {'x': 4.0, 'y': -8.0, 'z': 3.0},
                    {'x': 4.0, 'y': -5.0, 'z': 3.0}
                ]
            }
        ]
    })
if created is None or 'error' in created or 'error' in created['elements'][0]:
    print ('Could not create the routing element, this example needs Archicad 28 or newer.')
    sys.exit ()
routeId = created['elements'][0]['elementId']

modification = {
    'elementId': routeId,
    'anchor': 'BottomCenter',
    'referenceLineOffsetX': 0.05,
    'referenceLineOffsetY': 0.0,
    'wallThickness': 0.004
}
branchTables = aclib.RunTapirCommand ('GetMEPPreferenceTables', {
        'domain': 'Piping',
        'includeFittingTables': True
    }).get ('branchTables', [])
if branchTables:
    modification['branchPreferenceTableId'] = branchTables[-1]['guid']
    print (f"Using branch preference table '{branchTables[-1]['name']}'")

result = aclib.RunTapirCommand ('ModifyMEPRoutingElements', {
        'routingElementsData': [modification]
    })['executionResults'][0]
print (f"Modify: {'succeeded' if result['success'] else result['error']['message']}")

# GetMEPRoutingElements reports these values with the routing segment reads of #1287 part 1.
routingElement = aclib.RunTapirCommand ('GetMEPRoutingElements', {
        'elements': [{'elementId': routeId}]
    })['routingElements'][0]
print (f"Branch preference table: {routingElement.get ('branchPreferenceTableId', 'not reported')}")
for i, segment in enumerate (routingElement['segments']):
    print (f"Segment {i}: anchor {segment.get ('anchor', 'not reported')}, "
           f"reference line offset ({segment.get ('referenceLineOffsetX', '-')}, {segment.get ('referenceLineOffsetY', '-')}), "
           f"wall thickness {segment.get ('wallThickness', 'not reported')}")

# Clean up
aclib.RunTapirCommand ('DeleteElements', {
        'elements': [{'elementId': routeId}]
    })
