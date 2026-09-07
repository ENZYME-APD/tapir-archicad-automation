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

# Round-trip check: the created elements should be retrievable. Only the fields set above are
# printed - the response also carries Archicad-computed geometry (style.boxWidth/boxHeight), which
# depends on the font metrics of the machine running the test.
elements = result['elements']
details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': elements}, debug = False)
for detailsOfElement in details['detailsOfElements']:
    d = detailsOfElement['details']
    print ('{} {!r} at ({}, {}, {}) height={} angle={} justification={} lines={} paragraphs={}'.format (
        detailsOfElement['type'], d['text'], d['position']['x'], d['position']['y'], d['zCoordinate'],
        d['height'], d['angle'], d['justification'], d['style']['lineCount'], d['paragraphCount']))
