"""Creates a physical system in a new MEP system group and removes it again with DeleteMEPPhysicalSystems.

Leaves the new, empty system group "Heating" in the project. Requires Archicad 29 or later.
"""
import aclib

systemGroupId = aclib.RunTapirCommand ('CreateMEPSystemGroups', {
    'systemGroupsData': [{'name': 'Heating'}]
})['systemGroups'][0]['systemGroupId']
physicalSystemId = aclib.RunTapirCommand ('CreateMEPPhysicalSystems', {
    'physicalSystemsData': [{'systemGroupId': systemGroupId, 'name': 'Boiler Flow'}]
})['physicalSystems'][0]['physicalSystemId']

aclib.PrintJson (aclib.RunTapirCommand ('DeleteMEPPhysicalSystems', {
    'physicalSystems': [{'physicalSystemId': physicalSystemId}]
}))
