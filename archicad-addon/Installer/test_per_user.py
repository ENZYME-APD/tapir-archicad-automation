"""Tests of the "Install for me" mode of the Tapir Installer (the default).

Standard library only, and offline: the registry (Windows) and 'defaults'
(macOS) are replaced by dicts, as the community test runner's tests do, and
whether Archicad runs or the installer is elevated is set by each test. Every
case checks the stored add-on lists, not only the return codes. The functions
copied from the community test runner (tools/community-test-runner/
runner_windows.py, runner_macos.py) are compared with their twins.

Run: python test_per_user.py
"""

import contextlib
import io
import json
import os
import pathlib
import shutil
import sys
import tempfile
import unittest
import unittest.mock

INSTALLER_FOLDER_PATH = os.path.dirname (os.path.abspath (__file__))
sys.path.insert (0, INSTALLER_FOLDER_PATH)
import tapir_installer

RUNNER_FOLDER_PATH = os.path.join (os.path.dirname (os.path.dirname (INSTALLER_FOLDER_PATH)), 'tools', 'community-test-runner')
ARCHICAD_KEY = r'Software\GRAPHISOFT\Archicad'


def ManagerKey (version):
    return '{0}\\Archicad {1}.0.0 INT R1\\Add-On Manager'.format (ARCHICAD_KEY, version)


class FakeRegistry:
    # The registry as {key path: {value name: [value, kind]}}, as in the
    # community test runner's tests.
    def __init__ (self, keys):
        self.keys = { key : { name : list (value) for name, value in values.items () } for key, values in keys.items () }
        self.writeCount = 0

    def subkeys (self, path):
        prefix = path + '\\'
        return sorted ({ key[len (prefix):].split ('\\')[0] for key in self.keys if key.startswith (prefix) })

    def read (self, path):
        return { name : list (value) for name, value in self.keys.get (path, {}).items () }

    def write (self, path, values):
        self.writeCount += 1
        self.keys[path] = { name : list (value) for name, value in values.items () }

    def Dump (self):
        return json.dumps (self.keys, sort_keys = True)


class FakePreferences:
    # The preferences domains as {domain: {key: value}}, as in the community
    # test runner's tests.
    def __init__ (self, domains):
        self.data = json.loads (json.dumps (domains))
        self.writeCount = 0

    def domains (self):
        return sorted (self.data)

    def read (self, domain):
        return json.loads (json.dumps (self.data.get (domain, {})))

    def write (self, domain, key, value):
        self.writeCount += 1
        self.data.setdefault (domain, {})[key] = json.loads (json.dumps (value))

    def Dump (self):
        return json.dumps (self.data, sort_keys = True)


class PerUserTestBase (unittest.TestCase):
    isMac = False

    def setUp (self):
        self.tempFolderPath = tempfile.mkdtemp (prefix = 'TapirPerUserTest_')
        self.addCleanup (shutil.rmtree, self.tempFolderPath, True)
        self.perUserFolderPath = os.path.join (self.tempFolderPath, 'User', 'Tapir')
        self.Patch (tapir_installer, 'GetPerUserFolderPath', lambda : self.perUserFolderPath)
        self.Patch (tapir_installer, 'IsUsingMacOS', lambda : self.isMac)
        self.isElevated = False
        self.Patch (tapir_installer, 'IsRunningAsAdministratorWin', lambda : self.isElevated)
        self.isArchicadRunning = False
        self.Patch (tapir_installer, 'IsArchicadRunning', lambda installation : self.isArchicadRunning)
        self.Patch (tapir_installer, 'LANGUAGE', tapir_installer.DEFAULT_LANGUAGE)
        self.Patch (tapir_installer, 'GetSystemLanguageTags', lambda : [])
        # Tests must never start anything as administrator, nor ask for the
        # macOS administrator password.
        self.startedAsAdministrator = []
        self.Patch (tapir_installer, 'StartAsAdministratorWin', self.startedAsAdministrator.append)
        self.Patch (tapir_installer, 'RunShellCommandWithAdminPrivilegesMac', unittest.mock.Mock (side_effect = AssertionError ('administrator privileges requested')))
        self.downloadedFilePath = os.path.join (self.tempFolderPath, 'Download', 'TapirAddOn_AC30_Win.apx')
        os.makedirs (os.path.dirname (self.downloadedFilePath))
        with open (self.downloadedFilePath, 'w') as downloadedFile:
            downloadedFile.write ('MZ new')

    def Patch (self, target, attributeName, value):
        patcher = unittest.mock.patch.object (target, attributeName, value)
        patcher.start ()
        self.addCleanup (patcher.stop)

    def CreateInstallation (self, version, withAllUsersTapir = False):
        installPath = os.path.join (self.tempFolderPath, 'Program Files', 'Graphisoft', 'Archicad {0}'.format (version))
        addOnsFolderPath = os.path.join (installPath, 'Add-Ons')
        os.makedirs (addOnsFolderPath, exist_ok = True)
        if withAllUsersTapir:
            tapirFolderPath = os.path.join (addOnsFolderPath, 'Tapir')
            os.makedirs (tapirFolderPath, exist_ok = True)
            if self.isMac:
                os.makedirs (os.path.join (tapirFolderPath, 'TapirAddOn_AC{0}_Mac.bundle'.format (version), 'Contents'), exist_ok = True)
            else:
                with open (os.path.join (tapirFolderPath, 'TapirAddOn_AC{0}_Win.apx'.format (version)), 'w') as addOnFile:
                    addOnFile.write ('MZ all users')
        return tapir_installer.ArchicadInstallation (version, installPath, addOnsFolderPath)

    def RunMain (self, arguments):
        stdout = io.StringIO ()
        stderr = io.StringIO ()
        with unittest.mock.patch.object (sys, 'argv', ['tapir_installer.py'] + arguments), contextlib.redirect_stdout (stdout), contextlib.redirect_stderr (stderr):
            try:
                exitCode = tapir_installer.Main ()
            except SystemExit as e:
                exitCode = e.code
        return exitCode, stdout.getvalue (), stderr.getvalue ()


class WindowsListTests (PerUserTestBase):
    # The registry layout of Windows, on every platform.
    def setUp (self):
        PerUserTestBase.setUp (self)
        self.otherAddOnPath = os.path.join (self.tempFolderPath, 'Other', 'Other.apx')
        self.disabledAddOnPath = os.path.join (self.tempFolderPath, 'Other', 'Disabled.apx')
        self.oldTapirPath = os.path.join (self.tempFolderPath, 'Old', 'TapirAddOn_AC30_Win.apx')
        keys = {}
        for version in [28, 29, 30]:
            key = ManagerKey (version)
            keys[key] = { 'Actual Path' : [os.path.join (self.tempFolderPath, 'Add-Ons'), 1] }
            keys[key + '\\Include'] = tapir_installer.numbered ('Include', [tapir_installer.to_url (self.otherAddOnPath)])
            keys[key + '\\Disable'] = tapir_installer.numbered ('Disable', [tapir_installer.to_url (self.disabledAddOnPath)])
        self.registry = FakeRegistry (keys)
        self.Patch (tapir_installer, 'CreateAddOnList', lambda : tapir_installer.AddOnListWin (self.registry))

    def Lists (self, version):
        key = ManagerKey (version)
        return (tapir_installer.entries (self.registry.read (key + '\\Include')), tapir_installer.entries (self.registry.read (key + '\\Disable')))

    def Count (self, version, kind):
        return self.registry.read (ManagerKey (version) + '\\' + kind)['{0} Number'.format (kind)][0]

    def UserAddOnUrl (self, version):
        return tapir_installer.to_url (tapir_installer.GetPerUserAddOnPath (version))

    def testFreshInstall (self):
        installation = self.CreateInstallation (30)
        targetPath = tapir_installer.InstallAddOnForMe (installation, self.downloadedFilePath)
        self.assertEqual (targetPath, os.path.join (self.perUserFolderPath, 'AC30', 'TapirAddOn_AC30_Win.apx'))
        with open (targetPath) as addOnFile:
            self.assertEqual (addOnFile.read (), 'MZ new')
        include, disable = self.Lists (30)
        self.assertEqual (include, [tapir_installer.to_url (self.otherAddOnPath), self.UserAddOnUrl (30)])
        self.assertEqual (self.Count (30, 'Include'), '2')
        self.assertEqual (disable, [tapir_installer.to_url (self.disabledAddOnPath)])
        self.assertEqual (self.Count (30, 'Disable'), '1')
        # The lists as they were are saved before they are written.
        with open (os.path.join (self.perUserFolderPath, 'addon-list-backup-AC30.json')) as backupFile:
            backup = json.load (backupFile)
        self.assertEqual (backup['location'], ManagerKey (30))
        self.assertEqual (tapir_installer.entries (backup['lists']['Include']), [tapir_installer.to_url (self.otherAddOnPath)])
        self.assertEqual (backup['addedToDisable'], [])
        # The other versions are untouched.
        self.assertEqual (self.Lists (29), ([tapir_installer.to_url (self.otherAddOnPath)], [tapir_installer.to_url (self.disabledAddOnPath)]))
        self.assertFalse (os.path.lexists (os.path.join (self.perUserFolderPath, 'AC29')))

    def testReinstallKeepsOneEntry (self):
        installation = self.CreateInstallation (30)
        tapir_installer.InstallAddOnForMe (installation, self.downloadedFilePath)
        with open (self.downloadedFilePath, 'w') as downloadedFile:
            downloadedFile.write ('MZ newer')
        targetPath = tapir_installer.InstallAddOnForMe (installation, self.downloadedFilePath)
        with open (targetPath) as addOnFile:
            self.assertEqual (addOnFile.read (), 'MZ newer')
        self.assertEqual (os.listdir (os.path.dirname (targetPath)), [os.path.basename (targetPath)])
        include, disable = self.Lists (30)
        self.assertEqual (include, [tapir_installer.to_url (self.otherAddOnPath), self.UserAddOnUrl (30)])
        self.assertEqual (self.Count (30, 'Include'), '2')
        self.assertEqual (disable, [tapir_installer.to_url (self.disabledAddOnPath)])

    def testOtherAddOnsStayAsTheyWere (self):
        # Other add-ons keep their order and their entries; another Tapir on
        # Include is replaced, and the user's file is taken off Disable.
        secondAddOnPath = os.path.join (self.tempFolderPath, 'Other', 'Second.apx')
        key = ManagerKey (30)
        self.registry.write (key + '\\Include', tapir_installer.numbered ('Include', [tapir_installer.to_url (self.otherAddOnPath),
            tapir_installer.to_url (self.oldTapirPath), tapir_installer.to_url (secondAddOnPath)]))
        self.registry.write (key + '\\Disable', tapir_installer.numbered ('Disable', [self.UserAddOnUrl (30), tapir_installer.to_url (self.disabledAddOnPath)]))
        actualPath = self.registry.read (key)
        tapir_installer.InstallAddOnForMe (self.CreateInstallation (30), self.downloadedFilePath)
        include, disable = self.Lists (30)
        self.assertEqual (include, [tapir_installer.to_url (self.otherAddOnPath), tapir_installer.to_url (secondAddOnPath), self.UserAddOnUrl (30)])
        self.assertEqual (self.Count (30, 'Include'), '3')
        self.assertEqual (disable, [tapir_installer.to_url (self.disabledAddOnPath)])
        self.assertEqual (self.Count (30, 'Disable'), '1')
        self.assertEqual (self.registry.read (key), actualPath)

    def testArchicadRunningWritesNothing (self):
        before = self.registry.Dump ()
        self.isArchicadRunning = True
        with self.assertRaises (tapir_installer.ArchicadRunningError) as context:
            tapir_installer.InstallAddOnForMe (self.CreateInstallation (30), self.downloadedFilePath)
        self.assertEqual (str (context.exception), 'Close Archicad 30 to finish installing Tapir.')
        self.assertEqual (self.registry.Dump (), before)
        self.assertEqual (self.registry.writeCount, 0)
        self.assertFalse (os.path.lexists (self.perUserFolderPath))

    def testNoAddOnManagerKeyYet (self):
        before = self.registry.Dump ()
        installation = self.CreateInstallation (31)
        with self.assertRaises (tapir_installer.ArchicadNotStartedError) as context:
            tapir_installer.InstallAddOnForMe (installation, self.downloadedFilePath)
        self.assertEqual (str (context.exception), 'Start Archicad 31 once and close it, then press Retry.')
        # The file is copied, the registry is not written.
        self.assertTrue (os.path.isfile (tapir_installer.GetPerUserAddOnPath (31)))
        self.assertEqual (self.registry.Dump (), before)
        self.assertFalse (os.path.lexists (os.path.join (self.perUserFolderPath, 'addon-list-backup-AC31.json')))

    def testNoAddOnManagerKeyYetInConsoleExitsWith3 (self):
        installation = self.CreateInstallation (31)
        self.Patch (tapir_installer, 'DetectArchicadInstallations', lambda mockRootPath = None : [installation])
        releaseInfo = { 'tag_name' : '9.9.9', 'assets' : [{ 'name' : 'TapirAddOn_AC31_Win.apx',
            'browser_download_url' : pathlib.Path (self.downloadedFilePath).as_uri (), 'size' : os.path.getsize (self.downloadedFilePath) }] }
        self.Patch (tapir_installer, 'GetLatestReleaseInfo', lambda : releaseInfo)
        before = self.registry.Dump ()
        exitCode, stdout, stderr = self.RunMain (['--console', '--forMe', '--versions', '31'])
        self.assertEqual (exitCode, tapir_installer.EXIT_CODE_START_ARCHICAD_ONCE, stderr)
        self.assertIn ('Start Archicad 31 once and close it, then run the installer again.', stderr)
        self.assertTrue (os.path.isfile (tapir_installer.GetPerUserAddOnPath (31)))
        self.assertEqual (self.registry.Dump (), before)

    def testConsoleInstallForMe (self):
        installation = self.CreateInstallation (30)
        self.Patch (tapir_installer, 'DetectArchicadInstallations', lambda mockRootPath = None : [installation])
        releaseInfo = { 'tag_name' : '9.9.9', 'assets' : [{ 'name' : 'TapirAddOn_AC30_Win.apx',
            'browser_download_url' : pathlib.Path (self.downloadedFilePath).as_uri (), 'size' : os.path.getsize (self.downloadedFilePath) }] }
        self.Patch (tapir_installer, 'GetLatestReleaseInfo', lambda : releaseInfo)
        # "For me" is the default.
        exitCode, stdout, stderr = self.RunMain (['--console', '--versions', '30'])
        self.assertEqual (exitCode, 0, stderr)
        self.assertIn ('for you into', stdout)
        self.assertEqual (self.Lists (30)[0], [tapir_installer.to_url (self.otherAddOnPath), self.UserAddOnUrl (30)])
        self.assertEqual (self.startedAsAdministrator, [])

    def testAllUsersTapirUpToArchicad29IsNotOverridden (self):
        for version in [28, 29]:
            with self.subTest (version = version):
                before = self.registry.Dump ()
                with self.assertRaises (tapir_installer.AllUsersTapirError) as context:
                    tapir_installer.InstallAddOnForMe (self.CreateInstallation (version, withAllUsersTapir = True), self.downloadedFilePath)
                self.assertEqual (str (context.exception), 'Tapir for Archicad {0} is installed for all users, and it is updated there. '
                    'Ask your administrator to remove it to install your own copy.'.format (version))
                self.assertEqual (self.registry.Dump (), before)
                self.assertFalse (os.path.lexists (tapir_installer.GetPerUserAddOnPath (version)))

    def testAllUsersTapirFromArchicad30IsDisabled (self):
        installation = self.CreateInstallation (30, withAllUsersTapir = True)
        allUsersTapirPath = os.path.join (installation.addOnsFolderPath, 'Tapir', 'TapirAddOn_AC30_Win.apx')
        tapir_installer.InstallAddOnForMe (installation, self.downloadedFilePath)
        include, disable = self.Lists (30)
        self.assertEqual (include, [tapir_installer.to_url (self.otherAddOnPath), self.UserAddOnUrl (30)])
        self.assertEqual (disable, [tapir_installer.to_url (self.disabledAddOnPath), tapir_installer.to_url (allUsersTapirPath)])
        self.assertEqual (self.Count (30, 'Disable'), '2')
        # The file of all users is left as it is.
        self.assertTrue (os.path.isfile (allUsersTapirPath))
        # Installing again does not add it twice, and keeps it the installer's.
        tapir_installer.InstallAddOnForMe (installation, self.downloadedFilePath)
        self.assertEqual (self.Lists (30)[1], [tapir_installer.to_url (self.disabledAddOnPath), tapir_installer.to_url (allUsersTapirPath)])
        backup = tapir_installer.ReadAddOnListBackup (30)
        self.assertEqual (backup['addedToDisable'], [tapir_installer.to_url (allUsersTapirPath)])
        self.assertEqual (tapir_installer.entries (backup['lists']['Disable']), [tapir_installer.to_url (self.disabledAddOnPath)])

    def testUninstall (self):
        installation = self.CreateInstallation (30, withAllUsersTapir = True)
        allUsersTapirPath = os.path.join (installation.addOnsFolderPath, 'Tapir', 'TapirAddOn_AC30_Win.apx')
        tapir_installer.InstallAddOnForMe (installation, self.downloadedFilePath)
        # An add-on the user adds after the install stays.
        addedLaterPath = os.path.join (self.tempFolderPath, 'Other', 'Later.apx')
        include, disable = self.Lists (30)
        self.registry.write (ManagerKey (30) + '\\Include', tapir_installer.numbered ('Include', include + [tapir_installer.to_url (addedLaterPath)]))
        self.assertTrue (tapir_installer.UninstallAddOnForMe (installation))
        include, disable = self.Lists (30)
        self.assertEqual (include, [tapir_installer.to_url (self.otherAddOnPath), tapir_installer.to_url (addedLaterPath)])
        self.assertEqual (self.Count (30, 'Include'), '2')
        self.assertEqual (disable, [tapir_installer.to_url (self.disabledAddOnPath)])
        self.assertEqual (self.Count (30, 'Disable'), '1')
        self.assertFalse (os.path.lexists (os.path.join (self.perUserFolderPath, 'AC30')))
        self.assertFalse (os.path.lexists (os.path.join (self.perUserFolderPath, 'addon-list-backup-AC30.json')))
        self.assertTrue (os.path.isfile (allUsersTapirPath))
        # Nothing left to remove.
        writeCount = self.registry.writeCount
        self.assertFalse (tapir_installer.UninstallAddOnForMe (installation))
        self.assertEqual (self.registry.writeCount, writeCount)

    def testUninstallWhileArchicadRunsWritesNothing (self):
        installation = self.CreateInstallation (30)
        tapir_installer.InstallAddOnForMe (installation, self.downloadedFilePath)
        before = self.registry.Dump ()
        self.isArchicadRunning = True
        with self.assertRaises (tapir_installer.ArchicadRunningError):
            tapir_installer.UninstallAddOnForMe (installation)
        self.assertEqual (self.registry.Dump (), before)
        self.assertTrue (os.path.isfile (tapir_installer.GetPerUserAddOnPath (30)))

    def testUninstallForMeRemovesTheAllUsersCopyWhenAllowed (self):
        installation = self.CreateInstallation (30, withAllUsersTapir = True)
        tapir_installer.InstallAddOnForMe (installation, self.downloadedFilePath)
        self.assertTrue (tapir_installer.UninstallTapir (installation, tapir_installer.INSTALL_FOR_ME))
        self.assertFalse (os.path.lexists (os.path.join (installation.addOnsFolderPath, 'Tapir')))
        self.assertEqual (self.Lists (30)[0], [tapir_installer.to_url (self.otherAddOnPath)])

    def testUninstallForMeSaysWhenTheAllUsersCopyIsLeft (self):
        installation = self.CreateInstallation (30, withAllUsersTapir = True)
        tapir_installer.InstallAddOnForMe (installation, self.downloadedFilePath)
        self.Patch (tapir_installer, 'UninstallAddOn', unittest.mock.Mock (side_effect = tapir_installer.InstallerError (tapir_installer.PERMISSION_DENIED_TEXT)))
        with self.assertRaises (tapir_installer.InstallerError) as context:
            tapir_installer.UninstallTapir (installation, tapir_installer.INSTALL_FOR_ME)
        self.assertIn ('is also installed for all users', str (context.exception))
        # The user's own copy is gone all the same.
        self.assertFalse (os.path.lexists (tapir_installer.GetPerUserAddOnPath (30)))
        self.assertEqual (self.Lists (30)[0], [tapir_installer.to_url (self.otherAddOnPath)])

    def testElevatedForMeRefusesToWrite (self):
        self.isElevated = True
        before = self.registry.Dump ()
        installation = self.CreateInstallation (30)
        with self.assertRaises (tapir_installer.InstallerError) as context:
            tapir_installer.InstallAddOnForMe (installation, self.downloadedFilePath)
        self.assertIn ('runs as administrator', str (context.exception))
        with self.assertRaises (tapir_installer.InstallerError):
            tapir_installer.UninstallAddOnForMe (installation)
        self.assertEqual (self.registry.Dump (), before)
        self.assertFalse (os.path.lexists (self.perUserFolderPath))
        # In console mode too, before anything is downloaded.
        self.Patch (tapir_installer, 'DetectArchicadInstallations', unittest.mock.Mock (side_effect = AssertionError ('detected')))
        self.Patch (tapir_installer, 'GetLatestReleaseInfo', unittest.mock.Mock (side_effect = AssertionError ('downloaded')))
        exitCode, stdout, stderr = self.RunMain (['--console', '--forMe', '--versions', '30'])
        self.assertEqual (exitCode, 1)
        self.assertIn ('runs as administrator', stderr)
        self.assertEqual (self.registry.Dump (), before)

    def testInstallModes (self):
        def Mode (arguments):
            parsed = unittest.mock.Mock (forMe = '--forMe' in arguments, allUsers = '--allUsers' in arguments,
                addOnsFolderPath = 'x' if '--addOnsFolder' in arguments else None, mockRootPath = 'x' if '--mockRoot' in arguments else None)
            return tapir_installer.GetInstallMode (parsed)
        self.assertEqual (Mode ([]), tapir_installer.INSTALL_FOR_ME)
        self.assertEqual (Mode (['--allUsers']), tapir_installer.INSTALL_FOR_ALL_USERS)
        self.assertEqual (Mode (['--addOnsFolder']), tapir_installer.INSTALL_FOR_ALL_USERS)
        self.assertEqual (Mode (['--mockRoot']), tapir_installer.INSTALL_FOR_ALL_USERS)
        self.assertEqual (Mode (['--mockRoot', '--forMe']), tapir_installer.INSTALL_FOR_ME)
        with self.assertRaises (tapir_installer.InstallerError):
            Mode (['--forMe', '--allUsers'])
        # The installer built with --uac-admin runs elevated: it installs for
        # all users, as before.
        self.isElevated = True
        self.assertEqual (Mode ([]), tapir_installer.INSTALL_FOR_ALL_USERS)
        self.assertEqual (Mode (['--forMe']), tapir_installer.INSTALL_FOR_ME)

    def testAllUsersWhenNotElevatedStartsItselfAsAdministrator (self):
        self.Patch (tapir_installer, 'IsUsingWindows', lambda : True)
        self.Patch (tapir_installer, 'DetectArchicadInstallations', unittest.mock.Mock (side_effect = AssertionError ('detected')))
        self.Patch (tapir_installer, 'GetLatestReleaseInfo', unittest.mock.Mock (side_effect = AssertionError ('downloaded')))
        before = self.registry.Dump ()
        exitCode, stdout, stderr = self.RunMain (['--console', '--forMe', '--allUsers', '--versions', '30'])
        self.assertEqual (exitCode, 1)
        self.assertEqual (self.startedAsAdministrator, [])
        exitCode, stdout, stderr = self.RunMain (['--console', '--allUsers', '--versions', '30'])
        self.assertEqual (exitCode, 0, stderr)
        self.assertEqual (self.startedAsAdministrator, [['--console', '--versions', '30', '--allUsers']])
        self.assertIn ('started again with administrator rights', stdout)
        self.assertEqual (self.registry.Dump (), before)
        # Already elevated, it goes straight on (here into the detection,
        # which finds nothing).
        self.isElevated = True
        self.Patch (tapir_installer, 'DetectArchicadInstallations', lambda mockRootPath = None : [])
        exitCode, stdout, stderr = self.RunMain (['--console', '--allUsers', '--versions', '30'])
        self.assertEqual (exitCode, 1)
        self.assertIn ('No Archicad 30 installation was found.', stderr)
        self.assertEqual (len (self.startedAsAdministrator), 1)

    def testUpdateOfAnAddOnTheUserCannotReplaceStartsItselfAsAdministrator (self):
        self.Patch (tapir_installer, 'IsUsingWindows', lambda : True)
        self.Patch (tapir_installer, 'RunGuiInstaller', unittest.mock.Mock (side_effect = AssertionError ('updated without administrator rights')))
        self.Patch (tapir_installer, 'NeedsAdministratorToReplaceWin', lambda addOnFilePath : True)
        arguments = ['--addOnFile', 'C:\\Program Files\\Graphisoft\\Archicad 30\\Add-Ons\\Tapir\\TapirAddOn_AC30_Win.apx', '--versions', '30',
            '--archicadPort', '19723', '--archicadPid', '5']
        exitCode, stdout, stderr = self.RunMain (arguments)
        self.assertEqual (exitCode, 0)
        self.assertEqual (self.startedAsAdministrator, [arguments])
        # A writable add-on (as one installed for the user) is updated with
        # the user's rights.
        self.Patch (tapir_installer, 'NeedsAdministratorToReplaceWin', lambda addOnFilePath : False)
        updates = []
        self.Patch (tapir_installer, 'RunGuiInstaller', lambda args : updates.append (args.addOnFilePath) or 0)
        self.RunMain (arguments)
        self.assertEqual (len (self.startedAsAdministrator), 1)
        self.assertEqual (len (updates), 1)

    def testWriteCheck (self):
        addOnFilePath = os.path.join (self.tempFolderPath, 'TapirAddOn_AC30_Win.apx')
        with open (addOnFilePath, 'w') as addOnFile:
            addOnFile.write ('MZ')
        self.assertFalse (tapir_installer.NeedsAdministratorToReplaceWin (addOnFilePath))
        self.assertEqual (os.listdir (self.tempFolderPath).count ('.TapirWriteCheck_{0}'.format (os.getpid ())), 0)
        self.assertFalse (tapir_installer.NeedsAdministratorToReplaceWin (os.path.join (self.tempFolderPath, 'Missing.apx')))
        with unittest.mock.patch.object (tapir_installer.os, 'open', unittest.mock.Mock (side_effect = PermissionError (13, 'Access is denied'))):
            self.assertTrue (tapir_installer.NeedsAdministratorToReplaceWin (addOnFilePath))


class MacListTests (PerUserTestBase):
    # The preferences layout of macOS, on every platform. Copying the bundle
    # (ditto) is replaced by making an empty one.
    isMac = True
    DOMAIN = 'com.graphisoft.AC-ARM 30.0.0 INT v1'

    def setUp (self):
        PerUserTestBase.setUp (self)
        self.otherAddOnPath = '/Users/me/Add-Ons/Other.bundle'
        self.prefs = FakePreferences ({
            self.DOMAIN : { 'Add-On Manager' : { 'Actual Path' : '/Applications/Graphisoft/Archicad 30/Add-Ons',
                'Include' : tapir_installer.numbered_mac ('Include', [tapir_installer.to_url_mac (self.otherAddOnPath)]),
                'Disable' : tapir_installer.numbered_mac ('Disable', []) }, 'Other Setting' : 1 },
            'com.graphisoft.startup-ARM.AC-ARM 30.0.0 INT v1' : { 'Add-On Manager' : 'not this one' },
        })
        self.Patch (tapir_installer, 'CreateAddOnList', lambda : tapir_installer.AddOnListMac (self.prefs))
        self.Patch (tapir_installer, 'CopyAddOnForMe', lambda downloadedFilePath, targetPath : os.makedirs (targetPath, exist_ok = True))

    def testFreshInstallAndUninstall (self):
        installation = self.CreateInstallation (30)
        targetPath = tapir_installer.InstallAddOnForMe (installation, self.downloadedFilePath)
        self.assertEqual (targetPath, os.path.join (self.perUserFolderPath, 'AC30', 'TapirAddOn_AC30_Mac.bundle'))
        manager = self.prefs.read (self.DOMAIN)['Add-On Manager']
        self.assertEqual (sorted (manager['Include']), ['#1.', '#2.', 'Include Number'])
        self.assertEqual (manager['Include']['Include Number'], 2)
        self.assertEqual (manager['Include']['#1.'], tapir_installer.to_url_mac (self.otherAddOnPath))
        self.assertTrue (manager['Include']['#2.'].startswith ('lan.null2://'))
        self.assertEqual (tapir_installer.from_url_mac (manager['Include']['#2.']), pathlib.Path (targetPath))
        self.assertEqual (manager['Disable'], { 'Disable Number' : 0 })
        # The other settings stay.
        self.assertEqual (manager['Actual Path'], '/Applications/Graphisoft/Archicad 30/Add-Ons')
        self.assertEqual (self.prefs.read (self.DOMAIN)['Other Setting'], 1)
        self.assertEqual (self.prefs.read ('com.graphisoft.startup-ARM.AC-ARM 30.0.0 INT v1'), { 'Add-On Manager' : 'not this one' })
        self.assertTrue (tapir_installer.UninstallAddOnForMe (installation))
        manager = self.prefs.read (self.DOMAIN)['Add-On Manager']
        self.assertEqual (manager['Include'], { 'Include Number' : 1, '#1.' : tapir_installer.to_url_mac (self.otherAddOnPath) })
        self.assertFalse (os.path.lexists (targetPath))

    def testAllUsersTapirIsNeverOverridden (self):
        # On macOS a Tapir of the Add-Ons folder that is switched off keeps the
        # user's copy from loading in every version tested.
        before = self.prefs.Dump ()
        with self.assertRaises (tapir_installer.AllUsersTapirError):
            tapir_installer.InstallAddOnForMe (self.CreateInstallation (30, withAllUsersTapir = True), self.downloadedFilePath)
        self.assertEqual (self.prefs.Dump (), before)

    def testNoAddOnManagerYet (self):
        before = self.prefs.Dump ()
        with self.assertRaises (tapir_installer.ArchicadNotStartedError):
            tapir_installer.InstallAddOnForMe (self.CreateInstallation (31), self.downloadedFilePath)
        self.assertEqual (self.prefs.Dump (), before)

    def testArchicadRunningWritesNothing (self):
        before = self.prefs.Dump ()
        self.isArchicadRunning = True
        with self.assertRaises (tapir_installer.ArchicadRunningError):
            tapir_installer.InstallAddOnForMe (self.CreateInstallation (30), self.downloadedFilePath)
        self.assertEqual (self.prefs.Dump (), before)
        self.assertEqual (self.prefs.writeCount, 0)


@unittest.skipUnless (os.path.isdir (RUNNER_FOLDER_PATH), 'needs the community test runner of the repository')
class TwinTests (unittest.TestCase):
    # The functions copied from the community test runner give the same
    # results as their twins.
    @classmethod
    def setUpClass (cls):
        sys.path.insert (0, RUNNER_FOLDER_PATH)
        import runner_windows
        import runner_macos
        cls.rw = runner_windows
        cls.rm = runner_macos

    def testConstants (self):
        self.assertEqual (tapir_installer.ARCHICAD_KEY, self.rw.ARCHICAD_KEY)
        self.assertEqual (tapir_installer.LISTS, self.rw.LISTS)
        self.assertEqual (tapir_installer.REG_SZ, self.rw.REG_SZ)
        self.assertEqual (tapir_installer.MANAGER, self.rm.MANAGER)
        self.assertEqual (tapir_installer.SCHEME, self.rm.SCHEME)
        # Archicad 29 is not tested yet, so the installer counts it in.
        self.assertEqual (tapir_installer.COLLIDES_UP_TO, self.rw.COLLIDES_UP_TO + 1)

    def testUrls (self):
        paths = ['C:\\Program Files\\Graphisoft\\Archicad 29\\Add-Ons\\Tapir\\TapirAddOn_AC29_Win.apx',
            'C:\\Users\\Ünal Öz\\AppData\\Local\\Tapir\\AC30\\TapirAddOn_AC30_Win.apx', '/Users/me/Library/Application Support/Tapir/x#1.bundle']
        for path in paths:
            with self.subTest (path = path):
                self.assertEqual (tapir_installer.to_url (path), self.rw.to_url (path))
                self.assertEqual (tapir_installer.from_url (self.rw.to_url (path)), self.rw.from_url (self.rw.to_url (path)))
                self.assertEqual (tapir_installer.to_url_mac (path), self.rm.to_url (path))
                self.assertEqual (tapir_installer.from_url_mac (self.rm.to_url (path)), self.rm.from_url (self.rm.to_url (path)))
                self.assertEqual (tapir_installer.from_url_mac ('lan.flat:///x/y'), self.rm.from_url ('lan.flat:///x/y'))

    def testIsTapir (self):
        for name in ['TapirAddOn_AC26_Win.apx', 'TapirAddOn_AC26_Win.apx.bak', 'tapiraddon.BUNDLE', 'Other.apx', 'TapirAddOn_AC29_Mac.bundle.old']:
            with self.subTest (name = name):
                self.assertEqual (tapir_installer.is_tapir (name), self.rw.is_tapir (name))

    def testLists (self):
        urls = ['lan.flat:///C:/a.apx', 'lan.flat:///C:/b.apx']
        self.assertEqual (tapir_installer.numbered ('Include', urls), self.rw.numbered ('Include', urls))
        values = dict (self.rw.numbered ('Disable', urls * 6), Other = ['x', 1])
        self.assertEqual (tapir_installer.entries (values), self.rw.entries (values))
        self.assertEqual (tapir_installer.numbered_mac ('Include', urls), self.rm.numbered ('Include', urls))
        values = dict (self.rm.numbered ('Disable', urls * 6), Other = 'x')
        self.assertEqual (tapir_installer.entries_mac (values), self.rm.entries (values))

    def testManagerKeyAndDomain (self):
        registry = FakeRegistry ({
            ManagerKey (30) + '\\Include' : self.rw.numbered ('Include', ['lan.flat:///C:/a.apx']),
            '{0}\\Archicad 30.0.0 GER R1\\Add-On Manager\\Include'.format (ARCHICAD_KEY) : self.rw.numbered ('Include', ['lan.flat:///C:/a.apx', 'lan.flat:///C:/b.apx']),
            '{0}\\Archicad 300.0.0 INT R1\\Add-On Manager\\Include'.format (ARCHICAD_KEY) : self.rw.numbered ('Include', []),
        })
        for version in ['30', '29', '300']:
            with self.subTest (version = version):
                self.assertEqual (tapir_installer.manager_key (registry, version), self.rw.manager_key (registry, version))
        prefs = FakePreferences ({
            'com.graphisoft.AC-ARM 30.0.0 INT v1' : { 'Add-On Manager' : { 'Include' : { 'Include Number' : 0 } } },
            'com.graphisoft.AC 30.0.0 INT v1' : { 'Add-On Manager' : { 'Include' : { 'Include Number' : 1, '#1.' : 'x' } } },
            'com.graphisoft.startup-ARM.AC-ARM 30.0.0 INT v1' : { 'Add-On Manager' : { 'Include' : {} } },
        })
        for version in ['30', '29']:
            with self.subTest (version = version):
                self.assertEqual (tapir_installer.manager_domain (prefs, version), self.rm.manager_domain (prefs, version))


if __name__ == '__main__':
    unittest.main (verbosity = 2)
