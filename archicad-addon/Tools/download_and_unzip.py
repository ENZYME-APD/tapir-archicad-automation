import os
import sys
import subprocess
import platform
import zipfile
import urllib.request
import urllib.parse

url = sys.argv[1]
dest = sys.argv[2]

if os.path.exists (dest):
    sys.exit (0)
    
os.makedirs (dest)

fileName = os.path.split (urllib.parse.unquote (url).split ('?')[0])[1]
filePath = os.path.join (dest, fileName)

urllib.request.urlretrieve (url, filePath)

if platform.system () == 'Windows':
    with zipfile.ZipFile (filePath, 'r') as zip:
        zip.extractall (dest)
elif platform.system () == 'Darwin':
    subprocess.call ([
        'unzip', '-qq', filePath,
        '-d', dest
    ])

sys.exit (0)
