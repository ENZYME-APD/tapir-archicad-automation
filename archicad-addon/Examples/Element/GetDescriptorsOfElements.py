"""Lists the listing descriptors in the property objects of the first walls with GetDescriptorsOfElements.

Needs at least one Wall. Usually only projects migrated from older Archicad versions have property objects.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetDescriptorsOfElements', {'elements': walls}))
