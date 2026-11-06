"""Changes the Structural Function of a wall with SetCategoryValuesOfElements, reads it back, then restores it.

Needs at least one editable Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall', 'filters': ['IsEditable']})['elements'][:1]
if not walls:
    raise SystemExit ('No editable wall in the project.')
wallId = walls[0]['elementId']

def GetStructuralFunction ():
    result = aclib.RunTapirCommand ('GetCategoryValuesOfElements', {'elements': walls, 'categories': ['StructuralFunction']})
    return result['categoryValuesOfElements'][0]['categoryValues'][0]

categories = aclib.RunTapirCommand ('GetElementCategories', {})['categories']
structuralFunction = next (category for category in categories if category['type'] == 'StructuralFunction')
original = GetStructuralFunction ()
newValue = next (value for value in structuralFunction['values']
                 if value['isVisibleOnUI'] and value['categoryValueId'] != original['categoryValueId'])

aclib.PrintJson (aclib.RunTapirCommand ('SetCategoryValuesOfElements', {
    'elementCategoryValues': [{'elementId': wallId, 'type': 'StructuralFunction', 'categoryValueId': newValue['categoryValueId']}]
}))
print ('Structural Function: {} -> {}'.format (original['name'], GetStructuralFunction ()['name']))

aclib.RunTapirCommand ('SetCategoryValuesOfElements', {
    'elementCategoryValues': [{'elementId': wallId, 'type': 'StructuralFunction', 'categoryValueId': original['categoryValueId']}]
})
