"""Creates a plain text and a two-line text with a bold first line with CreateTexts.

height is the character height in millimeters (paper size).
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
    }
]}))
