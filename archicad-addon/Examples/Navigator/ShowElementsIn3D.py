"""Opens the 3D window on the first three walls of the project with ShowElementsIn3D, as Show Selection in 3D does.

Needs at least one Wall. The walls become the selection.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('ShowElementsIn3D', {'elements': walls}))
