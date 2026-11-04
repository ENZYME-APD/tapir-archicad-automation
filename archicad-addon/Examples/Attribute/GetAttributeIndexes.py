"""Prints how the indexes of a few attribute types are used, with GetAttributeIndexes.

For each type: the number of attributes, the first valid and the highest index, and how many indexes
between them are free (for example those of deleted attributes). Requires Archicad 27 or later.
"""
import aclib

result = aclib.RunTapirCommand ('GetAttributeIndexes', {
    'attributeTypes': ['Layer', 'Fill', 'Line', 'Surface', 'BuildingMaterial']
})

for item in result['attributeIndexes']:
    if 'error' in item:
        continue
    print ('{}: {} attributes, indexes {}..{}, {} free'.format (
        item['attributeType'], item['count'], item.get ('firstIndex'), item.get ('lastIndex'), len (item['freeIndexes'])))
