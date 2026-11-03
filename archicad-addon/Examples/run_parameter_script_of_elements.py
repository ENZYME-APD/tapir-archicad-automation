import aclib

# RunParameterScriptOfElements runs the parameter script of the library part of each given
# element, so that parameters computed from other parameters are updated - e.g. after
# SetGDLParametersOfElements changed a parameter that others depend on.

elements = []
for elementType in ['Object', 'Window', 'Door']:
    elements += aclib.RunTapirCommand ('GetElementsByType', {'elementType': elementType})['elements']

if len (elements) == 0:
    print ('No objects, windows or doors in the project.')
else:
    aclib.RunTapirCommand ('RunParameterScriptOfElements', {'elements': elements})
