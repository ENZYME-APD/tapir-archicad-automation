import sys
import aclib

# Organizes MEP elements into physical systems: creates a supply duct and a system group with a
# supply system that starts from the duct, then lists every system group of the project.
# Archicad keeps a root element only while it is part of an MEP network: the root of a stand-alone
# element (for example equipment with nothing connected) is cleared when the networks are recalculated.
# Every run creates a new group. Requires Archicad 29 or newer; on older versions the first
# command fails and the example stops.

systemGroups = aclib.RunTapirCommand ('CreateMEPSystemGroups', {
        'systemGroupsData': [{'name': 'Ventilation Ground Floor'}]
    })
if systemGroups is None or 'error' in systemGroups:
    print ('Could not create a system group, this example needs Archicad 29 or newer.')
    sys.exit ()
systemGroupId = systemGroups['systemGroups'][0]['systemGroupId']

# The MEP system attribute "Supply Air" is used as the category of the supply system.
mepSystems = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'MEPSystem'})['attributes']
supplyAirId = next ((attribute['attributeId'] for attribute in mepSystems if attribute['name'] == 'Supply Air'), None)

supplyDuctId = aclib.RunTapirCommand ('CreateMEPRoutingElements', {
        'routingElementsData': [{
            'domain': 'Ventilation',
            'nodeCoordinates': [{'x': 0.0, 'y': 0.0, 'z': 2.5}, {'x': 6.0, 'y': 0.0, 'z': 2.5}]
        }]
    })['elements'][0]['elementId']

supplySystem = {'systemGroupId': systemGroupId, 'name': 'AHU-01 Supply', 'rootElementId': supplyDuctId}
if supplyAirId is not None:
    supplySystem['mepSystemId'] = supplyAirId
aclib.RunTapirCommand ('CreateMEPPhysicalSystems', {
        'physicalSystemsData': [
            supplySystem,
            {'systemGroupId': systemGroupId, 'name': 'AHU-01 Extract'}
        ]
    })

for systemGroup in aclib.RunTapirCommand ('GetMEPSystemGroups')['systemGroups']:
    if 'error' in systemGroup:
        continue
    print (f"System group {systemGroup['name']}:")
    for physicalSystem in systemGroup['physicalSystems']:
        if 'error' in physicalSystem:
            continue
        rootElement = physicalSystem['rootElementId']['guid'] if 'rootElementId' in physicalSystem else 'none'
        print (f"  {physicalSystem['name']}, root element {rootElement}")
