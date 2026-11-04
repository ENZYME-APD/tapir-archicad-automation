"""Runs the parameter script of every object with RunParameterScriptOfElements.

Needs at least one Object. Parameters that depend on other parameters are recalculated.
"""
import aclib

objects = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Object'})['elements']
if not objects:
    raise SystemExit ('No object in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('RunParameterScriptOfElements', {'elements': objects}))
