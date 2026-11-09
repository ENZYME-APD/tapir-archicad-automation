"""Downloads a DevKit zip and unpacks it into a folder: download_and_unzip.py <url> <dest>.

Does nothing when <dest> exists. GitHub's release downloads fail now and then with a 5xx
answer or a dropped connection, which used to stop a whole build matrix (and the weekly
release) at this step, so a failed download is tried again a few times before giving up.
A download that still fails leaves no <dest> behind, so running the script again retries.
Standard library only.
"""

import os
import shutil
import sys
import subprocess
import platform
import time
import zipfile
import urllib.error
import urllib.request
import urllib.parse

ATTEMPTS = 6            # waits 10, 20, 40, 60, 60 s between them: about 3 minutes in all
TIMEOUT_SECONDS = 300   # per attempt, while connecting or between two reads


def IsWorthRetrying (error):
    """ A server error, a rate limit or a network failure; not a wrong URL or a missing file. """
    if isinstance (error, urllib.error.HTTPError):
        return error.code >= 500 or error.code in (403, 408, 429)
    if isinstance (error, urllib.error.URLError):
        return isinstance (error.reason, OSError)
    return isinstance (error, (OSError, zipfile.BadZipFile))


def Download (url, filePath):
    with urllib.request.urlopen (url, timeout=TIMEOUT_SECONDS) as response, open (filePath, 'wb') as file:
        shutil.copyfileobj (response, file, 1024 * 1024)
    if not zipfile.is_zipfile (filePath):
        raise zipfile.BadZipFile ('the download is not a zip file: ' + url)


def DownloadWithRetries (url, filePath):
    for attempt in range (1, ATTEMPTS + 1):
        try:
            Download (url, filePath)
            return
        except Exception as error:
            if attempt == ATTEMPTS or not IsWorthRetrying (error):
                raise
            wait = min (10 * 2 ** (attempt - 1), 60)
            print ('Download failed ({}), trying again in {} s ({} of {}): {}'.format (error, wait, attempt, ATTEMPTS, url), flush=True)
            time.sleep (wait)


def main ():
    url = sys.argv[1]
    dest = sys.argv[2]

    if os.path.exists (dest):
        return 0

    os.makedirs (dest)
    try:
        fileName = os.path.split (urllib.parse.unquote (url).split ('?')[0])[1]
        filePath = os.path.join (dest, fileName)

        DownloadWithRetries (url, filePath)

        if platform.system () == 'Windows':
            with zipfile.ZipFile (filePath, 'r') as zip:
                zip.extractall (dest)
        elif platform.system () == 'Darwin':
            subprocess.check_call ([
                'unzip', '-qq', filePath,
                '-d', dest
            ])
    except BaseException:
        shutil.rmtree (dest, ignore_errors=True)
        raise

    return 0


if __name__ == '__main__':
    sys.exit (main ())
