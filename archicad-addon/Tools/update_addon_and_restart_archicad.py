# Auto-update script of the Tapir Add-On. Released Add-Ons download it from
# main and run it without a console window:
#   uv run [--python X] --script update_addon_and_restart_archicad.py
#       --port <port> --downloadUrl <add-on asset URL> --addOnLocation <path>
# So its path, its name and these options must stay, and it may only use the
# standard library of any Python 3 that uv picks.
#
# The update is handed off to the Tapir Installer of the same release when that
# installer has update mode (--addOnFile), which replaces the add-on with an
# administrator prompt when needed. The release workflow builds the installer
# from the tagged commit, so the installer source of the release tag tells
# whether it has update mode; an installer without it would exit on the
# unknown option without a word. This script updates the add-on itself for
# releases without update mode, when that cannot be checked (no release tag in
# the URL, the source cannot be downloaded), when the installer cannot be
# downloaded or started, and when the administrator prompt was declined but
# the add-on is writable without it. There is no console, so errors, also
# those of the command line, are shown in a dialog.

import argparse
import contextlib
import json
import os
import platform
import re
import shutil
import socket
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.parse
import urllib.request
import zipfile

REPOSITORY_URL = 'https://github.com/ENZYME-APD/tapir-archicad-automation'
LATEST_RELEASE_URL = REPOSITORY_URL + '/releases/latest'
# The installer source of a release tag: '<url>/<tag>/<path>'.
INSTALLER_SOURCE_URL = 'https://raw.githubusercontent.com/ENZYME-APD/tapir-archicad-automation/{0}/archicad-addon/Installer/tapir_installer.py'
# The option of update mode as a string literal in the installer source (the
# argparse option), not only mentioned in a comment.
INSTALLER_UPDATE_MODE_PATTERN = re.compile (br'''['"]--addOnFile['"]''')
# The first bytes of the add-on assets: a page of a proxy or web filter served
# instead must not replace the add-on.
ASSET_FILE_SIGNATURES = { '.apx' : b'MZ', '.zip' : b'PK\x03\x04' }
ARCHICAD_DID_NOT_QUIT_TEXT = 'Archicad did not quit ({0}), so Tapir was not updated.'
# A declined administrator prompt and elevation blocked by a policy look the
# same, so the text fits both.
UPDATE_CANCELLED_TEXT = ('The update was cancelled. Updating needs administrator rights. You can also download the new version '
    'from {0} and replace the Add-On manually.'.format (LATEST_RELEASE_URL))
# Released Add-Ons pass an empty URL when the latest release has no add-on for
# their Archicad version, and also when their update check failed.
NO_DOWNLOAD_URL_TEXT = ('The latest Tapir release has no Add-On for this Archicad version, or the update check failed. '
    'See {0}'.format (LATEST_RELEASE_URL))
# Shown while another update runs. Quitting Archicad before that update asks
# it to quit would make the update fail (this script downloads without a
# window, so a second click is likely then). So the user is told to quit only
# while that update waits for it, for example after quitting was cancelled.
UPDATE_RUNNING_TEXT = 'Tapir is being updated already. Wait for that update to finish: it closes Archicad itself and starts it again.'
UPDATE_WAITING_FOR_QUIT_TEXT = ('Tapir is being updated already, and that update waits for Archicad to quit. '
    'Quit Archicad to finish it; Archicad is then started again.')
# What the update that holds the lock does, written after the first byte of
# the lock file, which is the locked one (on Windows, no other file handle can
# read a locked byte, not even one of this process): 'quitting' while it waits
# for Archicad to quit, otherwise 'updating'. Both have 8 bytes, so each overwrites the other.
LOCK_STATE_OFFSET = 1
LOCK_STATE_WAITING_FOR_QUIT = b'quitting'
LOCK_STATE_UPDATING = b'updating'
DIALOG_TITLE = 'Tapir Update'
USER_AGENT = 'TapirAddOnUpdater'
DOWNLOAD_TIMEOUT_SECONDS = 60
ARCHICAD_COMMAND_TIMEOUT_SECONDS = 10
# Generous: there is no window to wait in, and the user may answer Archicad's
# save prompt late. So if quitting was cancelled but the user quits Archicad
# within this time, Archicad is still updated and started again.
ARCHICAD_QUIT_TIMEOUT_SECONDS = 600
ARCHICAD_PROCESS_EXIT_TIMEOUT_SECONDS = 60
REPLACE_RETRY_TIMEOUT_SECONDS = 60
POLL_INTERVAL_SECONDS = 1.0
# ShellExecuteW returns it when the user declined the UAC prompt.
SE_ERR_ACCESSDENIED = 5


class UpdateError (Exception):
    pass


class UpdateStoppedError (UpdateError):
    # Its text tells all the user can do, so it is shown as it is.
    pass


class UpdateWarning (UpdateError):
    # Tapir was updated, but something is left to the user.
    pass


class ArgumentParser (argparse.ArgumentParser):
    # argparse would print its errors to the console, which nobody sees.
    def error (self, message):
        raise UpdateError (message)


def IsUsingWindows ():
    return platform.system () == 'Windows'


def IsUsingMacOS ():
    return platform.system () == 'Darwin'


def ShowDialog (text, isWarning = False):
    try:
        if IsUsingWindows ():
            import ctypes
            MB_ICONERROR = 0x10
            MB_ICONWARNING = 0x30
            MB_SETFOREGROUND = 0x10000
            MB_TOPMOST = 0x40000
            icon = MB_ICONWARNING if isWarning else MB_ICONERROR
            ctypes.windll.user32.MessageBoxW (None, text, DIALOG_TITLE, icon | MB_SETFOREGROUND | MB_TOPMOST)
            return
        if IsUsingMacOS ():
            # The texts are passed as arguments, so they need no escaping.
            subprocess.run (['/usr/bin/osascript',
                '-e', 'on run argv',
                '-e', 'try',
                '-e', 'activate',
                '-e', 'end try',
                '-e', 'display dialog (item 1 of argv) with title (item 2 of argv) buttons {"OK"} default button "OK" with icon ' + ('caution' if isWarning else 'stop'),
                '-e', 'end run',
                text, DIALOG_TITLE])
            return
    except Exception:
        pass
    print (text, file = sys.stderr)


def RemovePath (path):
    if os.path.isdir (path) and not os.path.islink (path):
        shutil.rmtree (path)
    elif os.path.lexists (path):
        os.remove (path)


def RemovePathQuietly (path):
    try:
        RemovePath (path)
    except OSError:
        pass


def GetFileName (url):
    return os.path.basename (urllib.parse.urlparse (url).path)


def GetCurlPath ():
    if IsUsingWindows ():
        curlPath = os.path.join (os.environ.get ('SystemRoot', 'C:\\Windows'), 'System32', 'curl.exe')
    elif IsUsingMacOS ():
        curlPath = '/usr/bin/curl'
    else:
        curlPath = shutil.which ('curl')
    return curlPath if curlPath is not None and os.path.isfile (curlPath) else None


def DownloadFile (url, filePath):
    try:
        request = urllib.request.Request (url, headers = { 'User-Agent' : USER_AGENT })
        with urllib.request.urlopen (request, timeout = DOWNLOAD_TIMEOUT_SECONDS) as response, open (filePath, 'wb') as file:
            shutil.copyfileobj (response, file)
            # A dropped connection only ends the download early, it raises no
            # error; a cut-off add-on would replace the working one.
            expectedSize = response.headers.get ('Content-Length', '').strip ()
            if expectedSize.isdigit () and file.tell () != int (expectedSize):
                raise urllib.error.ContentTooShortError ('Download incomplete: got only {0} out of {1} bytes.'.format (file.tell (), expectedSize), None)
    except Exception as urllibError:
        # The OpenSSL of the Python that uv picks may not find the system's
        # root certificates (CERTIFICATE_VERIFY_FAILED); curl uses the
        # system's TLS.
        curlPath = GetCurlPath ()
        if curlPath is None:
            raise
        result = subprocess.run ([curlPath, '--fail', '--silent', '--show-error', '--location',
            '--max-time', '600', '--output', filePath, url], capture_output = True)
        if result.returncode != 0:
            raise urllibError


def GetReleaseTag (downloadUrl):
    match = re.search (r'/releases/download/([^/]+)/', downloadUrl)
    return urllib.parse.unquote (match.group (1)) if match else None


def GetArchicadVersion (downloadUrl):
    match = re.search (r'AC(\d+)_(Win|Mac)', GetFileName (downloadUrl))
    return match.group (1) if match else None


def GetInstallerAssetName ():
    if IsUsingWindows ():
        return 'TapirInstaller_Win.exe'
    if IsUsingMacOS ():
        return 'TapirInstaller_Mac.zip'
    return None


def GetInstallerDownloadUrl (downloadUrl):
    # The installer of the release of the add-on; None if there is none for
    # this platform or the URL names no release. The URL is built from the
    # fixed repository, because the installer is started with administrator
    # rights.
    releaseTag = GetReleaseTag (downloadUrl)
    installerAssetName = GetInstallerAssetName ()
    if releaseTag is None or installerAssetName is None or GetArchicadVersion (downloadUrl) is None:
        return None
    return '{0}/releases/download/{1}/{2}'.format (REPOSITORY_URL, urllib.parse.quote (releaseTag, safe = ''), installerAssetName)


def HasInstallerUpdateMode (releaseTag):
    # Whether the installer of the release has update mode, read from its
    # source at the release tag. False also when the source cannot be
    # downloaded: the add-on is then updated in place, as for an old release.
    folderPath = tempfile.mkdtemp (prefix = 'TapirUpdate_')
    try:
        sourcePath = os.path.join (folderPath, 'tapir_installer.py')
        DownloadFile (INSTALLER_SOURCE_URL.format (urllib.parse.quote (releaseTag, safe = '')), sourcePath)
        with open (sourcePath, 'rb') as file:
            return INSTALLER_UPDATE_MODE_PATTERN.search (file.read ()) is not None
    except Exception:
        return False
    finally:
        shutil.rmtree (folderPath, ignore_errors = True)


def QuoteWindowsArgument (argument):
    # CommandLineToArgvW rules: backslashes are only special before a quote,
    # so the ones before a quote and before the closing quote are doubled.
    argument = re.sub (r'(\\*)"', r'\1\1\\"', argument)
    argument = re.sub (r'(\\+)$', r'\1\1', argument)
    return '"' + argument + '"'


def StartInstallerWin (installerPath, arguments):
    # The installer requires administrator rights, so subprocess cannot start
    # it (WinError 740). Returns False if the user declined the UAC prompt.
    import ctypes
    from ctypes import wintypes
    shell32 = ctypes.WinDLL ('shell32')
    shell32.ShellExecuteW.argtypes = [wintypes.HWND, wintypes.LPCWSTR, wintypes.LPCWSTR, wintypes.LPCWSTR, wintypes.LPCWSTR, ctypes.c_int]
    shell32.ShellExecuteW.restype = wintypes.HINSTANCE
    user32 = ctypes.WinDLL ('user32')
    user32.GetForegroundWindow.restype = wintypes.HWND
    SW_SHOWNORMAL = 1
    parameters = ' '.join (QuoteWindowsArgument (argument) for argument in arguments)
    # With the foreground (Archicad) window as parent the UAC prompt comes to
    # the front instead of only flashing in the taskbar.
    result = shell32.ShellExecuteW (user32.GetForegroundWindow (), 'runas', installerPath, parameters, None, SW_SHOWNORMAL) or 0
    if result == SE_ERR_ACCESSDENIED:
        return False
    if result <= 32:
        raise OSError ('Failed to start the Tapir Installer (error {0}).'.format (result))
    return True


def StartInstallerMac (installerZipPath, arguments):
    installerFolderPath = os.path.dirname (installerZipPath)
    # ditto keeps the symlinks and permissions of the signed app.
    subprocess.run (['/usr/bin/ditto', '-x', '-k', installerZipPath, installerFolderPath], check = True, capture_output = True)
    # -n: an installer that runs already would otherwise only be activated,
    # and the arguments would be dropped.
    subprocess.run (['/usr/bin/open', '-n', os.path.join (installerFolderPath, 'TapirInstaller.app'), '--args'] + arguments,
        check = True, capture_output = True)
    return True


def HandOffToInstaller (downloadUrl, addOnLocation, host, port):
    """Starts the Tapir Installer of the release to do the update. Returns
    True when the installer was started, False when this script has to do
    the update. Raises UpdateStoppedError when the administrator prompt was
    declined and the add-on is not writable without it."""
    installerUrl = GetInstallerDownloadUrl (downloadUrl)
    if installerUrl is None or not HasInstallerUpdateMode (GetReleaseTag (downloadUrl)):
        return False
    arguments = ['--addOnFile', addOnLocation, '--versions', GetArchicadVersion (downloadUrl), '--archicadPort', str (port)]
    # With the process id the installer waits for Archicad to exit, and on
    # Windows lets Archicad's save prompt come in front of its window.
    try:
        archicadPid = FindArchicadProcessId (GetArchicadLocation (host, port))
        if archicadPid is not None:
            arguments += ['--archicadPid', str (archicadPid)]
    except Exception:
        pass # the installer then waits for the port only
    # The installer runs from this folder, so it is not deleted once started.
    installerFolderPath = tempfile.mkdtemp (prefix = 'TapirInstaller_')
    try:
        installerPath = os.path.join (installerFolderPath, GetInstallerAssetName ())
        DownloadFile (installerUrl, installerPath)
        if IsUsingWindows ():
            started = StartInstallerWin (installerPath, arguments)
        else:
            started = StartInstallerMac (installerPath, arguments)
    except Exception:
        shutil.rmtree (installerFolderPath, ignore_errors = True)
        return False
    if started:
        return True
    shutil.rmtree (installerFolderPath, ignore_errors = True)
    # The administrator prompt was declined; a standard user can only decline
    # it. An add-on in a folder the user can write to needs no administrator
    # rights, so this script updates it, as it did before the installer took
    # over. Otherwise Archicad keeps running with the current version, and
    # the user is told why.
    try:
        CheckAddOnIsWritable (addOnLocation)
    except UpdateError:
        raise UpdateStoppedError (UPDATE_CANCELLED_TEXT)
    return False


def PostTapirCommand (host, port, commandName):
    # The opener ignores the proxy settings: a system proxy could otherwise
    # intercept the requests to 127.0.0.1.
    requestData = {
        'command' : 'API.ExecuteAddOnCommand',
        'parameters' : {
            'addOnCommandId' : { 'commandNamespace' : 'TapirCommand', 'commandName' : commandName },
            'addOnCommandParameters' : {}
        }
    }
    request = urllib.request.Request ('{0}:{1}'.format (host, port), data = json.dumps (requestData).encode ('utf-8'),
        headers = { 'Content-Type' : 'application/json' })
    opener = urllib.request.build_opener (urllib.request.ProxyHandler ({}))
    with opener.open (request, timeout = ARCHICAD_COMMAND_TIMEOUT_SECONDS) as response:
        return response.read ()


def RunTapirCommand (host, port, commandName):
    responseJson = json.loads (PostTapirCommand (host, port, commandName).decode ('utf-8'))
    try:
        return responseJson['result']['addOnCommandResponse']
    except (KeyError, TypeError):
        raise UpdateError ('Archicad failed to execute {0}: {1}'.format (commandName, responseJson))


def GetArchicadLocation (host, port):
    try:
        return RunTapirCommand (host, port, 'GetArchicadLocation')['archicadLocation']
    except Exception as e:
        raise UpdateError ('Failed to connect to Archicad on port {0}: {1}'.format (port, e))


def QuitArchicad (host, port):
    try:
        response = RunTapirCommand (host, port, 'QuitArchicad')
    except UpdateError as e:
        # Archicad answered without running the command.
        raise UpdateError (ARCHICAD_DID_NOT_QUIT_TEXT.format (e))
    except Exception:
        # The connection may drop while Archicad is quitting. Whether it quit
        # is checked afterwards.
        return
    if isinstance (response, dict) and response.get ('success') is False:
        # Archicad refuses to quit, for example while another add-on works.
        # Waiting for it would not change that.
        error = response.get ('error')
        reason = error.get ('message') if isinstance (error, dict) else None
        raise UpdateError (ARCHICAD_DID_NOT_QUIT_TEXT.format (reason or 'quitting was refused'))


def IsTimeoutError (error):
    if isinstance (error, urllib.error.URLError):
        error = error.reason
    return isinstance (error, (socket.timeout, TimeoutError))


def IsArchicadAnswering (host, port):
    try:
        PostTapirCommand (host, port, 'GetAddOnVersion')
    except urllib.error.HTTPError:
        return True
    except Exception as e:
        # A closed port refuses the connection at once. A timeout means that
        # Archicad still runs but is busy, for example with a save dialog.
        return IsTimeoutError (e)
    return True


def GetProcessesWin ():
    # {process id: (parent process id, executable file name)}
    import ctypes
    from ctypes import wintypes

    class PROCESSENTRY32W (ctypes.Structure):
        _fields_ = [('dwSize', wintypes.DWORD), ('cntUsage', wintypes.DWORD), ('th32ProcessID', wintypes.DWORD),
            ('th32DefaultHeapID', ctypes.c_size_t), ('th32ModuleID', wintypes.DWORD), ('cntThreads', wintypes.DWORD),
            ('th32ParentProcessID', wintypes.DWORD), ('pcPriClassBase', wintypes.LONG), ('dwFlags', wintypes.DWORD),
            ('szExeFile', wintypes.WCHAR * 260)]

    TH32CS_SNAPPROCESS = 0x2
    INVALID_HANDLE_VALUE = ctypes.c_void_p (-1).value
    kernel32 = ctypes.WinDLL ('kernel32')
    kernel32.CreateToolhelp32Snapshot.argtypes = [wintypes.DWORD, wintypes.DWORD]
    kernel32.CreateToolhelp32Snapshot.restype = wintypes.HANDLE
    for function in [kernel32.Process32FirstW, kernel32.Process32NextW]:
        function.argtypes = [wintypes.HANDLE, ctypes.POINTER (PROCESSENTRY32W)]
        function.restype = wintypes.BOOL
    kernel32.CloseHandle.argtypes = [wintypes.HANDLE]
    snapshot = kernel32.CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0)
    if not snapshot or snapshot == INVALID_HANDLE_VALUE:
        raise OSError ('Failed to list the processes.')
    processes = {}
    try:
        entry = PROCESSENTRY32W ()
        entry.dwSize = ctypes.sizeof (PROCESSENTRY32W)
        hasEntry = kernel32.Process32FirstW (snapshot, ctypes.byref (entry))
        while hasEntry:
            processes[entry.th32ProcessID] = (entry.th32ParentProcessID, entry.szExeFile)
            hasEntry = kernel32.Process32NextW (snapshot, ctypes.byref (entry))
    finally:
        kernel32.CloseHandle (snapshot)
    return processes


def FindArchicadProcessIdWin (archicadLocation):
    try:
        processes = GetProcessesWin ()
        # getppid lists the processes itself on Windows, so it can fail too.
        processId = os.getppid ()
    except Exception:
        return None
    # Only the file names are listed. The first Archicad among the ancestors
    # is the one that started this script.
    archicadFileName = os.path.basename (archicadLocation).lower ()
    for _ in range (8):
        if processId not in processes:
            break
        parentProcessId, fileName = processes[processId]
        if fileName.lower () == archicadFileName:
            return processId
        processId = parentProcessId
    return None


def FindArchicadProcessId (archicadLocation):
    # Archicad started this script through uv, so it is one of the script's
    # ancestor processes. None if it is not found, also if the lookup fails.
    if IsUsingWindows ():
        return FindArchicadProcessIdWin (archicadLocation)
    if not IsUsingMacOS ():
        return None
    archicadLocation = os.path.normpath (archicadLocation)
    processId = os.getppid ()
    for _ in range (8):
        if processId <= 1:
            break
        try:
            result = subprocess.run (['/bin/ps', '-o', 'ppid=,command=', '-p', str (processId)], capture_output = True, text = True)
            parentProcessId, command = result.stdout.strip ().split (None, 1)
            parentProcessId = int (parentProcessId)
        except (OSError, ValueError):
            break
        # The command starts with the executable inside the Archicad app.
        if command.startswith (archicadLocation) and command[len (archicadLocation):][:1] in ('', '/', ' '):
            return processId
        processId = parentProcessId
    return None


def WaitForProcessToExit (processId, timeoutSeconds):
    # POSIX only: os.kill (pid, 0) would terminate the process on Windows.
    deadline = time.monotonic () + timeoutSeconds
    while True:
        try:
            os.kill (processId, 0)
        except ProcessLookupError:
            return True
        except PermissionError:
            pass # the process exists, it only belongs to another user
        if time.monotonic () >= deadline:
            return False
        time.sleep (POLL_INTERVAL_SECONDS)


def WaitForArchicadToQuit (host, port, archicadPid):
    # Returns False if Archicad still runs, for example because quitting was
    # cancelled in a save dialog.
    deadline = time.monotonic () + ARCHICAD_QUIT_TIMEOUT_SECONDS
    while IsArchicadAnswering (host, port):
        if time.monotonic () >= deadline:
            return False
        time.sleep (POLL_INTERVAL_SECONDS)
    # The port closes before the process ends. On macOS, the new Archicad
    # would start while the old one still exits. Archicad cannot cancel
    # quitting any more once its port has closed, so the update goes on also
    # if the process takes longer to exit. On Windows the replace retries wait
    # for the add-on file lock instead.
    if archicadPid is not None and not IsUsingWindows ():
        WaitForProcessToExit (archicadPid, ARCHICAD_PROCESS_EXIT_TIMEOUT_SECONDS)
    return True


def AllowArchicadToComeToFront (archicadPid):
    # Windows does not let a background process bring its windows to the
    # front, so Archicad's save prompt might only flash in the taskbar. Fails
    # without harm when this script may not pass the foreground on.
    if not IsUsingWindows () or archicadPid is None:
        return
    try:
        import ctypes
        from ctypes import wintypes
        user32 = ctypes.WinDLL ('user32')
        user32.AllowSetForegroundWindow.argtypes = [wintypes.DWORD]
        user32.AllowSetForegroundWindow.restype = wintypes.BOOL
        user32.AllowSetForegroundWindow (archicadPid)
    except Exception:
        pass


def StartArchicad (archicadLocation):
    if IsUsingMacOS ():
        # Archicad is only started once its port closed, so an instance that
        # is still there is exiting: plain open would only activate that one.
        subprocess.Popen (['/usr/bin/open', '-n', archicadLocation])
    else:
        subprocess.Popen ([archicadLocation])


def CheckAddOnIsWritable (addOnLocation):
    # Checked before Archicad is quit: failing afterwards used to leave the
    # user without Archicad. The add-on is replaced in its folder.
    folderPath = os.path.dirname (addOnLocation)
    # Not tempfile.mkstemp: up to Python 3.12 it keeps trying new names for
    # hours on Windows when the folder is not writable.
    probeFilePath = os.path.join (folderPath, '.TapirWriteCheck_{0}'.format (os.getpid ()))
    try:
        os.close (os.open (probeFilePath, os.O_WRONLY | os.O_CREAT | os.O_TRUNC))
        os.remove (probeFilePath)
    except OSError as e:
        raise UpdateError ('The folder of the Tapir Add-On is not writable without administrator rights: {0}\n({1})'.format (folderPath, e))
    # A read-only file cannot be replaced on Windows.
    if IsUsingWindows () and os.path.isfile (addOnLocation) and not os.access (addOnLocation, os.W_OK):
        raise UpdateError ('The Tapir Add-On is read-only: {0}'.format (addOnLocation))


def CheckFileSignature (downloadUrl, filePath):
    signature = ASSET_FILE_SIGNATURES.get (os.path.splitext (filePath)[1].lower ())
    if signature is None:
        return
    with open (filePath, 'rb') as file:
        if file.read (len (signature)) != signature:
            raise UpdateError ('Failed to download {0}: the received file is not the add-on (was it blocked by a proxy?).'.format (downloadUrl))


def ExtractAddOnBundle (zipFilePath, workFolderPath):
    extractFolderPath = os.path.join (workFolderPath, 'Extracted')
    os.makedirs (extractFolderPath)
    if IsUsingMacOS ():
        # ditto keeps the permission bits and symlinks of the bundle, which
        # zipfile.extractall would drop.
        subprocess.run (['/usr/bin/ditto', '-x', '-k', zipFilePath, extractFolderPath], check = True, capture_output = True)
    else:
        with zipfile.ZipFile (zipFilePath, 'r') as zipFile:
            zipFile.extractall (extractFolderPath)
    bundleNames = [name for name in os.listdir (extractFolderPath) if name.lower ().endswith ('.bundle')]
    if len (bundleNames) != 1:
        raise UpdateError ('{0} does not contain exactly one add-on bundle.'.format (os.path.basename (zipFilePath)))
    return os.path.join (extractFolderPath, bundleNames[0])


def RemoveBundleLeftovers (targetPath):
    # What an earlier update left next to the bundle, for example an old
    # version that could not be deleted.
    for path in [targetPath + '.new', targetPath + '.old']:
        try:
            RemovePath (path)
        except OSError as e:
            raise UpdateError ('An earlier update left {0}, which could not be deleted.\nPlease delete it. ({1})'.format (path, e))


def ReplaceAddOnBundle (newBundlePath, targetPath):
    # The bundle is replaced, not merged, so files removed from the add-on do
    # not stay. It keeps the name of the target, which Archicad's add-on list
    # refers to. The old bundle is moved aside and only deleted once the new
    # one is in place, so the target is never missing or half deleted.
    copyPath = targetPath + '.new'
    oldPath = targetPath + '.old'
    RemoveBundleLeftovers (targetPath)
    try:
        if IsUsingMacOS ():
            subprocess.run (['/usr/bin/ditto', newBundlePath, copyPath], check = True, capture_output = True)
        else:
            shutil.copytree (newBundlePath, copyPath, symlinks = True)
        os.rename (targetPath, oldPath)
    except Exception:
        RemovePathQuietly (copyPath)
        raise
    try:
        os.rename (copyPath, targetPath)
    except OSError:
        os.rename (oldPath, targetPath)
        RemovePathQuietly (copyPath)
        raise
    try:
        RemovePath (oldPath)
    except OSError as e:
        raise UpdateWarning ('Its old version could not be deleted: {0}\nPlease delete it. ({1})'.format (oldPath, e))


def ReplaceAddOnFile (newFilePath, targetPath):
    # Archicad locks the loaded add-on until its process has fully exited,
    # which can be well after its port closed.
    copyPath = targetPath + '.new'
    try:
        shutil.copyfile (newFilePath, copyPath)
        deadline = time.monotonic () + REPLACE_RETRY_TIMEOUT_SECONDS
        while True:
            try:
                os.replace (copyPath, targetPath)
                return
            except OSError:
                if time.monotonic () >= deadline:
                    raise
                time.sleep (POLL_INTERVAL_SECONDS)
    except Exception:
        RemovePathQuietly (copyPath)
        raise


def UpdateInPlace (downloadUrl, addOnLocation, host, port, setWaitingForQuit = lambda isWaiting: None):
    if not os.path.lexists (addOnLocation):
        raise UpdateError ('The Tapir Add-On was not found: {0}'.format (addOnLocation))
    workFolderPath = tempfile.mkdtemp (prefix = 'TapirUpdate_')
    try:
        # Everything that can fail without touching Archicad comes first, so
        # that a failure leaves Archicad running.
        downloadedFilePath = os.path.join (workFolderPath, GetFileName (downloadUrl) or 'TapirAddOn')
        try:
            DownloadFile (downloadUrl, downloadedFilePath)
        except Exception as e:
            raise UpdateError ('Failed to download {0}: {1}'.format (downloadUrl, e))
        CheckFileSignature (downloadUrl, downloadedFilePath)
        isBundle = downloadedFilePath.lower ().endswith ('.zip')
        newAddOnPath = ExtractAddOnBundle (downloadedFilePath, workFolderPath) if isBundle else downloadedFilePath
        CheckAddOnIsWritable (addOnLocation)
        if isBundle:
            # A leftover that cannot be deleted would fail the replace only
            # after Archicad was quit.
            RemoveBundleLeftovers (addOnLocation)
        archicadLocation = GetArchicadLocation (host, port)
        archicadPid = FindArchicadProcessId (archicadLocation)

        AllowArchicadToComeToFront (archicadPid)
        # Only from here on may another update tell the user to quit Archicad.
        setWaitingForQuit (True)
        try:
            QuitArchicad (host, port)
            hasArchicadQuit = WaitForArchicadToQuit (host, port, archicadPid)
        finally:
            setWaitingForQuit (False)
        if not hasArchicadQuit:
            raise UpdateError (ARCHICAD_DID_NOT_QUIT_TEXT.format ('was quitting cancelled?'))
        try:
            if isBundle:
                ReplaceAddOnBundle (newAddOnPath, addOnLocation)
            else:
                ReplaceAddOnFile (newAddOnPath, addOnLocation)
        except UpdateError:
            raise
        except Exception as e:
            raise UpdateError ('Failed to replace {0}: {1}'.format (addOnLocation, e))
        finally:
            # Also when the replace failed: Archicad was quit by this script.
            StartArchicad (archicadLocation)
    finally:
        shutil.rmtree (workFolderPath, ignore_errors = True)


def WriteLockState (lockFile, state):
    # The state only picks the text of another update, so failing is
    # harmless.
    try:
        os.lseek (lockFile, LOCK_STATE_OFFSET, os.SEEK_SET)
        os.write (lockFile, state)
    except OSError:
        pass


def ReadLockState (lockFile):
    try:
        os.lseek (lockFile, LOCK_STATE_OFFSET, os.SEEK_SET)
        return os.read (lockFile, len (LOCK_STATE_WAITING_FOR_QUIT))
    except OSError:
        return None


@contextlib.contextmanager
def UpdateLock (port):
    # One update per Archicad: the Tapir menu starts a new update also while
    # one runs, and both would replace the add-on and start Archicad. The
    # system releases the lock when the process ends, also when it is killed.
    # The file is not deleted, as another update may have opened it already.
    # If the file cannot be used, the update goes on unlocked. Yields the
    # function that tells another update whether this one waits for Archicad
    # to quit. The installer uses the same lock; while it holds the lock
    # without telling that, the user is not told to quit.
    try:
        lockFile = os.open (os.path.join (tempfile.gettempdir (), 'TapirUpdate_{0}.lock'.format (port)), os.O_RDWR | os.O_CREAT)
    except OSError:
        lockFile = None
    isLocked = False
    try:
        if lockFile is not None:
            try:
                if os.name == 'nt':
                    import msvcrt
                    msvcrt.locking (lockFile, msvcrt.LK_NBLCK, 1)
                else:
                    import fcntl
                    fcntl.flock (lockFile, fcntl.LOCK_EX | fcntl.LOCK_NB)
                isLocked = True
            # Another update holds the lock (msvcrt raises EACCES then).
            except (BlockingIOError, PermissionError):
                isWaitingForQuit = ReadLockState (lockFile) == LOCK_STATE_WAITING_FOR_QUIT
                raise UpdateStoppedError (UPDATE_WAITING_FOR_QUIT_TEXT if isWaitingForQuit else UPDATE_RUNNING_TEXT)
            except OSError:
                pass

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


def Main ():
    parser = ArgumentParser (description = 'Updates the Tapir Add-On and restarts Archicad.')
    # nargs '?': an empty URL may be dropped from the command line, and the
    # option then has no value.
    parser.add_argument ('--downloadUrl', dest = 'downloadUrl', type = str, nargs = '?', const = '')
    parser.add_argument ('--addOnLocation', dest = 'addOnLocation', type = str)
    parser.add_argument ('--host', dest = 'host', type = str, default = 'http://127.0.0.1')
    parser.add_argument ('--port', dest = 'port', type = int, default = 19723)
    try:
        # Unknown options are ignored, so that Add-Ons can pass new ones.
        args, unknownArgs = parser.parse_known_args ()
        if not args.downloadUrl:
            raise UpdateStoppedError (NO_DOWNLOAD_URL_TEXT)
        if not args.addOnLocation:
            raise UpdateError ('The location of the Tapir Add-On is missing.')
        # normpath drops a trailing separator, which would put the new copy
        # of a bundle inside the bundle.
        addOnLocation = os.path.normpath (args.addOnLocation)
        with UpdateLock (args.port) as setWaitingForQuit:
            if not HandOffToInstaller (args.downloadUrl, addOnLocation, args.host, args.port):
                UpdateInPlace (args.downloadUrl, addOnLocation, args.host, args.port, setWaitingForQuit)
    except UpdateWarning as e:
        ShowDialog ('Tapir was updated.\n\n{0}'.format (e), isWarning = True)
    except UpdateStoppedError as e:
        ShowDialog (str (e))
        return 1
    except Exception as e:
        ShowDialog ('Tapir could not be updated.\n\n{0}\n\nYou can install the update with the Tapir Installer from {1}'.format (e, LATEST_RELEASE_URL))
        return 1
    return 0


if __name__ == '__main__':
    sys.exit (Main ())
