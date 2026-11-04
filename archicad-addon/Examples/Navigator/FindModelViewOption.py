"""Finds the first model view option combination by its name with FindModelViewOption, then by its id.

Requires Archicad 27 or later.
"""
import aclib

modelViewOptions = aclib.RunTapirCommand ('GetModelViewOptions')['modelViewOptions']
if not modelViewOptions:
    raise SystemExit ('No model view option combination in the project.')

found = aclib.RunTapirCommand ('FindModelViewOption', {'name': modelViewOptions[0]['name']})
aclib.PrintJson (found)
aclib.PrintJson (aclib.RunTapirCommand ('FindModelViewOption', {'modelViewOptionId': found['modelViewOptionId']}))
