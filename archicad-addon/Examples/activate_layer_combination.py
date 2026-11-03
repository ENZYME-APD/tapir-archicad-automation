import sys
import aclib

# Activates a layer combination in the current Archicad window, the way the
# Quick Layers palette does. Pass the name of the combination as the first
# argument; without one the first layer combination of the project is used.
layerCombinationName = sys.argv[1] if len(sys.argv) > 1 and sys.argv[1] != 'silent' else None

layerCombinations = aclib.RunTapirCommand('GetAttributesByType', {'attributeType': 'LayerCombination'})['attributes']
if layerCombinationName is None:
    layerCombination = layerCombinations[0] if layerCombinations else None
else:
    layerCombination = next((lc for lc in layerCombinations if lc['name'] == layerCombinationName), None)

if layerCombination is None:
    print('No layer combination found to activate.')
else:
    print(f"Activating layer combination: {layerCombination['name']}")
    result = aclib.RunTapirCommand('ActivateLayerCombination', {'attributeId': layerCombination['attributeId']})
    print(result)
