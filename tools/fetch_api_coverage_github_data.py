"""Write the live GitHub data tools/generate_api_coverage.py reads (--github-data).

Collects the open pull requests with the lines they add to the Add-On sources
(archicad-addon/Sources), and the open issues, of the repository. Standard library only; GITHUB_TOKEN authenticates.

A request GitHub refuses for its rate limit (403 or 429) is retried after the
wait GitHub names (Retry-After, or the reset time of an exhausted limit), or
after a minute for a secondary limit that names none. A wait longer than
MAX_WAIT_SECONDS is not worth holding the runner for: the script stops with a
message saying when the limit resets, and the next run (hourly at the latest)
picks the data up.

    python tools/fetch_api_coverage_github_data.py ENZYME-APD/tapir-archicad-automation github.json
"""

import datetime
import json
import os
import sys
import time
import urllib.error
import urllib.request

API = 'https://api.github.com'
RETRIES = 5
MAX_WAIT_SECONDS = 10 * 60


def rate_limit_wait (error, attempt):
    # Seconds to wait before retrying a request GitHub refused for its rate
    # limit, None when the refusal is not about the rate limit.
    if error.code not in (403, 429):
        return None
    retry_after = error.headers.get ('Retry-After') or ''
    if retry_after.isdigit ():
        return int (retry_after)
    reset = error.headers.get ('X-RateLimit-Reset') or ''
    if error.headers.get ('X-RateLimit-Remaining') == '0' and reset.isdigit ():
        return max (1, int (reset) - int (time.time ()) + 1)
    if 'rate limit' in error.read ().decode ('utf-8', 'replace').lower ():
        return 60 * (attempt + 1)
    return None


def get (path):
    request = urllib.request.Request (API + path, headers={
        'Accept': 'application/vnd.github+json',
        'Authorization': 'Bearer ' + os.environ['GITHUB_TOKEN'],
        'X-GitHub-Api-Version': '2022-11-28',
    })
    for attempt in range (RETRIES + 1):
        try:
            with urllib.request.urlopen (request, timeout=60) as response:
                return json.load (response)
        except urllib.error.HTTPError as error:
            wait = rate_limit_wait (error, attempt)
            if wait is None or attempt == RETRIES:
                raise
            if wait > MAX_WAIT_SECONDS:
                resets = datetime.datetime.now (datetime.timezone.utc) + datetime.timedelta (seconds=wait)
                sys.exit (f'GitHub API rate limit reached for {path}; it resets at {resets:%H:%M} UTC, the next run will fetch the data.')
            print (f'GitHub API rate limit reached for {path}; retrying in {wait} s', file=sys.stderr)
            time.sleep (wait)


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
        added = []
        for changed in pages (f'/repos/{repo}/pulls/{pr["number"]}/files'):
            # Only code counts: docs, examples or this checklist's own data
            # naming a function do not make it in progress.
            if not changed['filename'].startswith ('archicad-addon/Sources/'):
                continue
            for line in (changed.get ('patch') or '').splitlines ():
                if line.startswith ('+') and not line.startswith ('+++'):
                    added.append (line[1:])
        data['openPullRequests'].append ({'number': pr['number'], 'title': pr['title'], 'addedText': '\n'.join (added)})
    for issue in pages (f'/repos/{repo}/issues?state=open'):
        if 'pull_request' not in issue:
            data['openIssues'].append ({'number': issue['number'], 'title': issue['title'], 'body': issue.get ('body') or ''})
    with open (out, 'w', encoding='utf-8') as file:
        json.dump (data, file)
    print (f'{len (data["openPullRequests"])} open pull requests, {len (data["openIssues"])} open issues')


if __name__ == '__main__':
    main ()
