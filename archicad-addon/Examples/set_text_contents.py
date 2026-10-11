import aclib

# Create a Text element and a standalone text-type Label, then modify their contents
# via SetDetailsOfElements and read them back with GetDetailsOfElements.
# This is the workflow for e.g. translating the texts of an existing design (issue #611).

texts = aclib.RunTapirCommand ('CreateTexts', {
    'textsData': [
        {
            'coordinate': {'x': 0.0, 'y': -5.0, 'z': 0.0},
            'text': 'Original text',
            'height': 2.5
        }
    ]
})['elements']

labels = aclib.RunTapirCommand ('CreateLabels', {
    'labelsData': [
        {
            'begCoordinate': {'x': 0.0, 'y': -8.0},
            'text': 'Original label text'
        }
    ]
})['elements']

elements = texts + labels

aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': elements})

aclib.RunTapirCommand ('SetDetailsOfElements', {
    'elementsWithDetails': [
        {
            'elementId': elements[0]['elementId'],
            'details': {
                'typeSpecificDetails': {
                    'text': 'Modified\nmultiline text',
                    'height': 3.5,
                    'justification': 'Center'
                }
            }
        },
        {
            'elementId': elements[1]['elementId'],
            'details': {
                'typeSpecificDetails': {
                    'text': 'Modified label text'
                }
            }
        }
    ]
})

aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': elements})

# Change only the character height of the Text: the new height is written into every run
# of the content (issue #682 - a bare height change used to be silently ignored, because a
# multistyle Text takes its height from the runs, not from the element-level size field).
aclib.RunTapirCommand ('SetDetailsOfElements', {
    'elementsWithDetails': [
        {
            'elementId': elements[0]['elementId'],
            'details': {
                'typeSpecificDetails': {
                    'height': 5.0
                }
            }
        }
    ]
})

aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': elements})
