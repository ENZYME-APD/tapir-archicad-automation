"""Gets the Layer Combination in effect in the current window with GetCurrentLayerCombination."""
import aclib

result = aclib.RunTapirCommand ('GetCurrentLayerCombination')

print (result.get ('name', 'The layers match no layer combination.'))
