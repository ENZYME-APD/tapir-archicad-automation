"""Locks the last Layer of the project with ModifyLayers and puts it in wireframe, then sets both back.

Only the given fields are changed: the Layer keeps its name, its shown state and its intersection group.
"""
import aclib

layer = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Layer'})['attributes'][-1]
before = aclib.RunTapirCommand ('GetLayers', {'attributeIds': [{'attributeId': layer['attributeId']}]})['layers'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyLayers', {
    'layers': [{'attributeId': layer['attributeId'], 'isLocked': True, 'isWireframe': True}]
}))
aclib.PrintJson (aclib.RunTapirCommand ('GetLayers', {'attributeIds': [{'attributeId': layer['attributeId']}]}))

aclib.RunTapirCommand ('ModifyLayers', {
    'layers': [{'attributeId': layer['attributeId'], 'isLocked': before['isLocked'], 'isWireframe': before['isWireframe']}]
})
