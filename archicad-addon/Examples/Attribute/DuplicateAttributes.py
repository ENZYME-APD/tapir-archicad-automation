"""Duplicates the first Fill of the project with DuplicateAttributes, then deletes the copy again.

The copy gets every setting of the source (its pattern, its classification, its custom property values
and its attribute folder) under the new name; a name that is already used gives an error item.
"""
import aclib

fill = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Fill'})['attributes'][0]

result = aclib.RunTapirCommand ('DuplicateAttributes', {
    'attributes': [{'attributeId': fill['attributeId'], 'newName': fill['name'] + ' (copy)'}]
})
aclib.PrintJson (result)

copy = result['attributeIds'][0]
if 'attributeId' in copy:
    aclib.RunTapirCommand ('DeleteAttributes', {
        'attributesToDelete': [{'attributeType': 'Fill', 'attributeId': {'attributeId': copy['attributeId']}}]
    })
