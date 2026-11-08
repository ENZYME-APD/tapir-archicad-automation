"""Prints the current settings of the ventilation and piping MEP tools with GetMEPToolDefaults.

Leave out 'domains' to get all three domains. Requires Archicad 28 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetMEPToolDefaults', {
    'domains': ['Ventilation', 'Piping']
}))
