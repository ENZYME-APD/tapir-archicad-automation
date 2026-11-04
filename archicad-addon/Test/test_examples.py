"""Runs the test scripts and the examples against TestProject.pla and compares their output
with the recorded one.

- Test/Scripts/*.py are the regression tests: the output of <name>.py is compared with
  ExpectedOutputs/<name>.output.
- Examples/*.py (one per command) run as a smoke test: the output of <CommandName>.py is
  compared with ExpectedOutputs/Examples/<CommandName>.py.output.

A script without a recorded output passes and its output is recorded; a failing one gets its
output written over the recorded one, so `git diff` shows what changed. Test/Scripts/manual/
is not run (those scripts need a person). The project is reopened before every script. The
script that quits Archicad (quit_archicad.py / QuitArchicad.py) runs last, only once; the ones
that start another Archicad are not run.

Usage: python test_examples.py [<name> ...]   (only the scripts with these file names)"""
import os, subprocess, sys, time
import output_masks
from pathlib import Path
from archicad import ACConnection

testFolder = os.path.dirname (os.path.abspath (__file__))
scriptsFolder = os.path.join (testFolder, 'Scripts')
examplesFolder = os.path.join (testFolder, '..', 'Examples')
expectedOutputsFolder = os.path.join (testFolder, 'ExpectedOutputs')

def IsQuitScript (fileName):
    return fileName.lower ().replace ('_', '').startswith ('quitarchicad')

def IsStartScript (fileName):
    return fileName.lower ().replace ('_', '').startswith ('startarchicad')

def MeasureExecutionTime (name, function, *args):
    print ('{} ... '.format (name), end='', flush=True)
    start = time.time ()
    result = function (*args)
    end = time.time ()
    print ('{0:.3f}s'.format (end - start))
    return result

def ExecuteTapirCommand (commandName, inputParameters = None):
    acConnection = ACConnection.connect ()
    if not acConnection:
        print ('Start Archicad before testing')
        exit ()
    command = acConnection.types.AddOnCommandId ('TapirCommand', commandName)
    return acConnection.commands.ExecuteAddOnCommand (command, inputParameters)

def ListScripts (folderPath, expectedOutputsFolderPath):
    """(script path, expected output path) of every .py file of the folder, aclib.py excluded."""
    if not os.path.isdir (folderPath):
        return []
    return [(os.path.join (folderPath, fileName), os.path.join (expectedOutputsFolderPath, fileName + '.output'))
            for fileName in sorted (os.listdir (folderPath))
            if fileName.lower ().endswith ('.py') and fileName != 'aclib.py']

def GetTests (onlyNames):
    tests = ListScripts (scriptsFolder, expectedOutputsFolder) + \
            ListScripts (examplesFolder, os.path.join (expectedOutputsFolder, 'Examples'))
    if onlyNames:
        tests = [test for test in tests if os.path.basename (test[0]) in onlyNames]
    result = []
    quitTest = None
    for test in tests:
        fileName = os.path.basename (test[0])
        if IsQuitScript (fileName):
            # Only one of them can run: Archicad is gone afterwards. The regression test wins.
            if quitTest is None:
                quitTest = test
            continue
        # Starts another Archicad, so it is not run against the test project.
        if IsStartScript (fileName):
            continue
        result.append (test)
    if quitTest is not None:
        result.append (quitTest)
    return result

def OpenProject (projectFilePath):
    ExecuteTapirCommand ('OpenProject', {'projectFilePath': projectFilePath})
    return ACConnection.connect ()

def ExecuteScript (scriptFilePath):
    try:
        return subprocess.check_output (['python', scriptFilePath, 'silent'], timeout=60)
    except subprocess.TimeoutExpired:
        return b'timeout'
    except subprocess.CalledProcessError as error:
        return error.output + b'\nexit code ' + str (error.returncode).encode ()

def CompareOutput (binaryOutput, expectedOutputFilePath):
    output = '\n'.join (binaryOutput.decode ('utf-8').split ('\r\n'))
    output = output_masks.Mask (output)

    isPassed = True
    if os.path.isfile (expectedOutputFilePath):
        expectedOutput = Path (expectedOutputFilePath).read_text ()
        isPassed = output == expectedOutput
    os.makedirs (os.path.dirname (expectedOutputFilePath), exist_ok=True)
    Path (expectedOutputFilePath).write_text (output)

    return isPassed


projectName = 'TestProject.pla'
passedTests = []
failedTests = []
tests = GetTests (set (sys.argv[1:]))
print ('Found {} tests'.format (len (tests)))
for i, (scriptFilePath, expectedOutputFilePath) in enumerate (tests):
    testName = os.path.relpath (scriptFilePath, os.path.join (testFolder, '..'))
    print ('{}/{} {}'.format (i+1, len (tests), testName))

    MeasureExecutionTime (
        'Reopening project {}'.format (projectName),
        OpenProject, os.path.join (testFolder, projectName))
    output = MeasureExecutionTime (
        'Executing script {}'.format (testName),
        ExecuteScript, scriptFilePath)
    isPassed = MeasureExecutionTime (
        'Comparing output',
        CompareOutput, output, expectedOutputFilePath)

    if isPassed:
        passedTests.append (testName)
        print ('PASSED')
    else:
        failedTests.append (testName)
        print ('FAILED')

if failedTests:
    print ('{}/{} test FAILED'.format (len (failedTests), len (failedTests) + len (passedTests)))
    print ('Failed tests:\n' + '\n'.join (failedTests))
else:
    print ('All ({}) tests PASSED'.format (len (passedTests)))
