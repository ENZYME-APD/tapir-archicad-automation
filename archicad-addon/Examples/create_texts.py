import aclib

# Place a few standalone Text elements, then read them back to confirm.
result = aclib.RunTapirCommand (
    'CreateTexts', {
        'textsData': [
            {
                'coordinate': {'x': 0.0, 'y': 0.0, 'z': 0.0},
                'text': '100',
                'height': 2.5
            },
            {
                'coordinate': {'x': 5.0, 'y': 2.0, 'z': 0.0},
                'text': '102',
                'height': 2.5,
                'justification': 'Center'
            },
            {
                'coordinate': {'x': 10.0, 'y': 4.0, 'z': 0.0},
                'text': 'multi\nline',
                'height': 2.5,
                'angle': 0.0
            }
        ]
    })

# Round-trip check: the created elements should be retrievable.
elements = result['elements']
details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': elements})
print(details)
