"""Creates a flow and a return physical system in a new MEP system group with CreateMEPPhysicalSystems.

Adds a new system group "Heating" on every run. Requires Archicad 29 or later.
"""
import aclib

systemGroupId = aclib.RunTapirCommand ('CreateMEPSystemGroups', {
    'systemGroupsData': [{'name': 'Heating'}]
})['systemGroups'][0]['systemGroupId']

aclib.PrintJson (aclib.RunTapirCommand ('CreateMEPPhysicalSystems', {
    'physicalSystemsData': [
        {'systemGroupId': systemGroupId, 'name': 'Boiler Flow'},
        {'systemGroupId': systemGroupId, 'name': 'Boiler Return'}
    ]
}))
