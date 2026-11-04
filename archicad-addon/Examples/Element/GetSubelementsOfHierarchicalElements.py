"""Lists the subelements (segments, frames, panels, treads, posts ...) of the curtain walls, stairs and railings with GetSubelementsOfHierarchicalElements.

Needs at least one Curtain Wall, Stair or Railing.
"""
import aclib

elements = []
for elementType in ['CurtainWall', 'Stair', 'Railing']:
    elements += aclib.RunTapirCommand ('GetElementsByType', {'elementType': elementType})['elements']
if not elements:
    raise SystemExit ('No curtain wall, stair or railing in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetSubelementsOfHierarchicalElements', {'elements': elements[:3]}))
