import aclib

# Lists the element categories (Structural Function, Position, Renovation Status ...) with their
# values, sets a few walls to Load-Bearing and Exterior, then reads the values back.
# The names of the categories and of their values are localized, so a script selects a category
# by its 'type' and a value by its 'categoryValueId'. The English names below only pick the values
# for this example; on a localized Archicad the first value of the category is taken instead.

categories = aclib.RunTapirCommand ('GetElementCategories')['categories']
for category in categories:
    print ('{} ({}): {}'.format (category['type'], category['name'], ', '.join (value['name'] for value in category['values'])))


def FindValue (categoryType, englishName):
    category = next (c for c in categories if c['type'] == categoryType)
    for value in category['values']:
        if value['name'] == englishName:
            return value
    return next (value for value in category['values'] if value['isVisibleOnUI'])


loadBearing = FindValue ('StructuralFunction', 'Load-Bearing')
exterior = FindValue ('Position', 'Exterior')

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'}, debug = False)['elements'][:3]

aclib.RunTapirCommand ('SetCategoryValuesOfElements', {'elementCategoryValues':
    [{'elementId': wall['elementId'], 'type': 'StructuralFunction', 'categoryValueId': loadBearing['categoryValueId']} for wall in walls] +
    [{'elementId': wall['elementId'], 'type': 'Position', 'categoryValueId': exterior['categoryValueId']} for wall in walls]})

aclib.RunTapirCommand ('GetCategoryValuesOfElements', {
    'elements': walls,
    'categories': ['StructuralFunction', 'Position', 'RenovationStatus']})

# Without 'categories' every category of the element is returned; an element without categories
# (a Text, for example) gets an error item instead of failing the whole call.
texts = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Text'}, debug = False)['elements'][:1]
aclib.RunTapirCommand ('GetCategoryValuesOfElements', {'elements': walls[:1] + texts})
