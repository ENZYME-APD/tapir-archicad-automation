"""Finds the first named model view option combination by its name with FindModelViewOption, then by its id.

Requires Archicad 27 or later.
"""
import aclib

# Archicad may list a combination without a name once the view options of a window changed.
names = [modelViewOption['name'] for modelViewOption in aclib.RunTapirCommand ('GetModelViewOptions')['modelViewOptions'] if modelViewOption['name']]
if not names:
    raise SystemExit ('No model view option combination in the project.')

found = aclib.RunTapirCommand ('FindModelViewOption', {'name': names[0]})
aclib.PrintJson (found)
aclib.PrintJson (aclib.RunTapirCommand ('FindModelViewOption', {'modelViewOptionId': found['modelViewOptionId']}))
