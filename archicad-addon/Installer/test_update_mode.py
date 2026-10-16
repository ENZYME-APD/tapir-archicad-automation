"""Tests of the update mode of the Tapir Installer (--addOnFile).

Standard library only, and offline: the latest release is replaced by local
files, and Archicad by a small HTTP server that answers the Tapir commands
the update uses. On macOS the add-on bundle logic is tested, on the other
platforms the Windows add-on file logic. The update lock is tested against the
one of the legacy update script (../Tools/update_addon_and_restart_archicad.py).

Run: python test_update_mode.py
"""

import contextlib
import ctypes
import ctypes.wintypes
import errno
import http.server
import importlib
import importlib.util
import io
import itertools
import json
import os
import pathlib
import shlex
import shutil
import socket
import socketserver
import subprocess
import sys
import tempfile
import threading
import time
import types
import unittest
import unittest.mock

sys.path.insert (0, os.path.dirname (os.path.abspath (__file__)))
import tapir_installer

ARCHICAD_VERSION = 29
RELEASE_TAG = '9.9.9'
ARCHICAD_LOCATION = os.path.join (os.sep, 'Mock', 'Archicad')
IS_MAC = tapir_installer.IsUsingMacOS ()
LEGACY_UPDATE_SCRIPT_PATH = os.path.join (os.path.dirname (os.path.dirname (os.path.abspath (__file__))), 'Tools', 'update_addon_and_restart_archicad.py')
# The installer checks that a downloaded .apx starts like a DLL.
OLD_APX_CONTENT = 'MZ old'
NEW_APX_CONTENT = 'MZ new'


class MockArchicadServer (http.server.ThreadingHTTPServer):
    def server_bind (self):
        # HTTPServer.server_bind would look up the host name, which can take
        # seconds on Windows.
        socketserver.TCPServer.server_bind (self)
        self.server_name = '127.0.0.1'
        self.server_port = self.server_address[1]


class MockArchicad:
    # Answers the Tapir commands of the update like Archicad does. After
    # QuitArchicad it stops listening (after quitDelaySeconds), unless it is
    # told not to quit, or it quits before answering (quitsBeforeAnswering).
    # quitResponseJson replaces the answer to QuitArchicad, which comes after
    # quitAnswerDelaySeconds, like once the user answered the save prompt.
    # isBusyUntilQuit: like while Archicad asks whether to save the changes,
    # requests from QuitArchicad on get no answer until it quit; with
    # isQuitAnsweredFirst, QuitArchicad itself is answered.
    def __init__ (self, quits = True, quitsBeforeAnswering = False, quitResponseJson = None, quitDelaySeconds = 0.3, isBusyUntilQuit = False,
            isQuitAnsweredFirst = False, quitAnswerDelaySeconds = 0):
        self.quits = quits
        self.quitsBeforeAnswering = quitsBeforeAnswering
        self.quitResponseJson = quitResponseJson
        self.quitDelaySeconds = quitDelaySeconds
        self.isBusyUntilQuit = isBusyUntilQuit
        self.isQuitAnsweredFirst = isQuitAnsweredFirst
        self.quitAnswerDelaySeconds = quitAnswerDelaySeconds
        self.isQuitting = False
        self.receivedCommands = []
        self.stopLock = threading.Lock ()
        self.isStopped = False
        self.stoppedEvent = threading.Event ()

        class RequestHandler (http.server.BaseHTTPRequestHandler):
            def do_POST (self):
                requestJson = json.loads (self.rfile.read (int (self.headers['Content-Length'])))
                self.server.mockArchicad.HandleCommand (self, requestJson['parameters']['addOnCommandId']['commandName'])

            def log_message (self, format, *args):
                pass

        self.server = MockArchicadServer (('127.0.0.1', 0), RequestHandler)
        self.server.mockArchicad = self
        self.port = self.server.server_address[1]
        threading.Thread (target = self.server.serve_forever, daemon = True).start ()

    def HandleCommand (self, requestHandler, commandName):
        self.receivedCommands.append (commandName)
        if commandName == 'QuitArchicad' and self.quits:
            if self.quitsBeforeAnswering:
                # The port is closed before the request is dropped.
                self.Stop ()
                return
            self.isQuitting = True
            # A daemon, so that a late quit does not keep the tests running.
            quitTimer = threading.Timer (self.quitDelaySeconds, self.Stop)
            quitTimer.daemon = True
            quitTimer.start ()
        if self.isBusyUntilQuit and self.isQuitting and not (self.isQuitAnsweredFirst and commandName == 'QuitArchicad'):
            # The request is dropped when Archicad quits.
            self.stoppedEvent.wait ()
            return
        if commandName == 'QuitArchicad' and self.stoppedEvent.wait (self.quitAnswerDelaySeconds):
            return
        responses = {
            'GetArchicadLocation' : { 'archicadLocation' : ARCHICAD_LOCATION },
            'QuitArchicad' : { 'success' : True },
            'GetAddOnVersion' : { 'version' : '1.0.0' },
        }
        responseJson = { 'succeeded' : True, 'result' : { 'addOnCommandResponse' : responses.get (commandName, {}) } }
        if commandName == 'QuitArchicad' and self.quitResponseJson is not None:
            responseJson = self.quitResponseJson
        responseData = json.dumps (responseJson).encode ('utf-8')
        requestHandler.send_response (200)
        requestHandler.send_header ('Content-Type', 'application/json')
        requestHandler.send_header ('Content-Length', str (len (responseData)))
        requestHandler.end_headers ()
        requestHandler.wfile.write (responseData)

    def Stop (self):
        with self.stopLock:
            if self.isStopped:
                return
            self.isStopped = True
        self.server.shutdown ()
        # Releases the held requests: server_close waits for them.
        self.stoppedEvent.set ()
        self.server.server_close ()


def WriteFiles (folderPath, files):
    for relativePath, content in files.items ():
        filePath = os.path.join (folderPath, *relativePath.split ('/'))
        os.makedirs (os.path.dirname (filePath), exist_ok = True)
        with open (filePath, 'w') as file:
            file.write (content)


def ReadFile (filePath):
    with open (filePath) as file:
        return file.read ()


def ReadBinaryFile (filePath):
    with open (filePath, 'rb') as file:
        return file.read ()


class UpdateModeTestBase (unittest.TestCase):
    def setUp (self):
        self.tempFolderPath = tempfile.mkdtemp (prefix = 'TapirUpdateTest_')
        self.addCleanup (shutil.rmtree, self.tempFolderPath, True)
        # Short timeouts, so that the failure cases take seconds, not minutes.
        self.Patch (tapir_installer, 'ARCHICAD_QUIT_TIMEOUT_SECONDS', 2)
        self.Patch (tapir_installer, 'ARCHICAD_PROCESS_EXIT_TIMEOUT_SECONDS', 10)
        self.Patch (tapir_installer, 'ARCHICAD_POLL_INTERVAL_SECONDS', 0.1)
        self.Patch (tapir_installer, 'ARCHICAD_ANSWER_TIMEOUT_SECONDS', 0.3)
        self.Patch (tapir_installer, 'REPLACE_RETRY_TIMEOUT_SECONDS', 3)
        self.Patch (tapir_installer, 'UPDATE_LOCK_WAIT_SECONDS', 0.5)
        # The update lock files, and the downloads, go to the test folder.
        self.Patch (tempfile, 'tempdir', self.tempFolderPath)
        self.startedArchicadLocations = []
        self.Patch (tapir_installer, 'StartArchicad', self.startedArchicadLocations.append)
        self.releaseInfoRequestCount = 0
        self.Patch (tapir_installer, 'GetLatestReleaseInfo', self.GetReleaseInfo)
        # Tests must never show the macOS administrator password dialog.
        self.Patch (tapir_installer, 'RunShellCommandWithAdminPrivilegesMac', unittest.mock.Mock (side_effect = AssertionError ('administrator privileges requested')))

        # A space in the folder name checks the quoting of the paths.
        self.addOnsFolderPath = os.path.join (self.tempFolderPath, 'Add-Ons', 'Tapir Folder')
        releaseFolderPath = os.path.join (self.tempFolderPath, 'Release')
        os.makedirs (self.addOnsFolderPath)
        os.makedirs (releaseFolderPath)
        if IS_MAC:
            self.targetPath = os.path.join (self.addOnsFolderPath, 'TapirAddOn_AC29_Mac.bundle')
            WriteFiles (self.targetPath, { 'Contents/Info.plist' : 'old', 'Contents/Resources/Removed.txt' : 'old' })
            bundleFolderPath = os.path.join (self.tempFolderPath, 'Build', 'TapirAddOn_AC29_Mac.bundle')
            WriteFiles (bundleFolderPath, { 'Contents/Info.plist' : 'new', 'Contents/MacOS/TapirAddOn' : 'new' })
            self.assetPath = os.path.join (releaseFolderPath, 'TapirAddOn_AC29_Mac.zip')
            # The same zip layout as the release workflow builds.
            subprocess.run (['ditto', '-c', '-k', '--keepParent', bundleFolderPath, self.assetPath], check = True)
        else:
            self.targetPath = os.path.join (self.addOnsFolderPath, 'TapirAddOn_AC29_Win.apx')
            WriteFiles (self.addOnsFolderPath, { 'TapirAddOn_AC29_Win.apx' : OLD_APX_CONTENT })
            self.assetPath = os.path.join (releaseFolderPath, 'TapirAddOn_AC29_Win.apx')
            WriteFiles (releaseFolderPath, { 'TapirAddOn_AC29_Win.apx' : NEW_APX_CONTENT })
        # The real DownloadAsset downloads the asset from a file URL. The
        # size is listed like in the release info of the GitHub API.
        self.releaseInfo = {
            'tag_name' : RELEASE_TAG,
            'assets' : [{ 'name' : os.path.basename (self.assetPath), 'browser_download_url' : pathlib.Path (self.assetPath).as_uri (),
                'size' : os.path.getsize (self.assetPath) }]
        }

    def Patch (self, target, attributeName, value):
        patcher = unittest.mock.patch.object (target, attributeName, value)
        patcher.start ()
        self.addCleanup (patcher.stop)

    def GetReleaseInfo (self):
        self.releaseInfoRequestCount += 1
        return self.releaseInfo

    def StartMockArchicad (self, **mockArguments):
        mockArchicad = MockArchicad (**mockArguments)
        self.addCleanup (mockArchicad.Stop)
        return mockArchicad

    def ServeDownload (self, content, contentLength):
        # Announces contentLength bytes, but sends only the content, then
        # closes the connection like a dropped download.
        class RequestHandler (http.server.BaseHTTPRequestHandler):
            def do_GET (self):
                self.send_response (200)
                self.send_header ('Content-Type', 'application/octet-stream')
                self.send_header ('Content-Length', str (contentLength))
                self.end_headers ()
                self.wfile.write (content)

            def log_message (self, format, *args):
                pass

        server = MockArchicadServer (('127.0.0.1', 0), RequestHandler)
        threading.Thread (target = server.serve_forever, daemon = True).start ()
        self.addCleanup (server.server_close)
        self.addCleanup (server.shutdown)
        # The download must not go through a proxy of the environment.
        patcher = unittest.mock.patch.dict (os.environ, { 'no_proxy' : '*', 'NO_PROXY' : '*' })
        patcher.start ()
        self.addCleanup (patcher.stop)
        return 'http://127.0.0.1:{0}/{1}'.format (server.server_address[1], self.releaseInfo['assets'][0]['name'])

    def StartProcess (self, seconds):
        process = subprocess.Popen ([sys.executable, '-c', 'import time; time.sleep ({0})'.format (seconds)])
        # Reaps the process once it exits: an exited but not reaped child
        # process still exists on POSIX.
        threading.Thread (target = process.wait, daemon = True).start ()
        self.addCleanup (self.StopProcess, process)
        return process

    @staticmethod
    def StopProcess (process):
        if process.poll () is None:
            process.kill ()
        process.wait ()

    def CreateCancelEvent (self, cancelAfterSeconds = None):
        # The GUI's Cancel button, optionally clicked after the given time.
        cancelEvent = threading.Event ()
        if cancelAfterSeconds is not None:
            cancelTimer = threading.Timer (cancelAfterSeconds, cancelEvent.set)
            cancelTimer.daemon = True
            cancelTimer.start ()
            self.addCleanup (cancelTimer.cancel)
        return cancelEvent

    def CreateUpdateTarget (self, archicadPort = None, archicadPid = None, restartArchicad = False):
        return tapir_installer.AddOnUpdateTarget (ARCHICAD_VERSION, self.targetPath, archicadPort, archicadPid, restartArchicad)

    def RunMain (self, arguments):
        stdout = io.StringIO ()
        stderr = io.StringIO ()
        with unittest.mock.patch.object (sys, 'argv', ['tapir_installer.py'] + arguments), contextlib.redirect_stdout (stdout), contextlib.redirect_stderr (stderr):
            try:
                exitCode = tapir_installer.Main ()
            except SystemExit as e:
                exitCode = e.code
        return exitCode, stdout.getvalue (), stderr.getvalue ()

    def AssertAddOnIsNew (self):
        if IS_MAC:
            self.assertEqual (ReadFile (os.path.join (self.targetPath, 'Contents', 'Info.plist')), 'new')
            self.assertTrue (os.path.isfile (os.path.join (self.targetPath, 'Contents', 'MacOS', 'TapirAddOn')))
            # Replaced, not merged.
            self.assertFalse (os.path.exists (os.path.join (self.targetPath, 'Contents', 'Resources', 'Removed.txt')))
        else:
            self.assertEqual (ReadFile (self.targetPath), NEW_APX_CONTENT)
        self.AssertNoLeftovers ()

    def AssertAddOnIsOld (self):
        if IS_MAC:
            self.assertEqual (ReadFile (os.path.join (self.targetPath, 'Contents', 'Info.plist')), 'old')
            self.assertTrue (os.path.isfile (os.path.join (self.targetPath, 'Contents', 'Resources', 'Removed.txt')))
        else:
            self.assertEqual (ReadFile (self.targetPath), OLD_APX_CONTENT)
        self.AssertNoLeftovers ()

    def AssertNoLeftovers (self):
        self.assertEqual (os.listdir (self.addOnsFolderPath), [os.path.basename (self.targetPath)])


class UpdateTests (UpdateModeTestBase):
    def testUpdateQuitsArchicadAndReplacesAddOn (self):
        mockArchicad = self.StartMockArchicad ()
        exitCode, stdout, stderr = self.RunMain (['--console', '--addOnFile', self.targetPath, '--versions', str (ARCHICAD_VERSION),
            '--archicadPort', str (mockArchicad.port), '--noRestart'])
        self.assertEqual (exitCode, 0, stderr)
        self.assertIn ('Closing Archicad...', stdout)
        self.assertIn ('Tapir was updated to {0}.'.format (RELEASE_TAG), stdout)
        self.assertEqual (mockArchicad.receivedCommands[:2], ['GetArchicadLocation', 'QuitArchicad'])
        self.assertTrue (mockArchicad.isStopped)
        self.assertEqual (self.startedArchicadLocations, [])
        self.AssertAddOnIsNew ()

    def testUpdateRestartsArchicad (self):
        mockArchicad = self.StartMockArchicad ()
        statusTexts = []
        resultText = tapir_installer.UpdateAddOn (self.CreateUpdateTarget (mockArchicad.port, restartArchicad = True), self.releaseInfo, False, statusTexts.append)
        self.assertEqual (resultText, 'Tapir was updated to {0}. Archicad is starting again.'.format (RELEASE_TAG))
        self.assertEqual (statusTexts, ['Closing Archicad...', 'Installing...', 'Restarting Archicad...'])
        self.assertEqual (self.startedArchicadLocations, [ARCHICAD_LOCATION])
        self.AssertAddOnIsNew ()

    def testUpdateWithoutArchicadPort (self):
        resultText = tapir_installer.UpdateAddOn (self.CreateUpdateTarget (restartArchicad = True), self.releaseInfo, False)
        self.assertEqual (resultText, 'Tapir was updated to {0}.'.format (RELEASE_TAG))
        self.assertEqual (self.startedArchicadLocations, [])
        self.AssertAddOnIsNew ()

    def testReplaceFailureStillRestartsArchicad (self):
        mockArchicad = self.StartMockArchicad ()
        self.Patch (tapir_installer, 'ReplaceAddOn', unittest.mock.Mock (side_effect = tapir_installer.InstallerError ('Replace failed.')))
        updateTarget = self.CreateUpdateTarget (mockArchicad.port, restartArchicad = True)
        with self.assertRaises (tapir_installer.InstallerError) as context:
            tapir_installer.UpdateAddOn (updateTarget, self.releaseInfo, False)
        self.assertEqual (str (context.exception), 'Replace failed. Archicad was started again. Start the update again from Archicad.')
        self.assertEqual (self.startedArchicadLocations, [ARCHICAD_LOCATION])
        # The GUI does not offer to update again with this target: its port
        # and process id belong to the Archicad that quit.
        self.assertTrue (updateTarget.archicadWasQuit)
        self.AssertAddOnIsOld ()

    def testRestartFailureIsReported (self):
        mockArchicad = self.StartMockArchicad ()
        self.Patch (tapir_installer, 'StartArchicad', unittest.mock.Mock (side_effect = OSError ('not found')))
        with self.assertRaises (tapir_installer.InstallerError) as context:
            tapir_installer.UpdateAddOn (self.CreateUpdateTarget (mockArchicad.port, restartArchicad = True), self.releaseInfo, False)
        self.assertEqual (str (context.exception), 'Tapir was updated to {0}. Archicad could not be started again: not found.'.format (RELEASE_TAG))
        self.AssertAddOnIsNew ()

    def testDryRunDoesNotContactArchicad (self):
        mockArchicad = self.StartMockArchicad ()
        exitCode, stdout, stderr = self.RunMain (['--console', '--dryRun', '--addOnFile', self.targetPath, '--versions', str (ARCHICAD_VERSION),
            '--archicadPort', str (mockArchicad.port)])
        self.assertEqual (exitCode, 0, stderr)
        self.assertIn ('Would quit Archicad on port {0}, replace {1} with {2}, restart Archicad'.format (
            mockArchicad.port, self.targetPath, self.releaseInfo['assets'][0]['name']), stdout)
        self.assertEqual (mockArchicad.receivedCommands, [])
        self.assertEqual (self.startedArchicadLocations, [])
        self.AssertAddOnIsOld ()

    def testTrailingSeparatorIsIgnored (self):
        exitCode, stdout, stderr = self.RunMain (['--console', '--dryRun', '--addOnFile', self.targetPath + os.sep, '--versions', str (ARCHICAD_VERSION)])
        self.assertEqual (exitCode, 0, stderr)
        self.assertIn ('Would replace {0} with'.format (self.targetPath), stdout)

    def testMissingAssetDoesNotContactArchicad (self):
        mockArchicad = self.StartMockArchicad ()
        self.releaseInfo['assets'] = []
        with self.assertRaises (tapir_installer.InstallerError) as context:
            tapir_installer.UpdateAddOn (self.CreateUpdateTarget (mockArchicad.port, restartArchicad = True), self.releaseInfo, False)
        self.assertIn ('No Tapir Add-On is available for Archicad {0}'.format (ARCHICAD_VERSION), str (context.exception))
        self.assertEqual (mockArchicad.receivedCommands, [])
        self.assertEqual (self.startedArchicadLocations, [])
        self.AssertAddOnIsOld ()

    def testBrokenDownloadDoesNotContactArchicad (self):
        # A dropped connection ends the download early without an error, and a
        # proxy or web filter may answer with a page of its own.
        assetContent = ReadBinaryFile (self.assetPath)
        assetUrl = self.releaseInfo['assets'][0]['browser_download_url']
        page = b'<html><body>Blocked by the web filter</body></html>'
        cases = [
            ('incomplete', self.ServeDownload (assetContent[:len (assetContent) // 2], len (assetContent)), len (assetContent),
                'received {0} bytes instead of {1}.'.format (len (assetContent) // 2, len (assetContent))),
            ('size differs', assetUrl, len (assetContent) + 1, 'received {0} bytes instead of {1}.'.format (len (assetContent), len (assetContent) + 1)),
            ('not the add-on', self.ServeDownload (page, len (page)), None, 'the received file is not the add-on'),
        ]
        for caseName, downloadUrl, assetSize, expectedError in cases:
            with self.subTest (caseName):
                mockArchicad = self.StartMockArchicad ()
                self.releaseInfo['assets'][0].update ({ 'browser_download_url' : downloadUrl, 'size' : assetSize })
                with self.assertRaises (tapir_installer.InstallerError) as context:
                    tapir_installer.UpdateAddOn (self.CreateUpdateTarget (mockArchicad.port, restartArchicad = True), self.releaseInfo, False)
                self.assertIn ('Failed to download {0}: {1}'.format (self.releaseInfo['assets'][0]['name'], expectedError), str (context.exception))
                self.assertEqual (mockArchicad.receivedCommands, [])
                self.assertEqual (self.startedArchicadLocations, [])
                self.AssertAddOnIsOld ()

    def testDownloadFailureDoesNotContactArchicad (self):
        mockArchicad = self.StartMockArchicad ()
        self.Patch (tapir_installer, 'DownloadAsset', unittest.mock.Mock (side_effect = OSError ('connection reset')))
        exitCode, stdout, stderr = self.RunMain (['--console', '--addOnFile', self.targetPath, '--versions', str (ARCHICAD_VERSION),
            '--archicadPort', str (mockArchicad.port)])
        self.assertEqual (exitCode, 1)
        self.assertIn ('Failed to download', stderr)
        self.assertEqual (mockArchicad.receivedCommands, [])
        self.assertEqual (self.startedArchicadLocations, [])
        self.AssertAddOnIsOld ()

    def testUnreachableArchicadKeepsAddOn (self):
        mockArchicad = self.StartMockArchicad ()
        mockArchicad.Stop ()
        with self.assertRaises (tapir_installer.InstallerError) as context:
            tapir_installer.UpdateAddOn (self.CreateUpdateTarget (mockArchicad.port, restartArchicad = True), self.releaseInfo, False)
        self.assertIn ('Failed to connect to Archicad on port {0}'.format (mockArchicad.port), str (context.exception))
        self.assertEqual (self.startedArchicadLocations, [])
        self.AssertAddOnIsOld ()

    def testArchicadNotQuittingKeepsAddOn (self):
        # The port keeps answering, for example because quitting was
        # cancelled in a save dialog. Without a cancel event (console mode)
        # the wait ends at the deadline. Archicad quits long after it, so
        # that a missing deadline fails the test instead of hanging it.
        process = self.StartProcess (60)
        mockArchicad = self.StartMockArchicad (quitDelaySeconds = 8)
        updateTarget = self.CreateUpdateTarget (mockArchicad.port, process.pid, restartArchicad = True)
        with self.assertRaises (tapir_installer.InstallerError) as context:
            tapir_installer.UpdateAddOn (updateTarget, self.releaseInfo, False)
        self.assertEqual (type (context.exception), tapir_installer.InstallerError)
        self.assertIn ('Archicad did not quit (was quitting cancelled?)', str (context.exception))
        self.assertIn ('QuitArchicad', mockArchicad.receivedCommands)
        self.assertIn ('GetAddOnVersion', mockArchicad.receivedCommands)
        # Archicad still runs, so it must not be started a second time, and
        # the update can be tried again.
        self.assertEqual (self.startedArchicadLocations, [])
        self.assertFalse (updateTarget.archicadWasQuit)
        self.AssertAddOnIsOld ()

    def testRefusedQuitIsReportedAtOnce (self):
        # Archicad answers at once when it refuses to quit, for example while
        # another add-on works, so the update must not wait for the timeout.
        self.Patch (tapir_installer, 'ARCHICAD_QUIT_TIMEOUT_SECONDS', 60)
        error = { 'code' : -2130313112, 'message' : 'Failed to quit Archicad!' }
        cases = [
            # The failed execution result of the QuitArchicad command.
            ({ 'succeeded' : True, 'result' : { 'addOnCommandResponse' : { 'success' : False, 'error' : error } } }, 'Failed to quit Archicad!'),
            # The JSON API did not run the command at all.
            ({ 'succeeded' : False, 'error' : error }, 'Archicad failed to execute QuitArchicad'),
        ]
        for (quitResponseJson, expectedReason), hasCancelEvent in itertools.product (cases, [False, True]):
            with self.subTest (expectedReason, hasCancelEvent = hasCancelEvent):
                mockArchicad = self.StartMockArchicad (quits = False, quitResponseJson = quitResponseJson)
                updateTarget = self.CreateUpdateTarget (mockArchicad.port, restartArchicad = True)
                # The GUI has no deadline: a late cancel ends a wrong wait.
                cancelEvent = self.CreateCancelEvent (8) if hasCancelEvent else None
                startTime = time.monotonic ()
                with self.assertRaises (tapir_installer.InstallerError) as context:
                    tapir_installer.UpdateAddOn (updateTarget, self.releaseInfo, False, cancelEvent = cancelEvent)
                self.assertLess (time.monotonic () - startTime, 10)
                self.assertIn ('Archicad did not quit ({0}'.format (expectedReason), str (context.exception))
                self.assertNotIn ('GetAddOnVersion', mockArchicad.receivedCommands)
                # Archicad still runs: the update can be tried again.
                self.assertFalse (updateTarget.archicadWasQuit)
                self.assertEqual (self.startedArchicadLocations, [])
                self.AssertAddOnIsOld ()

    def RunLateQuit (self, isQuitAnsweredFirst):
        # Archicad asks whether to save the changes, and gives no answer
        # until the user answered, much later than the console deadline. The
        # GUI (with a cancel event) waits for it, and tells what it waits for.
        self.Patch (tapir_installer, 'ARCHICAD_QUIT_TIMEOUT_SECONDS', 1)
        self.Patch (tapir_installer, 'ARCHICAD_QUIT_NOTICE_SECONDS', 0.5)
        mockArchicad = self.StartMockArchicad (quitDelaySeconds = 2.5, isBusyUntilQuit = True, isQuitAnsweredFirst = isQuitAnsweredFirst)
        updateTarget = self.CreateUpdateTarget (mockArchicad.port, restartArchicad = True)
        statusTexts = []
        waitingStates = []
        startTime = time.monotonic ()
        resultText = tapir_installer.UpdateAddOn (updateTarget, self.releaseInfo, False, statusTexts.append, cancelEvent = self.CreateCancelEvent (),
            waitingCallback = lambda isWaiting : waitingStates.append ((isWaiting, time.monotonic () - startTime, mockArchicad.isStopped)))
        self.assertGreater (time.monotonic () - startTime, 2.4)
        self.assertEqual (resultText, 'Tapir was updated to {0}. Archicad is starting again.'.format (RELEASE_TAG))
        self.assertEqual (statusTexts, ['Closing Archicad...', 'Waiting for Archicad...', 'Installing...', 'Restarting Archicad...'])
        self.assertEqual ([isWaiting for isWaiting, seconds, isStopped in waitingStates], [True, False])
        # The notice comes about in time although no request is answered
        # (each would wait 10 s), and while Archicad still runs.
        self.assertLess (waitingStates[0][1], 1.4)
        self.assertFalse (waitingStates[0][2])
        self.assertEqual (self.startedArchicadLocations, [ARCHICAD_LOCATION])
        self.assertTrue (updateTarget.archicadWasQuit)
        self.AssertAddOnIsNew ()

    def testLateQuitStillUpdates (self):
        self.RunLateQuit (isQuitAnsweredFirst = False)

    def testLateQuitAfterAnsweredQuitStillUpdates (self):
        self.RunLateQuit (isQuitAnsweredFirst = True)

    def testCancelWhileArchicadGivesNoAnswer (self):
        # Archicad's save prompt is open, and Archicad answers no request
        # (or only the quit request) meanwhile. Cancel, clicked as soon as it
        # is shown, acts at once, although each request would wait 10 s.
        self.Patch (tapir_installer, 'ARCHICAD_QUIT_NOTICE_SECONDS', 0.5)
        for isQuitAnsweredFirst in [False, True]:
            with self.subTest (isQuitAnsweredFirst = isQuitAnsweredFirst):
                mockArchicad = self.StartMockArchicad (quitDelaySeconds = 60, isBusyUntilQuit = True, isQuitAnsweredFirst = isQuitAnsweredFirst)
                updateTarget = self.CreateUpdateTarget (mockArchicad.port, restartArchicad = True)
                cancelEvent = threading.Event ()
                waitingTimes = {}
                def OnWaiting (isWaiting):
                    waitingTimes[isWaiting] = time.monotonic ()
                    if isWaiting:
                        cancelTimer = threading.Timer (0.05, cancelEvent.set)
                        cancelTimer.daemon = True
                        cancelTimer.start ()
                startTime = time.monotonic ()
                with self.assertRaises (tapir_installer.UpdateCancelledError):
                    tapir_installer.UpdateAddOn (updateTarget, self.releaseInfo, False, cancelEvent = cancelEvent, waitingCallback = OnWaiting)
                self.assertLess (waitingTimes[True] - startTime, 1.4)
                self.assertLess (waitingTimes[False] - waitingTimes[True], 1.0)
                self.assertFalse (mockArchicad.isStopped)
                self.assertFalse (updateTarget.archicadWasQuit)
                self.assertEqual (self.startedArchicadLocations, [])
                self.AssertAddOnIsOld ()

    def testLateRefusedQuitIsReported (self):
        # The user cancels quitting in Archicad's save prompt long after the
        # quit request, which got no answer until then: the update fails at
        # once, as Archicad still runs.
        self.Patch (tapir_installer, 'ARCHICAD_QUIT_NOTICE_SECONDS', 0.5)
        self.Patch (tapir_installer, 'ARCHICAD_COMMAND_TIMEOUT_SECONDS', 0.5)
        error = { 'code' : -2130313112, 'message' : 'Failed to quit Archicad!' }
        mockArchicad = self.StartMockArchicad (quits = False, quitAnswerDelaySeconds = 1.2,
            quitResponseJson = { 'succeeded' : True, 'result' : { 'addOnCommandResponse' : { 'success' : False, 'error' : error } } })
        updateTarget = self.CreateUpdateTarget (mockArchicad.port, restartArchicad = True)
        waitingStates = []
        startTime = time.monotonic ()
        with self.assertRaises (tapir_installer.InstallerError) as context:
            tapir_installer.UpdateAddOn (updateTarget, self.releaseInfo, False, cancelEvent = self.CreateCancelEvent (8), waitingCallback = waitingStates.append)
        self.assertLess (time.monotonic () - startTime, 4)
        self.assertIn ('Archicad did not quit (Failed to quit Archicad!)', str (context.exception))
        self.assertEqual (waitingStates, [True, False])
        # Archicad still runs while the quit request gets no answer.
        self.assertNotIn ('GetAddOnVersion', mockArchicad.receivedCommands)
        self.assertFalse (updateTarget.archicadWasQuit)
        self.assertEqual (self.startedArchicadLocations, [])
        self.AssertAddOnIsOld ()

    def testNoNoticeOnceArchicadQuit (self):
        # Archicad quits while the wait sleeps past the notice time: the
        # notice comes only after a check found Archicad still running.
        self.Patch (tapir_installer, 'ARCHICAD_QUIT_NOTICE_SECONDS', 0.5)
        self.Patch (tapir_installer, 'ARCHICAD_POLL_INTERVAL_SECONDS', 1.0)
        mockArchicad = self.StartMockArchicad (quits = False)
        answers = iter ([True])
        self.Patch (tapir_installer, 'IsArchicadAnswering', lambda archicadPort : next (answers, False))
        statusTexts = []
        waitingStates = []
        resultText = tapir_installer.UpdateAddOn (self.CreateUpdateTarget (mockArchicad.port, restartArchicad = True), self.releaseInfo, False,
            statusTexts.append, cancelEvent = self.CreateCancelEvent (), waitingCallback = waitingStates.append)
        self.assertEqual (resultText, 'Tapir was updated to {0}. Archicad is starting again.'.format (RELEASE_TAG))
        self.assertEqual (statusTexts, ['Closing Archicad...', 'Installing...', 'Restarting Archicad...'])
        self.assertEqual (waitingStates, [])
        self.AssertAddOnIsNew ()

    def testSlowRefusalEndsTheWait (self):
        # Windows refuses a connection to a closed port only after retries,
        # which take about 2 s, longer than Archicad's answer is waited for.
        # The closed port must not be taken for a busy Archicad: the GUI
        # would wait for it until cancelled.
        refusalSeconds = 1.0
        createConnection = socket.create_connection
        def CreateConnection (address, timeout, *args):
            startTime = time.monotonic ()
            try:
                return createConnection (address, timeout, *args)
            except ConnectionRefusedError:
                if isinstance (timeout, (int, float)) and timeout < refusalSeconds:
                    time.sleep (timeout)
                    raise socket.timeout ('timed out')
                time.sleep (max (0.0, refusalSeconds - (time.monotonic () - startTime)))
                raise
        self.Patch (socket, 'create_connection', CreateConnection)
        mockArchicad = self.StartMockArchicad (quitsBeforeAnswering = True)
        updateTarget = self.CreateUpdateTarget (mockArchicad.port, restartArchicad = True)
        resultText = tapir_installer.UpdateAddOn (updateTarget, self.releaseInfo, False, cancelEvent = self.CreateCancelEvent (8))
        self.assertEqual (resultText, 'Tapir was updated to {0}. Archicad is starting again.'.format (RELEASE_TAG))
        self.assertEqual (self.startedArchicadLocations, [ARCHICAD_LOCATION])
        self.AssertAddOnIsNew ()

    def testCancelledWaitKeepsArchicadRunning (self):
        # The user cancels the wait for Archicad after the console deadline
        # (there is none in the GUI): nothing is replaced, and Archicad,
        # which still runs, is not started a second time. Archicad quits
        # long after that, so that an ignored cancel fails the test instead
        # of hanging it.
        self.Patch (tapir_installer, 'ARCHICAD_QUIT_NOTICE_SECONDS', 0.5)
        mockArchicad = self.StartMockArchicad (quitDelaySeconds = 8)
        updateTarget = self.CreateUpdateTarget (mockArchicad.port, restartArchicad = True)
        statusTexts = []
        waitingStates = []
        cancelSeconds = tapir_installer.ARCHICAD_QUIT_TIMEOUT_SECONDS + 0.5
        startTime = time.monotonic ()
        with self.assertRaises (tapir_installer.UpdateCancelledError) as context:
            tapir_installer.UpdateAddOn (updateTarget, self.releaseInfo, False, statusTexts.append,
                cancelEvent = self.CreateCancelEvent (cancelSeconds), waitingCallback = waitingStates.append)
        self.assertGreater (time.monotonic () - startTime, cancelSeconds - 0.1)
        self.assertEqual (str (context.exception), tapir_installer.UPDATE_CANCELLED_TEXT)
        self.assertEqual (statusTexts, ['Closing Archicad...', 'Waiting for Archicad...'])
        self.assertEqual (waitingStates, [True, False])
        self.assertFalse (mockArchicad.isStopped)
        self.assertEqual (self.startedArchicadLocations, [])
        self.assertFalse (updateTarget.archicadWasQuit)
        self.AssertAddOnIsOld ()

    def testCancelWhileArchicadQuitsIsIgnored (self):
        # The user cancels while Archicad quits, between two checks of its
        # port: Archicad has quit, so it is updated and started again.
        mockArchicad = self.StartMockArchicad (quits = False)
        class CancelEvent (threading.Event):
            def wait (self, timeout = None):
                mockArchicad.Stop ()
                self.set ()
                return threading.Event.wait (self, timeout)
        updateTarget = self.CreateUpdateTarget (mockArchicad.port, restartArchicad = True)
        resultText = tapir_installer.UpdateAddOn (updateTarget, self.releaseInfo, False, cancelEvent = CancelEvent ())
        self.assertIn ('GetAddOnVersion', mockArchicad.receivedCommands)
        self.assertEqual (resultText, 'Tapir was updated to {0}. Archicad is starting again.'.format (RELEASE_TAG))
        self.assertEqual (self.startedArchicadLocations, [ARCHICAD_LOCATION])
        self.assertTrue (updateTarget.archicadWasQuit)
        self.AssertAddOnIsNew ()

    def testArchicadMayComeToTheFrontOnWindows (self):
        # Before quitting Archicad, the installer in the foreground lets
        # Archicad bring its save prompt to the front.
        mockArchicad = self.StartMockArchicad ()
        calls = []
        class FakeFunction:
            def __call__ (self, *args):
                calls.append ((args, self.argtypes, list (mockArchicad.receivedCommands)))
                return 1
        fakeUser32 = types.SimpleNamespace (AllowSetForegroundWindow = FakeFunction ())
        def WinDLL (name, use_last_error = False):
            if name == 'user32':
                return fakeUser32
            # kernel32, to wait for the process: its errors are ignored.
            raise OSError ('not available')
        self.Patch (tapir_installer, 'IsUsingWindows', lambda : True)
        patcher = unittest.mock.patch.object (ctypes, 'WinDLL', WinDLL, create = True)
        patcher.start ()
        self.addCleanup (patcher.stop)
        tapir_installer.UpdateAddOn (self.CreateUpdateTarget (mockArchicad.port, 4321), self.releaseInfo, False)
        self.assertEqual (calls, [((4321,), [ctypes.wintypes.DWORD], ['GetArchicadLocation'])])
        self.AssertAddOnIsNew ()
        # Failures are ignored, and without a process id nothing is called.
        for fakeWinDLL in [unittest.mock.Mock (side_effect = OSError ('no user32')), lambda name, use_last_error = False : None]:
            patcher = unittest.mock.patch.object (ctypes, 'WinDLL', fakeWinDLL, create = True)
            patcher.start ()
            self.addCleanup (patcher.stop)
            tapir_installer.AllowArchicadToComeToFront (4321)
        tapir_installer.AllowArchicadToComeToFront (None)
        self.assertEqual (len (calls), 1)

    def testWarningIsNotAFailure (self):
        # Tapir was updated, but something is left to do by hand: console
        # mode prints it without ERROR and exits with 0.
        warningText = 'Its old version could not be deleted: delete it by hand.'
        self.Patch (tapir_installer, 'ReplaceAddOn', unittest.mock.Mock (return_value = warningText))
        exitCode, stdout, stderr = self.RunMain (['--console', '--addOnFile', self.targetPath, '--versions', str (ARCHICAD_VERSION)])
        self.assertEqual (exitCode, 0, stderr)
        self.assertIn ('Tapir was updated to {0}. {1}'.format (RELEASE_TAG, warningText), stdout)
        self.assertNotIn ('ERROR', stdout + stderr)
        with self.assertRaises (tapir_installer.InstallerWarning) as context:
            tapir_installer.UpdateAddOn (self.CreateUpdateTarget (), self.releaseInfo, False)
        self.assertEqual (str (context.exception), 'Tapir was updated to {0}. {1}'.format (RELEASE_TAG, warningText))

    def testUpdateWaitsForArchicadProcessToExit (self):
        processSeconds = 3
        process = self.StartProcess (processSeconds)
        mockArchicad = self.StartMockArchicad ()
        startTime = time.monotonic ()
        tapir_installer.UpdateAddOn (self.CreateUpdateTarget (mockArchicad.port, process.pid), self.releaseInfo, False)
        # The port closes after 0.3 seconds, the process only exits later.
        self.assertGreater (time.monotonic () - startTime, processSeconds - 0.5)
        self.AssertAddOnIsNew ()

    def testArchicadProcessNotExitingStillUpdates (self):
        # Archicad has quit once its port closed, although its process may
        # stay much longer, for example behind a crash report dialog.
        self.Patch (tapir_installer, 'ARCHICAD_PROCESS_EXIT_TIMEOUT_SECONDS', 1)
        process = self.StartProcess (60)
        mockArchicad = self.StartMockArchicad ()
        updateTarget = self.CreateUpdateTarget (mockArchicad.port, process.pid, restartArchicad = True)
        startTime = time.monotonic ()
        resultText = tapir_installer.UpdateAddOn (updateTarget, self.releaseInfo, False)
        # It waited for the process until the timeout.
        self.assertGreater (time.monotonic () - startTime, 0.9)
        self.assertIsNone (process.poll ())
        self.assertEqual (resultText, 'Tapir was updated to {0}. Archicad is starting again.'.format (RELEASE_TAG))
        self.assertEqual (self.startedArchicadLocations, [ARCHICAD_LOCATION])
        self.assertTrue (updateTarget.archicadWasQuit)
        self.AssertAddOnIsNew ()

    def testFailuresAfterArchicadQuitRestartArchicad (self):
        # Once the port closed, Archicad is started again whatever fails, and
        # the GUI does not offer to update again with this target.
        process = self.StartProcess (60)
        self.Patch (tapir_installer, 'WaitForProcessToExit', unittest.mock.Mock (side_effect = OSError ('wait failed')))
        self.Patch (tapir_installer, 'ReplaceAddOn', unittest.mock.Mock (side_effect = tapir_installer.InstallerError ('Replace failed.')))
        mockArchicad = self.StartMockArchicad ()
        updateTarget = self.CreateUpdateTarget (mockArchicad.port, process.pid, restartArchicad = True)
        with self.assertRaises (tapir_installer.InstallerError) as context:
            tapir_installer.UpdateAddOn (updateTarget, self.releaseInfo, False)
        self.assertEqual (str (context.exception), 'Replace failed. Archicad was started again. Start the update again from Archicad.')
        tapir_installer.WaitForProcessToExit.assert_called_once_with (process.pid, tapir_installer.ARCHICAD_PROCESS_EXIT_TIMEOUT_SECONDS)
        self.assertEqual (self.startedArchicadLocations, [ARCHICAD_LOCATION])
        self.assertTrue (updateTarget.archicadWasQuit)
        self.AssertAddOnIsOld ()


@unittest.skipIf (IS_MAC, 'tests the Windows add-on file logic')
class AddOnFileTests (UpdateModeTestBase):
    def PatchReplaceFailures (self, failureCount):
        originalReplace = os.replace
        self.replaceAttemptCount = 0
        def Replace (sourcePath, targetPath):
            if targetPath == self.targetPath:
                self.replaceAttemptCount += 1
                if self.replaceAttemptCount <= failureCount:
                    # What a running Archicad's lock on the add-on causes.
                    raise PermissionError (13, 'The process cannot access the file because it is being used by another process')
            originalReplace (sourcePath, targetPath)
        self.Patch (tapir_installer.os, 'replace', Replace)

    def testLockedAddOnIsRetried (self):
        self.PatchReplaceFailures (2)
        tapir_installer.UpdateAddOn (self.CreateUpdateTarget (), self.releaseInfo, False)
        self.assertEqual (self.replaceAttemptCount, 3)
        self.AssertAddOnIsNew ()

    def testLockedAddOnFailsAfterTimeout (self):
        self.Patch (tapir_installer, 'REPLACE_RETRY_TIMEOUT_SECONDS', 1)
        self.PatchReplaceFailures (1000)
        mockArchicad = self.StartMockArchicad ()
        with self.assertRaises (tapir_installer.InstallerError) as context:
            tapir_installer.UpdateAddOn (self.CreateUpdateTarget (mockArchicad.port, restartArchicad = True), self.releaseInfo, False)
        self.assertEqual (str (context.exception), tapir_installer.PERMISSION_DENIED_TEXT + ' Archicad was started again. Start the update again from Archicad.')
        self.assertEqual (self.startedArchicadLocations, [ARCHICAD_LOCATION])
        self.AssertAddOnIsOld ()

    def testUnwritableFolderKeepsArchicadRunning (self):
        mockArchicad = self.StartMockArchicad ()
        self.Patch (tapir_installer.shutil, 'copyfile', unittest.mock.Mock (side_effect = PermissionError (13, 'Access is denied')))
        with self.assertRaises (tapir_installer.InstallerError) as context:
            tapir_installer.UpdateAddOn (self.CreateUpdateTarget (mockArchicad.port, restartArchicad = True), self.releaseInfo, False)
        self.assertEqual (str (context.exception), tapir_installer.PERMISSION_DENIED_TEXT)
        self.assertEqual (mockArchicad.receivedCommands, [])
        self.AssertAddOnIsOld ()


@unittest.skipUnless (tapir_installer.IsUsingWindows (), 'tests Windows drive letters')
class MappedDriveTests (UpdateModeTestBase):
    def testAddOnOnMappedNetworkDrive (self):
        # The installer runs as administrator, which does not see the drives
        # the user mapped: it finds their network folder in the registry. The
        # temporary folder plays the network folder of an unused drive letter.
        driveLetter = next (letter for letter in 'ZYXWVUTSRQPONMLKJIHG' if not os.path.exists (letter + ':\\'))
        mappedTargetPath = driveLetter + ':' + self.targetPath[len (self.tempFolderPath):]
        networkFolderPaths = { driveLetter : self.tempFolderPath }

        class FakeKey:
            def __init__ (self, valuesByName):
                self.valuesByName = valuesByName

            def __enter__ (self):
                return self

            def __exit__ (self, *args):
                return False

        def OpenKey (rootKey, keyPath, *args):
            # Only the mapped drives exist. urllib also reads its proxy
            # settings from the registry: they are missing.
            letter = keyPath[len ('Network\\'):].upper ()
            if rootKey != 'HKEY_CURRENT_USER' or not keyPath.startswith ('Network\\') or letter not in networkFolderPaths:
                raise FileNotFoundError (2, 'The system cannot find the file specified')
            return FakeKey ({ 'RemotePath' : networkFolderPaths[letter] })

        def QueryValueEx (key, valueName):
            if valueName not in key.valuesByName:
                raise FileNotFoundError (2, 'The system cannot find the file specified')
            return (key.valuesByName[valueName], 1)

        fakeWinreg = types.SimpleNamespace (HKEY_CURRENT_USER = 'HKEY_CURRENT_USER', OpenKey = OpenKey, QueryValueEx = QueryValueEx)
        patcher = unittest.mock.patch.dict (sys.modules, { 'winreg' : fakeWinreg })
        patcher.start ()
        self.addCleanup (patcher.stop)
        arguments = ['--console', '--dryRun', '--addOnFile', mappedTargetPath, '--versions', str (ARCHICAD_VERSION)]
        exitCode, stdout, stderr = self.RunMain (arguments)
        self.assertEqual (exitCode, 0, stderr)
        self.assertIn ('Would replace {0} with'.format (self.targetPath), stdout)
        # Not a persistent mapping: the installer cannot find the add-on.
        networkFolderPaths.clear ()
        exitCode, stdout, stderr = self.RunMain (arguments)
        self.assertEqual (exitCode, 1)
        self.assertIn ('is on drive {0}:, which the installer, running as administrator, cannot see'.format (driveLetter), stderr)
        self.AssertAddOnIsOld ()


@unittest.skipUnless (IS_MAC, 'tests the macOS add-on bundle logic')
class AddOnBundleTests (UpdateModeTestBase):
    def testBundleKeepsItsName (self):
        # Archicad's add-on list refers to the loaded bundle, whose name may
        # differ from the bundle name in the release asset.
        renamedTargetPath = os.path.join (self.addOnsFolderPath, 'Tapir.bundle')
        os.rename (self.targetPath, renamedTargetPath)
        self.targetPath = renamedTargetPath
        tapir_installer.UpdateAddOn (self.CreateUpdateTarget (), self.releaseInfo, False)
        self.AssertAddOnIsNew ()

    def setUp (self):
        super ().setUp ()
        self.newBundlePath = self.targetPath + '.new'
        self.oldBundlePath = self.targetPath + '.old'

    def PatchAdministratorShell (self, cancelled = False, failingMvSourceSuffix = None, undeletablePathSuffix = None):
        # Runs the commands meant for administrator privileges without the
        # password dialog, or fails like a cancelled dialog. The mv command
        # can be made to fail for a source path ending with the given suffix,
        # and rm for an existing path ending with the other one, like for a
        # file locked in Finder, which even root cannot delete.
        self.administratorCommands = []
        environment = dict (os.environ)
        fakeCommands = {}
        if failingMvSourceSuffix is not None:
            fakeCommands['mv'] = '#!/bin/sh\ncase "$1" in *{0}) echo "mv: failed" >&2; exit 1;; esac\nexec /bin/mv "$@"\n'.format (failingMvSourceSuffix)
        if undeletablePathSuffix is not None:
            fakeCommands['rm'] = '#!/bin/sh\ncase "$2" in *{0}) if [ -e "$2" ]; then echo "rm: Operation not permitted" >&2; exit 1; fi;; esac\nexec /bin/rm "$@"\n'.format (undeletablePathSuffix)
        if len (fakeCommands) > 0:
            fakeCommandsFolderPath = os.path.join (self.tempFolderPath, 'FakeCommands')
            WriteFiles (fakeCommandsFolderPath, fakeCommands)
            for commandName in fakeCommands:
                os.chmod (os.path.join (fakeCommandsFolderPath, commandName), 0o755)
            environment['PATH'] = fakeCommandsFolderPath + os.pathsep + environment.get ('PATH', '')
        def RunShellCommand (shellCommand):
            self.administratorCommands.append (shellCommand)
            if cancelled:
                raise tapir_installer.InstallerError ('Failed to install with administrator privileges: execution error: User canceled. (-128)')
            result = subprocess.run (['/bin/sh', '-c', shellCommand], env = environment, capture_output = True, text = True)
            if result.returncode != 0:
                raise tapir_installer.InstallerError ('Failed to install with administrator privileges: {0}'.format (result.stderr.strip ()))
        self.Patch (tapir_installer, 'RunShellCommandWithAdminPrivilegesMac', RunShellCommand)

    def PatchRenameFailures (self, failingSourcePaths, error):
        originalRename = os.rename
        def Rename (sourcePath, targetPath):
            if sourcePath in failingSourcePaths:
                raise error
            originalRename (sourcePath, targetPath)
        self.Patch (tapir_installer.os, 'rename', Rename)

    def UpdateAndExpectError (self, expectedErrorText, errorClass = tapir_installer.InstallerError):
        # With an Archicad to restart: it is started again in every case.
        mockArchicad = self.StartMockArchicad ()
        with self.assertRaises (errorClass) as context:
            tapir_installer.UpdateAddOn (self.CreateUpdateTarget (mockArchicad.port, restartArchicad = True), self.releaseInfo, False)
        self.assertEqual (str (context.exception), expectedErrorText)
        self.assertEqual (self.startedArchicadLocations, [ARCHICAD_LOCATION])

    def UpdateAndExpectWarning (self, expectedWarningText):
        self.UpdateAndExpectError (expectedWarningText, tapir_installer.InstallerWarning)

    def testAdministratorFallback (self):
        # The same commands run without the password dialog.
        self.PatchAdministratorShell ()
        self.Patch (tapir_installer.os, 'rename', unittest.mock.Mock (side_effect = PermissionError (13, 'Permission denied')))
        tapir_installer.UpdateAddOn (self.CreateUpdateTarget (), self.releaseInfo, False)
        self.assertEqual (len (self.administratorCommands), 1)
        self.AssertAddOnIsNew ()

    def testLeftoversOfEarlierAttemptAreReplaced (self):
        for path in [self.newBundlePath, self.oldBundlePath]:
            WriteFiles (path, { 'Contents/Info.plist' : 'leftover' })
        tapir_installer.UpdateAddOn (self.CreateUpdateTarget (), self.releaseInfo, False)
        self.AssertAddOnIsNew ()

    def testFailedMoveIntoPlaceKeepsOldBundle (self):
        # The old bundle is moved aside, then moving the new one into place
        # fails: the old one is moved back.
        self.PatchRenameFailures ([self.newBundlePath], OSError (5, 'Input/output error'))
        self.UpdateAndExpectError ('Failed to replace {0}: [Errno 5] Input/output error. Archicad was started again. Start the update again from Archicad.'.format (self.targetPath))
        self.AssertAddOnIsOld ()

    def testCancelledAdministratorFallbackKeepsOldBundle (self):
        # The administrator fallback after the old bundle was moved aside and
        # back, then the password dialog is cancelled.
        self.PatchAdministratorShell (cancelled = True)
        self.PatchRenameFailures ([self.newBundlePath], PermissionError (13, 'Permission denied'))
        self.UpdateAndExpectError ('Failed to install with administrator privileges: execution error: User canceled. (-128). '
            'Archicad was started again. Start the update again from Archicad.')
        self.assertEqual (len (self.administratorCommands), 1)
        self.AssertAddOnIsOld ()

    def testFailedAdministratorMoveIntoPlaceKeepsOldBundle (self):
        # The administrator command fails between its steps: the old bundle
        # was moved aside, the new one cannot be moved into place.
        self.PatchAdministratorShell (failingMvSourceSuffix = '.new')
        self.Patch (tapir_installer.os, 'rename', unittest.mock.Mock (side_effect = PermissionError (13, 'Permission denied')))
        self.UpdateAndExpectError ('Failed to install with administrator privileges: mv: failed. Archicad was started again. Start the update again from Archicad.')
        self.AssertAddOnIsOld ()

    def testFailedMoveBackKeepsBothBundles (self):
        # Neither the new bundle nor the old one can be moved into place: the
        # cleanup must not delete either of them.
        self.PatchRenameFailures ([self.newBundlePath, self.oldBundlePath], OSError (5, 'Input/output error'))
        self.UpdateAndExpectError ('Failed to replace {0}: [Errno 5] Input/output error. The old add-on could not be moved back either: '
            'rename {1} back by hand. Archicad was started again. Start the update again from Archicad.'.format (self.targetPath, self.oldBundlePath))
        self.assertEqual (sorted (os.listdir (self.addOnsFolderPath)), sorted (os.path.basename (path) for path in [self.newBundlePath, self.oldBundlePath]))
        self.assertEqual (ReadFile (os.path.join (self.oldBundlePath, 'Contents', 'Info.plist')), 'old')
        self.assertEqual (ReadFile (os.path.join (self.newBundlePath, 'Contents', 'Info.plist')), 'new')

    def PatchOldBundleDeletionFailure (self):
        # Deleting the old bundle stops halfway: some of its files belong to
        # another user, although its folder is writable.
        originalRmtree = shutil.rmtree
        def Rmtree (path, *args, **kwargs):
            if path == self.oldBundlePath:
                os.remove (os.path.join (path, 'Contents', 'Info.plist'))
                raise PermissionError (13, 'Permission denied')
            originalRmtree (path, *args, **kwargs)
        self.Patch (tapir_installer.shutil, 'rmtree', Rmtree)

    def testOldBundleIsDeletedAsAdministrator (self):
        self.PatchOldBundleDeletionFailure ()
        self.PatchAdministratorShell ()
        tapir_installer.UpdateAddOn (self.CreateUpdateTarget (), self.releaseInfo, False)
        self.assertEqual (self.administratorCommands, ['rm -rf {0}'.format (shlex.quote (self.oldBundlePath))])
        self.AssertAddOnIsNew ()

    def AssertOldBundleNotDeletedIsReported (self):
        # A warning, not a failure: Tapir was updated.
        self.UpdateAndExpectWarning ('Tapir was updated to {0}. Its old version could not be deleted: delete {1} by hand. Archicad is starting again.'.format (
            RELEASE_TAG, self.oldBundlePath))
        self.assertEqual (ReadFile (os.path.join (self.targetPath, 'Contents', 'Info.plist')), 'new')
        self.assertEqual (sorted (os.listdir (self.addOnsFolderPath)), sorted (os.path.basename (path) for path in [self.targetPath, self.oldBundlePath]))

    def testOldBundleNotDeletedIsReported (self):
        self.PatchOldBundleDeletionFailure ()
        self.PatchAdministratorShell (cancelled = True)
        self.AssertOldBundleNotDeletedIsReported ()

    def testOldBundleNotDeletedAsAdministratorIsReported (self):
        # The administrator fallback moved the new bundle into place, but
        # cannot delete the old one.
        self.PatchAdministratorShell (undeletablePathSuffix = '.old')
        self.Patch (tapir_installer.os, 'rename', unittest.mock.Mock (side_effect = PermissionError (13, 'Permission denied')))
        self.AssertOldBundleNotDeletedIsReported ()
        self.assertEqual (len (self.administratorCommands), 1)

    def testMissingTargetKeepsLeftovers (self):
        # An earlier attempt moved the add-on aside and failed to move the
        # new one into place: the leftovers are the only copies left, so
        # they must not be deleted.
        os.rename (self.targetPath, self.oldBundlePath)
        WriteFiles (self.newBundlePath, { 'Contents/Info.plist' : 'leftover' })
        self.UpdateAndExpectError ('The Tapir Add-On to update was not found: {0}. Archicad was started again. Start the update again from Archicad.'.format (self.targetPath))
        self.assertEqual (sorted (os.listdir (self.addOnsFolderPath)), sorted (os.path.basename (path) for path in [self.newBundlePath, self.oldBundlePath]))
        self.assertEqual (ReadFile (os.path.join (self.oldBundlePath, 'Contents', 'Info.plist')), 'old')
        self.assertTrue (os.path.isfile (os.path.join (self.oldBundlePath, 'Contents', 'Resources', 'Removed.txt')))
        self.assertEqual (ReadFile (os.path.join (self.newBundlePath, 'Contents', 'Info.plist')), 'leftover')

    def testZipWithoutBundleKeepsArchicadRunning (self):
        mockArchicad = self.StartMockArchicad ()
        emptyFolderPath = os.path.join (self.tempFolderPath, 'Empty')
        WriteFiles (emptyFolderPath, { 'Readme.txt' : 'no bundle' })
        assetPath = os.path.join (self.tempFolderPath, 'Release', 'TapirAddOn_AC29_Mac.zip')
        os.remove (assetPath)
        subprocess.run (['ditto', '-c', '-k', '--keepParent', emptyFolderPath, assetPath], check = True)
        self.releaseInfo['assets'][0]['size'] = os.path.getsize (assetPath)
        with self.assertRaises (tapir_installer.InstallerError) as context:
            tapir_installer.UpdateAddOn (self.CreateUpdateTarget (mockArchicad.port, restartArchicad = True), self.releaseInfo, False)
        self.assertIn ('does not contain exactly one add-on bundle', str (context.exception))
        self.assertEqual (mockArchicad.receivedCommands, [])
        self.AssertAddOnIsOld ()


class UpdateLockTests (UpdateModeTestBase):
    @classmethod
    def setUpClass (cls):
        spec = importlib.util.spec_from_file_location ('update_addon_and_restart_archicad', LEGACY_UPDATE_SCRIPT_PATH)
        cls.legacyUpdateScript = importlib.util.module_from_spec (spec)
        spec.loader.exec_module (cls.legacyUpdateScript)

    def RunUpdate (self, mockArchicad, *arguments):
        return self.RunMain (['--console', '--addOnFile', self.targetPath, '--versions', str (ARCHICAD_VERSION),
            '--archicadPort', str (mockArchicad.port)] + list (arguments))

    def LockFilePath (self, archicadPort):
        return os.path.join (self.tempFolderPath, 'TapirUpdate_{0}.lock'.format (archicadPort))

    def ReadLockFile (self, archicadPort):
        # Only while no update holds the lock: on Windows the locked first
        # byte cannot be read, not even through another handle of the
        # process that holds the lock.
        with open (self.LockFilePath (archicadPort), 'rb') as file:
            return file.read ()

    def ReadLockState (self, archicadPort):
        # Also while an update holds the lock: like another update, reads
        # only the state beside the locked first byte.
        with open (self.LockFilePath (archicadPort), 'rb') as file:
            file.seek (tapir_installer.LOCK_STATE_OFFSET)
            return file.read ()

    def HoldLegacyUpdateLock (self, archicadPort, isWaitingForQuit = False):
        # Like the legacy update script while it updates, and with
        # isWaitingForQuit while it waits for Archicad to quit. Returns the
        # function that releases the lock.
        lock = contextlib.ExitStack ()
        self.addCleanup (lock.close)
        setWaitingForQuit = lock.enter_context (self.legacyUpdateScript.UpdateLock (archicadPort))
        setWaitingForQuit (isWaitingForQuit)
        return lock.close

    def HoldLockWithContent (self, archicadPort, content):
        # Another update holds the lock, and the lock file has this content.
        # Returns the function that releases the lock.
        with open (self.LockFilePath (archicadPort), 'wb') as file:
            file.write (content)
        lockFile = os.open (self.LockFilePath (archicadPort), os.O_RDWR)
        lock = contextlib.ExitStack ()
        self.addCleanup (lock.close)
        lock.callback (os.close, lockFile)
        if os.name == 'nt':
            msvcrt = importlib.import_module ('msvcrt')
            msvcrt.locking (lockFile, msvcrt.LK_NBLCK, 1)
        else:
            fcntl = importlib.import_module ('fcntl')
            fcntl.flock (lockFile, fcntl.LOCK_EX | fcntl.LOCK_NB)
        return lock.close

    def IsUpdateLocked (self, archicadPort):
        # Whether the legacy update script would find an update running.
        try:
            with self.legacyUpdateScript.UpdateLock (archicadPort):
                return False
        except self.legacyUpdateScript.UpdateStoppedError:
            return True

    def GetTextsOfAnotherUpdate (self, archicadPort):
        # What another update would show, (by the legacy update script, by
        # another installer), or None for one that would run. The installer
        # waits UPDATE_LOCK_WAIT_SECONDS for the lock.
        texts = []
        try:
            with self.legacyUpdateScript.UpdateLock (archicadPort):
                texts.append (None)
        except self.legacyUpdateScript.UpdateStoppedError as e:
            texts.append (str (e))
        try:
            with tapir_installer.UpdateLock (self.CreateUpdateTarget (archicadPort), False):
                texts.append (None)
        except tapir_installer.UpdateRunningError as e:
            texts.append (str (e))
        return tuple (texts)

    def AssertUpdateWasStopped (self, mockArchicad, exitCode, stderr, expectedText = tapir_installer.UPDATE_RUNNING_TEXT):
        # Nothing was touched, not even the network.
        self.assertEqual (exitCode, 1)
        self.assertIn ('ERROR: {0}'.format (expectedText), stderr)
        self.assertEqual (mockArchicad.receivedCommands, [])
        self.assertEqual (self.releaseInfoRequestCount, 0)
        self.assertEqual (self.startedArchicadLocations, [])
        self.AssertAddOnIsOld ()

    def testTextsAndStatesAreTheLegacyOnes (self):
        self.assertEqual (tapir_installer.UPDATE_RUNNING_TEXT,
            'Tapir is being updated already. Wait for that update to finish: it closes Archicad itself and starts it again.')
        self.assertEqual (tapir_installer.UPDATE_WAITING_FOR_QUIT_TEXT,
            'Tapir is being updated already, and that update waits for Archicad to quit. Quit Archicad to finish it; Archicad is then started again.')
        for name in ['UPDATE_RUNNING_TEXT', 'UPDATE_WAITING_FOR_QUIT_TEXT', 'LOCK_STATE_OFFSET', 'LOCK_STATE_WAITING_FOR_QUIT', 'LOCK_STATE_UPDATING']:
            self.assertEqual (getattr (tapir_installer, name), getattr (self.legacyUpdateScript, name), name)
        self.assertEqual ((tapir_installer.LOCK_STATE_OFFSET, tapir_installer.LOCK_STATE_WAITING_FOR_QUIT, tapir_installer.LOCK_STATE_UPDATING),
            (1, b'quitting', b'updating'))
        self.assertNotIn ('quit', tapir_installer.UPDATE_RUNNING_TEXT.lower ())

    def testRunningLegacyUpdateStopsTheUpdate (self):
        mockArchicad = self.StartMockArchicad ()
        self.HoldLegacyUpdateLock (mockArchicad.port)
        startTime = time.monotonic ()
        exitCode, stdout, stderr = self.RunUpdate (mockArchicad)
        # The lock was waited for.
        self.assertGreater (time.monotonic () - startTime, tapir_installer.UPDATE_LOCK_WAIT_SECONDS - 0.1)
        # That update does not wait for Archicad to quit: quitting Archicad
        # would make it fail.
        self.AssertUpdateWasStopped (mockArchicad, exitCode, stderr)
        self.assertNotIn ('Quit Archicad', stderr)

    def testLegacyUpdateWaitingForArchicadToQuitStopsTheUpdate (self):
        # The legacy update script tells that it waits for Archicad to quit:
        # quitting Archicad finishes that update.
        mockArchicad = self.StartMockArchicad ()
        self.HoldLegacyUpdateLock (mockArchicad.port, isWaitingForQuit = True)
        exitCode, stdout, stderr = self.RunUpdate (mockArchicad)
        self.AssertUpdateWasStopped (mockArchicad, exitCode, stderr, tapir_installer.UPDATE_WAITING_FOR_QUIT_TEXT)
        self.assertEqual (self.ReadLockState (mockArchicad.port), b'quitting')

    def testRunningInstallerStopsTheUpdate (self):
        # Another installer process updates the same Archicad, and then waits
        # for Archicad to quit. Its lock is released when its process ends,
        # also when it is killed while it waits.
        mockArchicad = self.StartMockArchicad ()
        code = ('import sys\n'
            'sys.path.insert (0, sys.argv[1])\n'
            'import tapir_installer\n'
            'with tapir_installer.UpdateLock (tapir_installer.AddOnUpdateTarget (29, sys.argv[2], int (sys.argv[3])), False) as setWaitingForQuit:\n'
            '    print ("locked", flush = True)\n'
            '    for line in sys.stdin:\n'
            '        setWaitingForQuit (line.strip () == "quitting")\n'
            '        print (line.strip (), flush = True)\n')
        process = subprocess.Popen ([sys.executable, '-c', code, os.path.dirname (os.path.abspath (tapir_installer.__file__)), self.targetPath, str (mockArchicad.port)],
            stdin = subprocess.PIPE, stdout = subprocess.PIPE, text = True, env = dict (os.environ, TMPDIR = self.tempFolderPath))
        self.addCleanup (process.stdout.close)
        self.addCleanup (process.stdin.close)
        self.addCleanup (self.StopProcess, process)
        self.assertEqual (process.stdout.readline ().strip (), 'locked')
        exitCode, stdout, stderr = self.RunUpdate (mockArchicad)
        self.AssertUpdateWasStopped (mockArchicad, exitCode, stderr)
        process.stdin.write ('quitting\n')
        process.stdin.flush ()
        self.assertEqual (process.stdout.readline ().strip (), 'quitting')
        exitCode, stdout, stderr = self.RunUpdate (mockArchicad)
        self.AssertUpdateWasStopped (mockArchicad, exitCode, stderr, tapir_installer.UPDATE_WAITING_FOR_QUIT_TEXT)
        self.StopProcess (process)
        # Windows may release the locks of a killed process a little later.
        self.Patch (tapir_installer, 'UPDATE_LOCK_WAIT_SECONDS', 5)
        exitCode, stdout, stderr = self.RunUpdate (mockArchicad)
        self.assertEqual (exitCode, 0, stderr)
        self.AssertAddOnIsNew ()
        # The state the killed process left was overwritten.
        self.assertEqual (self.ReadLockFile (mockArchicad.port)[1:], b'updating')

    def testLockIsHeldUntilArchicadIsStartedAgain (self):
        # Also the legacy update script finds the update running, from before
        # Archicad is contacted until it was started again.
        mockArchicad = self.StartMockArchicad ()
        lockStates = []
        handleCommand = mockArchicad.HandleCommand
        def HandleCommand (requestHandler, commandName):
            if commandName != 'GetAddOnVersion':
                lockStates.append ((commandName, self.IsUpdateLocked (mockArchicad.port)))
            handleCommand (requestHandler, commandName)
        mockArchicad.HandleCommand = HandleCommand
        self.Patch (tapir_installer, 'StartArchicad', lambda archicadLocation : lockStates.append (('restart', self.IsUpdateLocked (mockArchicad.port))))
        exitCode, stdout, stderr = self.RunUpdate (mockArchicad)
        self.assertEqual (exitCode, 0, stderr)
        self.assertEqual (lockStates, [('GetArchicadLocation', True), ('QuitArchicad', True), ('restart', True)])
        self.assertFalse (self.IsUpdateLocked (mockArchicad.port))
        self.AssertAddOnIsNew ()

    def testLockIsReleasedAfterAFailure (self):
        # Archicad still runs, so the update can be started again.
        error = { 'code' : -2130313112, 'message' : 'Failed to quit Archicad!' }
        mockArchicad = self.StartMockArchicad (quits = False,
            quitResponseJson = { 'succeeded' : True, 'result' : { 'addOnCommandResponse' : { 'success' : False, 'error' : error } } })
        exitCode, stdout, stderr = self.RunUpdate (mockArchicad)
        self.assertEqual (exitCode, 1)
        self.assertIn ('Archicad did not quit (Failed to quit Archicad!)', stderr)
        self.assertFalse (self.IsUpdateLocked (mockArchicad.port))
        self.AssertAddOnIsOld ()

    def testLockOfHandOffIsWaitedFor (self):
        # The legacy update script holds the lock while it starts the
        # installer, and releases it right after.
        self.Patch (tapir_installer, 'UPDATE_LOCK_WAIT_SECONDS', 5)
        mockArchicad = self.StartMockArchicad ()
        releaseTimer = threading.Timer (0.5, self.HoldLegacyUpdateLock (mockArchicad.port))
        releaseTimer.daemon = True
        releaseTimer.start ()
        self.addCleanup (releaseTimer.cancel)
        startTime = time.monotonic ()
        exitCode, stdout, stderr = self.RunUpdate (mockArchicad)
        self.assertEqual (exitCode, 0, stderr)
        self.assertGreater (time.monotonic () - startTime, 0.4)
        self.assertEqual (self.startedArchicadLocations, [ARCHICAD_LOCATION])
        self.AssertAddOnIsNew ()

    def testDryRunTakesNoLock (self):
        mockArchicad = self.StartMockArchicad ()
        exitCode, stdout, stderr = self.RunUpdate (mockArchicad, '--dryRun')
        self.assertEqual (exitCode, 0, stderr)
        self.assertFalse (os.path.lexists (os.path.join (self.tempFolderPath, 'TapirUpdate_{0}.lock'.format (mockArchicad.port))))
        self.HoldLegacyUpdateLock (mockArchicad.port)
        exitCode, stdout, stderr = self.RunUpdate (mockArchicad, '--dryRun')
        self.assertEqual (exitCode, 0, stderr)
        self.assertIn ('Would quit Archicad on port {0}'.format (mockArchicad.port), stdout)
        self.assertEqual (mockArchicad.receivedCommands, [])
        self.AssertAddOnIsOld ()

    def RunUpdateWithUnusableLockFile (self, mockArchicad):
        # The update goes on unlocked.
        exitCode, stdout, stderr = self.RunUpdate (mockArchicad)
        self.assertEqual (exitCode, 0, stderr)
        self.assertEqual (mockArchicad.receivedCommands[:2], ['GetArchicadLocation', 'QuitArchicad'])
        self.assertEqual (self.startedArchicadLocations, [ARCHICAD_LOCATION])
        self.AssertAddOnIsNew ()

    def testLockFileThatCannotBeOpenedIsIgnored (self):
        mockArchicad = self.StartMockArchicad ()
        os.makedirs (os.path.join (self.tempFolderPath, 'TapirUpdate_{0}.lock'.format (mockArchicad.port)))
        self.RunUpdateWithUnusableLockFile (mockArchicad)

    def testLockFileThatCannotBeLockedIsIgnored (self):
        # For example on a file system without locks. An update without the
        # lock tells nothing to other updates.
        if os.name == 'nt':
            lockModule, lockFunctionName = importlib.import_module ('msvcrt'), 'locking'
        else:
            lockModule, lockFunctionName = importlib.import_module ('fcntl'), 'flock'
        self.Patch (lockModule, lockFunctionName, unittest.mock.Mock (side_effect = OSError (errno.ENOLCK, 'No locks available')))
        mockArchicad = self.StartMockArchicad ()
        self.RunUpdateWithUnusableLockFile (mockArchicad)
        self.assertEqual (self.ReadLockFile (mockArchicad.port), b'')

    def testStatesAlongAnUpdate (self):
        # Other updates, by the legacy update script or by another installer,
        # tell the user to quit Archicad only while this update waits for
        # Archicad to quit: before, quitting Archicad would make this update
        # fail, and after, it would quit the Archicad that is starting again.
        self.Patch (tapir_installer, 'UPDATE_LOCK_WAIT_SECONDS', 0)
        mockArchicad = self.StartMockArchicad (quitDelaySeconds = 0.5)
        archicadPort = mockArchicad.port
        seen = []
        def Record (step):
            seen.append ((step, self.ReadLockState (archicadPort)) + self.GetTextsOfAnotherUpdate (archicadPort))
        handleCommand = mockArchicad.HandleCommand
        def HandleCommand (requestHandler, commandName):
            Record (commandName)
            handleCommand (requestHandler, commandName)
        mockArchicad.HandleCommand = HandleCommand
        downloadAsset = tapir_installer.DownloadAsset
        self.Patch (tapir_installer, 'DownloadAsset', lambda *arguments : Record ('download') or downloadAsset (*arguments))
        self.Patch (tapir_installer, 'StartArchicad', lambda archicadLocation : Record ('restart'))
        exitCode, stdout, stderr = self.RunUpdate (mockArchicad)
        self.assertEqual (exitCode, 0, stderr)
        self.AssertAddOnIsNew ()
        Record ('after')
        updating = (b'updating', tapir_installer.UPDATE_RUNNING_TEXT, tapir_installer.UPDATE_RUNNING_TEXT)
        quitting = (b'quitting', tapir_installer.UPDATE_WAITING_FOR_QUIT_TEXT, tapir_installer.UPDATE_WAITING_FOR_QUIT_TEXT)
        expectedStates = { 'download' : updating, 'GetArchicadLocation' : updating, 'QuitArchicad' : quitting, 'GetAddOnVersion' : quitting,
            'restart' : updating, 'after' : (b'updating', None, None) }
        for entry in seen:
            self.assertEqual (entry[1:], expectedStates[entry[0]], entry[0])
        steps = [step for index, (step, *rest) in enumerate (seen) if index == 0 or seen[index - 1][0] != step]
        self.assertEqual (steps, ['download', 'GetArchicadLocation', 'QuitArchicad', 'GetAddOnVersion', 'restart', 'after'])

    def testStateIsResetWhenTheWaitEnds (self):
        # However the wait for Archicad to quit ends, Archicad still runs,
        # and other updates must not tell the user to quit it any more.
        self.Patch (tapir_installer, 'UPDATE_LOCK_WAIT_SECONDS', 0)
        self.Patch (tapir_installer, 'ARCHICAD_QUIT_TIMEOUT_SECONDS', 1)
        refusal = { 'succeeded' : True, 'result' : { 'addOnCommandResponse' : { 'success' : False, 'error' : { 'code' : -1, 'message' : 'Busy' } } } }
        def FailWait (archicadPort):
            raise KeyboardInterrupt ()
        cases = [
            ('refused', { 'quits' : False, 'quitResponseJson' : refusal }, None, None, tapir_installer.InstallerError),
            ('timeout', { 'quits' : False }, None, None, tapir_installer.InstallerError),
            ('cancelled', { 'quits' : False }, 0.5, None, tapir_installer.UpdateCancelledError),
            ('raised', { 'quits' : False }, None, FailWait, KeyboardInterrupt),
        ]
        for name, mockArguments, cancelSeconds, isArchicadAnswering, expectedError in cases:
            with self.subTest (name):
                mockArchicad = self.StartMockArchicad (**mockArguments)
                archicadPort = mockArchicad.port
                seen = []
                handleCommand = mockArchicad.HandleCommand
                def HandleCommand (requestHandler, commandName):
                    if commandName == 'QuitArchicad':
                        seen.append ((self.ReadLockState (archicadPort),) + self.GetTextsOfAnotherUpdate (archicadPort))
                    handleCommand (requestHandler, commandName)
                mockArchicad.HandleCommand = HandleCommand
                updateTarget = self.CreateUpdateTarget (archicadPort, restartArchicad = True)
                cancelEvent = self.CreateCancelEvent (cancelSeconds) if cancelSeconds is not None else None
                with unittest.mock.patch.object (tapir_installer, 'IsArchicadAnswering', isArchicadAnswering or tapir_installer.IsArchicadAnswering):
                    with tapir_installer.UpdateLock (updateTarget, False) as setWaitingForQuit:
                        with self.assertRaises (expectedError):
                            tapir_installer.UpdateAddOn (updateTarget, self.releaseInfo, False, cancelEvent = cancelEvent, setWaitingForQuit = setWaitingForQuit)
                        self.assertEqual (seen, [(b'quitting', tapir_installer.UPDATE_WAITING_FOR_QUIT_TEXT, tapir_installer.UPDATE_WAITING_FOR_QUIT_TEXT)])
                        self.assertEqual (self.ReadLockState (archicadPort), b'updating')
                        self.assertEqual (self.GetTextsOfAnotherUpdate (archicadPort), (tapir_installer.UPDATE_RUNNING_TEXT, tapir_installer.UPDATE_RUNNING_TEXT))
                self.assertFalse (self.IsUpdateLocked (archicadPort))
                self.assertEqual (self.startedArchicadLocations, [])
                self.AssertAddOnIsOld ()

    def testStateOfAKilledUpdateIsOverwritten (self):
        # An update that was killed while it waited for Archicad to quit left
        # its state.
        self.Patch (tapir_installer, 'UPDATE_LOCK_WAIT_SECONDS', 0)
        mockArchicad = self.StartMockArchicad ()
        with open (self.LockFilePath (mockArchicad.port), 'wb') as file:
            file.write (b'\x00quitting')
        seen = []
        getArchicadLocation = tapir_installer.GetArchicadLocation
        def GetArchicadLocation (archicadPort):
            seen.append ((self.ReadLockState (archicadPort),) + self.GetTextsOfAnotherUpdate (archicadPort))
            return getArchicadLocation (archicadPort)
        self.Patch (tapir_installer, 'GetArchicadLocation', GetArchicadLocation)
        exitCode, stdout, stderr = self.RunUpdate (mockArchicad)
        self.assertEqual (exitCode, 0, stderr)
        self.assertEqual (seen, [(b'updating', tapir_installer.UPDATE_RUNNING_TEXT, tapir_installer.UPDATE_RUNNING_TEXT)])
        self.assertEqual (self.ReadLockFile (mockArchicad.port), b'\x00updating')
        self.AssertAddOnIsNew ()

    def testLockedByteIsNotTouched (self):
        # On Windows the first byte is the locked one, which cannot be read
        # or written through any other handle, not even one of the same
        # process; the state is beside it.
        mockArchicad = self.StartMockArchicad ()
        with open (self.LockFilePath (mockArchicad.port), 'wb') as file:
            file.write (b'Z')
        exitCode, stdout, stderr = self.RunUpdate (mockArchicad)
        self.assertEqual (exitCode, 0, stderr)
        self.assertEqual (self.ReadLockFile (mockArchicad.port), b'Zupdating')
        # Another update holds the lock: only the state is read, once the
        # wait for the lock has ended (the lock is retried meanwhile, at the
        # first byte).
        release = self.HoldLockWithContent (mockArchicad.port, b'Zquitting')
        accesses = []
        lseek, read, write = os.lseek, os.read, os.write
        with unittest.mock.patch.object (os, 'lseek', lambda fd, position, how : accesses.append (('seek', position, how)) or lseek (fd, position, how)), \
                unittest.mock.patch.object (os, 'read', lambda fd, count : accesses.append (('read', count)) or read (fd, count)), \
                unittest.mock.patch.object (os, 'write', lambda fd, data : accesses.append (('write', data)) or write (fd, data)):
            with self.assertRaises (tapir_installer.UpdateRunningError) as context:
                with tapir_installer.UpdateLock (self.CreateUpdateTarget (mockArchicad.port), False):
                    self.fail ('ran while locked')
        self.assertEqual (str (context.exception), tapir_installer.UPDATE_WAITING_FOR_QUIT_TEXT)
        # (The emulated msvcrt of the tests asks for the position when it locks.)
        self.assertEqual ([access for access in accesses if access != ('seek', 0, os.SEEK_CUR)], [('seek', 1, os.SEEK_SET), ('read', 8)])
        release ()
        self.assertEqual (self.ReadLockFile (mockArchicad.port), b'Zquitting')

    def testTextOfAStoppedUpdateByState (self):
        # Only an exact 'quitting' beside the locked byte tells the user to
        # quit Archicad.
        self.Patch (tapir_installer, 'UPDATE_LOCK_WAIT_SECONDS', 0)
        cases = [
            (b'\x00quitting', tapir_installer.UPDATE_WAITING_FOR_QUIT_TEXT),
            (b'\x00quittingX', tapir_installer.UPDATE_WAITING_FOR_QUIT_TEXT),
            (b'\x00updating', tapir_installer.UPDATE_RUNNING_TEXT),
            # An update without a state (the installer of 1.6.0 had no lock,
            # but a newer one may write none).
            (b'', tapir_installer.UPDATE_RUNNING_TEXT),
            (b'\x00', tapir_installer.UPDATE_RUNNING_TEXT),
            # Cut off, not beside the locked byte, or another case.
            (b'\x00quit', tapir_installer.UPDATE_RUNNING_TEXT),
            (b'quitting', tapir_installer.UPDATE_RUNNING_TEXT),
            (b'\x00QUITTING', tapir_installer.UPDATE_RUNNING_TEXT),
        ]
        mockArchicad = self.StartMockArchicad ()
        for content, expectedText in cases:
            with self.subTest (content = content):
                release = self.HoldLockWithContent (mockArchicad.port, content)
                exitCode, stdout, stderr = self.RunUpdate (mockArchicad)
                self.AssertUpdateWasStopped (mockArchicad, exitCode, stderr, expectedText)
                if expectedText == tapir_installer.UPDATE_RUNNING_TEXT:
                    self.assertNotIn ('Quit Archicad', stderr)
                # The stopped update writes nothing.
                release ()
                self.assertEqual (self.ReadLockFile (mockArchicad.port), content)

    def testUnreadableStateGivesTheTextWithoutQuitting (self):
        self.Patch (tapir_installer, 'UPDATE_LOCK_WAIT_SECONDS', 0)
        self.HoldLockWithContent (12345, b'\x00quitting')
        def Read (fd, count):
            raise OSError (errno.EIO, 'Input/output error')
        with unittest.mock.patch.object (os, 'read', Read):
            with self.assertRaises (tapir_installer.UpdateRunningError) as context:
                with tapir_installer.UpdateLock (self.CreateUpdateTarget (12345), False):
                    self.fail ('ran while locked')
        self.assertEqual (str (context.exception), tapir_installer.UPDATE_RUNNING_TEXT)

    def testStateThatCannotBeWrittenDoesNotStopTheUpdate (self):
        mockArchicad = self.StartMockArchicad ()
        write = os.write
        lseek = os.lseek
        stateWrites = []
        def Write (fd, data):
            if data in [b'quitting', b'updating']:
                stateWrites.append (data)
                raise OSError (errno.ENOSPC, 'No space left on device')
            return write (fd, data)
        def Seek (fd, position, how):
            if (position, how) == (1, os.SEEK_SET) and len (stateWrites) == 2:
                # Also when seeking fails, while waiting for Archicad to quit.
                raise OSError (errno.EIO, 'Input/output error')
            return lseek (fd, position, how)
        self.Patch (os, 'write', Write)
        self.Patch (os, 'lseek', Seek)
        exitCode, stdout, stderr = self.RunUpdate (mockArchicad)
        self.assertEqual (exitCode, 0, stderr)
        self.assertEqual (stateWrites, [b'updating', b'quitting'])
        self.assertEqual (self.startedArchicadLocations, [ARCHICAD_LOCATION])
        self.AssertAddOnIsNew ()

    def testDryRunAndUnlockedUpdatesTellNothing (self):
        # Without the lock the state function does nothing.
        mockArchicad = self.StartMockArchicad ()
        with tapir_installer.UpdateLock (self.CreateUpdateTarget (mockArchicad.port), True) as setWaitingForQuit:
            setWaitingForQuit (True)
        with tapir_installer.UpdateLock (self.CreateUpdateTarget (), False) as setWaitingForQuit:
            setWaitingForQuit (True)
        self.assertFalse (os.path.lexists (self.LockFilePath (mockArchicad.port)))
        fcntlOrMsvcrt = ('msvcrt', 'locking') if os.name == 'nt' else ('fcntl', 'flock')
        with unittest.mock.patch.object (importlib.import_module (fcntlOrMsvcrt[0]), fcntlOrMsvcrt[1], unittest.mock.Mock (side_effect = OSError (errno.ENOLCK, 'No locks available'))):
            with tapir_installer.UpdateLock (self.CreateUpdateTarget (mockArchicad.port), False) as setWaitingForQuit:
                setWaitingForQuit (True)
        self.assertEqual (self.ReadLockFile (mockArchicad.port), b'')


class StartArchicadTests (unittest.TestCase):
    def testMacStartsANewInstance (self):
        # Archicad is started once its port closed: an instance that is still
        # there is exiting, and plain open would only activate it.
        archicadLocation = '/Applications/GRAPHISOFT/Archicad 29/Archicad 29.app'
        with unittest.mock.patch.object (tapir_installer, 'IsUsingMacOS', lambda : True), unittest.mock.patch.object (tapir_installer.subprocess, 'Popen') as popen:
            tapir_installer.StartArchicad (archicadLocation)
        popen.assert_called_once_with (['open', '-n', archicadLocation])


class ArgumentTests (UpdateModeTestBase):
    def testInvalidArguments (self):
        missingPath = os.path.join (self.tempFolderPath, 'Missing', os.path.basename (self.targetPath))
        if IS_MAC:
            wrongKindPath = os.path.join (self.tempFolderPath, 'Release', 'TapirAddOn_AC29_Mac.zip')
        else:
            wrongKindPath = self.addOnsFolderPath
        update = ['--console', '--addOnFile', self.targetPath]
        cases = [
            (update, '--addOnFile requires --versions with exactly one Archicad version.'),
            (update + ['--versions', '28,29'], '--addOnFile requires --versions with exactly one Archicad version.'),
            (update + ['--versions', 'x'], 'Invalid --versions value'),
            (update + ['--versions', '29', '--addOnsFolder', self.addOnsFolderPath], 'cannot be combined'),
            (update + ['--versions', '29', '--uninstall'], 'cannot be combined'),
            (update + ['--versions', '29', '--mockRoot', self.tempFolderPath], 'cannot be combined'),
            (update + ['--versions', '29', '--archicadPid', '5'], '--archicadPid requires --archicadPort.'),
            (update + ['--versions', '29', '--archicadPort', '70000'], 'Invalid --archicadPort value'),
            (update + ['--versions', '29', '--archicadPort', '19723', '--archicadPid', '0'], 'Invalid --archicadPid value'),
            (['--console', '--addOnFile', '', '--versions', '29'], '--addOnFile requires the path'),
            (['--console', '--addOnFile', missingPath, '--versions', '29'], 'was not found'),
            (['--console', '--addOnFile', wrongKindPath, '--versions', '29'], 'Not an add-on'),
            (['--console', '--archicadPort', '19723'], 'require --addOnFile'),
            (['--console', '--versions', '29', '--archicadPid', '5'], 'require --addOnFile'),
        ]
        for arguments, expectedError in cases:
            with self.subTest (arguments = arguments):
                exitCode, stdout, stderr = self.RunMain (arguments)
                self.assertEqual (exitCode, 1)
                self.assertIn (expectedError, stderr)
        # Arguments are checked before anything else happens.
        self.assertEqual (self.releaseInfoRequestCount, 0)
        self.AssertAddOnIsOld ()

    def testInvalidNumberIsAnArgumentError (self):
        exitCode, stdout, stderr = self.RunMain (['--console', '--addOnFile', self.targetPath, '--versions', '29', '--archicadPort', 'abc'])
        self.assertEqual (exitCode, 2)
        self.assertIn ('--archicadPort', stderr)

    def testUnknownOptionsAreIgnored (self):
        # Options are never abbreviated, so a new option that starts like an
        # existing one is ignored too, instead of being read as that option
        # or being ambiguous, which would make the parser exit.
        update = ['--console', '--dryRun', '--addOnFile', self.targetPath, '--versions', '29']
        cases = [
            ['--futureOption', 'value', '--futureFlag'],
            ['--archicad', 'x'],
            ['--addOn', 'y'],
            ['--version', '1'],
            ['--archicadP', '1', '--addOnF', 'z', '--noRestar', '--uninst'],
        ]
        for unknownArguments in cases:
            with self.subTest (unknownArguments = unknownArguments):
                exitCode, stdout, stderr = self.RunMain (update + unknownArguments)
                self.assertEqual (exitCode, 0, stderr)
                self.assertIn ('Ignoring unknown options: {0}'.format (' '.join (unknownArguments)), stderr)
                self.assertIn ('Would replace {0} with {1}'.format (self.targetPath, self.releaseInfo['assets'][0]['name']), stdout)
        # Nothing of them changes the parsed options.
        parsedOptions = []
        self.Patch (tapir_installer, 'RunConsoleInstaller', lambda args : parsedOptions.append (vars (args)) or 0)
        self.RunMain (update)
        self.RunMain (update + sum (cases, []))
        self.assertEqual (len (parsedOptions), 2)
        self.assertEqual (parsedOptions[1], parsedOptions[0])

    def testModeRouting (self):
        calledModes = []
        self.Patch (tapir_installer, 'RunGuiInstaller', lambda args : calledModes.append ('gui') or 0)
        self.Patch (tapir_installer, 'RunConsoleInstaller', lambda args : calledModes.append ('console') or 0)
        cases = [
            # Update mode shows the GUI although --versions is given.
            (['--addOnFile', self.targetPath, '--versions', '29', '--archicadPort', '19723'], 'gui'),
            (['--addOnFile', self.targetPath, '--versions', '29', '--dryRun'], 'gui'),
            (['--console', '--addOnFile', self.targetPath, '--versions', '29'], 'console'),
            # The routing without update mode is unchanged.
            ([], 'gui'),
            (['--mockRoot', self.tempFolderPath], 'gui'),
            (['--versions', '29'], 'console'),
            (['--dryRun'], 'console'),
            (['--uninstall'], 'console'),
            (['--addOnsFolder', self.addOnsFolderPath], 'console'),
        ]
        for arguments, expectedMode in cases:
            with self.subTest (arguments = arguments):
                del calledModes[:]
                self.RunMain (arguments)
                self.assertEqual (calledModes, [expectedMode])


if __name__ == '__main__':
    unittest.main (verbosity = 2)
