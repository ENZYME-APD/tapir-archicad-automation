import aclib

# Without an analyticalModelId the model of the current view is used (Archicad 27 or newer).
modelVariation = aclib.RunTapirCommand ('GetAnalyticalModelVariation')

# The variation gives back the model it was read from.
if modelVariation and 'variation' in modelVariation:
    aclib.RunTapirCommand ('GetAnalyticalModel', {
            'variation': modelVariation['variation']
        })

# Requires Archicad 26 or newer.
visibilitySettings = aclib.RunTapirCommand ('GetAnalyticalVisibilitySettings')

# Only the given fields change.
if visibilitySettings and 'visibilitySettings' in visibilitySettings:
    aclib.RunTapirCommand ('SetAnalyticalVisibilitySettings', {
            'visibilitySettings': {
                'areNodesVisible': visibilitySettings['visibilitySettings']['areNodesVisible']
            }
        })

elements = []
for elementType in ['Wall', 'Beam', 'Column', 'Slab', 'Roof']:
    elements.extend (aclib.RunCommand ('API.GetElementsByType', { 'elementType' : elementType })['elements'])

aclib.RunTapirCommand ('GetAnalyticalMemberTypesOfElements', {
        'elements': elements
    })
