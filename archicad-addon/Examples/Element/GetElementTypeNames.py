"""Prints the names of some element types as the running Archicad shows them, with GetElementTypeNames.

The names are localized, e.g. "Wand" for Wall in German Archicad. Requires Archicad 27 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetElementTypeNames', {
    'elementTypes': ['Wall', 'Slab', 'Zone', 'Door', 'Window']
}))
