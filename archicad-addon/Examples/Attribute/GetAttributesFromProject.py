"""Reads the line types and fills of another project or template file with GetAttributesFromProject, without opening it.

Set the TAPIR_ATTRIBUTE_SOURCE environment variable to the absolute path of a .pln or .tpl file.
Nothing is imported into the open project. Requires Archicad 27 or later.
"""
import os
import aclib

projectFilePath = os.environ.get ('TAPIR_ATTRIBUTE_SOURCE', '')
if not os.path.isfile (projectFilePath):
    raise SystemExit ('Set TAPIR_ATTRIBUTE_SOURCE to the path of a project or template file.')

result = aclib.RunTapirCommand ('GetAttributesFromProject', {
    'projectFilePath': projectFilePath,
    'attributeTypes': ['Line', 'Fill']
})

for typeResult in result['attributesByType']:
    if 'attributes' in typeResult:
        print (typeResult['attributeType'], [attribute['name'] for attribute in typeResult['attributes']])
