import aclib

# Create a Text element, then change its content, style and position with ModifyTexts and read
# the result back. ModifyTexts changes only the fields it is given; SetDetailsOfElements covers
# the plain fields (text, position, angle, height, justification), ModifyTexts adds the full
# style (font, effects, frame, anchor, ...) and multi-run content.
elements = aclib.RunTapirCommand ('CreateTexts', {
    'textsData': [
        {
            'coordinate': {'x': 0.0, 'y': -20.0, 'z': 0.0},
            'text': 'Original text',
            'height': 2.5
        }
    ]
})['elements']

aclib.RunTapirCommand ('ModifyTexts', {
    'textsWithDetails': [
        {
            'elementId': elements[0]['elementId'],
            'coordinate': {'x': 1.0, 'y': -21.0, 'z': 0.0},
            'text': 'Modified text',
            'style': {
                'penIndex': 9,
                'bold': True,
                'anchor': 'MiddleMiddle',
                'usedContour': True,
                'contourPenIndex': 7
            }
        }
    ]
})

# Multi-run content: each run carries its own pen/face/font/size, concatenated in order.
aclib.RunTapirCommand ('ModifyTexts', {
    'textsWithDetails': [
        {
            'elementId': elements[0]['elementId'],
            'runs': [
                {'text': 'Bold ', 'bold': True, 'penIndex': 1},
                {'text': 'and plain', 'bold': False, 'penIndex': 3}
            ]
        }
    ]
})

# A style-only change keeps the styled runs as they are.
aclib.RunTapirCommand ('ModifyTexts', {
    'textsWithDetails': [
        {
            'elementId': elements[0]['elementId'],
            'style': {'angle': 0.5}
        }
    ]
})

details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': elements}, debug = False)
d = details['detailsOfElements'][0]['details']
print ('text={!r} position=({}, {}) angle={} anchor={} usedContour={} contourPen={}'.format (
    d['text'], d['position']['x'], d['position']['y'], d['angle'], d['style']['anchor'],
    d['style']['usedContour'], d['style']['contourPenIndex']))
for run in d.get ('runs', []):
    print ('run {!r} bold={} pen={}'.format (run['text'], run['bold'], run['penIndex']))

aclib.RunTapirCommand ('DeleteElements', {'elements': elements})
