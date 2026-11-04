"""Creates the text 'Hello World', then replaces 'World' with 'Tapir' with ChangeTextContent.

from is the character position, range the number of characters replaced (0 inserts). The styles of
the rest of the content and the autotexts in it are kept. A range must stay within one line.
"""
import aclib

text = aclib.RunTapirCommand ('CreateTexts', {'textsData': [{
    'coordinate': {'x': 0.0, 'y': 0.0, 'z': 0.0},
    'text': 'Hello World'
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ChangeTextContent', {'contentChanges': [{
    'elementId': text['elementId'],
    'from': len ('Hello '),
    'range': len ('World'),
    'content': 'Tapir'
}]}))
