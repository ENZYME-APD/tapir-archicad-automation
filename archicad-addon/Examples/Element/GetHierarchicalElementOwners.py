"""Finds the curtain wall each curtain wall frame belongs to with GetHierarchicalElementOwners.

Needs at least one Curtain Wall.
"""
import aclib

curtainWalls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'CurtainWall'})['elements'][:1]
if not curtainWalls:
    raise SystemExit ('No curtain wall in the project.')

subelements = aclib.RunTapirCommand ('GetSubelementsOfHierarchicalElements', {'elements': curtainWalls})['subelements']
frames = subelements[0].get ('cWallFrames', [])[:3]

aclib.PrintJson (aclib.RunTapirCommand ('GetHierarchicalElementOwners', {
    'elements': frames,
    'ownerMode': 'root'
}))
