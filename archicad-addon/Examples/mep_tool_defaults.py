import sys
import aclib

# Requires Archicad 28 or newer; the floor plan display settings and the elbow default require
# Archicad 29, the connection style of the ports Archicad 30.

# GetMEPToolDefaults reads the current settings of the MEP tools without changing them.
defaults = aclib.RunTapirCommand ('GetMEPToolDefaults', {
        'domains': ['Piping', 'Ventilation']
    })
if defaults is None or 'error' in defaults:
    print ('Could not read the MEP tool defaults, this example needs Archicad 28 or newer.')
    sys.exit ()

for domainDefaults in defaults['toolDefaults']:
    print (f"{domainDefaults['domain']}:")
    routingElement = domainDefaults.get ('routingElement')
    if routingElement is not None:
        segment = routingElement['routingSegment']
        print (f"  Route: {segment.get ('crossSectionShape')} {segment.get ('crossSectionWidth')} x {segment.get ('crossSectionHeight')}, "
               f"floor plan display {routingElement.get ('floorPlanDisplayOption', 'not reported (Archicad 28)')}")
    for tool in ['terminal', 'accessory', 'fitting']:
        if tool in domainDefaults:
            print (f"  {tool.capitalize ()}: {len (domainDefaults[tool]['ports'])} port(s)")
if 'equipment' in defaults:
    print (f"Equipment: {len (defaults['equipment']['ports'])} port(s)")

# TransferMEPSettings picks up the settings of one pipe route and injects them into another, like
# Archicad's pick up and inject parameters.
created = aclib.RunTapirCommand ('CreateMEPRoutingElements', {
        'routingElementsData': [
            {
                'domain': 'Piping',
                'nodeCoordinates': [
                    {'x': 0.0, 'y': -20.0, 'z': 1.0},
                    {'x': 4.0, 'y': -20.0, 'z': 1.0}
                ],
                'crossSectionShape': 'Circular',
                'crossSectionWidth': 0.1,
                'crossSectionHeight': 0.1
            },
            {
                'domain': 'Piping',
                'nodeCoordinates': [
                    {'x': 0.0, 'y': -22.0, 'z': 1.0},
                    {'x': 4.0, 'y': -22.0, 'z': 1.0}
                ]
            }
        ]
    })
if created is None or 'error' in created or any ('error' in e for e in created['elements']):
    print ('Could not create the routing elements.')
    sys.exit ()
sourceId, targetId = [e['elementId'] for e in created['elements']]

def PrintSegmentWidths (title):
    routingElements = aclib.RunTapirCommand ('GetMEPRoutingElements', {
            'elements': [{'elementId': sourceId}, {'elementId': targetId}]
        })['routingElements']
    print (title, [r['segments'][0]['crossSectionWidth'] for r in routingElements])

PrintSegmentWidths ('Segment widths before:')
result = aclib.RunTapirCommand ('TransferMEPSettings', {
        'sourceElementId': sourceId,
        'targetElements': [{'elementId': targetId}]
    })
if 'error' in result:
    print (f"Transfer failed: {result['error']['message']}")
else:
    executionResult = result['executionResults'][0]
    print (f"Transfer: {'succeeded' if executionResult['success'] else executionResult['error']['message']}")
PrintSegmentWidths ('Segment widths after:')

# Clean up
aclib.RunTapirCommand ('DeleteElements', {
        'elements': [{'elementId': sourceId}, {'elementId': targetId}]
    })
