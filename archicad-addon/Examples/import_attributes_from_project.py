import os
import aclib

# Imports fills, line types, zone categories, pen tables and operation profiles from another
# project or template file into the open project, like Append in Archicad's Attribute Manager:
# attributes whose name already exists are kept as they are. A dry run first lists what the
# import would do.
#
# Set TAPIR_ATTRIBUTE_SOURCE to the absolute path of a .pln or .tpl; without it the
# example does nothing.

sourcePath = os.environ.get ('TAPIR_ATTRIBUTE_SOURCE')
if not sourcePath:
    print ('Set TAPIR_ATTRIBUTE_SOURCE to the path of a project or template file.')
else:
    parameters = {
        'projectFilePath': sourcePath,
        'attributeTypes': ['Fill', 'Line', 'ZoneCategory', 'PenTable', 'OperationProfile']
    }

    def PrintResult (result):
        for typeResult in result['attributesByType']:
            if 'error' in typeResult:
                print ('Error: {}'.format (typeResult['error']['message']))
                continue
            counts = {}
            for attribute in typeResult['attributes']:
                counts[attribute['result']] = counts.get (attribute['result'], 0) + 1
            print ('{}: {}'.format (typeResult['attributeType'], ', '.join ('{} {}'.format (n, r) for r, n in sorted (counts.items ())) or 'nothing in the file'))
            for attribute in typeResult['attributes']:
                if attribute['result'] in ('created', 'failed'):
                    print ('    {} {}'.format (attribute['result'], attribute['name']))

    print ('Dry run:')
    PrintResult (aclib.RunTapirCommand ('ImportAttributesFromProject', dict (parameters, dryRun=True), debug=False))

    print ('Import:')
    PrintResult (aclib.RunTapirCommand ('ImportAttributesFromProject', parameters, debug=False))
