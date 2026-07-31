import argparse
import platform
import urllib.request
import json
import os
import time
import shutil
import zipfile
import subprocess

def IsUsingMacOS ():
    return platform.system () == 'Darwin'

def RunTapirCommand (host, port, command):
    connection_object = urllib.request.Request ('{}:{}'.format (host, port))
    connection_object.add_header ('Content-Type', 'application/json')
    request_data = {
        'command' : 'API.ExecuteAddOnCommand',
        'parameters': {
            'addOnCommandId': {
                'commandNamespace': 'TapirCommand',
                'commandName': command
            },
            'addOnCommandParameters': {}
        }
    }
    request_string = json.dumps (request_data).encode ('utf8')
    response_data = urllib.request.urlopen (connection_object, request_string, timeout=10)
    response_json = json.loads (response_data.read())
    return response_json['result']['addOnCommandResponse']

def QuitArchicad (host, port):
    archicadLocation = RunTapirCommand (host, port, 'GetArchicadLocation')['archicadLocation']
    try:
        RunTapirCommand (host, port, 'QuitArchicad')
    except Exception:
        pass # the connection may drop while Archicad is quitting
    return archicadLocation

def WaitForArchicadToQuit (host, port, maxRetries = 60):
    # On Windows the file replacement below fails while Archicad still locks
    # the add-on, so the retry loop is enough to wait. On macOS the files of a
    # running process can be overwritten, so the shutdown must be awaited
    # explicitly, otherwise Archicad would be relaunched while the old
    # instance is still running.
    for _ in range (maxRetries):
        try:
            RunTapirCommand (host, port, 'GetAddOnVersion')
        except Exception:
            return
        time.sleep (1)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument ('--downloadUrl', dest='downloadUrl', type=str)
    parser.add_argument ('--addOnLocation', dest='addOnLocation', type=str)
    parser.add_argument ('--host', dest='host', type=str, default='http://127.0.0.1')
    parser.add_argument ('--port', dest='port', type=int, default=19723)
    args, unknownArgs = parser.parse_known_args()
    host = 'http://127.0.0.1' if 'host' not in args else args.host
    port = 19723 if 'port' not in args else args.port

    downloadedFile, headers = urllib.request.urlretrieve (args.downloadUrl)

    archicadLocation = QuitArchicad (host, port)
    WaitForArchicadToQuit (host, port)

    maxRetries = 20
    for attempt in range(maxRetries):
        try:
            if downloadedFile.endswith ('.zip'):
                if IsUsingMacOS ():
                    # ditto keeps the permission bits and symlinks of the
                    # bundle, which zipfile.extractall would drop.
                    subprocess.run (['ditto', '-x', '-k', downloadedFile, os.path.dirname (args.addOnLocation)], check=True)
                else:
                    with zipfile.ZipFile (downloadedFile, 'r') as zip_ref:
                        zip_ref.extractall (os.path.dirname (args.addOnLocation))
            else:
                shutil.copyfile(downloadedFile, args.addOnLocation)
            break
        except Exception as e:
            if attempt == maxRetries - 1:
                raise
            time.sleep(1)

    # A list argument combined with shell=True must be avoided: on POSIX it
    # runs 'sh -c <first item>', and the Archicad path contains spaces, so the
    # shell would split it into multiple words.
    if IsUsingMacOS ():
        subprocess.Popen (['open', archicadLocation])
    else:
        subprocess.Popen ([archicadLocation])

if __name__ == '__main__':
    main()