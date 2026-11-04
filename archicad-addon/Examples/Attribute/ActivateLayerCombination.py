"""Activates the first Layer Combination in the current window with ActivateLayerCombination, then restores the previous one.

Saved views are not changed. The previous state is only restored when it matched a layer combination.
"""
import aclib

layerCombinations = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'LayerCombination'})['attributes']
if not layerCombinations:
    raise SystemExit ('No Layer Combination in the project.')
current = aclib.RunTapirCommand ('GetCurrentLayerCombination')

aclib.PrintJson (aclib.RunTapirCommand ('ActivateLayerCombination', {'attributeId': layerCombinations[0]['attributeId']}))

if 'layerCombinationId' in current:
    aclib.PrintJson (aclib.RunTapirCommand ('ActivateLayerCombination', {'attributeId': current['layerCombinationId']}))
