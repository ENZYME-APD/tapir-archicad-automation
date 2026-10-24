import aclib

# GetOpeningDetails needs Archicad 29 or newer.
openings = aclib.RunCommand ('API.GetElementsByType', { 'elementType': 'Opening' })['elements']

openingDetails = aclib.RunTapirCommand ('GetOpeningDetails', {
        'elements': openings
    })['openings']
