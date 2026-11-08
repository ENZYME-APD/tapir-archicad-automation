"""Creates a plain text, a two-line text with a bold first line and a text that wraps within 30 mm with CreateTexts.

height is the character height and width the box width of a wrapped text, both in millimeters (paper size).
Without a width the text is auto-width: it grows with its content.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateTexts', {'textsData': [
    {
        'coordinate': {'x': 0.0, 'y': 0.0, 'z': 0.0},
        'text': 'Created by Tapir',
        'height': 3.0
    },
    {
        'coordinate': {'x': 0.0, 'y': -2.0, 'z': 0.0},
        'runs': [
            {'text': 'Title\n', 'bold': True},
            {'text': 'Second line'}
        ],
        'style': {'height': 2.5, 'justification': 'Left'}
    },
    {
        'coordinate': {'x': 0.0, 'y': -5.0, 'z': 0.0},
        'text': 'A longer text that wraps within the 30 mm wide text box',
        'height': 2.5,
        'width': 30.0
    }
]}))
