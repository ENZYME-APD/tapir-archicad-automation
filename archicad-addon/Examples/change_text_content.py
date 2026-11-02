import aclib

# ChangeTextContent replaces a range of a Text's (or a text Label's) content in place: unlike
# ModifyTexts, which rebuilds the whole content, the other runs keep their styles and any autotext
# in them stays live. A range must stay inside one line and the new piece must not contain line ends.
elements = aclib.RunTapirCommand ('CreateTexts', {
    'textsData': [
        {
            'coordinate': {'x': 0.0, 'y': -30.0, 'z': 0.0},
            'text': 'First line\nSecond line\n<PROJECTNAME>',
            'height': 2.5
        }
    ]
})['elements']

def GetText ():
    details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': elements}, debug = False)
    return details['detailsOfElements'][0]['details']

# "text" shows the autotext's value, "rawText" its key.
d = GetText ()
print ('text={!r} rawText={!r}'.format (d['text'], d.get ('rawText')))

# Replace "Second" with "2nd": "from" counts the characters of the content, line ends included.
secondLineStart = len ('First line\n')
aclib.RunTapirCommand ('ChangeTextContent', {
    'contentChanges': [
        {
            'elementId': elements[0]['elementId'],
            'from': secondLineStart,
            'range': len ('Second'),
            'content': '2nd'
        }
    ]
})

# A range reaching over a line end is refused.
overLineEndResult = aclib.RunTapirCommand ('ChangeTextContent', {
    'contentChanges': [
        {
            'elementId': elements[0]['elementId'],
            'from': 0,
            'range': len ('First line\n2nd'),
            'content': 'One line'
        }
    ]
}, debug = False)
print ('range over a line end: success={}'.format (overLineEndResult['executionResults'][0]['success']))

d = GetText ()
print ('text={!r} rawText={!r}'.format (d['text'], d.get ('rawText')))

aclib.RunTapirCommand ('DeleteElements', {'elements': elements})
