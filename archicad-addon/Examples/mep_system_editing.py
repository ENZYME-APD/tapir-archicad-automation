import sys
import aclib

# Renames, modifies and deletes MEP physical systems: creates a system group with two physical
# systems, renames the group and one of the systems, then deletes the other system and lists the
# group again. Every run creates a new group. Requires Archicad 29 or newer; on older versions the
# first command fails and the example stops.

systemGroups = aclib.RunTapirCommand ('CreateMEPSystemGroups', {
        'systemGroupsData': [{'name': 'Heating'}]
    })
if systemGroups is None or 'error' in systemGroups:
    print ('Could not create a system group, this example needs Archicad 29 or newer.')
    sys.exit ()
systemGroupId = systemGroups['systemGroups'][0]['systemGroupId']

physicalSystems = aclib.RunTapirCommand ('CreateMEPPhysicalSystems', {
        'physicalSystemsData': [
            {'systemGroupId': systemGroupId, 'name': 'Boiler Flow'},
            {'systemGroupId': systemGroupId, 'name': 'Boiler Return'}
        ]
    })['physicalSystems']
flowSystemId = physicalSystems[0]['physicalSystemId']
returnSystemId = physicalSystems[1]['physicalSystemId']

# The MEP system attribute "Heating Flow" is used as the category of the flow system, when the project has one.
mepSystems = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'MEPSystem'})['attributes']
heatingFlowId = next ((attribute['attributeId'] for attribute in mepSystems if attribute['name'] == 'Heating Flow'), None)

aclib.RunTapirCommand ('ModifyMEPSystemGroups', {
        'systemGroupsData': [{'systemGroupId': systemGroupId, 'name': 'Heating Ground Floor'}]
    })

flowSystemData = {'physicalSystemId': flowSystemId, 'name': 'Boiler 1 Flow'}
if heatingFlowId is not None:
    flowSystemData['mepSystemId'] = heatingFlowId
aclib.RunTapirCommand ('ModifyMEPPhysicalSystems', {
        'physicalSystemsData': [flowSystemData]
    })

aclib.RunTapirCommand ('DeleteMEPPhysicalSystems', {
        'physicalSystems': [{'physicalSystemId': returnSystemId}]
    })

for systemGroup in aclib.RunTapirCommand ('GetMEPSystemGroups')['systemGroups']:
    if 'error' in systemGroup or systemGroup['systemGroupId'] != systemGroupId:
        continue
    print (f"System group {systemGroup['name']}, editable: {systemGroup.get ('isEditable')}")
    for physicalSystem in systemGroup['physicalSystems']:
        if 'error' in physicalSystem:
            continue
        print (f"  {physicalSystem['name']}, editable: {physicalSystem.get ('isEditable')}")
