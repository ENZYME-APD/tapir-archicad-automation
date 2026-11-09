"""Write community.json, the counts the Community page (docs/community/) draws.

Only counts are collected, never the text of a message. The format is described at the top of
docs/community/community.js; this script writes:

    {"version": 1,
     "generatedAt": "2026-11-06T03:17:00Z",
     "months": [{"month": "2022-07", "commits": 98, "issuesOpened": 3, "pullRequestsMerged": 0,
                 "discordMessages": 41, "discordMembers": 12}, ...],
     "totals": {"commits": ..., "discordMessages": ..., "discordMembers": ...,
                "discordMembersChange": {"diff": 23, "days": 30}, "participants": ...},
     "topContributors": [{"name": "...", "login": "...", "commits": ..., "reports": ..., "tests": ...}, ...],
     "memberHistory": [{"date": "2026-11-06", "members": 637}, ...]}

Sources, each optional: a series that cannot be read is left out of every month, so the page
hides it instead of drawing it wrong.
  - commits per month and per author: `git log` of the checkout (needs the full history),
    without merge commits and without bots (Claude, github-actions and names ending in [bot]).
    A person who committed under several names or addresses counts once, under the name of most
    of their commits: names that differ only in case, accents, punctuation or word order, and
    names with a shared address (a GitHub no-reply address by its account) are one person.
    The top contributors and participants also count the people in a commit's Co-authored-by
    trailers (bots and AI assistants left out), so the issue bot's commits credit the reporter;
    a co-author who also committed is the same person, by the same rules.
  - top contributors: per person their commits (co-authors included, as above), their reports
    (issues they opened that a merged pull request closed with a closing keyword, "Fixes #12")
    and their tests (pull requests they posted a community test runner result on, a comment
    starting with "tapir-test:"), ranked by the three together; the page shows each separately.
    "login" is their GitHub account: from a GitHub no-reply address, else from GitHub's own
    link of the commit's author address to an account (the commits API), and the same account
    on several names is one person. Reports and tests are counted by account, so a person who
    only reports or tests is listed under their login.
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
import unicodedata
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

# A co-author that is a bot or an AI assistant. Claude adds itself as a co-author under several
# names ("Claude", "Claude Opus 5.5", ...) but always with an @anthropic.com address, so it is
# known by its address: a person whose name starts with Claude still counts.
BOT_COAUTHOR = re.compile (r'(\[bot\]|^github-actions\b|^dependabot|@anthropic\.com>?$)', re.I)


# A GitHub no-reply address, old (login@...) or new (id+login@...): the same account either way.
NOREPLY_EMAIL = re.compile (r'^(?:\d+\+)?([^@]+)@users\.noreply\.github\.com$', re.I)
# Addresses that many people share, so they never tie two names together.
SHARED_EMAILS = {'', 'none', 'noreply@github.com', 'you@example.com', 'root@localhost'}


def email_key (email):
    """The account an author address stands for, or None for an address many people share."""
    email = email.strip ().lower ()
    if email in SHARED_EMAILS or '@' not in email:
        return None
    noreply = NOREPLY_EMAIL.match (email)
    return 'github:' + noreply.group (1) if noreply else 'email:' + email


def name_key (name):
    """A name with case, accents, punctuation and word order left out: "Barancsuk Ádám" is "Ádám Barancsuk"."""
    plain = ''.join (c for c in unicodedata.normalize ('NFKD', name.casefold ()) if not unicodedata.combining (c))
    words = re.findall (r'[a-z0-9]+', plain)
    return 'name:' + ' '.join (sorted (words)) if words else None


def group_authors (authors, coauthors=()):
    """{(name, email): display name}: one name for every name and address that belong to one person.

    authors is (date, name, email) per commit, coauthors the same per Co-authored-by trailer. Two
    entries are one person when they share an address (a GitHub no-reply address counts by its
    account) or a name, the same up to case, accents, punctuation and word order. The person is
    shown under the name they made most commits with, the newer one on a tie; a person who never
    committed, under the name of most of their trailers. .mailmap still wins for commit authors:
    git log has applied it already."""
    parent = {}

    def find (key):
        parent.setdefault (key, key)
        while parent[key] != key:
            parent[key] = parent[parent[key]]
            key = parent[key]
        return key

    def group_of (name, email):
        return find (name_key (name) or email_key (email) or 'raw:' + name)

    entries = list (authors) + list (coauthors)
    for _, name, email in entries:
        keys = [key for key in (name_key (name), email_key (email)) if key]
        for key in keys[1:]:
            parent[find (key)] = find (keys[0])

    def names_by_group (source):
        counts = collections.defaultdict (collections.Counter)
        newest = {}
        for date, name, email in source:
            group = group_of (name, email)
            counts[group][name] += 1
            newest[(group, name)] = max (date, newest.get ((group, name), ''))
        return {group: max (names, key=lambda name: (names[name], newest[(group, name)])) for group, names in counts.items ()}

    shown = names_by_group (coauthors)
    shown.update (names_by_group (authors))
    return {(name, email): shown[group_of (name, email)] for _, name, email in entries}


def read_commits ():
    """The commits of the checkout: (commits, credits, addresses).

    commits is (date, author) of every commit by a person, merges and bots left out. credits is
    (date, name) of everyone a commit credits: its author when a person, and each person in its
    Co-authored-by trailers, so a commit the issue bot made for someone's issue credits the
    person who reported it. The commit counts use commits; the contributors use credits.
    Each person is counted once, under one name, whatever names and addresses their commits and
    trailers carry (see group_authors), so nobody needs a .mailmap line to be counted once.
    addresses is {name: set of their email_key values}, which link a name to a GitHub account."""
    output = subprocess.run (['git', 'log', '--no-merges',
                              '--format=%aI%x09%aN%x09%aE%x09%(trailers:key=Co-authored-by,valueonly,separator=%x1f)%x1e'],
                             capture_output=True, text=True, check=True).stdout
    records = []
    for record in output.split ('\x1e'):
        date, _, rest = record.strip ('\n').partition ('\t')
        name, _, rest = rest.partition ('\t')
        email, _, trailers = rest.partition ('\t')
        if not date:
            continue
        name = name.strip ()
        author = (date, name, email.strip ()) if name and not BOT_NAME.search (name) else None
        coauthors = []
        for value in trailers.split ('\x1f'):
            value = value.strip ()
            co_name, _, co_email = value.partition ('<')
            co_name = co_name.strip ()
            if co_name and not BOT_COAUTHOR.search (value):
                coauthors.append ((date, co_name, co_email.rstrip ('>').strip ()))
        records.append ((author, coauthors))

    shown = group_authors ([author for author, _ in records if author],
                           [coauthor for _, coauthors in records for coauthor in coauthors])
    commits, credits = [], []
    addresses = collections.defaultdict (set)
    for author, coauthors in records:
        names = []
        if author:
            commits.append ((author[0], shown[author[1:]]))
        for date, name, email in ([author] if author else []) + coauthors:
            if email_key (email):
                addresses[shown[(name, email)]].add (email_key (email))
            if shown[(name, email)] not in names:
                names.append (shown[(name, email)])
                credits.append ((date, shown[(name, email)]))
    return commits, credits, addresses


# ---- GitHub ----

# The first line of a community test runner result (tools/community-test-runner, RESULT_MARKER).
TEST_RESULT_MARKER = 'tapir-test:'
# A closing keyword with the issue it closes: "Fixes #12", "closes: owner/repo#12" or the issue's URL.
CLOSING_REFERENCE = re.compile (r'\b(?:close[sd]?|fix(?:e[sd])?|resolve[sd]?):?\s+'
                                r'(?:https://github\.com/([\w.-]+/[\w.-]+)/issues/|([\w.-]+/[\w.-]+)#|#)(\d+)\b', re.I)

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
    return opened, merged, pull_authors, count_reports (repo, issues, pulls)


def count_reports (repo, issues, pulls):
    """{login: issues they opened that a merged pull request closed}: closed issues named after a
    closing keyword in the body of a merged pull request of this repository. Bots left out."""
    authors = {i['number']: i['user']['login'] for i in issues
               if 'pull_request' not in i and i.get ('user') and i.get ('state') == 'closed'}
    fixed = set ()
    for pull in pulls:
        if not pull.get ('merged_at'):
            continue
        for url_repo, short_repo, number in CLOSING_REFERENCE.findall (pull.get ('body') or ''):
            other = url_repo or short_repo
            if not other or other.lower () == repo.lower ():
                fixed.add (int (number))
    reports = collections.Counter (authors[n] for n in fixed if n in authors and not BOT_NAME.search (authors[n]))
    return dict (reports)


def count_tests (repo):
    """{login: pull requests they posted a community test runner result on}. Bots left out."""
    tested = collections.defaultdict (set)
    for comment in github_pages (repo, 'issues/comments?sort=created&direction=asc'):
        login = (comment.get ('user') or {}).get ('login')
        if login and not BOT_NAME.search (login) and (comment.get ('body') or '').startswith (TEST_RESULT_MARKER):
            tested[login].add (comment['issue_url'].rsplit ('/', 1)[-1])
    return {login: len (numbers) for login, numbers in tested.items ()}


def commit_logins (repo):
    """{author address: GitHub login}, as GitHub links the commit addresses of the default branch
    to accounts (an address a person added to their account)."""
    logins = {}
    for commit in github_pages (repo, 'commits?'):
        login = (commit.get ('author') or {}).get ('login')
        email = ((commit.get ('commit') or {}).get ('author') or {}).get ('email') or ''
        if login and email_key (email):
            logins.setdefault (email_key (email), login)
    return logins


def top_contributors (credits, addresses, logins, reports, tests, count=40):
    """[{"name", "login", "commits", "reports", "tests"}] of the count people with the most of the
    three together. A name gets the login its addresses link to; names on one login are one person,
    shown under the name with the most commits. Reports and tests are matched by login."""
    people = {}

    def person (key, name):
        return people.setdefault (key, {'name': name, 'login': None, 'commits': 0, 'reports': 0, 'tests': 0, 'shownBy': -1})

    for name, commits in collections.Counter (name for _, name in credits).items ():
        linked = sorted ({key[len ('github:'):] for key in addresses.get (name, ()) if key.startswith ('github:')} |
                         {logins[key] for key in addresses.get (name, ()) if key in logins})
        linked = [login for login in linked if not BOT_NAME.search (login)]
        # A name on several accounts: the account named like the person, else the first.
        login = next ((l for l in linked if name_key (l) == name_key (name)), linked[0] if linked else None)
        entry = person ('login:' + login.lower () if login else 'name:' + name, name)
        entry['login'] = login
        entry['commits'] += commits
        if commits > entry['shownBy']:
            entry['name'], entry['shownBy'] = name, commits
    for counts, field in ((reports, 'reports'), (tests, 'tests')):
        for login, value in counts.items ():
            entry = person ('login:' + login.lower (), login)
            entry['login'] = entry['login'] or login
            entry[field] += value
    ranked = sorted (people.values (), key=lambda p: (-(p['commits'] + p['reports'] + p['tests']), -p['commits'], p['name'].lower ()))
    top = []
    for p in ranked[:count]:
        entry = {'name': p['name']}
        if p['login']:
            entry['login'] = p['login']
        entry.update ({field: p[field] for field in ('commits', 'reports', 'tests')})
        top.append (entry)
    return top


# ---- Discord ----

def discord_token ():
    # Stripped: a token pasted into a secret often ends with a line break, which is not a valid
    # header value and would stop every Discord request.
    for name in ('DISCORD_STATS_TOKEN', 'DISCORD_BOT_TOKEN'):
        token = (os.environ.get (name) or '').strip ()
        if token:
            return token
    return ''


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

    commits, credits, addresses = read_commits ()
    for date, author in commits:
        series['commits'][month_of (date)] += 1
    for date, name in credits:
        if date >= year_ago:
            people.add ('git:' + name.lower ())
    totals['commits'] = len (commits)
    top = top_contributors (credits, addresses, {}, {}, {})

    try:
        opened, merged, pull_authors, reports = read_github (args.repo)
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
    else:
        # Without the tests or the logins the reports still count: a person is then matched by name.
        try:
            tests = count_tests (args.repo)
        except (urllib.error.URLError, KeyError, ValueError) as error:
            print (f'Test results not read: {error}', file=sys.stderr)
            tests = {}
        try:
            logins = commit_logins (args.repo)
        except (urllib.error.URLError, KeyError, ValueError) as error:
            print (f'Commit accounts not read: {error}', file=sys.stderr)
            logins = {}
        top = top_contributors (credits, addresses, logins, reports, tests)

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
        print (f'Discord not read with the bot: {error}', file=sys.stderr)
        if members_now is None:
            # The public invite still gives the member count when the bot cannot be used.
            try:
                _, members_now = read_invite (os.environ.get ('DISCORD_INVITE') or DEFAULT_INVITE)
            except (urllib.error.URLError, KeyError, ValueError) as invite_error:
                print (f'Discord invite not read either: {invite_error}', file=sys.stderr)

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
