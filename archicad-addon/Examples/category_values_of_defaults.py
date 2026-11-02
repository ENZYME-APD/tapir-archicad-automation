import aclib

# Reads the element category values (Structural Function, Position, Renovation Status ...) of the
# tool defaults, the values new elements of the type get. The names are localized, so a script
# compares the 'type' and the 'categoryValueId', as GetElementCategories lists them.

aclib.RunTapirCommand ('GetCategoryValuesOfDefaults', {
    'elementTypes': ['Wall', 'Slab', 'Column'],
    'categories': ['StructuralFunction', 'Position', 'RenovationStatus']})

# Without 'categories' every category of the project is read; an element type without categories
# (a Text, for example) gets an error item instead of failing the whole call.
aclib.RunTapirCommand ('GetCategoryValuesOfDefaults', {'elementTypes': ['Wall', 'Text']})
