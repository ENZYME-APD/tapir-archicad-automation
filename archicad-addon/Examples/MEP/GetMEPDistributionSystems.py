"""Prints the MEP distribution systems of the project and their element trees with GetMEPDistributionSystems.

The physical systems of a distribution system are returned from Archicad 29. Requires Archicad 28 or later.
"""
import aclib

systems = aclib.RunTapirCommand ('GetMEPDistributionSystems', {'includeTree': True})['distributionSystems']
for system in systems:
    print (f"{system['domain']} system: {len (system['elements'])} element(s)")
    for node in system.get ('tree', []):
        print (f"{'  ' * (node['depth'] + 1)}{node['elementId']['guid']}")
