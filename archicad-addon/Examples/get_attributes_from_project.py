import os
import aclib

# Reads the attributes of another project or template file without opening it and lists
# the ones the open project does not have (compared by name), for example to see what a
# template is missing before bringing attributes over.
#
# Set TAPIR_ATTRIBUTE_SOURCE to the absolute path of a .pln or .tpl; without it the
# example does nothing.

sourcePath = os.environ.get ('TAPIR_ATTRIBUTE_SOURCE')
if not sourcePath:
    print ('Set TAPIR_ATTRIBUTE_SOURCE to the path of a project or template file.')
else:
    # Layer, Surface, BuildingMaterial and MEPSystem are not read from a file that is not open
    # (they get an error item), so they are left out here.
    attributeTypes = ['Line', 'Fill', 'Composite', 'LayerCombination', 'Profile', 'PenTable']
    result = aclib.RunTapirCommand ('GetAttributesFromProject', {
        'projectFilePath': sourcePath,
        'attributeTypes': attributeTypes
    })

    for typeResult in result['attributesByType']:
        if 'error' in typeResult:
            print ('Error: {}'.format (typeResult['error']['message']))
            continue
        attributeType = typeResult['attributeType']
        ownNames = {a['name'] for a in aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': attributeType})['attributes']}
        missing = [a['name'] for a in typeResult['attributes'] if a['name'] not in ownNames]
        print ('{}: {} in the file, {} missing from the open project'.format (attributeType, len (typeResult['attributes']), len (missing)))
        for name in missing:
            print ('    {}'.format (name))
