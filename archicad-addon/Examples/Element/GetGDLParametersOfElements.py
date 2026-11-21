"""Lists the GDL parameters (name, type, value) of a placed object with GetGDLParametersOfElements.

includePossibleValues false leaves out the value lists of the Parameter Script, which keeps the
answer small; parameterNames (e.g. ['A', 'B']) would return only the named parameters.
Needs at least one Object.
"""
import aclib

objects = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Object'})['elements'][:1]
if not objects:
    raise SystemExit ('No object in the project.')

parameters = aclib.RunTapirCommand ('GetGDLParametersOfElements', {'elements': objects, 'includePossibleValues': False})['gdlParametersOfElements'][0]['parameters']
for parameter in parameters:
    print ('{} ({}): {}'.format (parameter['name'], parameter['type'], parameter['value']))
