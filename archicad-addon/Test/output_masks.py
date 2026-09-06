"""The masks test_examples.py and record_expected_output.py apply before an example's output
is compared with its golden: guids, timestamps and path-like fields vary per machine.

The path mask is the one test_examples.py has always used, moved here unchanged so the
recorder and the harness cannot drift: its Windows branch accepts one or two backslashes
per separator, because JSON output doubles them."""
import re

MASKS = [(re.compile(r'[{]?[0-9a-fA-F]{8}-([0-9a-fA-F]{4}-){3}[0-9a-fA-F]{12}[}]?'), '<GUID>'),
         (re.compile(r'Time": [0-9]+'), 'Time": <TIME>'),
         (re.compile(r'"(?P<fieldName>[^"]*(folder|path|directory|location)[^"]*)": "([A-Z]:(\\\\?[^\\"]+)+\\\\?|/?([^/"]+/)+)', re.IGNORECASE), r'"\g<fieldName>": "<PATH>')]


def Mask (output):
    for mask, replacement in MASKS:
        output = mask.sub (replacement, output)
    return output
