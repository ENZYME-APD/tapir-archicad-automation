"""Gets the element type of the tool active in the Toolbox with GetCurrentTool."""
import aclib

result = aclib.RunTapirCommand ('GetCurrentTool')

print (result.get ('elementType', 'The active tool makes no element (Arrow or Marquee).'))
