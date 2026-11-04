"""Reads the key-value pairs scripts stored on the first three Layers with GetUserDataOfAttributes."""
import aclib

layers = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Layer'})['attributes'][:3]

aclib.PrintJson (aclib.RunTapirCommand ('GetUserDataOfAttributes', {
    'attributeIds': [{'attributeId': layer['attributeId']} for layer in layers]
}))
