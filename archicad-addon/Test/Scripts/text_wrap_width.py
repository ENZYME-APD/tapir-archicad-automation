import aclib

# Word wrap of standalone Texts: the optional width of CreateTexts and ModifyTexts (in mm, paper
# size) turns word wrap on (style.nonBreaking false) with that box width (style.boxWidth). The box
# width of an auto-width text and the number of wrapped lines depend on the font metrics of the
# machine running the test, so only the wrap flag, the widths set here, whether the text runs over
# more than one line and the faces of the runs (all set here) are printed.
longText = 'A long line of text that does not fit into a narrow text box'

def PrintTexts (elements):
    details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': elements}, debug = False)
    for detailsOfElement in details['detailsOfElements']:
        d = detailsOfElement['details']
        style = d['style']
        width = 'auto' if style['nonBreaking'] else '{:.2f}'.format (style['boxWidth'])
        print ('{!r} nonBreaking={} width={} multiline={} bold runs={}'.format (
            d['text'], style['nonBreaking'], width, style['lineCount'] > 1, [run['bold'] for run in d['runs']]))

# A wrapped text, an auto-width one (no width, as before) and an auto-width text of two runs.
elements = aclib.RunTapirCommand (
    'CreateTexts', {
        'textsData': [
            {
                'coordinate': {'x': 0.0, 'y': 0.0, 'z': 0.0},
                'text': longText,
                'height': 2.5,
                'justification': 'Left',
                'style': {'bold': False},
                'width': 30.0
            },
            {
                'coordinate': {'x': 0.0, 'y': -5.0, 'z': 0.0},
                'text': longText,
                'height': 2.5,
                'justification': 'Left',
                'style': {'bold': False}
            },
            {
                'coordinate': {'x': 0.0, 'y': -10.0, 'z': 0.0},
                'runs': [
                    {'text': 'Title\n', 'bold': True},
                    {'text': longText, 'bold': False}
                ],
                'style': {'height': 2.5, 'justification': 'Left'}
            }
        ]
    })['elements']
PrintTexts (elements)

# A width alone wraps the text and keeps its content and runs; a new text without a width
# keeps the wrap the text had.
aclib.RunTapirCommand (
    'ModifyTexts', {
        'textsWithDetails': [
            {'elementId': elements[1]['elementId'], 'width': 40.0},
            {'elementId': elements[2]['elementId'], 'width': 50.0},
            {'elementId': elements[0]['elementId'], 'text': 'Replaced'}
        ]
    })
PrintTexts (elements)

# A new text and a width in the same item: the width is applied after the content.
aclib.RunTapirCommand (
    'ModifyTexts', {
        'textsWithDetails': [
            {'elementId': elements[0]['elementId'], 'text': longText, 'width': 25.0}
        ]
    })
PrintTexts (elements[:1])

# A pen change rebuilds the content of the two-run text: it keeps its wrap and its runs. Width 0
# turns word wrap off.
aclib.RunTapirCommand (
    'ModifyTexts', {
        'textsWithDetails': [
            {'elementId': elements[2]['elementId'], 'style': {'penIndex': 3}},
            {'elementId': elements[0]['elementId'], 'width': 0.0}
        ]
    })
PrintTexts ([elements[2], elements[0]])
