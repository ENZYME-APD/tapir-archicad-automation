import aclib

# GetOpeningSymbolParameters and SetOpeningSymbolParameters need Archicad 29 or newer.
openings = aclib.RunCommand ('API.GetElementsByType', { 'elementType': 'Opening' })['elements']

symbolParametersOfOpenings = aclib.RunTapirCommand ('GetOpeningSymbolParameters', {
        'elements': openings
    })['openings']

# Writes the first Length parameter of each cut symbol back with its value doubled.
elements = []
for opening, symbolParameters in zip (openings, symbolParametersOfOpenings):
    for parameter in symbolParameters.get ('cutSymbolParameters', []):
        if parameter['type'] == 'Length' and 'dimension1' not in parameter:
            elements.append ({
                'elementId': opening['elementId'],
                'cutSymbolParameters': [
                    { 'name': parameter['name'], 'value': parameter['value'] * 2 }
                ]
            })
            break

aclib.RunTapirCommand ('SetOpeningSymbolParameters', {
        'elements': elements
    })

aclib.RunTapirCommand ('GetOpeningSymbolParameters', {
        'elements': openings
    })
