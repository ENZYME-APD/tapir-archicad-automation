"""Record one example's expected output the way test_examples.py records it, without
reopening the project: python record_expected_output.py <example>.py [...]

test_examples.py runs every example against a freshly opened TestProject.pla and writes
ExpectedOutputs/<name>.output through the same masks (guids, times, path-like fields). This
does the same for the named examples against whatever project is open, for recording a new
example's golden without a full suite run."""
import os
import re
import subprocess
import sys
from pathlib import Path

MASKS = [(re.compile(r'[{]?[0-9a-fA-F]{8}-([0-9a-fA-F]{4}-){3}[0-9a-fA-F]{12}[}]?'), '<GUID>'),
         (re.compile(r'Time": [0-9]+'), 'Time": <TIME>'),
         (re.compile(r'"(?P<fieldName>[^"]*(folder|path|directory|location)[^"]*)": "([A-Z]:(\\\\?[^\\"]+)+\\\\?|/?([^/"]+/)+)', re.IGNORECASE), '"\\g<fieldName>": "<PATH>')]

examplesFolder = os.path.join (os.path.dirname (os.path.abspath (__file__)), '..', 'Examples')
for name in sys.argv[1:]:
    script = os.path.join (examplesFolder, name)
    try:
        raw = subprocess.check_output (['python', script, 'silent'], timeout = 120, cwd = examplesFolder)
    except subprocess.TimeoutExpired:
        raw = b'timeout'
    output = '\n'.join (raw.decode ('utf-8').split ('\r\n'))
    for mask, replacement in MASKS:
        output = mask.sub (replacement, output)
    target = Path (os.path.dirname (os.path.abspath (__file__))) / 'ExpectedOutputs' / (name + '.output')
    target.write_text (output)
    print ('{}: {} characters{}'.format (name, len (output), ', has "error"' if '"error"' in output else ''))
