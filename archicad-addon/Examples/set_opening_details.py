import aclib

# SetOpeningDetails needs Archicad 29 or newer.
openings = aclib.RunCommand ('API.GetElementsByType', { 'elementType': 'Opening' })['elements']

# Makes every opening 0.1 wider, cuts it through its whole parent element and moves it by 0.5 along X.
openingDetails = aclib.RunTapirCommand ('GetOpeningDetails', {
        'elements': openings
    })['openings']

openingsWithDetails = []
for opening in openingDetails:
    if 'elementId' not in opening or 'width' not in opening['extrusionParameters']:
        continue
    openingsWithDetails.append ({
        'elementId': opening['elementId'],
        'extrusionParameters': {
            'limitType': 'Infinite',
            'width': opening['extrusionParameters']['width'] + 0.1
        },
        'transformation': [
            1.0, 0.0, 0.0, 0.5,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0
        ]
    })

aclib.RunTapirCommand ('SetOpeningDetails', {
        'openingsWithDetails': openingsWithDetails
    })

aclib.RunTapirCommand ('GetOpeningDetails', {
        'elements': openings
    })
