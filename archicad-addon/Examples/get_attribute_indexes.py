import aclib

# How the indexes of some attribute types are used: the number of attributes, the first valid and the
# highest index, and the free indexes between them (for example those of deleted attributes).
result = aclib.RunTapirCommand ('GetAttributeIndexes', {
    'attributeTypes': ['Layer', 'Fill', 'Line', 'Surface', 'BuildingMaterial']
})

for item in result['attributeIndexes']:
    if 'error' in item:
        continue
    print ('{}: {} attributes, indexes {}..{}, {} free'.format (
        item['attributeType'], item['count'], item.get ('firstIndex'), item.get ('lastIndex'), len (item['freeIndexes'])))
