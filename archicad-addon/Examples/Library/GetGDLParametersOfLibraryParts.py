"""Lists the default GDL parameters of an object library part, by name, with GetGDLParametersOfLibraryParts.

Nothing is placed and no defaults change. Needs a loaded library with at least one object.
"""
import aclib

objects = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Object'})['libraryParts'][:1]
if not objects:
    raise SystemExit ('No object library part is loaded.')

libraryPartName = objects[0]['documentName']
result = aclib.RunTapirCommand ('GetGDLParametersOfLibraryParts', {
    'libraryParts': [{'libraryPartName': libraryPartName}]
})['gdlParametersOfLibraryParts'][0]

print (libraryPartName)
for parameter in result['parameters']:
    if 'dimension1' in parameter:
        print ('  {} ({}): {} array {} x {}'.format (parameter['name'], parameter['displayName'], parameter['type'], parameter['dimension1'], parameter['dimension2']))
    else:
        print ('  {} ({}): {} = {}'.format (parameter['name'], parameter['displayName'], parameter['type'], parameter.get ('value')))
