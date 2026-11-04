"""Keeps only the editable walls and slabs of all elements with FilterElements."""
import aclib

elements = aclib.RunTapirCommand ('GetAllElements', {})['elements']

filtered = aclib.RunTapirCommand ('FilterElements', {
    'elements': elements,
    'filters': ['IsEditable'],
    'elementTypes': ['Wall', 'Slab']
})['elements']

print ('{} of {} elements are editable walls or slabs.'.format (len (filtered), len (elements)))
