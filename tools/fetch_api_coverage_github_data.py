"""Write the live GitHub data tools/generate_api_coverage.py reads (--github-data).

Collects the open pull requests with the lines they add to each Add-On source
file (archicad-addon/Sources), and the open issues, of the repository. Standard library only; GITHUB_TOKEN authenticates.

    python tools/fetch_api_coverage_github_data.py ENZYME-APD/tapir-archicad-automation github.json
"""

import datetime
import json
import os
import sys
import urllib.request

API = 'https://api.github.com'


def get (path):
    request = urllib.request.Request (API + path, headers={
        'Accept': 'application/vnd.github+json',
        'Authorization': 'Bearer ' + os.environ['GITHUB_TOKEN'],
        'X-GitHub-Api-Version': '2022-11-28',
    })
    with urllib.request.urlopen (request, timeout=60) as response:
        return json.load (response)


def pages (path):
    result, page = [], 1
    while True:
        chunk = get (f'{path}{"&" if "?" in path else "?"}per_page=100&page={page}')
        result += chunk
        if len (chunk) < 100:
            return result
        page += 1


def main ():
    repo, out = sys.argv[1], sys.argv[2]
    data = {
        'generatedAt': datetime.datetime.now (datetime.timezone.utc).strftime ('%Y-%m-%d %H:%M UTC'),
        'openPullRequests': [],
        'openIssues': [],
    }
    for pr in pages (f'/repos/{repo}/pulls?state=open'):
        files = []
        for changed in pages (f'/repos/{repo}/pulls/{pr["number"]}/files'):
            # Only code counts: docs, examples or this checklist's own data
            # naming a function do not make it in progress.
            if not changed['filename'].startswith ('archicad-addon/Sources/'):
                continue
            added = [line[1:] for line in (changed.get ('patch') or '').splitlines ()
                     if line.startswith ('+') and not line.startswith ('+++')]
            # Per file: a C++ member counts when a file that uses its class calls it.
            files.append ({'path': changed['filename'], 'addedText': '\n'.join (added)})
        data['openPullRequests'].append ({'number': pr['number'], 'title': pr['title'], 'addedFiles': files})
    for issue in pages (f'/repos/{repo}/issues?state=open'):
        if 'pull_request' not in issue:
            data['openIssues'].append ({'number': issue['number'], 'title': issue['title'], 'body': issue.get ('body') or ''})
    with open (out, 'w', encoding='utf-8') as file:
        json.dump (data, file)
    print (f'{len (data["openPullRequests"])} open pull requests, {len (data["openIssues"])} open issues')


if __name__ == '__main__':
    main ()
