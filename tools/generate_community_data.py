"""Write community.json, the counts the Community page (docs/community/) draws.

Only counts are collected, never the text of a message. The format is described at the top of
docs/community/community.js; this script writes:

    {"version": 1,
     "generatedAt": "2026-11-06T03:17:00Z",
     "months": [{"month": "2022-07", "commits": 98, "issuesOpened": 3, "pullRequestsMerged": 0,
                 "discordMessages": 41, "discordMembers": 12}, ...],
     "totals": {"commits": ..., "discordMessages": ..., "discordMembers": ...,
                "discordMembersChange": {"diff": 23, "days": 30}, "participants": ...},
     "topContributors": [{"name": "...", "commits": ...}, ...],
     "memberHistory": [{"date": "2026-11-06", "members": 637}, ...]}

Sources, each optional: a series that cannot be read is left out of every month, so the page
hides it instead of drawing it wrong.
  - commits per month and per author: `git log` of the checkout (needs the full history),
    without merge commits and without bots (Claude, github-actions and names ending in [bot]).
  - issues opened and pull requests merged per month: the GitHub REST API (GITHUB_TOKEN).
  - Discord messages per month (people, not bots): the history of every text and announcement
    channel the bot can read, and of the threads in them and in forum channels. Discord members
    per month: the members still in the server, by the month they joined (people who left are
    not counted), which needs the bot's Server Members Intent.
    The current member count falls back to the server's public invite. The bot token is read
    from DISCORD_STATS_TOKEN, else DISCORD_BOT_TOKEN. The server is the one the bot is in
    (DISCORD_GUILD_ID picks one if it is in several); without a token, the invite DISCORD_INVITE
    (default: the one the site links to) gives the server's member count.
  - discordMembersChange compares today's count with the one stored about 30 days earlier in
    memberHistory, carried over from the previous community.json (--previous).
  - participants: distinct people who, in the last 365 days, committed, opened an issue or a
    pull request, or posted on Discord (each platform's name; bots left out).

The current, unfinished month is left out of months.

    python tools/generate_community_data.py --repo ENZYME-APD/tapir-archicad-automation \
        --previous previous.json --out out/community.json
"""

import argparse
import collections
import datetime
import json
import os
import re
import subprocess
import sys
import time
import urllib.error
import urllib.request

GITHUB_API = 'https://api.github.com'
DISCORD_API = 'https://discord.com/api/v10'
USER_AGENT = 'TapirCommunityData (https://github.com/ENZYME-APD/tapir-archicad-automation, 1.0)'
HISTORY_DAYS = 400
# The invite the site links to (docs/), for the member count when there is no bot token.
DEFAULT_INVITE = 'NAnSennmpY'
BOT_NAME = re.compile (r'(\[bot\]$|^claude$|^github-actions$|^dependabot)', re.I)


def now ():
    return datetime.datetime.now (datetime.timezone.utc)


def month_of (timestamp):
    return timestamp[:7]


def request_json (url, headers, retries=5):
    request = urllib.request.Request (url, headers=dict (headers, **{'User-Agent': USER_AGENT}))
    for attempt in range (retries + 1):
        try:
            with urllib.request.urlopen (request, timeout=60) as response:
                return json.load (response), response.headers
        except urllib.error.HTTPError as error:
            # Discord answers 429 with retry_after; GitHub names its wait in Retry-After.
            if error.code in (403, 429) and attempt < retries:
                body = error.read ().decode ('utf-8', 'replace')
                wait = error.headers.get ('Retry-After') or ''
                try:
                    wait = float (json.loads (body).get ('retry_after', wait or 0))
                except (ValueError, AttributeError):
                    wait = float (wait) if wait.replace ('.', '', 1).isdigit () else 0
                if error.code == 429 or 'rate limit' in body.lower ():
                    time.sleep (min (max (wait, 1), 120) + 0.5)
                    continue
            raise


# ---- git ----

def read_commits ():
    """(date, author) of every commit of the checkout by a person: merges and bots are left out."""
    output = subprocess.run (['git', 'log', '--no-merges', '--format=%aI%x09%aN'], capture_output=True, text=True, check=True).stdout
    commits = []
    for line in output.splitlines ():
        date, _, author = line.partition ('\t')
        author = author.strip ()
        if date and author and not BOT_NAME.search (author):
            commits.append ((date, author))
    return commits


# ---- GitHub ----

def github_pages (repo, path):
    headers = {'Accept': 'application/vnd.github+json', 'X-GitHub-Api-Version': '2022-11-28'}
    if os.environ.get ('GITHUB_TOKEN'):
        headers['Authorization'] = 'Bearer ' + os.environ['GITHUB_TOKEN']
    page, result = 1, []
    while True:
        chunk, _ = request_json (f'{GITHUB_API}/repos/{repo}/{path}&per_page=100&page={page}', headers)
        result += chunk
        if len (chunk) < 100:
            return result
        page += 1


def read_github (repo):
    issues = github_pages (repo, 'issues?state=all&sort=created&direction=asc')
    pulls = github_pages (repo, 'pulls?state=all&sort=created&direction=asc')
    opened = [(i['created_at'], i['user']['login']) for i in issues if 'pull_request' not in i and i.get ('user')]
    pull_authors = [(p['created_at'], p['user']['login']) for p in pulls if p.get ('user')]
    merged = [p['merged_at'] for p in pulls if p.get ('merged_at')]
    return opened, merged, pull_authors


# ---- Discord ----

def discord_token ():
    return os.environ.get ('DISCORD_STATS_TOKEN') or os.environ.get ('DISCORD_BOT_TOKEN') or ''


def discord_get (path, token=None):
    headers = {'Authorization': 'Bot ' + token} if token else {}
    data, _ = request_json (DISCORD_API + path, headers)
    return data


def read_invite (code):
    data = discord_get (f'/invites/{code}?with_counts=true')
    return data['guild']['id'], data.get ('approximate_member_count')


def read_members (guild, token):
    """(joined_at of every person still in the server, number of bots), or None without the intent."""
    joined, bots, after = [], 0, '0'
    while True:
        try:
            page = discord_get (f'/guilds/{guild}/members?limit=1000&after={after}', token)
        except urllib.error.HTTPError as error:
            if error.code in (401, 403):
                print ('Discord members not readable: enable the Server Members Intent of the bot.', file=sys.stderr)
                return None
            raise
        for member in page:
            if member['user'].get ('bot'):
                bots += 1
            elif member.get ('joined_at'):
                joined.append (member['joined_at'])
        if len (page) < 1000:
            return joined, bots
        after = page[-1]['user']['id']


def read_history (channel_id, token, messages, seen):
    """Adds the messages by people of one channel or thread; False when the bot cannot read it."""
    before = ''
    while True:
        try:
            page = discord_get (f'/channels/{channel_id}/messages?limit=100{before}', token)
        except urllib.error.HTTPError as error:
            if error.code in (401, 403):
                return False
            raise
        for message in page:
            if message['id'] not in seen and not message['author'].get ('bot'):
                seen.add (message['id'])
                messages.append ((message['timestamp'], 'discord:' + message['author']['id']))
        if len (page) < 100:
            return True
        before = '&before=' + page[-1]['id']


def archived_threads (channel_id, token):
    """The archived public threads of a text, announcement or forum channel."""
    threads, before = [], ''
    while True:
        try:
            page = discord_get (f'/channels/{channel_id}/threads/archived/public?limit=100{before}', token)
        except urllib.error.HTTPError as error:
            if error.code in (401, 403):
                return threads
            raise
        threads += page.get ('threads', [])
        if not page.get ('has_more') or not page.get ('threads'):
            return threads
        before = '&before=' + page['threads'][-1]['thread_metadata']['archive_timestamp']


def read_messages (guild, token):
    """(created_at, author id) of every message by a person that the bot can read: text and
    announcement channels, and the threads of those and of forum channels (posts), active and
    archived. Each message is counted once."""
    channels = [c for c in discord_get (f'/guilds/{guild}/channels', token) if c.get ('type') in (0, 5, 15)]
    threads = {t['id']: t for t in discord_get (f'/guilds/{guild}/threads/active', token).get ('threads', [])}
    messages, seen, skipped = [], set (), 0
    for channel in channels:
        if channel['type'] != 15 and not read_history (channel['id'], token, messages, seen):  # a forum has only threads
            skipped += 1
            continue
        for thread in archived_threads (channel['id'], token):
            threads.setdefault (thread['id'], thread)
    for thread in threads.values ():
        if not read_history (thread['id'], token, messages, seen):
            skipped += 1
    if skipped:
        print (f'{skipped} Discord channels or threads are not readable by the bot and were left out.', file=sys.stderr)
    return messages


# ---- assembling ----

def month_range (first, last):
    year, month = int (first[:4]), int (first[5:7])
    while f'{year:04d}-{month:02d}' <= last:
        yield f'{year:04d}-{month:02d}'
        month += 1
        if month > 12:
            year, month = year + 1, 1


def members_change (history, today, members):
    """The change against the stored count closest to 30 days before today, at least a day old."""
    today_date = datetime.date.fromisoformat (today)
    older = [h for h in history if (today_date - datetime.date.fromisoformat (h['date'])).days >= 1]
    if not older:
        return None
    target = [h for h in older if (today_date - datetime.date.fromisoformat (h['date'])).days >= 30]
    base = target[-1] if target else older[0]
    return {'diff': members - base['members'], 'days': (today_date - datetime.date.fromisoformat (base['date'])).days}


def main ():
    parser = argparse.ArgumentParser (description=__doc__.split ('\n')[0])
    parser.add_argument ('--repo', required=True)
    parser.add_argument ('--previous', help='the previous community.json, for the member history')
    parser.add_argument ('--out', required=True)
    args = parser.parse_args ()

    stamp = now ()
    today = stamp.date ().isoformat ()
    current_month = today[:7]
    year_ago = (stamp - datetime.timedelta (days=365)).isoformat ()
    series = collections.defaultdict (collections.Counter)
    totals, people = {}, set ()

    commits = read_commits ()
    for date, author in commits:
        series['commits'][month_of (date)] += 1
        if date >= year_ago:
            people.add ('git:' + author.lower ())
    totals['commits'] = len (commits)
    authors = collections.Counter (author for _, author in commits)
    top = [{'name': name, 'commits': count} for name, count in authors.most_common (40)]

    try:
        opened, merged, pull_authors = read_github (args.repo)
        for date, _ in opened:
            series['issuesOpened'][month_of (date)] += 1
        for date in merged:
            series['pullRequestsMerged'][month_of (date)] += 1
        for date, login in opened + pull_authors:
            if date >= year_ago and not BOT_NAME.search (login):
                people.add ('github:' + login.lower ())
    except (urllib.error.URLError, KeyError, ValueError) as error:
        print (f'GitHub issues and pull requests not read: {error}', file=sys.stderr)
        series.pop ('issuesOpened', None)
        series.pop ('pullRequestsMerged', None)

    history = []
    if args.previous and os.path.exists (args.previous):
        try:
            with open (args.previous, encoding='utf-8') as file:
                history = [h for h in json.load (file).get ('memberHistory', [])
                           if isinstance (h, dict) and isinstance (h.get ('members'), int) and isinstance (h.get ('date'), str)]
        except (ValueError, AttributeError):
            history = []

    token = discord_token ()
    members_now = None
    guild = None
    try:
        if token:
            # The server the bot is in, so a changed or expired invite never stops the counts.
            guilds = discord_get ('/users/@me/guilds', token)
            wanted = os.environ.get ('DISCORD_GUILD_ID') or ''
            guild = next ((g['id'] for g in guilds if g['id'] == wanted), guilds[0]['id'] if len (guilds) == 1 else None)
            if guild is None:
                print ('The bot is in several Discord servers: set DISCORD_GUILD_ID.', file=sys.stderr)
        if guild is None:
            guild, members_now = read_invite (os.environ.get ('DISCORD_INVITE') or DEFAULT_INVITE)
            if token:
                print ('No Discord bot token for the server: only the member count from the invite is read.', file=sys.stderr)
                token = ''
        if token:
            result = read_members (guild, token)
            if result is not None:
                joined, _ = result
                members_now = len (joined)
                for date in joined:
                    series['discordMembers'][month_of (date)] += 1  # joins; made cumulative below
            else:
                _, members_now = read_invite (os.environ.get ('DISCORD_INVITE') or DEFAULT_INVITE)
            messages = read_messages (guild, token)
            if messages:
                for date, author in messages:
                    series['discordMessages'][month_of (date)] += 1
                    if date >= year_ago:
                        people.add (author)
                totals['discordMessages'] = len (messages)
        else:
            print ('No Discord bot token: only the member count from the invite is read.', file=sys.stderr)
    except (urllib.error.URLError, KeyError, ValueError, IndexError) as error:
        print (f'Discord not read: {error}', file=sys.stderr)

    if isinstance (members_now, int):
        totals['discordMembers'] = members_now
        change = members_change (history, today, members_now)
        if change:
            totals['discordMembersChange'] = change
        history = [h for h in history if h['date'] != today] + [{'date': today, 'members': members_now}]
        cutoff = (stamp.date () - datetime.timedelta (days=HISTORY_DAYS)).isoformat ()
        history = sorted ((h for h in history if h['date'] >= cutoff), key=lambda h: h['date'])
    if people:
        totals['participants'] = len (people)

    # Months from the first one with any count to the last finished one, every series in each.
    known = sorted (m for counter in series.values () for m in counter if m < current_month)
    months = []
    if known:
        running = sum (count for month, count in series.get ('discordMembers', {}).items () if month < known[0])
        for month in month_range (known[0], max (known)):
            entry = {'month': month}
            for key, counter in series.items ():
                if key == 'discordMembers':
                    running += counter.get (month, 0)
                    entry[key] = running
                else:
                    entry[key] = counter.get (month, 0)
            months.append (entry)

    data = {
        'version': 1,
        'generatedAt': stamp.strftime ('%Y-%m-%dT%H:%M:%SZ'),
        'months': months,
        'totals': totals,
        'topContributors': top,
        'memberHistory': history,
    }
    os.makedirs (os.path.dirname (args.out) or '.', exist_ok=True)
    with open (args.out, 'w', encoding='utf-8') as file:
        json.dump (data, file, indent=1, ensure_ascii=False)
        file.write ('\n')
    print (f'{len (months)} months ({months[0]["month"] if months else "-"} to {months[-1]["month"] if months else "-"}), '
           f'series: {", ".join (sorted (series))}; totals: {json.dumps (totals)}')


if __name__ == '__main__':
    main ()
