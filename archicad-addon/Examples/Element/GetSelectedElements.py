"""Prints the elements currently selected in Archicad with GetSelectedElements.

Select some elements in Archicad before running it.
"""
import aclib

selectedElements = aclib.RunTapirCommand ('GetSelectedElements', {})['elements']
print ('{} elements are selected.'.format (len (selectedElements)))
