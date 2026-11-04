"""Lists the GDL parameters (name, type, value) of a placed object with GetGDLParametersOfElements.

Needs at least one Object.
"""
import aclib

objects = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Object'})['elements'][:1]
if not objects:
    raise SystemExit ('No object in the project.')

parameters = aclib.RunTapirCommand ('GetGDLParametersOfElements', {'elements': objects})['gdlParametersOfElements'][0]['parameters']
for parameter in parameters:
    print ('{} ({}): {}'.format (parameter['name'], parameter['type'], parameter['value']))
