"""Renames the first editable MEP system group of the project with ModifyMEPSystemGroups and renames it back.

Needs at least one system group. Requires Archicad 29 or later.
"""
import aclib

systemGroups = [g for g in aclib.RunTapirCommand ('GetMEPSystemGroups', {})['systemGroups'] if g.get ('isEditable')]
if not systemGroups:
    raise SystemExit ('No editable MEP system group in the project.')
systemGroup = systemGroups[0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyMEPSystemGroups', {
    'systemGroupsData': [{'systemGroupId': systemGroup['systemGroupId'], 'name': systemGroup['name'] + ' (renamed)'}]
}))
aclib.RunTapirCommand ('GetMEPSystemGroups', {})

aclib.PrintJson (aclib.RunTapirCommand ('ModifyMEPSystemGroups', {
    'systemGroupsData': [{'systemGroupId': systemGroup['systemGroupId'], 'name': systemGroup['name']}]
}))
