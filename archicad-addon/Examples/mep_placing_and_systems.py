import sys
import aclib

# Requires Archicad 28 or newer; GetMEPPhysicalSystemsOfElements and the physical systems of the
# distribution systems require Archicad 29.

# GetMEPDistributionSystems with includeTree gives each system's elements from its root element on.
systems = aclib.RunTapirCommand ('GetMEPDistributionSystems', {
        'includeTree': True
    })
if systems is None or 'error' in systems:
    print ('Could not read the MEP distribution systems, this example needs Archicad 28 or newer.')
    sys.exit ()

for system in systems['distributionSystems']:
    print (f"{system['domain']} system: {len (system['elements'])} element(s), "
           f"{len (system.get ('physicalSystemIds', []))} physical system(s)")
    for node in system.get ('tree', []):
        print (f"{'  ' * (node['depth'] + 1)}{node['elementId']['guid']} ({len (node['nextElements'])} next)")

# GetMEPPhysicalSystemsOfElements tells which physical systems the elements are part of.
mepElements = aclib.RunTapirCommand ('GetMEPElements', {})['elements']
if mepElements:
    physicalSystems = aclib.RunTapirCommand ('GetMEPPhysicalSystemsOfElements', {
            'elements': [{'elementId': e['elementId']} for e in mepElements[:5]]
        })
    if 'error' in physicalSystems:
        print (f"Physical systems: {physicalSystems['error']['message']}")
    else:
        for element, systemsOfElement in zip (mepElements, physicalSystems['physicalSystemsOfElements']):
            print (f"{element['type']}: {len (systemsOfElement['physicalSystemIds'])} physical system(s)")

# A pipe route to place a take-off on.
created = aclib.RunTapirCommand ('CreateMEPRoutingElements', {
        'routingElementsData': [
            {
                'domain': 'Piping',
                'nodeCoordinates': [
                    {'x': 0.0, 'y': -30.0, 'z': 1.0},
                    {'x': 6.0, 'y': -30.0, 'z': 1.0}
                ]
            }
        ]
    })
if created is None or 'error' in created or 'error' in created['elements'][0]:
    print ('Could not create the routing element.')
    sys.exit ()
routeId = created['elements'][0]['elementId']
segmentId = aclib.RunTapirCommand ('GetMEPRoutingElements', {
        'elements': [{'elementId': routeId}]
    })['routingElements'][0]['segments'][0]['elementId']

placedIds = []

# PlaceMEPTakeOffs places take-offs with the settings of the Take-Off tool.
takeOffs = aclib.RunTapirCommand ('PlaceMEPTakeOffs', {
        'takeOffsData': [
            {
                'domain': 'Piping',
                'segmentId': segmentId,
                'distanceFromBeginning': 2.0,
                'rotationAroundMainAxis': 0.0,
                'length': 0.3
            }
        ]
    })
for takeOff in takeOffs['elements']:
    if 'error' in takeOff:
        print (f"Take-off: {takeOff['error']['message']}")
    else:
        print ('Take-off placed.')
        placedIds.append (takeOff['elementId'])

# PlaceMEPFlexibleSegments places flexible segments with the settings of the Flexible Segment tool.
flexibleSegments = aclib.RunTapirCommand ('PlaceMEPFlexibleSegments', {
        'flexibleSegmentsData': [
            {
                'domain': 'Ventilation',
                'controlPoints': [
                    {'x': 0.0, 'y': -32.0, 'z': 2.5},
                    {'x': 1.5, 'y': -32.5, 'z': 2.5},
                    {'x': 3.0, 'y': -32.0, 'z': 2.5}
                ]
            }
        ]
    })
for flexibleSegment in flexibleSegments['elements']:
    if 'error' in flexibleSegment:
        print (f"Flexible segment: {flexibleSegment['error']['message']}")
    else:
        print ('Flexible segment placed.')
        placedIds.append (flexibleSegment['elementId'])

# Clean up
aclib.RunTapirCommand ('DeleteElements', {
        'elements': [{'elementId': elementId} for elementId in placedIds + [routeId]]
    })
