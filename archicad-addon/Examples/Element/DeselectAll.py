"""Clears the selection in the current window with DeselectAll.

The original selection is restored at the end.
"""
import aclib

originalSelection = aclib.RunTapirCommand ('GetSelectedElements', {}, debug = False)['elements']
print ('{} elements were selected.'.format (len (originalSelection)))

aclib.RunTapirCommand ('DeselectAll', {})
print ('{} elements are selected now.'.format (len (aclib.RunTapirCommand ('GetSelectedElements', {}, debug = False)['elements'])))

aclib.RunTapirCommand ('ChangeSelectionOfElements', {'addElementsToSelection': originalSelection}, debug = False)
