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
    response_data = urllib.request.urlopen (connection_object, request_string)
    response_json = json.loads (response_data.read())
    return response_json['result']['addOnCommandResponse']

def QuitArchicad (host, port):
    archicadLocation = RunTapirCommand (host, port, 'GetArchicadLocation')['archicadLocation']
    if IsUsingMacOS ():
        return f"{archicadLocation}/Contents/MacOS/ARCHICAD"
    RunTapirCommand (host, port, 'QuitArchicad')
    return archicadLocation

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

    maxRetries = 20
    for attempt in range(maxRetries):
        try:
            if downloadedFile.endswith ('.zip'):
                with zipfile.ZipFile (downloadedFile, 'r') as zip_ref:
                    zip_ref.extractall (os.path.dirname (args.addOnLocation))
            else:
                shutil.copyfile(downloadedFile, args.addOnLocation)
            break
        except Exception as e:
            if attempt == maxRetries - 1:
                raise
            time.sleep(1)

    subprocess.Popen ([archicadLocation], shell=True)

if __name__ == '__main__':
    main()