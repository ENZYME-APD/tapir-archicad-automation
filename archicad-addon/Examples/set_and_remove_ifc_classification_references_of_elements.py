import aclib

# SetIFCClassificationReferencesOfElements and RemoveIFCClassificationReferencesOfElements
# are available only in Archicad 25, 26 and 27 for now, in newer versions they
# report a not supported error.

elements = aclib.RunCommand ('API.GetAllElements', {})['elements']
element = elements[0]

aclib.RunTapirCommand ('SetIFCClassificationReferencesOfElements', {
        'elementIFCClassificationReferences': [{
            'elementId': element['elementId'],
            'identification': 'Ss_25_10_30',
            'name': 'Curtain wall systems',
            'location': 'https://uniclass.thenbs.com/',
            'referencedSource': {
                'name': 'Uniclass 2015',
                'edition': 'v1.30'
            }
        }]
    })

aclib.RunTapirCommand ('GetIFCClassificationReferencesOfElements', {
        'elements': [element],
        'storedOnly': True
    })

# Without identification every stored reference of the classification would be removed
aclib.RunTapirCommand ('RemoveIFCClassificationReferencesOfElements', {
        'elementIFCClassificationReferences': [{
            'elementId': element['elementId'],
            'referencedSource': {
                'name': 'Uniclass 2015'
            },
            'identification': 'Ss_25_10_30'
        }]
    })

aclib.RunTapirCommand ('GetIFCClassificationReferencesOfElements', {
        'elements': [element],
        'storedOnly': True
    })
