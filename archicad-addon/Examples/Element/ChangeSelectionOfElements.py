"""Adds walls to the selection and removes one of them again with ChangeSelectionOfElements.

Needs at least one Wall. The original selection is restored at the end.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')
originalSelection = aclib.RunTapirCommand ('GetSelectedElements', {})['elements']

aclib.PrintJson (aclib.RunTapirCommand ('ChangeSelectionOfElements', {'addElementsToSelection': walls}))
aclib.PrintJson (aclib.RunTapirCommand ('ChangeSelectionOfElements', {'removeElementsFromSelection': walls[:1]}))
aclib.RunTapirCommand ('GetSelectedElements', {})

# Restore the original selection.
aclib.RunTapirCommand ('ChangeSelectionOfElements', {'removeElementsFromSelection': walls})
aclib.RunTapirCommand ('ChangeSelectionOfElements', {'addElementsToSelection': originalSelection})
