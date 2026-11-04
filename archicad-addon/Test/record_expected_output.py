"""Record one script's expected output the way test_examples.py records it, without
reopening the project: python record_expected_output.py <name>.py [...]

test_examples.py runs every script of Test/Scripts/ and every example of Examples/ against a
freshly opened TestProject.pla and writes their output through the same masks (guids, times,
path-like fields): Test/Scripts/<name>.py to ExpectedOutputs/<name>.py.output,
Examples/<GroupFolder>/<name>.py to ExpectedOutputs/Examples/<GroupFolder>/<name>.py.output. This
does the same for the named scripts against whatever project is open, for recording a new golden
without a full suite run. A name is looked up in Test/Scripts/ first, then in the group folders
of Examples/; a path to the script works too."""
import os
import subprocess
import sys
from pathlib import Path

from output_masks import Mask

testFolder = os.path.dirname (os.path.abspath (__file__))
scriptsFolder = os.path.join (testFolder, 'Scripts')
examplesFolder = os.path.normpath (os.path.join (testFolder, '..', 'Examples'))
expectedOutputsFolder = os.path.join (testFolder, 'ExpectedOutputs')

def FindScript (name):
    """(script path, expected output path) of a script name or path, or None."""
    fileName = os.path.basename (name)
    candidates = [os.path.join (scriptsFolder, fileName)]
    candidates += [os.path.join (examplesFolder, groupFolder, fileName) for groupFolder in sorted (os.listdir (examplesFolder)) if groupFolder != 'aclib']
    if os.path.dirname (name):
        candidates.insert (0, os.path.abspath (name))
    for script in candidates:
        if os.path.isfile (script) and fileName != 'aclib.py':
            groupFolder = os.path.normpath (os.path.dirname (script))
            if os.path.dirname (groupFolder) == examplesFolder:
                target = os.path.join (expectedOutputsFolder, 'Examples', os.path.basename (groupFolder), fileName)
            else:
                target = os.path.join (expectedOutputsFolder, fileName)
            return script, target + '.output'
    return None

for name in sys.argv[1:]:
    found = FindScript (name)
    if found is None:
        print ('{}: not found in Test/Scripts or Examples/<GroupFolder>'.format (name))
        continue
    script, target = found
    try:
        raw = subprocess.check_output (['python', script, 'silent'], timeout = 120, cwd = os.path.dirname (script))
    except subprocess.TimeoutExpired:
        raw = b'timeout'
    except subprocess.CalledProcessError as error:
        raw = error.output + b'\nexit code ' + str (error.returncode).encode ()
    output = '\n'.join (raw.decode ('utf-8', errors = 'replace').split ('\r\n'))
    output = Mask (output)
    os.makedirs (os.path.dirname (target), exist_ok = True)
    Path (target).write_text (output)
    print ('{}: {} characters{}'.format (os.path.relpath (target, testFolder), len (output), ', has "error"' if '"error"' in output else ''))
