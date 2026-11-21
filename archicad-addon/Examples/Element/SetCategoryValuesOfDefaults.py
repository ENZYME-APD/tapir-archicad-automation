"""Sets the Renovation Status new walls, slabs and windows get, like the Renovation palette, with SetCategoryValuesOfDefaults, reads it back, then restores it.

A dry run first shows what would change. The values are identified by their categoryValueId,
never by their localized names.
"""
import aclib

elementTypes = ['Wall', 'Slab', 'Window']

def GetRenovationStatusOfDefaults ():
    result = aclib.RunTapirCommand ('GetCategoryValuesOfDefaults', {'elementTypes': elementTypes, 'categories': ['RenovationStatus']})
    return [item['categoryValues'][0] for item in result['categoryValuesOfDefaults']]

categories = aclib.RunTapirCommand ('GetElementCategories', {})['categories']
renovationStatus = next (category for category in categories if category['type'] == 'RenovationStatus')
originals = GetRenovationStatusOfDefaults ()
newValue = next (value for value in renovationStatus['values']
                 if value['isVisibleOnUI'] and value['categoryValueId'] != originals[0].get ('categoryValueId'))

parameters = {
    'elementTypes': elementTypes,
    'categoryValues': [{'type': 'RenovationStatus', 'categoryValueId': newValue['categoryValueId']}]
}
aclib.PrintJson (aclib.RunTapirCommand ('SetCategoryValuesOfDefaults', dict (parameters, dryRun=True)))
aclib.PrintJson (aclib.RunTapirCommand ('SetCategoryValuesOfDefaults', parameters))
for elementType, value in zip (elementTypes, GetRenovationStatusOfDefaults ()):
    print ('{}: {}'.format (elementType, value.get ('name')))

for elementType, original in zip (elementTypes, originals):
    if 'categoryValueId' in original:
        aclib.RunTapirCommand ('SetCategoryValuesOfDefaults', {
            'elementTypes': [elementType],
            'categoryValues': [{'type': 'RenovationStatus', 'categoryValueId': original['categoryValueId']}]
        })
