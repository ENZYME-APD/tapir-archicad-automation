"""Reads the category values new walls, slabs and columns get from their tool defaults, with GetCategoryValuesOfDefaults."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetCategoryValuesOfDefaults', {
    'elementTypes': ['Wall', 'Slab', 'Column'],
    'categories': ['StructuralFunction', 'Position', 'RenovationStatus']
}))
