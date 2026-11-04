"""Lists the MEP system groups of the project and their physical systems with GetMEPSystemGroups.

Requires Archicad 29 or later.
"""
import aclib

for systemGroup in aclib.RunTapirCommand ('GetMEPSystemGroups', {})['systemGroups']:
    if 'error' in systemGroup:
        continue
    print (f"{systemGroup['name']}: {', '.join (s['name'] for s in systemGroup['physicalSystems'] if 'name' in s)}")
