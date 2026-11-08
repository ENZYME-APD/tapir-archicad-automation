"""Places new Walls on the last Layer of the project with SetLayerOfElementDefaults, then restores the previous Layer of the Wall tool defaults."""
import aclib

layers = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Layer'})['attributes']
previous = aclib.RunTapirCommand ('GetLayerOfElementDefaults', {'elementTypes': ['Wall']})['layersOfElementDefaults'][0]

aclib.PrintJson (aclib.RunTapirCommand ('SetLayerOfElementDefaults', {
    'layersOfElementDefaults': [{'elementType': 'Wall', 'layerId': layers[-1]['attributeId']}]
}))
aclib.PrintJson (aclib.RunTapirCommand ('GetLayerOfElementDefaults', {'elementTypes': ['Wall']}))

aclib.RunTapirCommand ('SetLayerOfElementDefaults', {
    'layersOfElementDefaults': [{'elementType': 'Wall', 'layerId': previous['layerId']}]
})
