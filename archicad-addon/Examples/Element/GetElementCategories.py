"""Lists the element categories of the project (Structural Function, Position, Renovation Status ...) and their values with GetElementCategories.

The names are localized; scripts should pick a category by its type and a value by its categoryValueId.
"""
import aclib

categories = aclib.RunTapirCommand ('GetElementCategories', {}, debug = False)['categories']
for category in categories:
    print ('{} ({}): {}'.format (category['type'], category['name'], ', '.join (value['name'] for value in category['values'])))
