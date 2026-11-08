"""Lists the fills of the current database in Archicad's drawing order (Bring to Front / Send to Back) with GetElementsInDrawOrder."""
import aclib

fills = aclib.RunTapirCommand ('GetElementsInDrawOrder', {'elementType': 'Hatch'})['elements']
print ('{} fills, from the bottom to the top:'.format (len (fills)))
for fill in fills:
    print (fill['elementId']['guid'])
