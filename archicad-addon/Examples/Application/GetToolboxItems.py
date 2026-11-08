"""Lists the tools of the Toolbox with GetToolboxItems, the tools of add-ons included, and the active tool.

Each item can be given to SetCurrentTool as it is. includeHidden also lists the tools without an icon,
e.g. the Slab Accessory.
"""
import aclib

result = aclib.RunTapirCommand ('GetToolboxItems', {'includeHidden': False})

for item in result['toolboxItems']:
    print (item)
print ('Active tool:', result['currentTool'] or 'Arrow or Marquee')
