"""Finds where the walls and slabs of the project touch each other with GetElementConnections.

Needs Walls or Slabs. Requires Archicad 28 or later.
"""
import aclib

elements = []
for elementType in ['Wall', 'Slab']:
    elements += aclib.RunTapirCommand ('GetElementsByType', {'elementType': elementType})['elements']
if not elements:
    raise SystemExit ('No wall or slab in the project.')

connections = aclib.RunTapirCommand ('GetElementConnections', {'elements': elements})['connections']
for connection in connections:
    print ('{} touches {}'.format (connection['elementId1']['guid'], connection['elementId2']['guid']))
