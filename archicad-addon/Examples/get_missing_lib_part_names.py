import aclib

# Placed objects, lamps, windows and doors whose library part is missing are shown as dots.
# GetMissingLibPartNames reports the name of their library part, also when the library
# containing it is not loaded, so the right part can be found and loaded again.
elements = []
for elementType in ['Object', 'Lamp', 'Window', 'Door']:
    elements += aclib.RunCommand ('API.GetElementsByType', { 'elementType': elementType })['elements']

response = aclib.RunTapirCommand ('GetMissingLibPartNames', { 'elements': elements })
