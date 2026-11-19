"""Labels the selected elements (or else the first wall) with the Label tool's current settings with LabelElements.

The same as Document > Label Selected Items: each element gets an associative label at its
default position, with the content of the Label tool settings, autotext included. Give
favoriteName to label them with a Label favorite instead.
"""
import aclib

elements = aclib.RunTapirCommand ('GetSelectedElements', {})['elements']
if not elements:
    elements = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not elements:
    raise SystemExit ('Select the elements to label first.')

aclib.PrintJson (aclib.RunTapirCommand ('LabelElements', {'elements': elements}))
