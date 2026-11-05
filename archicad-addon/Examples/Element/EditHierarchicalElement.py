"""Opens a curtain wall in its hierarchical editing mode with EditHierarchicalElement.

While the mode is open, Archicad refuses every JSON command with error 4001 (ongoing user input),
so the script cannot leave it: leave it yourself in Archicad, with the button of the edit mode palette.
Needs at least one Curtain Wall. Requires Archicad 27 or later.
"""
import aclib

curtainWalls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'CurtainWall'})['elements']
if not curtainWalls:
    raise SystemExit ('No curtain wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('EditHierarchicalElement', {'elementId': curtainWalls[0]['elementId']}))
