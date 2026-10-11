"""Tapir Add-On Installer.

Detects the installed Archicad versions, downloads the matching Tapir Add-On
from the latest GitHub release, and places it into the Add-Ons folder of each
Archicad installation so that Archicad loads it automatically on the next start.

In update mode (--addOnFile) it replaces one given Tapir Add-On instead, and
quits and restarts the Archicad that runs it: the Add-On's auto-update starts
the installer this way.

Works on Windows and macOS. Uses only the Python standard library, so it can be
run directly with any Python 3 or packaged into a standalone executable with
PyInstaller (see the release workflow).
"""

import argparse
import contextlib
import http.client
import json
import os
import platform
import re
import shlex
import shutil
import socket
import ssl
import subprocess
import sys
import tempfile
import threading
import time
import unicodedata
import urllib.request

INSTALLER_TITLE = 'Tapir Add-On Installer'
LATEST_RELEASE_API_URL = 'https://api.github.com/repos/ENZYME-APD/tapir-archicad-automation/releases/latest'
LATEST_RELEASE_DOWNLOAD_URL = 'https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest/download/'
MANUAL_INSTALL_URL = 'https://github.com/ENZYME-APD/tapir-archicad-automation#installation'
TAPIR_SUBFOLDER_NAME = 'Tapir'
# The Add-Ons folder name of each Archicad language version, as listed by
# Graphisoft ("Addon folder name in different languages" in the Archicad C++
# API community forum). Most versions use Add-Ons; the localized ones are below.
# Older versions spelled ArchiCAD, newer ones Archicad: names are compared
# case-insensitively.
LOCALIZED_ADDONS_FOLDER_NAMES = [
    'Add-Ons',                      # AUS, AUT, CHE, CHI, DEN, GER, GRE, INT, ITA, JPN, NED, NOR, NZE, SWE, TAI, USA
    'Extensões',                    # BRA, POR
    'Doplnky ArchiCADu',            # CZE
    'ArchiCAD-laajennukset',        # FIN
    'Extensions',                   # FRA
    'Kiegészítök',                  # HUN
    'Kiegészítők',                  # HUN, alternative spelling
    '애드온',                        # KOR
    'Dodatki',                      # POL
    'Расширения ArchiCAD',          # RUS
    'Extensiones ArchiCAD',         # SPA
    'Add-On\'lar',                  # TUR
]
ADDONS_FOLDER_MARKER_FILE_NAME = 'XReadCfg.txt'
ADDONS_FOLDER_MARKER_MAX_DEPTH = 2
USER_AGENT = 'TapirInstaller'
DOWNLOAD_TIMEOUT_SECONDS = 60
PERMISSION_DENIED_TEXT = 'Permission denied. Close Archicad if it is running, or run the installer as administrator.'
# The first bytes of the add-on assets: the Windows add-on is a DLL, the macOS
# one is zipped. A proxy or web filter may answer with a page of its own.
ASSET_FILE_SIGNATURES = { '.apx' : b'MZ', '.zip' : b'PK\x03\x04' }
ARCHICAD_DID_NOT_QUIT_TEXT = 'Archicad did not quit ({0}), so Tapir was not updated. Start the update again from Archicad.'
WAITING_FOR_ARCHICAD_TEXT = 'If Archicad asks whether to save your changes, answer it in Archicad.'
UPDATE_CANCELLED_TEXT = ('The update was cancelled. Archicad was not closed by the installer. '
    'If Archicad closes anyway, start it again and retry the update from Archicad.')
# Shown when another update of the same Archicad runs, the same texts as the
# legacy update script shows. Quitting Archicad before that update asks it to
# quit would make that update fail, so the user is told to quit only while
# that update waits for Archicad to quit.
UPDATE_RUNNING_TEXT = 'Tapir is being updated already. Wait for that update to finish: it closes Archicad itself and starts it again.'
UPDATE_WAITING_FOR_QUIT_TEXT = ('Tapir is being updated already, and that update waits for Archicad to quit. '
    'Quit Archicad to finish it; Archicad is then started again.')
# What the update that holds the lock does, written after the first byte of
# the lock file, which is the locked one (on Windows, no other file handle
# can read a locked byte, not even one of the same process), as the legacy
# update script does: 'quitting' while it waits for Archicad to quit,
# otherwise 'updating'. Both have 8 bytes, so each overwrites the other.
LOCK_STATE_OFFSET = 1
LOCK_STATE_WAITING_FOR_QUIT = b'quitting'
LOCK_STATE_UPDATING = b'updating'
# Update mode: how long Archicad gets to answer a command (and, once
# connected, when checked whether it still runs), how long it gets to quit in
# console mode (the GUI waits until the user cancels), when the GUI says what
# it waits for, and how long the add-on file may stay locked after that.
ARCHICAD_COMMAND_TIMEOUT_SECONDS = 10
ARCHICAD_ANSWER_TIMEOUT_SECONDS = 2
ARCHICAD_QUIT_TIMEOUT_SECONDS = 120
ARCHICAD_QUIT_NOTICE_SECONDS = 5
ARCHICAD_PROCESS_EXIT_TIMEOUT_SECONDS = 60
ARCHICAD_POLL_INTERVAL_SECONDS = 1.0
REPLACE_RETRY_TIMEOUT_SECONDS = 60
# How long the lock of another update is waited for: the legacy update script
# holds it while it starts the installer, and releases it right after.
UPDATE_LOCK_WAIT_SECONDS = 3


def IsUsingWindows ():
    return platform.system () == 'Windows'


def IsUsingMacOS ():
    return platform.system () == 'Darwin'


class InstallerError (Exception):
    pass


class UpdateCancelledError (InstallerError):
    pass


class UpdateRunningError (InstallerError):
    pass


class InstallerWarning (Exception):
    # Tapir was updated, but something is left to do by hand. Not an
    # InstallerError, so that it is not reported as a failure.
    pass


class ArchicadInstallation:
    def __init__ (self, version, installPath, addOnsFolderPath):
        self.version = version
        self.installPath = installPath
        self.addOnsFolderPath = addOnsFolderPath

    def __repr__ (self):
        return 'Archicad {0} ({1})'.format (self.version, self.installPath)


def GetArchicadVersionFromName (name):
    match = re.search (r'archicad\s+(\d+)', name, re.IGNORECASE)
    if match is None:
        return None
    return int (match.group (1))


def GetMarkerFileDepth (folderPath, depth = 0):
    # Returns how deep below folderPath the marker file is, or None.
    try:
        entryNames = os.listdir (folderPath)
    except OSError:
        return None
    if any (entryName.lower () == ADDONS_FOLDER_MARKER_FILE_NAME.lower () for entryName in entryNames):
        return depth
    if depth >= ADDONS_FOLDER_MARKER_MAX_DEPTH:
        return None
    foundDepths = []
    for entryName in entryNames:
        entryPath = os.path.join (folderPath, entryName)
        if os.path.isdir (entryPath) and not os.path.islink (entryPath):
            foundDepth = GetMarkerFileDepth (entryPath, depth + 1)
            if foundDepth is not None:
                foundDepths.append (foundDepth)
    return min (foundDepths) if len (foundDepths) > 0 else None


def NormalizeFolderName (folderName):
    # macOS returns file names in decomposed Unicode form, so accented names
    # must be normalized before comparing them.
    return unicodedata.normalize ('NFC', folderName).casefold ()


def FindAddOnsFolder (installPath):
    # The name of the Add-Ons folder is localized in the language versions of
    # Archicad. A folder with one of the known localized names is used first.
    # Otherwise the folder is identified by the XReadCfg.txt file inside it,
    # as the Graphisoft multi-language add-on guide recommends.
    try:
        entryNames = sorted (os.listdir (installPath))
    except OSError:
        return None
    folderPaths = []
    for entryName in entryNames:
        entryPath = os.path.join (installPath, entryName)
        if os.path.isdir (entryPath) and not entryName.endswith ('.app'):
            folderPaths.append (entryPath)
    knownNames = [NormalizeFolderName (folderName) for folderName in LOCALIZED_ADDONS_FOLDER_NAMES]
    for folderPath in folderPaths:
        if NormalizeFolderName (os.path.basename (folderPath)) in knownNames:
            return folderPath
    # Links are not followed by the marker search, so it cannot wander out
    # of the installation folder.
    bestFolderPath = None
    bestDepth = None
    for folderPath in folderPaths:
        if os.path.islink (folderPath):
            continue
        depth = GetMarkerFileDepth (folderPath)
        if depth is not None and (bestDepth is None or depth < bestDepth):
            bestFolderPath = folderPath
            bestDepth = depth
    return bestFolderPath


def AddInstallationCandidate (installations, name, folderPath):
    version = GetArchicadVersionFromName (name)
    # An application bundle is never an installation folder, even if it
    # happens to contain the Add-Ons folder marker file.
    if version is None or not os.path.isdir (folderPath) or os.path.normpath (folderPath).endswith ('.app'):
        return
    normalizedPath = os.path.normcase (os.path.normpath (folderPath))
    for installation in installations:
        if os.path.normcase (os.path.normpath (installation.installPath)) == normalizedPath:
            return
    addOnsFolderPath = FindAddOnsFolder (folderPath)
    if addOnsFolderPath is None:
        return
    installations.append (ArchicadInstallation (version, folderPath, addOnsFolderPath))


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
        try:
            vendorFolderNames = os.listdir (programFilesFolder)
        except OSError:
            continue
        for vendorFolderName in vendorFolderNames:
            if not vendorFolderName.lower ().startswith ('graphisoft'):
                continue
            vendorFolderPath = os.path.join (programFilesFolder, vendorFolderName)
            if not os.path.isdir (vendorFolderPath):
                continue
            try:
                folderNames = os.listdir (vendorFolderPath)
            except OSError:
                continue
            for folderName in folderNames:
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
    try:
        applicationsFolderNames = os.listdir (applicationsFolderPath)
    except OSError:
        return installations
    for folderName in applicationsFolderNames:
        folderPath = os.path.join (applicationsFolderPath, folderName)
        if os.path.isdir (folderPath) and not folderName.endswith ('.app'):
            foldersToScan.append (folderPath)
    for folderToScan in foldersToScan:
        try:
            folderNames = os.listdir (folderToScan)
        except OSError:
            continue
        for folderName in folderNames:
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


def CreateSSLContext ():
    # A PyInstaller-bundled Python ships its own OpenSSL, which does not see
    # the macOS Keychain and has no CA bundle of its own, so HTTPS fails with
    # CERTIFICATE_VERIFY_FAILED. Prefer the OS trust store (truststore, which
    # also honors corporate root certificates), then the certifi bundle, and
    # only then OpenSSL's defaults. Both packages are optional when running
    # from source and are bundled into the released executables.
    try:
        import truststore
        return truststore.SSLContext (ssl.PROTOCOL_TLS_CLIENT)
    except Exception:
        pass
    try:
        import certifi
        return ssl.create_default_context (cafile = certifi.where ())
    except Exception:
        pass
    return ssl.create_default_context ()


SSL_CONTEXT = None


def UrlOpen (request):
    global SSL_CONTEXT
    if SSL_CONTEXT is None:
        SSL_CONTEXT = CreateSSLContext ()
    return urllib.request.urlopen (request, timeout = DOWNLOAD_TIMEOUT_SECONDS, context = SSL_CONTEXT)


def DownloadUrl (url):
    request = urllib.request.Request (url, headers = { 'User-Agent' : USER_AGENT })
    with UrlOpen (request) as response:
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
    with UrlOpen (request) as response:
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
    # A connection that closes early ends the download without an error.
    for expectedSize in [totalSize, asset.get ('size') or 0]:
        if expectedSize > 0 and downloadedSize != expectedSize:
            raise InstallerError ('Failed to download {0}: received {1} bytes instead of {2}.'.format (asset['name'], downloadedSize, expectedSize))
    signature = ASSET_FILE_SIGNATURES.get (os.path.splitext (asset['name'])[1].lower ())
    if signature is not None:
        with open (targetPath, 'rb') as downloadedFile:
            if downloadedFile.read (len (signature)) != signature:
                raise InstallerError ('Failed to download {0}: the received file is not the add-on (was it blocked by a proxy?).'.format (asset['name']))
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
        # A running Archicad locks the loaded add-on file, which also surfaces
        # as PermissionError.
        raise InstallerError (PERMISSION_DENIED_TEXT)
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
    # Removes the Tapir subfolder and also the stray copies from earlier
    # manual installs, otherwise Archicad would keep loading the add-on.
    tapirFolderPath = os.path.join (addOnsFolderPath, TAPIR_SUBFOLDER_NAME)
    pathsToRemove = [path for path in [tapirFolderPath] + GetStrayTapirAddOnPaths (addOnsFolderPath) if os.path.lexists (path)]
    if len (pathsToRemove) == 0:
        return False
    try:
        for pathToRemove in pathsToRemove:
            RemovePath (pathToRemove)
    except PermissionError:
        if IsUsingMacOS ():
            RunShellCommandWithAdminPrivilegesMac (' && '.join (
                ['rm -rf {0}'.format (shlex.quote (pathToRemove)) for pathToRemove in pathsToRemove]))
        else:
            raise InstallerError (PERMISSION_DENIED_TEXT)
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


def GetRequestedVersions (args):
    if args.versions is None:
        return None
    try:
        return [int (version) for version in args.versions.split (',')]
    except ValueError:
        raise InstallerError ('Invalid --versions value: {0}'.format (args.versions))


def GetTargetInstallations (args):
    requestedVersions = GetRequestedVersions (args)
    if args.addOnsFolderPath is not None:
        if requestedVersions is None or len (requestedVersions) != 1:
            raise InstallerError ('--addOnsFolder requires --versions with exactly one Archicad version.')
        return [ArchicadInstallation (requestedVersions[0], os.path.dirname (os.path.normpath (args.addOnsFolderPath)), args.addOnsFolderPath)]
    installations = DetectArchicadInstallations (args.mockRootPath)
    if requestedVersions is not None:
        installations = [installation for installation in installations if installation.version in requestedVersions]
        detectedVersions = [installation.version for installation in installations]
        for requestedVersion in requestedVersions:
            if requestedVersion not in detectedVersions:
                raise InstallerError ('No Archicad {0} installation was found.'.format (requestedVersion))
    return installations


class AddOnUpdateTarget:
    # The add-on that update mode replaces, and the Archicad that runs it.
    def __init__ (self, version, addOnFilePath, archicadPort = None, archicadPid = None, restartArchicad = True):
        self.version = version
        self.addOnFilePath = addOnFilePath
        self.archicadPort = archicadPort
        self.archicadPid = archicadPid
        self.restartArchicad = restartArchicad
        # Once Archicad quit, the port and the process id no longer belong to
        # the Archicad to quit (a restarted one has a new process id), so the
        # update cannot be repeated with this target.
        self.archicadWasQuit = False


def IsUpdateMode (args):
    # The Archicad options alone also mean update mode, so that update mode
    # reports them as an error instead of an install silently ignoring them.
    return args.addOnFilePath is not None or args.archicadPort is not None or args.archicadPid is not None


def FindAddOnOnMappedDriveWin (addOnFilePath):
    # The installer runs as administrator, and an administrator process does
    # not see the drive letters that the user mapped to network folders. The
    # network folder of a persistent mapping is in the user's registry, so the
    # add-on is updated through that.
    drive, pathOnDrive = os.path.splitdrive (addOnFilePath)
    if os.path.lexists (addOnFilePath) or re.match (r'[A-Za-z]:$', drive) is None or os.path.exists (drive + os.sep):
        return addOnFilePath
    networkFilePath = None
    try:
        import winreg
        with winreg.OpenKey (winreg.HKEY_CURRENT_USER, 'Network\\' + drive[0]) as key:
            networkFilePath = winreg.QueryValueEx (key, 'RemotePath')[0].rstrip ('\\') + pathOnDrive
    except OSError:
        pass
    if networkFilePath is None or not os.path.lexists (networkFilePath):
        raise InstallerError ('The Tapir Add-On to update is on drive {0}, which the installer, running as administrator, cannot see '
            '(a mapped network drive?): {1}. Install the update manually: {2}'.format (drive, addOnFilePath, MANUAL_INSTALL_URL))
    return networkFilePath


def GetUpdateTarget (args):
    if args.addOnFilePath is None:
        raise InstallerError ('--archicadPort and --archicadPid require --addOnFile.')
    if args.addOnsFolderPath is not None or args.uninstall or args.mockRootPath is not None:
        raise InstallerError ('--addOnFile cannot be combined with --addOnsFolder, --uninstall or --mockRoot.')
    requestedVersions = GetRequestedVersions (args)
    if requestedVersions is None or len (requestedVersions) != 1:
        raise InstallerError ('--addOnFile requires --versions with exactly one Archicad version.')
    if args.archicadPid is not None and args.archicadPort is None:
        raise InstallerError ('--archicadPid requires --archicadPort.')
    if args.archicadPort is not None and not 0 < args.archicadPort < 65536:
        raise InstallerError ('Invalid --archicadPort value: {0}'.format (args.archicadPort))
    if args.archicadPid is not None and args.archicadPid <= 0:
        raise InstallerError ('Invalid --archicadPid value: {0}'.format (args.archicadPid))
    if args.addOnFilePath == '':
        raise InstallerError ('--addOnFile requires the path of the Tapir Add-On to update.')
    # normpath also drops a trailing separator, which would put the
    # temporary copy of a bundle inside the bundle.
    addOnFilePath = os.path.normpath (os.path.abspath (args.addOnFilePath))
    if IsUsingWindows ():
        addOnFilePath = FindAddOnOnMappedDriveWin (addOnFilePath)
    if not os.path.lexists (addOnFilePath):
        raise InstallerError ('The Tapir Add-On to update was not found: {0}'.format (addOnFilePath))
    # The installer runs as administrator on Windows, so it must not replace
    # anything else than an add-on by mistake.
    if IsUsingMacOS ():
        if not os.path.isdir (addOnFilePath) or not addOnFilePath.lower ().endswith ('.bundle'):
            raise InstallerError ('Not an add-on bundle: {0}'.format (addOnFilePath))
    elif os.path.isdir (addOnFilePath) or not addOnFilePath.lower ().endswith ('.apx'):
        raise InstallerError ('Not an add-on file (.apx): {0}'.format (addOnFilePath))
    return AddOnUpdateTarget (requestedVersions[0], addOnFilePath, args.archicadPort, args.archicadPid, not args.noRestart)


def GetUpdateLockFilePath (archicadPort):
    # The lock file of the legacy update script, so that an update by the
    # installer and one by the script exclude each other too.
    return os.path.join (tempfile.gettempdir (), 'TapirUpdate_{0}.lock'.format (archicadPort))


def WriteLockState (lockFile, state):
    # The state only picks the text of another update, so failing is
    # harmless and never stops the update.
    try:
        os.lseek (lockFile, LOCK_STATE_OFFSET, os.SEEK_SET)
        os.write (lockFile, state)
    except Exception:
        pass


def ReadLockState (lockFile):
    # Never reads the locked first byte. Returns None if the state cannot be
    # read.
    try:
        os.lseek (lockFile, LOCK_STATE_OFFSET, os.SEEK_SET)
        return os.read (lockFile, len (LOCK_STATE_WAITING_FOR_QUIT))
    except Exception:
        return None


def LockUpdateLockFile (lockFile):
    # Returns whether the lock was taken: False if the file cannot be locked,
    # then the update goes on unlocked. Raises UpdateRunningError if another
    # update still holds the lock after the wait. The system releases the
    # lock when the process ends, also when it is killed.
    deadline = time.monotonic () + UPDATE_LOCK_WAIT_SECONDS
    while True:
        try:
            if os.name == 'nt':
                # Locks the byte at the file position, which is the first one:
                # the state is only read once the wait has ended.
                import msvcrt
                msvcrt.locking (lockFile, msvcrt.LK_NBLCK, 1)
            else:
                import fcntl
                fcntl.flock (lockFile, fcntl.LOCK_EX | fcntl.LOCK_NB)
            return True
        # Another update holds the lock (msvcrt raises EACCES then).
        except (BlockingIOError, PermissionError):
            if time.monotonic () >= deadline:
                # Anything but an exact 'quitting' (an empty, cut off or
                # unreadable state) gets the text that does not ask to quit.
                isWaitingForQuit = ReadLockState (lockFile) == LOCK_STATE_WAITING_FOR_QUIT
                raise UpdateRunningError (UPDATE_WAITING_FOR_QUIT_TEXT if isWaitingForQuit else UPDATE_RUNNING_TEXT)
        except OSError:
            return False
        time.sleep (0.2)


@contextlib.contextmanager
def UpdateLock (updateTarget, dryRun):
    # One update per Archicad: the Add-On starts a new update also while one
    # runs, and both would quit Archicad, replace the add-on and start
    # Archicad again. Held from before Archicad is contacted until it was
    # started again. The file is not deleted, as another update may have
    # opened it already. If the file cannot be used, the update goes on
    # unlocked. A dry run touches nothing, so it takes no lock. Yields the
    # function that tells other updates whether this one waits for Archicad
    # to quit (see QuitArchicadAndWait); without the lock it does nothing.
    if dryRun or updateTarget.archicadPort is None:
        yield lambda isWaiting : None
        return
    try:
        lockFile = os.open (GetUpdateLockFilePath (updateTarget.archicadPort), os.O_RDWR | os.O_CREAT)
    except OSError:
        lockFile = None
    try:
        isLocked = lockFile is not None and LockUpdateLockFile (lockFile)

        def SetWaitingForQuit (isWaiting):
            if isLocked:
                WriteLockState (lockFile, LOCK_STATE_WAITING_FOR_QUIT if isWaiting else LOCK_STATE_UPDATING)

        # Also overwrites the state of an update that was killed while it
        # waited.
        SetWaitingForQuit (False)
        yield SetWaitingForQuit
    finally:
        if lockFile is not None:
            os.close (lockFile)


def GetTapirCommandRequestData (commandName):
    requestData = {
        'command' : 'API.ExecuteAddOnCommand',
        'parameters' : {
            'addOnCommandId' : { 'commandNamespace' : 'TapirCommand', 'commandName' : commandName },
            'addOnCommandParameters' : {}
        }
    }
    return json.dumps (requestData).encode ('utf-8')


def RunTapirCommand (archicadPort, commandName, timeoutSeconds = None):
    # The opener ignores the proxy settings: a system proxy could otherwise
    # intercept the requests to 127.0.0.1.
    if timeoutSeconds is None:
        timeoutSeconds = ARCHICAD_COMMAND_TIMEOUT_SECONDS
    request = urllib.request.Request ('http://127.0.0.1:{0}'.format (archicadPort), data = GetTapirCommandRequestData (commandName),
        headers = { 'Content-Type' : 'application/json' })
    opener = urllib.request.build_opener (urllib.request.ProxyHandler ({}))
    with opener.open (request, timeout = timeoutSeconds) as response:
        responseJson = json.loads (response.read ().decode ('utf-8'))
    try:
        return responseJson['result']['addOnCommandResponse']
    except (KeyError, TypeError):
        raise InstallerError ('Archicad failed to execute {0}: {1}'.format (commandName, responseJson))


def GetArchicadLocation (archicadPort):
    try:
        return RunTapirCommand (archicadPort, 'GetArchicadLocation')['archicadLocation']
    except Exception as e:
        raise InstallerError ('Failed to connect to Archicad on port {0}: {1}'.format (archicadPort, e))


def AllowArchicadToComeToFront (archicadPid):
    # Windows only lets the foreground process, the installer, pass the
    # foreground on: otherwise Archicad's save dialog may stay behind the
    # installer window, flashing in the taskbar only.
    if not IsUsingWindows () or archicadPid is None:
        return
    try:
        import ctypes
        import ctypes.wintypes
        user32 = ctypes.WinDLL ('user32', use_last_error = True)
        user32.AllowSetForegroundWindow.argtypes = [ctypes.wintypes.DWORD]
        user32.AllowSetForegroundWindow.restype = ctypes.wintypes.BOOL
        user32.AllowSetForegroundWindow (archicadPid)
    except Exception:
        pass


def QuitArchicad (archicadPort):
    # Archicad may answer only once the user answered its save prompt. A
    # refusal then still fails the update at once.
    try:
        response = RunTapirCommand (archicadPort, 'QuitArchicad', ARCHICAD_QUIT_TIMEOUT_SECONDS)
    except InstallerError as e:
        # Archicad answered without running the command.
        raise InstallerError (ARCHICAD_DID_NOT_QUIT_TEXT.format (e))
    except Exception:
        # The connection may drop while Archicad is quitting. Whether it
        # quit is checked afterwards.
        return
    if isinstance (response, dict) and response.get ('success') is False:
        # Archicad refuses to quit, for example while another add-on works.
        # Waiting for it would not change that.
        error = response.get ('error')
        reason = error.get ('message') if isinstance (error, dict) else None
        raise InstallerError (ARCHICAD_DID_NOT_QUIT_TEXT.format (reason or 'quitting was refused'))


class QuitArchicadRequest (threading.Thread):
    # Sends QuitArchicad in the background, so that the wait for its answer
    # can show what it waits for and react to Cancel.
    def __init__ (self, archicadPort):
        threading.Thread.__init__ (self, daemon = True)
        self.archicadPort = archicadPort
        self.error = None

    def run (self):
        try:
            QuitArchicad (self.archicadPort)
        except InstallerError as e:
            self.error = e


def IsArchicadAnswering (archicadPort):
    # Connecting gets the full timeout: Windows refuses a closed port only
    # after retries, which take about 2 s. A connection means that Archicad
    # still runs, so its answer is not waited for long: Archicad gives none
    # while busy, for example with its save prompt. A timeout of either
    # means that Archicad still runs. http.client ignores the proxy settings.
    connection = http.client.HTTPConnection ('127.0.0.1', archicadPort, timeout = ARCHICAD_COMMAND_TIMEOUT_SECONDS)
    try:
        connection.connect ()
        connection.sock.settimeout (ARCHICAD_ANSWER_TIMEOUT_SECONDS)
        connection.request ('POST', '/', GetTapirCommandRequestData ('GetAddOnVersion'),
            { 'Content-Type' : 'application/json', 'Connection' : 'close' })
        connection.getresponse ().read ()
    except socket.timeout:
        return True
    except (OSError, http.client.HTTPException):
        # The port is closed, or Archicad dropped the connection as it quits.
        return False
    finally:
        connection.close ()
    return True


def WaitForProcessToExit (pid, timeoutSeconds):
    # Returns False if the process still runs after the timeout.
    if IsUsingWindows ():
        # os.kill (pid, 0) would terminate the process on Windows.
        import ctypes
        import ctypes.wintypes
        SYNCHRONIZE = 0x00100000
        WAIT_TIMEOUT = 0x00000102
        kernel32 = ctypes.WinDLL ('kernel32', use_last_error = True)
        kernel32.OpenProcess.argtypes = [ctypes.wintypes.DWORD, ctypes.wintypes.BOOL, ctypes.wintypes.DWORD]
        kernel32.OpenProcess.restype = ctypes.wintypes.HANDLE
        kernel32.WaitForSingleObject.argtypes = [ctypes.wintypes.HANDLE, ctypes.wintypes.DWORD]
        kernel32.WaitForSingleObject.restype = ctypes.wintypes.DWORD
        kernel32.CloseHandle.argtypes = [ctypes.wintypes.HANDLE]
        processHandle = kernel32.OpenProcess (SYNCHRONIZE, False, pid)
        if not processHandle:
            # The process is gone (or not accessible; then the replace
            # retries wait for the add-on file lock instead).
            return True
        try:
            return kernel32.WaitForSingleObject (processHandle, int (timeoutSeconds * 1000)) != WAIT_TIMEOUT
        finally:
            kernel32.CloseHandle (processHandle)
    deadline = time.monotonic () + timeoutSeconds
    while True:
        try:
            os.kill (pid, 0)
        except ProcessLookupError:
            return True
        except PermissionError:
            # The process exists, it only belongs to another user.
            pass
        if time.monotonic () >= deadline:
            return False
        time.sleep (ARCHICAD_POLL_INTERVAL_SECONDS)


def QuitArchicadAndWait (updateTarget, cancelEvent = None, waitingCallback = None, setWaitingForQuit = None):
    # Raises only while Archicad still runs: then nothing may be replaced
    # and it must not be started a second time. The user may answer
    # Archicad's save prompt any time later, so with a cancel event (the
    # GUI) there is no deadline. Without one (console mode, for automated
    # runs) there is, so that it never hangs. waitingCallback (True) tells
    # that Archicad takes long to quit, (False) that the wait ended.
    # setWaitingForQuit (from UpdateLock) tells other updates that this one
    # waits for Archicad to quit, from just before Archicad is asked to quit
    # until the wait ended, however it ended: only meanwhile may they tell
    # the user to quit Archicad.
    startTime = time.monotonic ()
    quitRequest = QuitArchicadRequest (updateTarget.archicadPort)
    isWaitingReported = False
    if setWaitingForQuit is not None:
        setWaitingForQuit (True)
    try:
        quitRequest.start ()
        while True:
            # While the quit request gets no answer, Archicad still runs:
            # quitting would drop the connection.
            if not quitRequest.is_alive ():
                if quitRequest.error is not None:
                    raise quitRequest.error
                if not IsArchicadAnswering (updateTarget.archicadPort):
                    break
            # Only reported once Archicad was found running, as it may have
            # quit while the previous check or wait took long.
            if waitingCallback is not None and not isWaitingReported and time.monotonic () - startTime >= ARCHICAD_QUIT_NOTICE_SECONDS:
                isWaitingReported = True
                waitingCallback (True)
            if cancelEvent is not None:
                if cancelEvent.is_set ():
                    raise UpdateCancelledError (UPDATE_CANCELLED_TEXT)
            elif time.monotonic () - startTime >= ARCHICAD_QUIT_TIMEOUT_SECONDS:
                raise InstallerError (ARCHICAD_DID_NOT_QUIT_TEXT.format ('was quitting cancelled?'))
            if quitRequest.is_alive ():
                # Wakes up at once on the answer.
                quitRequest.join (ARCHICAD_POLL_INTERVAL_SECONDS)
            elif cancelEvent is not None:
                # Wakes up at once on cancel, and checks the port first.
                cancelEvent.wait (ARCHICAD_POLL_INTERVAL_SECONDS)
            else:
                time.sleep (ARCHICAD_POLL_INTERVAL_SECONDS)
    finally:
        if setWaitingForQuit is not None:
            setWaitingForQuit (False)
        if isWaitingReported:
            waitingCallback (False)
    # Once the port closed, Archicad has quit: from here on it is started
    # again whatever happens, also when the wait was cancelled meanwhile, and
    # the update cannot be repeated with this target.
    updateTarget.archicadWasQuit = True
    if updateTarget.archicadPid is None:
        return
    # The port closes before the process ends. On Windows the process keeps
    # the add-on file locked, and on macOS the new Archicad would start
    # while the old one still exits. The process may also stay much longer,
    # for example behind a crash report dialog: then the update goes on
    # anyway (the Windows replace retries wait for the file lock), as
    # Archicad would otherwise not be started again.
    try:
        WaitForProcessToExit (updateTarget.archicadPid, ARCHICAD_PROCESS_EXIT_TIMEOUT_SECONDS)
    except Exception:
        pass


def StartArchicad (archicadLocation):
    if IsUsingMacOS ():
        # Archicad is only started once its port closed, so an instance that
        # is still there is exiting: plain open would only activate that one.
        subprocess.Popen (['open', '-n', archicadLocation])
    elif IsUsingWindows ():
        # The installer runs as administrator (--uac-admin), so Archicad
        # started directly would too. Explorer starts it as the logged in user.
        subprocess.Popen ([os.path.join (os.environ.get ('SystemRoot', 'C:\\Windows'), 'explorer.exe'), archicadLocation])
    else:
        subprocess.Popen ([archicadLocation])


def RemovePathQuietly (path):
    try:
        RemovePath (path)
    except OSError:
        pass


def GetNewAddOnPath (targetPath):
    # The new copy is created next to the target, on the same volume, so
    # that it can be renamed into place.
    return targetPath + '.new'


def GetOldAddOnPath (targetPath):
    # Where the old macOS bundle is moved aside until the new one is in place.
    return targetPath + '.old'


def RemoveNewAddOnCopy (targetPath):
    # Only while the add-on is in place: if a failed replace left it missing,
    # the copies next to it are the only ones left.
    if os.path.lexists (targetPath):
        RemovePathQuietly (GetNewAddOnPath (targetPath))


def PrepareAddOnUpdate (downloadedFilePath, targetPath, workFolderPath):
    # Does what can fail without touching the installed add-on, so that a
    # failure leaves Archicad running. Returns the path to move into place.
    if IsUsingMacOS ():
        extractFolderPath = os.path.join (workFolderPath, 'Extracted')
        os.makedirs (extractFolderPath)
        result = subprocess.run (['ditto', '-x', '-k', downloadedFilePath, extractFolderPath], capture_output = True, text = True)
        if result.returncode != 0:
            raise InstallerError ('Failed to extract {0}: {1}'.format (os.path.basename (downloadedFilePath), result.stderr.strip ()))
        bundleNames = [entryName for entryName in os.listdir (extractFolderPath) if entryName.lower ().endswith ('.bundle')]
        if len (bundleNames) != 1:
            raise InstallerError ('{0} does not contain exactly one add-on bundle.'.format (os.path.basename (downloadedFilePath)))
        return os.path.join (extractFolderPath, bundleNames[0])
    # Copying next to the add-on also checks that its folder is writable.
    newFilePath = GetNewAddOnPath (targetPath)
    try:
        RemovePath (newFilePath)
        shutil.copyfile (downloadedFilePath, newFilePath)
    except PermissionError:
        raise InstallerError (PERMISSION_DENIED_TEXT)
    except OSError as e:
        raise InstallerError ('Failed to copy the add-on next to {0}: {1}'.format (targetPath, e))
    return newFilePath


def ReplaceAddOnMac (extractedBundlePath, targetPath):
    # The bundle is replaced, not merged, so files removed from the add-on
    # do not stay. The new bundle gets the name of the target, which
    # Archicad's add-on list refers to; the asset's bundle name may differ.
    # The old bundle is moved aside and only deleted once the new one is in
    # place, so it is never half deleted, and a failure puts it back. Returns
    # a warning text when the old bundle could not be deleted.
    newBundlePath = GetNewAddOnPath (targetPath)
    oldBundlePath = GetOldAddOnPath (targetPath)
    quotedNewPath = shlex.quote (newBundlePath)
    quotedOldPath = shlex.quote (oldBundlePath)
    quotedTargetPath = shlex.quote (targetPath)
    oldBundleNotDeletedText = 'Its old version could not be deleted: delete {0} by hand.'.format (oldBundlePath)
    # Leftovers of an earlier attempt are only deleted while the target is
    # there, so that they are never the only copy.
    if not os.path.isdir (targetPath):
        raise InstallerError ('The Tapir Add-On to update was not found: {0}'.format (targetPath))
    try:
        RemovePath (newBundlePath)
        RemovePath (oldBundlePath)
        subprocess.run (['ditto', extractedBundlePath, newBundlePath], check = True, capture_output = True)
        os.rename (targetPath, oldBundlePath)
        try:
            os.rename (newBundlePath, targetPath)
        except OSError as e:
            try:
                os.rename (oldBundlePath, targetPath)
            except OSError:
                # Not a PermissionError, so that the fallback below does not
                # run: it expects the target in place.
                raise InstallerError ('Failed to replace {0}: {1}. The old add-on could not be moved back either: rename {2} back by hand.'.format (
                    targetPath, e, oldBundlePath))
            raise
    except (PermissionError, subprocess.CalledProcessError):
        # The same steps. If the new bundle cannot be moved into place, the
        # old one is moved back and the command fails. Once it is in place,
        # an old bundle that cannot be deleted (even root cannot delete a
        # locked file) is only a warning, as below.
        RunShellCommandWithAdminPrivilegesMac (' && '.join ([
            'rm -rf {0}'.format (quotedNewPath),
            'rm -rf {0}'.format (quotedOldPath),
            'ditto {0} {1}'.format (shlex.quote (extractedBundlePath), quotedNewPath),
            'mv {0} {1}'.format (quotedTargetPath, quotedOldPath),
            '{{ mv {0} {1} || {{ mv {2} {1}; false; }}; }}'.format (quotedNewPath, quotedTargetPath, quotedOldPath),
            '{{ rm -rf {0} || true; }}'.format (quotedOldPath),
        ]))
        return oldBundleNotDeletedText if os.path.lexists (oldBundlePath) else None
    try:
        RemovePath (oldBundlePath)
    except PermissionError:
        # Some files of the old bundle may belong to another user, although
        # its folder is writable.
        try:
            RunShellCommandWithAdminPrivilegesMac ('rm -rf {0}'.format (quotedOldPath))
        except InstallerError:
            return oldBundleNotDeletedText
    except OSError:
        return oldBundleNotDeletedText
    return None


def ReplaceAddOnWin (newFilePath, targetPath):
    # Archicad releases the lock on the loaded add-on only once its process
    # has fully exited, which can be well after its port closed.
    deadline = time.monotonic () + REPLACE_RETRY_TIMEOUT_SECONDS
    while True:
        try:
            os.replace (newFilePath, targetPath)
            return
        except OSError as e:
            if time.monotonic () >= deadline:
                if isinstance (e, PermissionError):
                    raise InstallerError (PERMISSION_DENIED_TEXT)
                raise InstallerError ('Failed to replace {0}: {1}'.format (targetPath, e))
            time.sleep (1.0)


def ReplaceAddOn (preparedPath, targetPath):
    # Returns a warning text, or None.
    if IsUsingMacOS ():
        return ReplaceAddOnMac (preparedPath, targetPath)
    ReplaceAddOnWin (preparedPath, targetPath)
    return None


def GetReleaseName (releaseInfo):
    tagName = releaseInfo.get ('tag_name', '?')
    # The fallback release info has no real tag.
    return 'the latest version' if tagName == 'latest' else tagName


def UpdateAddOn (updateTarget, releaseInfo, dryRun, statusCallback = None, progressCallback = None, cancelEvent = None, waitingCallback = None,
        setWaitingForQuit = None):
    """Replaces the add-on of update mode with the latest release, quitting
    Archicad before and starting it again after. Returns a human readable
    result string, or raises InstallerError (UpdateCancelledError when the
    wait for Archicad to quit was cancelled through cancelEvent), or
    InstallerWarning when Tapir was updated, but something is left to do by
    hand. cancelEvent, waitingCallback and setWaitingForQuit (the function
    that UpdateLock yields): see QuitArchicadAndWait."""
    def ReportStatus (statusText):
        if statusCallback is not None:
            statusCallback (statusText)

    def OnWaitingForArchicad (isWaiting):
        if isWaiting:
            ReportStatus ('Waiting for Archicad...')
        if waitingCallback is not None:
            waitingCallback (isWaiting)

    asset = FindAssetForVersion (releaseInfo, updateTarget.version)
    if asset is None:
        raise InstallerError ('No Tapir Add-On is available for Archicad {0} in release {1}.'.format (
            updateTarget.version, releaseInfo.get ('tag_name', '?')))
    targetPath = updateTarget.addOnFilePath
    archicadPort = updateTarget.archicadPort
    restartArchicad = archicadPort is not None and updateTarget.restartArchicad
    workFolderPath = tempfile.mkdtemp (prefix = 'TapirInstaller_')
    try:
        # Archicad is only quit once the download succeeded.
        try:
            downloadedFilePath = DownloadAsset (asset, workFolderPath, progressCallback)
        except InstallerError:
            raise
        except Exception as e:
            raise InstallerError ('Failed to download {0}: {1}'.format (asset['name'], e))
        if dryRun:
            steps = ['replace {0} with {1}'.format (targetPath, asset['name'])]
            if archicadPort is not None:
                steps.insert (0, 'quit Archicad on port {0}'.format (archicadPort))
            if restartArchicad:
                steps.append ('restart Archicad')
            return 'Would {0}'.format (', '.join (steps))
        try:
            preparedPath = PrepareAddOnUpdate (downloadedFilePath, targetPath, workFolderPath)
            archicadLocation = None
            if archicadPort is not None:
                ReportStatus ('Closing Archicad...')
                # Asked before quitting: Archicad is started again from here.
                archicadLocation = GetArchicadLocation (archicadPort)
                AllowArchicadToComeToFront (updateTarget.archicadPid)
                # Raises when Archicad still runs: then nothing is replaced,
                # and Archicad is not started a second time.
                QuitArchicadAndWait (updateTarget, cancelEvent, OnWaitingForArchicad, setWaitingForQuit)
            # From here on every error is caught, so that Archicad is always
            # started again below, also when the replace failed.
            replaceErrorText = None
            replaceWarningText = None
            try:
                ReportStatus ('Installing...')
                replaceWarningText = ReplaceAddOn (preparedPath, targetPath)
            except InstallerError as e:
                replaceErrorText = str (e)
            except Exception as e:
                replaceErrorText = 'Failed to replace {0}: {1}'.format (targetPath, e)
        finally:
            RemoveNewAddOnCopy (targetPath)
        restartErrorText = None
        if restartArchicad:
            ReportStatus ('Restarting Archicad...')
            try:
                StartArchicad (archicadLocation)
            except Exception as e:
                restartErrorText = 'Archicad could not be started again: {0}'.format (e)
        resultTexts = [replaceErrorText or 'Tapir was updated to {0}.'.format (GetReleaseName (releaseInfo))]
        if replaceWarningText is not None:
            resultTexts.append (replaceWarningText)
        if restartArchicad:
            resultTexts.append (restartErrorText or ('Archicad was started again.' if replaceErrorText else 'Archicad is starting again.'))
        if replaceErrorText is not None and updateTarget.archicadWasQuit:
            resultTexts.append ('Start the update again from Archicad.')
        # Error texts may end without a period.
        resultText = ' '.join (text if text.endswith ('.') else text + '.' for text in resultTexts)
        if replaceErrorText is not None or restartErrorText is not None:
            raise InstallerError (resultText)
        if replaceWarningText is not None:
            raise InstallerWarning (resultText)
        return resultText
    finally:
        shutil.rmtree (workFolderPath, ignore_errors = True)


def RunConsoleUpdate (args):
    updateTarget = GetUpdateTarget (args)
    print ('Updating the Tapir Add-On of Archicad {0}: {1}'.format (updateTarget.version, updateTarget.addOnFilePath))
    with UpdateLock (updateTarget, args.dryRun) as setWaitingForQuit:
        releaseInfo = GetLatestReleaseInfo ()
        print ('Latest Tapir release: {0}'.format (releaseInfo.get ('tag_name', '?')))
        print ('Downloading...')
        try:
            print (UpdateAddOn (updateTarget, releaseInfo, args.dryRun, statusCallback = print, setWaitingForQuit = setWaitingForQuit))
        except InstallerWarning as e:
            print (e)
    return 0


def RunConsoleInstaller (args):
    try:
        if IsUpdateMode (args):
            return RunConsoleUpdate (args)
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


# The Tapir logo (branding/logo/png/tapir_logo_black_512.png on a light disc,
# so it reads on both light and dark window backgrounds), 80x80 PNG, embedded
# so that the installer stays a single file both from source and when bundled.
TAPIR_LOGO_PNG_BASE64 = (
    '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'
    '0Zj2nAAAAABJRU5ErkJggg=='
)


def IsTapirInstalled (addOnsFolderPath):
    return os.path.isdir (os.path.join (addOnsFolderPath, TAPIR_SUBFOLDER_NAME)) or len (GetStrayTapirAddOnPaths (addOnsFolderPath)) > 0


def EnableWindowsHighDpi ():
    # Without this Windows renders the window at 96 DPI and stretches it,
    # which makes every text blurry on high resolution displays.
    if not IsUsingWindows ():
        return
    try:
        import ctypes
        ctypes.windll.shcore.SetProcessDpiAwareness (1)
    except Exception:
        pass


def RunGuiInstaller (args):
    import tkinter
    import tkinter.font
    import webbrowser

    EnableWindowsHighDpi ()

    # Update mode shows the one add-on to update instead of the detected
    # installations, and starts as soon as the latest release is known.
    isUpdateMode = IsUpdateMode (args)

    # Colors follow the Tapir branding: charcoal on white, green accent.
    COLORS = {
        'window' : '#F4F5F7',
        'surface' : '#FFFFFF',
        'border' : '#E2E4E8',
        'borderSelected' : '#2E7D32',
        'text' : '#202225',
        'mutedText' : '#6B7078',
        'accent' : '#2E7D32',
        'accentHover' : '#256B29',
        'accentText' : '#FFFFFF',
        'secondary' : '#FFFFFF',
        'secondaryHover' : '#F0F1F3',
        'disabled' : '#C9CCD1',
        'disabledText' : '#FFFFFF',
        'progressTrack' : '#E8EAED',
        'success' : '#2E7D32',
        'successTint' : '#E7F3E8',
        'error' : '#C62828',
        'errorTint' : '#FDECEC',
        'busy' : '#0277BD',
        'busyTint' : '#E3F1FA',
        'warning' : '#9A5B00',
        'warningTint' : '#FFF3E0',
        'neutral' : '#5F6368',
        'neutralTint' : '#EEF0F2',
    }
    STATUS_COLORS = {
        'neutral' : ('neutral', 'neutralTint'),
        'busy' : ('busy', 'busyTint'),
        'success' : ('success', 'successTint'),
        'warning' : ('warning', 'warningTint'),
        'error' : ('error', 'errorTint'),
    }

    # Fonts are sized in points, so Tk scales them to the display DPI, but
    # pixel sizes are not scaled: Px converts the pixel sizes designed for
    # 96 DPI, so the layout grows with the text on high DPI Windows displays.
    # macOS scales the whole window itself, so the factor stays 1 there.
    uiScale = [1.0]

    def Px (value):
        return int (round (value * uiScale[0]))

    def GetFontFamily (root):
        availableFamilies = set (tkinter.font.families (root))
        for family in ['Segoe UI', 'SF Pro Text', 'Helvetica Neue', 'Inter', 'Cantarell', 'DejaVu Sans']:
            if family in availableFamilies:
                return family
        return tkinter.font.nametofont ('TkDefaultFont').actual ('family')

    class FlatButton (tkinter.Label):
        # tkinter.Button ignores background colors on macOS, so a label is
        # used to get the same flat look on both platforms.
        def __init__ (self, parent, text, command, font, style):
            tkinter.Label.__init__ (self, parent, text = text, font = font, padx = Px (18), pady = Px (7), cursor = 'hand2')
            self.command = command
            self.style = style
            self.isEnabled = True
            self.isHovered = False
            self.bind ('<Enter>', lambda event : self.SetHovered (True))
            self.bind ('<Leave>', lambda event : self.SetHovered (False))
            self.bind ('<ButtonRelease-1>', lambda event : self.OnClicked ())
            self.UpdateLook ()

        def SetEnabled (self, isEnabled):
            self.isEnabled = isEnabled
            self.configure (cursor = 'hand2' if isEnabled else 'arrow')
            self.UpdateLook ()

        def SetHovered (self, isHovered):
            self.isHovered = isHovered
            self.UpdateLook ()

        def OnClicked (self):
            if self.isEnabled:
                self.command ()

        def UpdateLook (self):
            if self.style == 'primary':
                if not self.isEnabled:
                    self.configure (bg = COLORS['disabled'], fg = COLORS['disabledText'], highlightthickness = 0)
                else:
                    self.configure (bg = COLORS['accentHover' if self.isHovered else 'accent'], fg = COLORS['accentText'], highlightthickness = 0)
            else:
                background = COLORS['secondaryHover'] if self.isHovered and self.isEnabled else COLORS['secondary']
                self.configure (bg = background, fg = COLORS['text'] if self.isEnabled else COLORS['disabled'],
                    highlightthickness = Px (1), highlightbackground = COLORS['border'], highlightcolor = COLORS['border'])

    class CheckBox (tkinter.Canvas):
        def __init__ (self, parent, background):
            tkinter.Canvas.__init__ (self, parent, width = Px (18), height = Px (18), bg = background, highlightthickness = 0, cursor = 'hand2')

        def Draw (self, isChecked, isEnabled):
            # Coordinates are designed for an 18 pixel box at 96 DPI.
            self.delete ('all')
            scale = uiScale[0]
            box = (scale, scale, 17 * scale, 17 * scale)
            if isChecked:
                color = COLORS['accent'] if isEnabled else COLORS['disabled']
                self.create_rectangle (*box, fill = color, outline = color)
                checkMark = [coordinate * scale for coordinate in (4.5, 9.5, 7.5, 12.5, 13.5, 5.5)]
                self.create_line (*checkMark, fill = COLORS['accentText'], width = 2 * scale, capstyle = 'round', joinstyle = 'round')
            else:
                self.create_rectangle (*box, fill = COLORS['surface'], outline = COLORS['disabled'], width = 1.5 * scale)

    class ProgressBar (tkinter.Canvas):
        def __init__ (self, parent):
            tkinter.Canvas.__init__ (self, parent, width = 1, height = Px (4), bg = COLORS['progressTrack'], highlightthickness = 0)
            self.fraction = 0.0
            self.bind ('<Configure>', lambda event : self.Draw ())

        def SetFraction (self, fraction):
            self.fraction = max (0.0, min (1.0, fraction))
            self.Draw ()

        def Draw (self):
            self.delete ('all')
            width = self.winfo_width ()
            if self.fraction > 0.0:
                self.create_rectangle (0, 0, int (width * self.fraction), Px (4), fill = COLORS['busy'], width = 0)

    class InstallationRow:
        def __init__ (self, app, parent, installation):
            self.app = app
            self.installation = installation
            self.isSelected = True
            self.card = tkinter.Frame (parent, bg = COLORS['surface'], highlightthickness = Px (1), cursor = 'arrow' if isUpdateMode else 'hand2')
            self.card.pack (fill = 'x', pady = (0, Px (8)))
            contentFrame = tkinter.Frame (self.card, bg = COLORS['surface'])
            contentFrame.pack (fill = 'x', padx = Px (14), pady = Px (11))
            self.checkBox = CheckBox (contentFrame, COLORS['surface'])
            if not isUpdateMode:
                self.checkBox.pack (side = 'left', padx = (0, Px (12)))
            textFrame = tkinter.Frame (contentFrame, bg = COLORS['surface'])
            textFrame.pack (side = 'left', fill = 'x', expand = True)
            self.titleLabel = tkinter.Label (textFrame, text = 'Archicad {0}'.format (installation.version),
                font = app.fonts['cardTitle'], bg = COLORS['surface'], fg = COLORS['text'], anchor = 'w')
            self.titleLabel.pack (fill = 'x')
            self.pathLabel = tkinter.Label (textFrame, text = installation.addOnFilePath if isUpdateMode else installation.addOnsFolderPath,
                font = app.fonts['small'], bg = COLORS['surface'], fg = COLORS['mutedText'], anchor = 'w')
            self.pathLabel.pack (fill = 'x')
            # The update statuses are longer ("Restarting Archicad...").
            self.statusLabel = tkinter.Label (contentFrame, font = app.fonts['badge'], width = 19 if isUpdateMode else 16, padx = Px (6), pady = Px (3))
            self.statusLabel.pack (side = 'right', padx = (Px (12), 0))
            # The bar is always packed, so the card does not change height when
            # it appears; it is only invisible while there is no progress.
            self.progressBar = ProgressBar (self.card)
            self.progressBar.pack (fill = 'x', side = 'bottom')
            if isUpdateMode:
                self.SetStatus ('Ready to update', 'neutral')
                self.UpdateLook ()
                return
            for widget in [self.card, contentFrame, self.checkBox, textFrame, self.titleLabel, self.pathLabel]:
                widget.bind ('<Button-1>', lambda event : self.Toggle ())
            if IsTapirInstalled (installation.addOnsFolderPath):
                self.SetStatus ('Installed', 'success')
            else:
                self.SetStatus ('Not installed', 'neutral')
            self.UpdateLook ()

        def Toggle (self):
            if self.app.isWorking:
                return
            self.isSelected = not self.isSelected
            self.UpdateLook ()
            self.app.UpdateButtonStates ()

        def SetSelected (self, isSelected):
            self.isSelected = isSelected
            self.UpdateLook ()

        def UpdateLook (self):
            borderColor = COLORS['borderSelected'] if self.isSelected else COLORS['border']
            self.card.configure (highlightbackground = borderColor, highlightcolor = borderColor)
            self.checkBox.Draw (self.isSelected, not self.app.isWorking)

        def SetStatus (self, statusText, kind, progressFraction = None):
            foreground, background = STATUS_COLORS[kind]
            self.statusLabel.configure (text = statusText, fg = COLORS[foreground], bg = COLORS[background])
            if progressFraction is None:
                self.progressBar.configure (bg = COLORS['surface'])
                self.progressBar.SetFraction (0.0)
            else:
                self.progressBar.configure (bg = COLORS['progressTrack'])
                self.progressBar.SetFraction (progressFraction)

    class InstallerApp (tkinter.Tk):
        def __init__ (self):
            tkinter.Tk.__init__ (self)
            if IsUsingWindows ():
                uiScale[0] = max (1.0, self.winfo_fpixels ('1i') / 96.0)
            self.title (INSTALLER_TITLE)
            self.configure (bg = COLORS['window'])
            self.minsize (Px (560), 0)
            self.resizable (True, False)
            self.rows = []
            self.releaseInfo = None
            self.isWorking = False
            self.canUpdateAgain = True
            # Set while the update waits for Archicad to quit: then the Close
            # button cancels the wait.
            self.isWaitingForArchicad = False
            self.cancelEvent = None

            family = GetFontFamily (self)
            self.fonts = {
                'title' : tkinter.font.Font (self, family = family, size = 15, weight = 'bold'),
                'body' : tkinter.font.Font (self, family = family, size = 10),
                'bodyBold' : tkinter.font.Font (self, family = family, size = 10, weight = 'bold'),
                'cardTitle' : tkinter.font.Font (self, family = family, size = 11, weight = 'bold'),
                'small' : tkinter.font.Font (self, family = family, size = 9),
                'badge' : tkinter.font.Font (self, family = family, size = 9, weight = 'bold'),
                'section' : tkinter.font.Font (self, family = family, size = 8, weight = 'bold'),
                'link' : tkinter.font.Font (self, family = family, size = 9, underline = True),
            }

            # Header
            headerFrame = tkinter.Frame (self, bg = COLORS['surface'])
            headerFrame.pack (fill = 'x')
            headerContentFrame = tkinter.Frame (headerFrame, bg = COLORS['surface'])
            headerContentFrame.pack (fill = 'x', padx = Px (24), pady = Px (18))
            try:
                # Keep a reference, otherwise Tk drops the image.
                # The embedded logo is 80 pixels: halved for 40 pixels at 96 DPI,
                # used as it is from 150% display scaling.
                self.logoImage = tkinter.PhotoImage (data = TAPIR_LOGO_PNG_BASE64)
                if uiScale[0] < 1.5:
                    self.logoImage = self.logoImage.subsample (2, 2)
                tkinter.Label (headerContentFrame, image = self.logoImage, bg = COLORS['surface']).pack (side = 'left', padx = (0, Px (14)))
            except tkinter.TclError:
                # PNG support requires Tk 8.6; the logo is only decoration.
                pass
            titleFrame = tkinter.Frame (headerContentFrame, bg = COLORS['surface'])
            titleFrame.pack (side = 'left', fill = 'x', expand = True)
            tkinter.Label (titleFrame, text = INSTALLER_TITLE, font = self.fonts['title'], bg = COLORS['surface'], fg = COLORS['text'], anchor = 'w').pack (fill = 'x')
            self.releaseLabel = tkinter.Label (titleFrame, text = 'Checking the latest Tapir release...', font = self.fonts['body'],
                bg = COLORS['surface'], fg = COLORS['mutedText'], anchor = 'w')
            self.releaseLabel.pack (fill = 'x')
            tkinter.Frame (self, bg = COLORS['border'], height = Px (1)).pack (fill = 'x')

            # Body
            bodyFrame = tkinter.Frame (self, bg = COLORS['window'])
            bodyFrame.pack (fill = 'both', expand = True, padx = Px (24), pady = (Px (18), Px (10)))
            sectionFrame = tkinter.Frame (bodyFrame, bg = COLORS['window'])
            sectionFrame.pack (fill = 'x', pady = (0, Px (8)))
            tkinter.Label (sectionFrame, text = 'TAPIR UPDATE' if isUpdateMode else 'ARCHICAD INSTALLATIONS', font = self.fonts['section'], bg = COLORS['window'], fg = COLORS['mutedText']).pack (side = 'left')
            self.selectAllLabel = tkinter.Label (sectionFrame, text = '', font = self.fonts['link'], bg = COLORS['window'], fg = COLORS['accent'], cursor = 'hand2')
            if not isUpdateMode:
                self.selectAllLabel.pack (side = 'right')
            self.selectAllLabel.bind ('<Button-1>', lambda event : self.OnSelectAllClicked ())
            self.rowsFrame = tkinter.Frame (bodyFrame, bg = COLORS['window'])
            self.rowsFrame.pack (fill = 'x')
            self.messageLabel = tkinter.Label (self.rowsFrame, text = 'Preparing the update...' if isUpdateMode else 'Detecting Archicad installations...', font = self.fonts['body'],
                bg = COLORS['window'], fg = COLORS['mutedText'], anchor = 'w', justify = 'left')
            self.messageLabel.pack (fill = 'x', pady = Px (10))
            self.resultLabel = tkinter.Label (bodyFrame, text = '', font = self.fonts['body'], anchor = 'w', justify = 'left', padx = Px (12), pady = Px (9), bg = COLORS['window'])

            # Footer
            tkinter.Frame (self, bg = COLORS['border'], height = Px (1)).pack (fill = 'x')
            footerFrame = tkinter.Frame (self, bg = COLORS['surface'])
            footerFrame.pack (fill = 'x')
            footerContentFrame = tkinter.Frame (footerFrame, bg = COLORS['surface'])
            footerContentFrame.pack (fill = 'x', padx = Px (24), pady = Px (14))
            self.selectionLabel = tkinter.Label (footerContentFrame, text = '', font = self.fonts['small'], bg = COLORS['surface'], fg = COLORS['mutedText'])
            if not isUpdateMode:
                self.selectionLabel.pack (side = 'left')
            self.installButton = FlatButton (footerContentFrame, 'Update' if isUpdateMode else 'Install', self.OnInstallClicked, self.fonts['bodyBold'], 'primary')
            self.installButton.pack (side = 'right')
            self.uninstallButton = FlatButton (footerContentFrame, 'Uninstall', self.OnUninstallClicked, self.fonts['body'], 'secondary')
            if not isUpdateMode:
                self.uninstallButton.pack (side = 'right', padx = (0, Px (8)))
            self.closeButton = FlatButton (footerContentFrame, 'Close', self.OnCloseClicked, self.fonts['body'], 'secondary')
            if isUpdateMode:
                # Wide enough for Cancel too (while waiting for Archicad), so
                # that the button does not move. The width counts digits.
                textWidth = max (self.fonts['body'].measure (text) for text in ['Close', 'Cancel'])
                digitWidth = self.fonts['body'].measure ('0')
                self.closeButton.configure (width = (textWidth + digitWidth - 1) // digitWidth)
            self.closeButton.pack (side = 'right', padx = (0, Px (8)))
            self.bind ('<Return>', lambda event : self.installButton.OnClicked ())
            self.bind ('<Escape>', lambda event : self.closeButton.OnClicked ())
            if isUpdateMode:
                # Closing the window in the middle of the update could leave
                # Archicad closed, or the add-on half replaced.
                self.protocol ('WM_DELETE_WINDOW', self.OnCloseRequested)
                if IsUsingMacOS ():
                    self.createcommand ('::tk::mac::Quit', self.OnCloseRequested)
            self.UpdateButtonStates ()

            threading.Thread (target = self.InitializeInBackground, daemon = True).start ()

        def RunOnUiThread (self, function):
            self.after (0, function)

        def OnCloseRequested (self):
            # Does what the Close button does: closes the window, or cancels
            # the wait for Archicad, and is ignored during the rest of the work.
            self.closeButton.OnClicked ()

        def OnCloseClicked (self):
            if not self.isWorking:
                self.destroy ()
                return
            # The button is only enabled while waiting for Archicad to quit.
            # If Archicad quits meanwhile, the update still goes on.
            self.cancelEvent.set ()
            self.rows[0].SetStatus ('Cancelling...', 'busy', 1.0)
            self.UpdateButtonStates ()

        def ShowWaitingForArchicad (self, isWaiting):
            self.isWaitingForArchicad = isWaiting
            if isWaiting:
                self.ShowResult (WAITING_FOR_ARCHICAD_TEXT, 'busy')
            else:
                self.resultLabel.pack_forget ()
            self.UpdateButtonStates ()

        def InitializeInBackground (self):
            try:
                if isUpdateMode:
                    # No detection: the Add-On tells which add-on to update.
                    installations = [GetUpdateTarget (args)]
                else:
                    installations = DetectArchicadInstallations (args.mockRootPath)
                self.RunOnUiThread (lambda : self.ShowInstallations (installations))
            except InstallerError as e:
                updateErrorText = str (e)
                self.RunOnUiThread (lambda : self.ShowDetectionError (updateErrorText))
            except Exception as e:
                errorText = 'Failed to detect Archicad installations: {0}'.format (e)
                self.RunOnUiThread (lambda : self.ShowDetectionError (errorText))
            try:
                releaseInfo = GetLatestReleaseInfo ()
                self.RunOnUiThread (lambda : self.ShowReleaseInfo (releaseInfo))
            except InstallerError as e:
                # 'e' is unbound once the except block ends, so the message is
                # captured before the lambda runs later on the UI thread.
                releaseErrorText = str (e)
                self.RunOnUiThread (lambda : self.ShowReleaseError (releaseErrorText))

        def ShowInstallations (self, installations):
            if len (installations) == 0:
                self.messageLabel.configure (text = 'No Archicad installation was found on this computer.')
                manualLabel = tkinter.Label (self.rowsFrame, text = 'Show the manual installation instructions', font = self.fonts['link'],
                    bg = COLORS['window'], fg = COLORS['accent'], cursor = 'hand2', anchor = 'w')
                manualLabel.pack (fill = 'x')
                manualLabel.bind ('<Button-1>', lambda event : webbrowser.open (MANUAL_INSTALL_URL))
                return
            self.messageLabel.pack_forget ()
            for installation in installations:
                self.rows.append (InstallationRow (self, self.rowsFrame, installation))
            self.UpdateButtonStates ()

        def ShowDetectionError (self, errorText):
            self.messageLabel.configure (text = errorText, fg = COLORS['error'], wraplength = Px (480))
            self.UpdateButtonStates ()

        def ShowReleaseInfo (self, releaseInfo):
            self.releaseInfo = releaseInfo
            self.releaseLabel.configure (text = 'Latest Tapir release: {0}'.format (releaseInfo.get ('tag_name', '?')))
            self.UpdateButtonStates ()
            if isUpdateMode:
                # The Add-On has already asked the user to confirm the update.
                self.OnInstallClicked ()

        def ShowReleaseError (self, errorText):
            self.releaseLabel.configure (text = errorText, fg = COLORS['error'], wraplength = Px (440), justify = 'left')
            self.UpdateButtonStates ()

        def ShowResult (self, resultText, kind):
            foreground, background = STATUS_COLORS[kind]
            wrapLength = Px (480)
            if isUpdateMode:
                # The update window is wider: it shows the full add-on path.
                wrapLength = max (wrapLength, self.rowsFrame.winfo_width () - Px (24))
            self.resultLabel.configure (text = resultText, fg = COLORS[foreground], bg = COLORS[background], wraplength = wrapLength)
            self.resultLabel.pack (fill = 'x', pady = (Px (4), 0))

        def UpdateButtonStates (self):
            selectedCount = len (self.GetSelectedRows ())
            canInstall = selectedCount > 0 and self.releaseInfo is not None and not self.isWorking and self.canUpdateAgain
            self.installButton.SetEnabled (canInstall)
            self.uninstallButton.SetEnabled (selectedCount > 0 and not self.isWorking)
            canCancel = self.isWaitingForArchicad and not self.cancelEvent.is_set ()
            self.closeButton.configure (text = 'Cancel' if self.isWaitingForArchicad else 'Close')
            self.closeButton.SetEnabled (not self.isWorking or canCancel)
            if len (self.rows) > 0:
                self.selectionLabel.configure (text = '{0} of {1} selected'.format (selectedCount, len (self.rows)))
                self.selectAllLabel.configure (text = 'Select none' if selectedCount == len (self.rows) else 'Select all')
            for row in self.rows:
                row.UpdateLook ()

        def OnSelectAllClicked (self):
            if self.isWorking or len (self.rows) == 0:
                return
            selectAll = len (self.GetSelectedRows ()) != len (self.rows)
            for row in self.rows:
                row.SetSelected (selectAll)
            self.UpdateButtonStates ()

        def SetRowStatus (self, row, statusText, kind = 'neutral', progressFraction = None):
            self.RunOnUiThread (lambda : row.SetStatus (statusText, kind, progressFraction))

        def GetSelectedRows (self):
            return [row for row in self.rows if row.isSelected]

        def OnInstallClicked (self):
            self.StartWork (self.UpdateInBackground if isUpdateMode else self.InstallInBackground)

        def OnUninstallClicked (self):
            self.StartWork (self.UninstallInBackground)

        def StartWork (self, workFunction):
            selectedRows = self.GetSelectedRows ()
            if len (selectedRows) == 0:
                return
            self.isWorking = True
            self.resultLabel.pack_forget ()
            self.UpdateButtonStates ()
            threading.Thread (target = workFunction, args = (selectedRows,), daemon = True).start ()

        def InstallInBackground (self, selectedRows):
            # The downloads run in parallel, but the installs take turns: on
            # macOS each one may ask for the administrator password, and
            # parallel password dialogs would be confusing.
            installLock = threading.Lock ()
            errorTexts = [None] * len (selectedRows)

            def InstallRow (rowIndex, row):
                try:
                    self.SetRowStatus (row, 'Downloading...', 'busy', 0.0)
                    def progressCallback (downloadedSize, totalSize):
                        if totalSize > 0:
                            self.SetRowStatus (row, 'Downloading {0}%'.format (int (downloadedSize * 100 / totalSize)), 'busy', downloadedSize / totalSize)
                    asset = FindAssetForVersion (self.releaseInfo, row.installation.version)
                    if asset is None:
                        raise InstallerError ('No add-on for this version')
                    downloadFolderPath = tempfile.mkdtemp (prefix = 'TapirInstaller_')
                    try:
                        downloadedFilePath = DownloadAsset (asset, downloadFolderPath, progressCallback)
                        self.SetRowStatus (row, 'Waiting to install', 'busy', 1.0)
                        with installLock:
                            self.SetRowStatus (row, 'Installing...', 'busy', 1.0)
                            InstallAddOn (row.installation.addOnsFolderPath, downloadedFilePath)
                    finally:
                        shutil.rmtree (downloadFolderPath, ignore_errors = True)
                    self.SetRowStatus (row, 'Installed', 'success')
                except Exception as e:
                    self.SetRowStatus (row, 'Failed', 'error')
                    errorTexts[rowIndex] = 'Archicad {0}: {1}'.format (row.installation.version, e)

            threads = [threading.Thread (target = InstallRow, args = (rowIndex, row), daemon = True) for rowIndex, row in enumerate (selectedRows)]
            for thread in threads:
                thread.start ()
            for thread in threads:
                thread.join ()
            failures = [errorText for errorText in errorTexts if errorText is not None]
            succeededCount = len (selectedRows) - len (failures)
            if len (failures) == 0:
                resultText = 'Tapir was installed for {0} Archicad version(s). Restart Archicad to load the Add-On.'.format (succeededCount)
            else:
                resultText = 'Tapir was installed for {0} of {1} Archicad version(s).\n{2}'.format (succeededCount, len (selectedRows), '\n'.join (failures))
            self.RunOnUiThread (lambda : self.FinishWork (resultText, 'success' if len (failures) == 0 else 'error'))

        def UpdateInBackground (self, selectedRows):
            row = selectedRows[0]
            self.cancelEvent = threading.Event ()
            def progressCallback (downloadedSize, totalSize):
                if totalSize > 0:
                    self.SetRowStatus (row, 'Downloading {0}%'.format (int (downloadedSize * 100 / totalSize)), 'busy', downloadedSize / totalSize)
            def waitingCallback (isWaiting):
                self.RunOnUiThread (lambda : self.ShowWaitingForArchicad (isWaiting))
            canUpdateAgain = True
            try:
                with UpdateLock (row.installation, args.dryRun) as setWaitingForQuit:
                    self.SetRowStatus (row, 'Downloading...', 'busy', 0.0)
                    resultText = UpdateAddOn (row.installation, self.releaseInfo, args.dryRun,
                        lambda statusText : self.SetRowStatus (row, statusText, 'busy', 1.0), progressCallback, self.cancelEvent, waitingCallback,
                        setWaitingForQuit)
                self.SetRowStatus (row, 'Downloaded' if args.dryRun else 'Updated', 'success')
                kind = 'success'
            except InstallerWarning as e:
                resultText = str (e)
                self.SetRowStatus (row, 'Updated', 'success')
                kind = 'warning'
            except UpdateCancelledError as e:
                resultText = str (e)
                self.SetRowStatus (row, 'Cancelled', 'neutral')
                kind = 'neutral'
            except UpdateRunningError as e:
                # Nothing was touched, but the other update starts Archicad
                # again: updating from here would then quit that Archicad.
                resultText = str (e)
                self.SetRowStatus (row, 'Failed', 'error')
                kind = 'error'
                canUpdateAgain = False
            except Exception as e:
                resultText = str (e)
                self.SetRowStatus (row, 'Failed', 'error')
                kind = 'error'
            self.RunOnUiThread (lambda : self.FinishWork (resultText, kind, canUpdateAgain))

        def UninstallInBackground (self, selectedRows):
            errorTexts = []
            for row in selectedRows:
                try:
                    if UninstallAddOn (row.installation.addOnsFolderPath):
                        self.SetRowStatus (row, 'Uninstalled', 'neutral')
                    else:
                        self.SetRowStatus (row, 'Not installed', 'neutral')
                except Exception as e:
                    self.SetRowStatus (row, 'Failed', 'error')
                    errorTexts.append ('Archicad {0}: {1}'.format (row.installation.version, e))
            if len (errorTexts) == 0:
                resultText = 'Finished uninstalling the Tapir Add-On.'
            else:
                resultText = 'Failed to uninstall the Tapir Add-On.\n{0}'.format ('\n'.join (errorTexts))
            self.RunOnUiThread (lambda : self.FinishWork (resultText, 'success' if len (errorTexts) == 0 else 'error'))

        def FinishWork (self, resultText, kind, canUpdateAgain = True):
            self.isWorking = False
            if isUpdateMode and (not canUpdateAgain or kind in ['success', 'warning'] or self.rows[0].installation.archicadWasQuit):
                # Updating again would quit the Archicad that is starting,
                # without waiting for its process, which is not known here.
                # After a warning, Tapir is updated already.
                self.canUpdateAgain = False
            self.UpdateButtonStates ()
            self.ShowResult (resultText, kind)
            if isUpdateMode and kind == 'success' and not args.dryRun:
                self.after (3000, self.destroy)

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
    # No abbreviations: a new option that starts like an existing one
    # would otherwise be read as that option, or make the parser exit.
    parser = argparse.ArgumentParser (description = INSTALLER_TITLE, allow_abbrev = False)
    parser.add_argument ('--console', dest = 'console', action = 'store_true', help = 'run in console mode without the GUI')
    parser.add_argument ('--versions', dest = 'versions', type = str, default = None, help = 'comma separated Archicad versions to install for (e.g. 28,29); default: all detected')
    parser.add_argument ('--addOnsFolder', dest = 'addOnsFolderPath', type = str, default = None, help = 'install into this Add-Ons folder instead of the detected ones (requires --versions with one version)')
    parser.add_argument ('--uninstall', dest = 'uninstall', action = 'store_true', help = 'remove the installed Tapir Add-On instead of installing it')
    parser.add_argument ('--dryRun', dest = 'dryRun', action = 'store_true', help = 'detect and download only, do not modify the Add-Ons folders')
    parser.add_argument ('--mockRoot', dest = 'mockRootPath', type = str, default = None, help = 'detect installations under this folder instead of the real system locations (for testing)')
    # The legacy update script (archicad-addon/Tools/update_addon_and_restart_archicad.py) detects update mode by this option string in this file at a release tag: keep the option name and this file's path.
    parser.add_argument ('--addOnFile', dest = 'addOnFilePath', type = str, default = None, help = 'update mode: replace this Tapir Add-On (.apx file or .bundle folder) with the latest release (requires --versions with one version)')
    parser.add_argument ('--archicadPort', dest = 'archicadPort', type = int, default = None, help = 'update mode: quit the Archicad listening on this port before replacing the add-on, and start it again afterwards')
    parser.add_argument ('--archicadPid', dest = 'archicadPid', type = int, default = None, help = 'update mode: also wait for this Archicad process to exit (requires --archicadPort)')
    parser.add_argument ('--noRestart', dest = 'noRestart', action = 'store_true', help = 'update mode: do not start Archicad again after the update (for testing)')
    # Unknown options are ignored, so that a newer Add-On can pass new options
    # to an older installer: the windowed executable would exit silently.
    args, unknownArgs = parser.parse_known_args ()

    # Update mode shows its progress in the GUI, although --versions would
    # otherwise mean console mode.
    isUpdateMode = IsUpdateMode (args)
    if args.console or (not isUpdateMode and (args.dryRun or args.uninstall or args.versions is not None or args.addOnsFolderPath is not None)):
        TryAttachWindowsConsole ()
        if len (unknownArgs) > 0:
            print ('Ignoring unknown options: {0}'.format (' '.join (unknownArgs)), file = sys.stderr)
        return RunConsoleInstaller (args)
    return RunGuiInstaller (args)


if __name__ == '__main__':
    sys.exit (Main ())
