"""Adds walls to the selection and removes one of them again with ChangeSelectionOfElements.

Needs at least one Wall. The original selection is restored at the end.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'}, debug = False)['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')
originalSelection = aclib.RunTapirCommand ('GetSelectedElements', {}, debug = False)['elements']

aclib.RunTapirCommand ('ChangeSelectionOfElements', {'addElementsToSelection': walls})
aclib.RunTapirCommand ('ChangeSelectionOfElements', {'removeElementsFromSelection': walls[:1]})
aclib.RunTapirCommand ('GetSelectedElements', {})

# Restore the original selection.
aclib.RunTapirCommand ('ChangeSelectionOfElements', {'removeElementsFromSelection': walls}, debug = False)
aclib.RunTapirCommand ('ChangeSelectionOfElements', {'addElementsToSelection': originalSelection}, debug = False)
