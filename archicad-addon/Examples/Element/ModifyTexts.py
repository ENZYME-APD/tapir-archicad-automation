"""Creates a text, then replaces its content, makes it bold and 5 mm high, and moves it with ModifyTexts.

Only the given fields change; text replaces the whole content (see ChangeTextContent to change a part).
"""
import aclib

text = aclib.RunTapirCommand ('CreateTexts', {'textsData': [{
    'coordinate': {'x': 0.0, 'y': 0.0, 'z': 0.0},
    'text': 'Draft'
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyTexts', {'textsWithDetails': [{
    'elementId': text['elementId'],
    'coordinate': {'x': 2.0, 'y': 1.0, 'z': 0.0},
    'text': 'Final',
    'style': {'bold': True, 'height': 5.0}
}]}))
