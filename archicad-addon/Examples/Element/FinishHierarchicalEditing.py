"""Leaves the hierarchical editing mode of a curtain wall with FinishHierarchicalEditing, discarding the changes (Cancel).

Opens the first Curtain Wall in editing mode first. accept True would keep the changes (OK). Requires Archicad 27 or later.
"""
import aclib

curtainWalls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'CurtainWall'})['elements']
if not curtainWalls:
    raise SystemExit ('No curtain wall in the project.')

aclib.RunTapirCommand ('EditHierarchicalElement', {'elementId': curtainWalls[0]['elementId']})

aclib.PrintJson (aclib.RunTapirCommand ('FinishHierarchicalEditing', {'accept': False}))
