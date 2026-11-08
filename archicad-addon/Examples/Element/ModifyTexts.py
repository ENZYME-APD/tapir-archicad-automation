"""Creates a text, then replaces its content, makes it bold and 5 mm high, and moves it with ModifyTexts.
Then it gives the text a fixed width of 20 mm, so it wraps, without changing its content.

Only the given fields change; text replaces the whole content (see ChangeTextContent to change a part).
A text that wraps keeps its wrap width when its content or style is rebuilt; width 0 turns word wrap off.
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

aclib.PrintJson (aclib.RunTapirCommand ('ModifyTexts', {'textsWithDetails': [{
    'elementId': text['elementId'],
    'width': 20.0
}]}))
