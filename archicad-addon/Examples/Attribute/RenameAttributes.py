"""Renames the last Layer of the project with RenameAttributes, then gives it back its old name.

The Layer keeps its index, its id and its settings.
"""
import aclib

layer = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Layer'})['attributes'][-1]

aclib.PrintJson (aclib.RunTapirCommand ('RenameAttributes', {
    'attributes': [{'attributeId': layer['attributeId'], 'newName': layer['name'] + ' (renamed)'}]
}))

aclib.RunTapirCommand ('RenameAttributes', {
    'attributes': [{'attributeId': layer['attributeId'], 'newName': layer['name']}]
})
