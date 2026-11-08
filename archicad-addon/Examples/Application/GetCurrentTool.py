"""Gets the tool active in the Toolbox with GetCurrentTool.

The result (element type, variation, class id) can be given to SetCurrentTool as it is. It is empty
when the Arrow or the Marquee is active.
"""
import aclib

result = aclib.RunTapirCommand ('GetCurrentTool')

if result:
    aclib.PrintJson (result)
else:
    print ('The active tool makes no element (Arrow or Marquee).')
