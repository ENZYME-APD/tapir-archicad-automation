"""Tapir Add-On Installer.

Detects the installed Archicad versions, downloads the matching Tapir Add-On
from the latest GitHub release, and places it into the Add-Ons folder of each
Archicad installation so that Archicad loads it automatically on the next start.

Works on Windows and macOS. Uses only the Python standard library, so it can be
run directly with any Python 3 or packaged into a standalone executable with
PyInstaller (see the release workflow).
"""

import argparse
import json
import os
import platform
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import urllib.request

INSTALLER_TITLE = 'Tapir Add-On Installer'
LATEST_RELEASE_API_URL = 'https://api.github.com/repos/ENZYME-APD/tapir-archicad-automation/releases/latest'
LATEST_RELEASE_DOWNLOAD_URL = 'https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/'
MANUAL_INSTALL_URL = 'https://github.com/ENZYME-APD/tapir-archicad-automation#installation'
TAPIR_SUBFOLDER_NAME = 'Tapir'
ADDONS_FOLDER_NAME = 'Add-Ons'
USER_AGENT = 'TapirInstaller'
DOWNLOAD_TIMEOUT_SECONDS = 60


def IsUsingWindows ():
    return platform.system () == 'Windows'


def IsUsingMacOS ():
    return platform.system () == 'Darwin'


class InstallerError (Exception):
    pass


class ArchicadInstallation:
    def __init__ (self, version, installPath):
        self.version = version
        self.installPath = installPath
        self.addOnsFolderPath = os.path.join (installPath, ADDONS_FOLDER_NAME)

    def __repr__ (self):
        return 'Archicad {0} ({1})'.format (self.version, self.installPath)


def GetArchicadVersionFromName (name):
    match = re.search (r'archicad\s+(\d+)', name, re.IGNORECASE)
    if match is None:
        return None
    return int (match.group (1))


def IsValidArchicadInstallationFolder (folderPath):
    return os.path.isdir (folderPath) and os.path.isdir (os.path.join (folderPath, ADDONS_FOLDER_NAME))


def AddInstallationCandidate (installations, name, folderPath):
    version = GetArchicadVersionFromName (name)
    if version is None or not IsValidArchicadInstallationFolder (folderPath):
        return
    normalizedPath = os.path.normcase (os.path.normpath (folderPath))
    for installation in installations:
        if os.path.normcase (os.path.normpath (installation.installPath)) == normalizedPath:
            return
    installations.append (ArchicadInstallation (version, folderPath))


def CollectCandidatesFromUninstallRegistry (installations, winreg):
    uninstallKeyPath = r'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall'
    try:
        with winreg.OpenKey (winreg.HKEY_LOCAL_MACHINE, uninstallKeyPath, 0, winreg.KEY_READ | winreg.KEY_WOW64_64KEY) as uninstallKey:
            subKeyCount = winreg.QueryInfoKey (uninstallKey)[0]
            for subKeyIndex in range (subKeyCount):
                try:
                    subKeyName = winreg.EnumKey (uninstallKey, subKeyIndex)
                    with winreg.OpenKey (uninstallKey, subKeyName) as subKey:
                        displayName = winreg.QueryValueEx (subKey, 'DisplayName')[0]
                        if GetArchicadVersionFromName (displayName) is None:
                            continue
                        installLocation = winreg.QueryValueEx (subKey, 'InstallLocation')[0]
                        AddInstallationCandidate (installations, displayName, installLocation)
                except OSError:
                    continue
    except OSError:
        pass


def CollectCandidatesFromVendorRegistry (installations, winreg, keyPath, depth = 0):
    # The GRAPHISOFT registry keys are not documented and their layout changed
    # between Archicad versions, so instead of assuming a structure this walks
    # the vendor key and picks up every string value that points to an existing
    # Archicad installation folder.
    maxDepth = 4
    try:
        with winreg.OpenKey (winreg.HKEY_LOCAL_MACHINE, keyPath, 0, winreg.KEY_READ | winreg.KEY_WOW64_64KEY) as key:
            subKeyCount, valueCount = winreg.QueryInfoKey (key)[0:2]
            for valueIndex in range (valueCount):
                try:
                    value = winreg.EnumValue (key, valueIndex)[1]
                except OSError:
                    continue
                if not isinstance (value, str) or GetArchicadVersionFromName (value) is None:
                    continue
                folderPath = value.strip ('"')
                AddInstallationCandidate (installations, os.path.basename (os.path.normpath (folderPath)), folderPath)
            if depth < maxDepth:
                for subKeyIndex in range (subKeyCount):
                    try:
                        subKeyName = winreg.EnumKey (key, subKeyIndex)
                    except OSError:
                        continue
                    CollectCandidatesFromVendorRegistry (installations, winreg, keyPath + '\\' + subKeyName, depth + 1)
    except OSError:
        pass


def CollectCandidatesFromProgramFilesFolders (installations, programFilesFolders):
    for programFilesFolder in programFilesFolders:
        if not os.path.isdir (programFilesFolder):
            continue
        for vendorFolderName in os.listdir (programFilesFolder):
            if not vendorFolderName.lower ().startswith ('graphisoft'):
                continue
            vendorFolderPath = os.path.join (programFilesFolder, vendorFolderName)
            if not os.path.isdir (vendorFolderPath):
                continue
            for folderName in os.listdir (vendorFolderPath):
                AddInstallationCandidate (installations, folderName, os.path.join (vendorFolderPath, folderName))


def DetectArchicadInstallationsWin (mockRootPath = None):
    installations = []
    if mockRootPath is None:
        import winreg
        CollectCandidatesFromUninstallRegistry (installations, winreg)
        CollectCandidatesFromVendorRegistry (installations, winreg, r'SOFTWARE\GRAPHISOFT')
        CollectCandidatesFromVendorRegistry (installations, winreg, r'SOFTWARE\GRAPHISOFT SE')
        programFilesFolders = []
        for environmentVariable in ['ProgramW6432', 'ProgramFiles']:
            programFilesFolder = os.environ.get (environmentVariable)
            if programFilesFolder is not None and programFilesFolder not in programFilesFolders:
                programFilesFolders.append (programFilesFolder)
    else:
        programFilesFolders = [mockRootPath]
    CollectCandidatesFromProgramFilesFolders (installations, programFilesFolders)
    return installations


def DetectArchicadInstallationsMac (mockRootPath = None):
    installations = []
    applicationsFolderPath = mockRootPath if mockRootPath is not None else '/Applications'
    if not os.path.isdir (applicationsFolderPath):
        return installations
    foldersToScan = [applicationsFolderPath]
    for folderName in os.listdir (applicationsFolderPath):
        folderPath = os.path.join (applicationsFolderPath, folderName)
        if os.path.isdir (folderPath) and not folderName.endswith ('.app'):
            foldersToScan.append (folderPath)
    for folderToScan in foldersToScan:
        for folderName in os.listdir (folderToScan):
            AddInstallationCandidate (installations, folderName, os.path.join (folderToScan, folderName))
    return installations


def DetectArchicadInstallations (mockRootPath = None):
    if IsUsingWindows ():
        installations = DetectArchicadInstallationsWin (mockRootPath)
    elif IsUsingMacOS ():
        installations = DetectArchicadInstallationsMac (mockRootPath)
    elif mockRootPath is not None:
        # Non-Windows/macOS platforms are for testing only: run both scanners
        # on the mocked folder structure, deduplicating across the two lists.
        installations = []
        for installation in DetectArchicadInstallationsWin (mockRootPath) + DetectArchicadInstallationsMac (mockRootPath):
            AddInstallationCandidate (installations, 'Archicad {0}'.format (installation.version), installation.installPath)
    else:
        installations = []
    installations.sort (key = lambda installation: installation.version)
    return installations


def DownloadUrl (url):
    request = urllib.request.Request (url, headers = { 'User-Agent' : USER_AGENT })
    with urllib.request.urlopen (request, timeout = DOWNLOAD_TIMEOUT_SECONDS) as response:
        return response.read ()


def CreateFallbackReleaseInfo ():
    # When the GitHub API is not reachable (rate limited or blocked), fall
    # back to the stable "latest release" download links - the same ones the
    # README links to. Assets are listed for a generous version range; a
    # version without a real asset fails at download time instead of being
    # reported upfront.
    assets = []
    for acVersion in range (25, 40):
        for postfix, extension in [('_Win', '.apx'), ('_Mac', '.zip')]:
            assetName = 'TapirAddOn_AC{0}{1}{2}'.format (acVersion, postfix, extension)
            assets.append ({ 'name' : assetName, 'browser_download_url' : LATEST_RELEASE_DOWNLOAD_URL + assetName })
    return { 'tag_name' : 'latest', 'assets' : assets }


def GetLatestReleaseInfo ():
    try:
        return json.loads (DownloadUrl (LATEST_RELEASE_API_URL))
    except Exception:
        try:
            # Check that the fallback URLs are reachable at all before
            # claiming that a release was found.
            DownloadUrl ('https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest')
        except Exception as e:
            raise InstallerError ('Failed to get the latest Tapir release from GitHub: {0}'.format (e))
        return CreateFallbackReleaseInfo ()


def GetPlatformAssetPostfix ():
    return '_Mac' if IsUsingMacOS () else '_Win'


def FindAssetForVersion (releaseInfo, acVersion):
    # Assets are matched the same way as in VersionChecker.cpp: by the
    # AC<version>_<platform> substring of the asset name.
    namePostfix = 'AC{0}{1}'.format (acVersion, GetPlatformAssetPostfix ())
    for asset in releaseInfo.get ('assets', []):
        if namePostfix in asset['name']:
            return asset
    return None


def DownloadAsset (asset, targetFolderPath, progressCallback = None):
    targetPath = os.path.join (targetFolderPath, asset['name'])
    request = urllib.request.Request (asset['browser_download_url'], headers = { 'User-Agent' : USER_AGENT })
    with urllib.request.urlopen (request, timeout = DOWNLOAD_TIMEOUT_SECONDS) as response:
        totalSize = int (response.headers.get ('Content-Length', 0))
        downloadedSize = 0
        with open (targetPath, 'wb') as targetFile:
            while True:
                chunk = response.read (65536)
                if not chunk:
                    break
                targetFile.write (chunk)
                downloadedSize += len (chunk)
                if progressCallback is not None:
                    progressCallback (downloadedSize, totalSize)
    return targetPath


def GetStrayTapirAddOnPaths (addOnsFolderPath):
    # Tapir copies from earlier manual installs, placed directly into the
    # Add-Ons folder. They must be removed, otherwise Archicad would try to
    # load the add-on twice.
    strayPaths = []
    try:
        for entryName in os.listdir (addOnsFolderPath):
            if re.match (r'TapirAddOn_.*\.(apx|bundle)$', entryName, re.IGNORECASE):
                strayPaths.append (os.path.join (addOnsFolderPath, entryName))
    except OSError:
        pass
    return strayPaths


def RemovePath (path):
    if os.path.isdir (path) and not os.path.islink (path):
        shutil.rmtree (path)
    elif os.path.lexists (path):
        os.remove (path)


def RunShellCommandWithAdminPrivilegesMac (shellCommand):
    # Triggers the native macOS authentication dialog. The command is built
    # from shlex-quoted fixed paths only, never from arbitrary user input.
    appleScript = 'do shell script "{0}" with administrator privileges'.format (
        shellCommand.replace ('\\', '\\\\').replace ('"', '\\"'))
    result = subprocess.run (['osascript', '-e', appleScript], capture_output = True, text = True)
    if result.returncode != 0:
        raise InstallerError ('Failed to install with administrator privileges: {0}'.format (result.stderr.strip ()))


def InstallAddOnMac (downloadedFilePath, tapirFolderPath, strayPaths):
    try:
        RemovePath (tapirFolderPath)
        for strayPath in strayPaths:
            RemovePath (strayPath)
        os.makedirs (tapirFolderPath)
        # ditto keeps the permission bits and symlinks of the bundle, which
        # zipfile.extractall would drop.
        subprocess.run (['ditto', '-x', '-k', downloadedFilePath, tapirFolderPath], check = True, capture_output = True)
    except (PermissionError, subprocess.CalledProcessError):
        commands = []
        for pathToRemove in [tapirFolderPath] + strayPaths:
            commands.append ('rm -rf {0}'.format (shlex.quote (pathToRemove)))
        commands.append ('mkdir -p {0}'.format (shlex.quote (tapirFolderPath)))
        commands.append ('ditto -x -k {0} {1}'.format (shlex.quote (downloadedFilePath), shlex.quote (tapirFolderPath)))
        RunShellCommandWithAdminPrivilegesMac (' && '.join (commands))


def InstallAddOnWin (downloadedFilePath, tapirFolderPath, strayPaths):
    try:
        RemovePath (tapirFolderPath)
        for strayPath in strayPaths:
            RemovePath (strayPath)
        os.makedirs (tapirFolderPath)
        shutil.copyfile (downloadedFilePath, os.path.join (tapirFolderPath, os.path.basename (downloadedFilePath)))
    except PermissionError:
        raise InstallerError ('Permission denied. Please run the installer as administrator.')
    except OSError as e:
        raise InstallerError ('Failed to install (close Archicad and retry): {0}'.format (e))


def InstallAddOn (addOnsFolderPath, downloadedFilePath):
    if not os.path.isdir (addOnsFolderPath):
        raise InstallerError ('The Add-Ons folder does not exist: {0}'.format (addOnsFolderPath))
    tapirFolderPath = os.path.join (addOnsFolderPath, TAPIR_SUBFOLDER_NAME)
    strayPaths = GetStrayTapirAddOnPaths (addOnsFolderPath)
    if IsUsingMacOS ():
        InstallAddOnMac (downloadedFilePath, tapirFolderPath, strayPaths)
    else:
        InstallAddOnWin (downloadedFilePath, tapirFolderPath, strayPaths)


def UninstallAddOn (addOnsFolderPath):
    tapirFolderPath = os.path.join (addOnsFolderPath, TAPIR_SUBFOLDER_NAME)
    if not os.path.lexists (tapirFolderPath):
        return False
    try:
        RemovePath (tapirFolderPath)
    except PermissionError:
        if IsUsingMacOS ():
            RunShellCommandWithAdminPrivilegesMac ('rm -rf {0}'.format (shlex.quote (tapirFolderPath)))
        else:
            raise InstallerError ('Permission denied. Please run the installer as administrator.')
    return True


def DownloadAndInstallAddOn (installation, releaseInfo, dryRun, progressCallback = None):
    """Returns a human readable status string, or raises InstallerError."""
    asset = FindAssetForVersion (releaseInfo, installation.version)
    if asset is None:
        raise InstallerError ('No Tapir Add-On is available for Archicad {0} in release {1}.'.format (
            installation.version, releaseInfo.get ('tag_name', '?')))
    downloadFolderPath = tempfile.mkdtemp (prefix = 'TapirInstaller_')
    try:
        downloadedFilePath = DownloadAsset (asset, downloadFolderPath, progressCallback)
        if dryRun:
            return 'Would install {0} into {1}'.format (asset['name'], os.path.join (installation.addOnsFolderPath, TAPIR_SUBFOLDER_NAME))
        InstallAddOn (installation.addOnsFolderPath, downloadedFilePath)
        return 'Installed {0} into {1}'.format (asset['name'], os.path.join (installation.addOnsFolderPath, TAPIR_SUBFOLDER_NAME))
    finally:
        shutil.rmtree (downloadFolderPath, ignore_errors = True)


def GetTargetInstallations (args):
    requestedVersions = None
    if args.versions is not None:
        try:
            requestedVersions = [int (version) for version in args.versions.split (',')]
        except ValueError:
            raise InstallerError ('Invalid --versions value: {0}'.format (args.versions))
    if args.addOnsFolderPath is not None:
        if requestedVersions is None or len (requestedVersions) != 1:
            raise InstallerError ('--addOnsFolder requires --versions with exactly one Archicad version.')
        installation = ArchicadInstallation (requestedVersions[0], os.path.dirname (os.path.normpath (args.addOnsFolderPath)))
        installation.addOnsFolderPath = args.addOnsFolderPath
        return [installation]
    installations = DetectArchicadInstallations (args.mockRootPath)
    if requestedVersions is not None:
        installations = [installation for installation in installations if installation.version in requestedVersions]
        detectedVersions = [installation.version for installation in installations]
        for requestedVersion in requestedVersions:
            if requestedVersion not in detectedVersions:
                raise InstallerError ('No Archicad {0} installation was found.'.format (requestedVersion))
    return installations


def RunConsoleInstaller (args):
    try:
        installations = GetTargetInstallations (args)
        if len (installations) == 0:
            raise InstallerError ('No Archicad installation was found. See {0} for manual installation.'.format (MANUAL_INSTALL_URL))
        print ('Found Archicad installations:')
        for installation in installations:
            print ('  {0}'.format (installation))
        if args.uninstall:
            for installation in installations:
                if args.dryRun:
                    print ('Would uninstall Tapir from {0}'.format (installation.addOnsFolderPath))
                elif UninstallAddOn (installation.addOnsFolderPath):
                    print ('Uninstalled Tapir from {0}'.format (installation.addOnsFolderPath))
                else:
                    print ('Tapir was not installed in {0}'.format (installation.addOnsFolderPath))
            return 0
        releaseInfo = GetLatestReleaseInfo ()
        print ('Latest Tapir release: {0}'.format (releaseInfo.get ('tag_name', '?')))
        for installation in installations:
            print (DownloadAndInstallAddOn (installation, releaseInfo, args.dryRun))
        if not args.dryRun:
            print ('Done. Restart Archicad to load the Tapir Add-On.')
        return 0
    except InstallerError as e:
        print ('ERROR: {0}'.format (e), file = sys.stderr)
        return 1


def RunGuiInstaller (args):
    import tkinter
    import tkinter.font
    import tkinter.messagebox
    import threading
    import webbrowser

    class InstallationRow:
        def __init__ (self, installation):
            self.installation = installation
            self.isSelected = None
            self.checkButton = None
            self.statusLabel = None

    class InstallerApp (tkinter.Tk):
        def __init__ (self):
            tkinter.Tk.__init__ (self)
            self.title (INSTALLER_TITLE)
            self.resizable (False, False)
            self.rows = []
            self.releaseInfo = None
            self.isWorking = False

            padding = { 'padx' : 10, 'pady' : 5 }
            titleFont = tkinter.font.nametofont ('TkDefaultFont').copy ()
            titleFont.configure (size = 14, weight = 'bold')
            tkinter.Label (self, text = INSTALLER_TITLE, font = titleFont).pack (**padding)
            self.releaseLabel = tkinter.Label (self, text = 'Getting the latest Tapir release...')
            self.releaseLabel.pack (**padding)
            self.rowsFrame = tkinter.Frame (self)
            self.rowsFrame.pack (fill = 'x', **padding)
            self.detectionLabel = tkinter.Label (self.rowsFrame, text = 'Detecting Archicad installations...')
            self.detectionLabel.grid (row = 0, column = 0, columnspan = 2)
            buttonsFrame = tkinter.Frame (self)
            buttonsFrame.pack (**padding)
            self.installButton = tkinter.Button (buttonsFrame, text = 'Install', width = 12, state = 'disabled', command = self.OnInstallClicked)
            self.installButton.grid (row = 0, column = 0, padx = 5)
            self.uninstallButton = tkinter.Button (buttonsFrame, text = 'Uninstall', width = 12, state = 'disabled', command = self.OnUninstallClicked)
            self.uninstallButton.grid (row = 0, column = 1, padx = 5)
            self.closeButton = tkinter.Button (buttonsFrame, text = 'Close', width = 12, command = self.destroy)
            self.closeButton.grid (row = 0, column = 2, padx = 5)

            threading.Thread (target = self.InitializeInBackground, daemon = True).start ()

        def RunOnUiThread (self, function):
            self.after (0, function)

        def InitializeInBackground (self):
            installations = DetectArchicadInstallations (args.mockRootPath)
            self.RunOnUiThread (lambda : self.ShowInstallations (installations))
            try:
                releaseInfo = GetLatestReleaseInfo ()
                self.RunOnUiThread (lambda : self.ShowReleaseInfo (releaseInfo))
            except InstallerError as e:
                self.RunOnUiThread (lambda : self.ShowReleaseError (str (e)))

        def ShowInstallations (self, installations):
            self.detectionLabel.grid_forget ()
            if len (installations) == 0:
                tkinter.Label (self.rowsFrame, text = 'No Archicad installation was found on this computer.').grid (row = 0, column = 0, columnspan = 2)
                manualLabel = tkinter.Label (self.rowsFrame, text = 'Click here for manual installation instructions.', fg = 'blue', cursor = 'hand2')
                manualLabel.grid (row = 1, column = 0, columnspan = 2)
                manualLabel.bind ('<Button-1>', lambda event : webbrowser.open (MANUAL_INSTALL_URL))
                return
            for rowIndex, installation in enumerate (installations):
                row = InstallationRow (installation)
                row.isSelected = tkinter.BooleanVar (value = True)
                row.checkButton = tkinter.Checkbutton (self.rowsFrame,
                    text = 'Archicad {0}  ({1})'.format (installation.version, installation.installPath),
                    variable = row.isSelected, anchor = 'w')
                row.checkButton.grid (row = rowIndex, column = 0, sticky = 'w')
                row.statusLabel = tkinter.Label (self.rowsFrame, text = '', width = 32, anchor = 'w')
                row.statusLabel.grid (row = rowIndex, column = 1, sticky = 'w')
                self.rows.append (row)
            self.UpdateButtonStates ()

        def ShowReleaseInfo (self, releaseInfo):
            self.releaseInfo = releaseInfo
            self.releaseLabel.configure (text = 'Latest Tapir release: {0}'.format (releaseInfo.get ('tag_name', '?')))
            self.UpdateButtonStates ()

        def ShowReleaseError (self, errorText):
            self.releaseLabel.configure (text = errorText, fg = 'red')
            self.UpdateButtonStates ()

        def UpdateButtonStates (self):
            hasRows = len (self.rows) > 0
            canInstall = hasRows and self.releaseInfo is not None and not self.isWorking
            self.installButton.configure (state = 'normal' if canInstall else 'disabled')
            self.uninstallButton.configure (state = 'normal' if hasRows and not self.isWorking else 'disabled')
            self.closeButton.configure (state = 'disabled' if self.isWorking else 'normal')

        def SetRowStatus (self, row, statusText, color = 'black'):
            self.RunOnUiThread (lambda : row.statusLabel.configure (text = statusText, fg = color))

        def GetSelectedRows (self):
            return [row for row in self.rows if row.isSelected.get ()]

        def OnInstallClicked (self):
            self.StartWork (self.InstallInBackground)

        def OnUninstallClicked (self):
            self.StartWork (self.UninstallInBackground)

        def StartWork (self, workFunction):
            selectedRows = self.GetSelectedRows ()
            if len (selectedRows) == 0:
                tkinter.messagebox.showinfo (INSTALLER_TITLE, 'Select at least one Archicad version.')
                return
            self.isWorking = True
            self.UpdateButtonStates ()
            threading.Thread (target = workFunction, args = (selectedRows,), daemon = True).start ()

        def InstallInBackground (self, selectedRows):
            succeededCount = 0
            for row in selectedRows:
                try:
                    self.SetRowStatus (row, 'Downloading...')
                    progressCallback = lambda downloadedSize, totalSize : self.SetRowStatus (row,
                        'Downloading... {0}%'.format (int (downloadedSize * 100 / totalSize)) if totalSize > 0 else 'Downloading...')
                    asset = FindAssetForVersion (self.releaseInfo, row.installation.version)
                    if asset is None:
                        raise InstallerError ('No add-on for this version')
                    downloadFolderPath = tempfile.mkdtemp (prefix = 'TapirInstaller_')
                    try:
                        downloadedFilePath = DownloadAsset (asset, downloadFolderPath, progressCallback)
                        self.SetRowStatus (row, 'Installing...')
                        InstallAddOn (row.installation.addOnsFolderPath, downloadedFilePath)
                    finally:
                        shutil.rmtree (downloadFolderPath, ignore_errors = True)
                    self.SetRowStatus (row, 'Installed', 'green')
                    succeededCount += 1
                except InstallerError as e:
                    self.SetRowStatus (row, 'Failed: {0}'.format (e), 'red')
                except Exception as e:
                    self.SetRowStatus (row, 'Failed: {0}'.format (e), 'red')
            self.RunOnUiThread (lambda : self.FinishWork (
                'Installed the Tapir Add-On for {0} of {1} selected Archicad version(s).\n\nRestart Archicad to load the Add-On.'.format (
                    succeededCount, len (selectedRows))))

        def UninstallInBackground (self, selectedRows):
            for row in selectedRows:
                try:
                    if UninstallAddOn (row.installation.addOnsFolderPath):
                        self.SetRowStatus (row, 'Uninstalled', 'green')
                    else:
                        self.SetRowStatus (row, 'Was not installed')
                except InstallerError as e:
                    self.SetRowStatus (row, 'Failed: {0}'.format (e), 'red')
                except Exception as e:
                    self.SetRowStatus (row, 'Failed: {0}'.format (e), 'red')
            self.RunOnUiThread (lambda : self.FinishWork ('Finished uninstalling the Tapir Add-On.'))

        def FinishWork (self, summaryText):
            self.isWorking = False
            self.UpdateButtonStates ()
            tkinter.messagebox.showinfo (INSTALLER_TITLE, summaryText)

    app = InstallerApp ()
    app.mainloop ()
    return 0


def TryAttachWindowsConsole ():
    # The Windows executable is built with --windowed, so it has no console by
    # default. When run from a terminal with console flags, attach to the
    # parent console so print output becomes visible (best effort).
    if not IsUsingWindows () or (sys.stdout is not None and sys.stderr is not None):
        return
    try:
        import ctypes
        if ctypes.windll.kernel32.AttachConsole (-1):
            if sys.stdout is None:
                sys.stdout = open ('CONOUT$', 'w')
            if sys.stderr is None:
                sys.stderr = open ('CONOUT$', 'w')
    except Exception:
        pass


def Main ():
    parser = argparse.ArgumentParser (description = INSTALLER_TITLE)
    parser.add_argument ('--console', dest = 'console', action = 'store_true', help = 'run in console mode without the GUI')
    parser.add_argument ('--versions', dest = 'versions', type = str, default = None, help = 'comma separated Archicad versions to install for (e.g. 28,29); default: all detected')
    parser.add_argument ('--addOnsFolder', dest = 'addOnsFolderPath', type = str, default = None, help = 'install into this Add-Ons folder instead of the detected ones (requires --versions with one version)')
    parser.add_argument ('--uninstall', dest = 'uninstall', action = 'store_true', help = 'remove the installed Tapir Add-On instead of installing it')
    parser.add_argument ('--dryRun', dest = 'dryRun', action = 'store_true', help = 'detect and download only, do not modify the Add-Ons folders')
    parser.add_argument ('--mockRoot', dest = 'mockRootPath', type = str, default = None, help = 'detect installations under this folder instead of the real system locations (for testing)')
    args = parser.parse_args ()

    if args.console or args.dryRun or args.addOnsFolderPath is not None:
        TryAttachWindowsConsole ()
        return RunConsoleInstaller (args)
    return RunGuiInstaller (args)


if __name__ == '__main__':
    sys.exit (Main ())
