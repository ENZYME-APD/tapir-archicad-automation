"""Opens a curtain wall in its hierarchical editing mode with EditHierarchicalElement, then leaves it without changes.

Needs at least one Curtain Wall. Requires Archicad 27 or later.
"""
import aclib

curtainWalls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'CurtainWall'})['elements']
if not curtainWalls:
    raise SystemExit ('No curtain wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('EditHierarchicalElement', {'elementId': curtainWalls[0]['elementId']}))

aclib.RunTapirCommand ('FinishHierarchicalEditing', {'accept': False})
