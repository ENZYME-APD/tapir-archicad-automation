import aclib

elements = aclib.RunTapirCommand ('GetAllElements', {})['elements']

# Set all elements to draw index 1 (back of the draw order)
aclib.RunTapirCommand (
    'SetElementsDrawIndex', {
        'elementsWithDrawIndex': [{'elementId': e['elementId'], 'drawIndex': 1} for e in elements]
    })

# Use drawIndex 0 to reset elements to the ArchiCAD default for their type
# aclib.RunTapirCommand (
#     'SetElementsDrawIndex', {
#         'elementsWithDrawIndex': [{'elementId': e['elementId'], 'drawIndex': 0} for e in elements]
#     })
