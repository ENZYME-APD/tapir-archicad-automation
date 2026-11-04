"""Prints the first autotext keys Archicad knows, each with its current value, with GetAllAutoTextKeys.

The values are resolved without an element context; the keys and values depend on the project,
the Archicad version and the UI language.
"""
import aclib

autoTexts = aclib.RunTapirCommand ('GetAllAutoTextKeys', {})['autoTexts']
print (f'{len (autoTexts)} autotext keys, the first ones:')
aclib.PrintJson (autoTexts[:10])
