import aclib

# Returns the layer combination in effect in the current window. When the
# current state of the layers matches no layer combination, the response has
# no layerCombinationId.
result = aclib.RunTapirCommand ('GetCurrentLayerCombination')

if 'layerCombinationId' in result:
    print ('Current layer combination: ' + result['name'])
else:
    print ('The current state of the layers matches no layer combination.')
