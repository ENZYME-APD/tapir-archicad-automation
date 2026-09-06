import subprocess, os, time, re
import output_masks
from pathlib import Path
from archicad import ACConnection

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

def GetExampleScripts ():
    result = []
    quitExampleScriptFullPath = None
    examplesFolderPath = os.path.join (os.path.dirname (__file__), '..', 'Examples')
    for exampleFileName in os.listdir (examplesFolderPath):
        fileExtension = os.path.splitext (exampleFileName)[1]
        if fileExtension.lower () == '.py':
            scriptFullPath = os.path.join (examplesFolderPath, exampleFileName)
            if 'quit_archicad' in exampleFileName:
                quitExampleScriptFullPath = scriptFullPath
                continue
            result.append (scriptFullPath)
    result.append (quitExampleScriptFullPath)
    return result

def OpenProject (projectFilePath):
    ExecuteTapirCommand ('OpenProject', {'projectFilePath': projectFilePath})
    return ACConnection.connect ()

def ExecuteScript (exampleScriptFilePath):
    try:
        return subprocess.check_output (['python', exampleScriptFilePath, 'silent'], timeout=60)
    except subprocess.TimeoutExpired:
        return b'timeout'

def CompareOutput (bynaryOutput, expectedOutputFilePath):
    output = '\n'.join (bynaryOutput.decode ('utf-8').split ('\r\n'))
    output = output_masks.Mask (output)

    expectedOutputFilePath = os.path.join (os.path.dirname (__file__), 'ExpectedOutputs', testName + '.output')
    isPassed = True
    if os.path.isfile (expectedOutputFilePath):
        expectedOutput = Path (expectedOutputFilePath).read_text ()
        isPassed = output == expectedOutput
    Path (expectedOutputFilePath).write_text (output)

    return isPassed


projectName = 'TestProject.pla'
passedTests = []
failedTests = []
exampleScripts = GetExampleScripts ()
print ('Found {} tests'.format (len (exampleScripts)))
for i in range (len (exampleScripts)):
    exampleScriptFilePath = exampleScripts[i]
    testName = os.path.basename (exampleScriptFilePath)
    print ('{}/{} {}'.format (i+1, len (exampleScripts), testName))

    MeasureExecutionTime (
        'Reopening project {}'.format (projectName),
        OpenProject, os.path.join (os.path.dirname (__file__), projectName))
    output = MeasureExecutionTime (
        'Executing script {}'.format (testName),
        ExecuteScript, exampleScriptFilePath)
    isPassed = MeasureExecutionTime (
        'Comparing output',
        CompareOutput, output, os.path.join (os.path.dirname (__file__), 'ExpectedOutputs', testName + '.output'))
    
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