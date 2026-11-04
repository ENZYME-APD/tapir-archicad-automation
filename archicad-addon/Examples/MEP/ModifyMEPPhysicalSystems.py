"""Renames the first editable MEP physical system of the project with ModifyMEPPhysicalSystems and renames it back.

Needs at least one physical system. Requires Archicad 29 or later.
"""
import aclib

physicalSystems = [s for g in aclib.RunTapirCommand ('GetMEPSystemGroups', {})['systemGroups']
                   for s in g.get ('physicalSystems', []) if s.get ('isEditable')]
if not physicalSystems:
    raise SystemExit ('No editable MEP physical system in the project.')
physicalSystem = physicalSystems[0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyMEPPhysicalSystems', {
    'physicalSystemsData': [{'physicalSystemId': physicalSystem['physicalSystemId'], 'name': physicalSystem['name'] + ' (renamed)'}]
}))
aclib.RunTapirCommand ('GetMEPSystemGroups', {})

aclib.PrintJson (aclib.RunTapirCommand ('ModifyMEPPhysicalSystems', {
    'physicalSystemsData': [{'physicalSystemId': physicalSystem['physicalSystemId'], 'name': physicalSystem['name']}]
}))
