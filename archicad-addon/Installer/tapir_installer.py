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
import ssl
import subprocess
import sys
import tempfile
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


def IsUsingWindows ():
    return platform.system () == 'Windows'


def IsUsingMacOS ():
    return platform.system () == 'Darwin'


class InstallerError (Exception):
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
        raise InstallerError ('Permission denied. Close Archicad if it is running, or run the installer as administrator.')
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
            raise InstallerError ('Permission denied. Close Archicad if it is running, or run the installer as administrator.')
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
        return [ArchicadInstallation (requestedVersions[0], os.path.dirname (os.path.normpath (args.addOnsFolderPath)), args.addOnsFolderPath)]
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
    import threading
    import webbrowser

    EnableWindowsHighDpi ()

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
        'neutral' : '#5F6368',
        'neutralTint' : '#EEF0F2',
    }
    STATUS_COLORS = {
        'neutral' : ('neutral', 'neutralTint'),
        'busy' : ('busy', 'busyTint'),
        'success' : ('success', 'successTint'),
        'error' : ('error', 'errorTint'),
    }

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
            tkinter.Label.__init__ (self, parent, text = text, font = font, padx = 18, pady = 7, cursor = 'hand2')
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
                    highlightthickness = 1, highlightbackground = COLORS['border'], highlightcolor = COLORS['border'])

    class CheckBox (tkinter.Canvas):
        SIZE = 18

        def __init__ (self, parent, background):
            tkinter.Canvas.__init__ (self, parent, width = self.SIZE, height = self.SIZE, bg = background, highlightthickness = 0, cursor = 'hand2')

        def Draw (self, isChecked, isEnabled):
            self.delete ('all')
            size = self.SIZE
            if isChecked:
                color = COLORS['accent'] if isEnabled else COLORS['disabled']
                self.create_rectangle (1, 1, size - 1, size - 1, fill = color, outline = color)
                self.create_line (4.5, 9.5, 7.5, 12.5, 13.5, 5.5, fill = COLORS['accentText'], width = 2, capstyle = 'round', joinstyle = 'round')
            else:
                self.create_rectangle (1, 1, size - 1, size - 1, fill = COLORS['surface'], outline = COLORS['disabled'], width = 1.5)

    class ProgressBar (tkinter.Canvas):
        def __init__ (self, parent):
            tkinter.Canvas.__init__ (self, parent, width = 1, height = 4, bg = COLORS['progressTrack'], highlightthickness = 0)
            self.fraction = 0.0
            self.bind ('<Configure>', lambda event : self.Draw ())

        def SetFraction (self, fraction):
            self.fraction = max (0.0, min (1.0, fraction))
            self.Draw ()

        def Draw (self):
            self.delete ('all')
            width = self.winfo_width ()
            if self.fraction > 0.0:
                self.create_rectangle (0, 0, int (width * self.fraction), 4, fill = COLORS['busy'], width = 0)

    class InstallationRow:
        def __init__ (self, app, parent, installation):
            self.app = app
            self.installation = installation
            self.isSelected = True
            self.card = tkinter.Frame (parent, bg = COLORS['surface'], highlightthickness = 1, cursor = 'hand2')
            self.card.pack (fill = 'x', pady = (0, 8))
            contentFrame = tkinter.Frame (self.card, bg = COLORS['surface'])
            contentFrame.pack (fill = 'x', padx = 14, pady = 11)
            self.checkBox = CheckBox (contentFrame, COLORS['surface'])
            self.checkBox.pack (side = 'left', padx = (0, 12))
            textFrame = tkinter.Frame (contentFrame, bg = COLORS['surface'])
            textFrame.pack (side = 'left', fill = 'x', expand = True)
            self.titleLabel = tkinter.Label (textFrame, text = 'Archicad {0}'.format (installation.version),
                font = app.fonts['cardTitle'], bg = COLORS['surface'], fg = COLORS['text'], anchor = 'w')
            self.titleLabel.pack (fill = 'x')
            self.pathLabel = tkinter.Label (textFrame, text = installation.addOnsFolderPath,
                font = app.fonts['small'], bg = COLORS['surface'], fg = COLORS['mutedText'], anchor = 'w')
            self.pathLabel.pack (fill = 'x')
            self.statusLabel = tkinter.Label (contentFrame, font = app.fonts['badge'], width = 16, padx = 6, pady = 3)
            self.statusLabel.pack (side = 'right', padx = (12, 0))
            # The bar is always packed, so the card does not change height when
            # it appears; it is only invisible while there is no progress.
            self.progressBar = ProgressBar (self.card)
            self.progressBar.pack (fill = 'x', side = 'bottom')
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
            self.title (INSTALLER_TITLE)
            self.configure (bg = COLORS['window'])
            self.minsize (560, 0)
            self.resizable (True, False)
            self.rows = []
            self.releaseInfo = None
            self.isWorking = False

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
            headerContentFrame.pack (fill = 'x', padx = 24, pady = 18)
            try:
                # Keep a reference, otherwise Tk drops the image.
                self.logoImage = tkinter.PhotoImage (data = TAPIR_LOGO_PNG_BASE64).subsample (2, 2)
                tkinter.Label (headerContentFrame, image = self.logoImage, bg = COLORS['surface']).pack (side = 'left', padx = (0, 14))
            except tkinter.TclError:
                # PNG support requires Tk 8.6; the logo is only decoration.
                pass
            titleFrame = tkinter.Frame (headerContentFrame, bg = COLORS['surface'])
            titleFrame.pack (side = 'left', fill = 'x', expand = True)
            tkinter.Label (titleFrame, text = INSTALLER_TITLE, font = self.fonts['title'], bg = COLORS['surface'], fg = COLORS['text'], anchor = 'w').pack (fill = 'x')
            self.releaseLabel = tkinter.Label (titleFrame, text = 'Checking the latest Tapir release...', font = self.fonts['body'],
                bg = COLORS['surface'], fg = COLORS['mutedText'], anchor = 'w')
            self.releaseLabel.pack (fill = 'x')
            tkinter.Frame (self, bg = COLORS['border'], height = 1).pack (fill = 'x')

            # Body
            bodyFrame = tkinter.Frame (self, bg = COLORS['window'])
            bodyFrame.pack (fill = 'both', expand = True, padx = 24, pady = (18, 10))
            sectionFrame = tkinter.Frame (bodyFrame, bg = COLORS['window'])
            sectionFrame.pack (fill = 'x', pady = (0, 8))
            tkinter.Label (sectionFrame, text = 'ARCHICAD INSTALLATIONS', font = self.fonts['section'], bg = COLORS['window'], fg = COLORS['mutedText']).pack (side = 'left')
            self.selectAllLabel = tkinter.Label (sectionFrame, text = '', font = self.fonts['link'], bg = COLORS['window'], fg = COLORS['accent'], cursor = 'hand2')
            self.selectAllLabel.pack (side = 'right')
            self.selectAllLabel.bind ('<Button-1>', lambda event : self.OnSelectAllClicked ())
            self.rowsFrame = tkinter.Frame (bodyFrame, bg = COLORS['window'])
            self.rowsFrame.pack (fill = 'x')
            self.messageLabel = tkinter.Label (self.rowsFrame, text = 'Detecting Archicad installations...', font = self.fonts['body'],
                bg = COLORS['window'], fg = COLORS['mutedText'], anchor = 'w', justify = 'left')
            self.messageLabel.pack (fill = 'x', pady = 10)
            self.resultLabel = tkinter.Label (bodyFrame, text = '', font = self.fonts['body'], anchor = 'w', justify = 'left', padx = 12, pady = 9, bg = COLORS['window'])

            # Footer
            tkinter.Frame (self, bg = COLORS['border'], height = 1).pack (fill = 'x')
            footerFrame = tkinter.Frame (self, bg = COLORS['surface'])
            footerFrame.pack (fill = 'x')
            footerContentFrame = tkinter.Frame (footerFrame, bg = COLORS['surface'])
            footerContentFrame.pack (fill = 'x', padx = 24, pady = 14)
            self.selectionLabel = tkinter.Label (footerContentFrame, text = '', font = self.fonts['small'], bg = COLORS['surface'], fg = COLORS['mutedText'])
            self.selectionLabel.pack (side = 'left')
            self.installButton = FlatButton (footerContentFrame, 'Install', self.OnInstallClicked, self.fonts['bodyBold'], 'primary')
            self.installButton.pack (side = 'right')
            self.uninstallButton = FlatButton (footerContentFrame, 'Uninstall', self.OnUninstallClicked, self.fonts['body'], 'secondary')
            self.uninstallButton.pack (side = 'right', padx = (0, 8))
            self.closeButton = FlatButton (footerContentFrame, 'Close', self.destroy, self.fonts['body'], 'secondary')
            self.closeButton.pack (side = 'right', padx = (0, 8))
            self.bind ('<Return>', lambda event : self.installButton.OnClicked ())
            self.bind ('<Escape>', lambda event : self.closeButton.OnClicked ())
            self.UpdateButtonStates ()

            threading.Thread (target = self.InitializeInBackground, daemon = True).start ()

        def RunOnUiThread (self, function):
            self.after (0, function)

        def InitializeInBackground (self):
            try:
                installations = DetectArchicadInstallations (args.mockRootPath)
                self.RunOnUiThread (lambda : self.ShowInstallations (installations))
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
            self.messageLabel.configure (text = errorText, fg = COLORS['error'])
            self.UpdateButtonStates ()

        def ShowReleaseInfo (self, releaseInfo):
            self.releaseInfo = releaseInfo
            self.releaseLabel.configure (text = 'Latest Tapir release: {0}'.format (releaseInfo.get ('tag_name', '?')))
            self.UpdateButtonStates ()

        def ShowReleaseError (self, errorText):
            self.releaseLabel.configure (text = errorText, fg = COLORS['error'], wraplength = 440, justify = 'left')
            self.UpdateButtonStates ()

        def ShowResult (self, resultText, kind):
            foreground, background = STATUS_COLORS[kind]
            self.resultLabel.configure (text = resultText, fg = COLORS[foreground], bg = COLORS[background], wraplength = 480)
            self.resultLabel.pack (fill = 'x', pady = (4, 0))

        def UpdateButtonStates (self):
            selectedCount = len (self.GetSelectedRows ())
            canInstall = selectedCount > 0 and self.releaseInfo is not None and not self.isWorking
            self.installButton.SetEnabled (canInstall)
            self.uninstallButton.SetEnabled (selectedCount > 0 and not self.isWorking)
            self.closeButton.SetEnabled (not self.isWorking)
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
            self.StartWork (self.InstallInBackground)

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

        def FinishWork (self, resultText, kind):
            self.isWorking = False
            self.UpdateButtonStates ()
            self.ShowResult (resultText, kind)

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

    if args.console or args.dryRun or args.uninstall or args.versions is not None or args.addOnsFolderPath is not None:
        TryAttachWindowsConsole ()
        return RunConsoleInstaller (args)
    return RunGuiInstaller (args)


if __name__ == '__main__':
    sys.exit (Main ())
