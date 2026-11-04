"""Reads every Layer Combination with the state of its layers with GetLayerCombinations."""
import aclib

layerCombinations = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'LayerCombination'})['attributes']

aclib.PrintJson (aclib.RunTapirCommand ('GetLayerCombinations', {
    'attributes': [{'attributeId': layerCombination['attributeId']} for layerCombination in layerCombinations]
}))
