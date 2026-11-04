"""Reads the absolute bottom level of the curtain walls of the project with GetCurtainWallAbsoluteBottoms.

Needs at least one Curtain Wall. Its subelements (panels, frames ...) are accepted too.
"""
import aclib

curtainWalls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'CurtainWall'}, debug = False)['elements']
if not curtainWalls:
    raise SystemExit ('No curtain wall in the project.')

aclib.RunTapirCommand ('GetCurtainWallAbsoluteBottoms', {'elements': curtainWalls})
