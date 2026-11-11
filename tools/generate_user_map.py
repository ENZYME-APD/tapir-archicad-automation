"""Write user-map.json, the places on the Community page's map of where Tapir is used.

People say where they are on Discord; this reads it and places them on the map by town. Only the
town, its country, its coordinates and the number of people there are published: never a name, a
user id or the text of a message.

Which messages count, per person (by their Discord account, never written out):
  1. An answer to the location question in the welcome channel: every message posted in that
     channel after the question (the latest message there asking where people are from, found by
     QUESTION; USER_MAP_SINCE, a message id, sets it by hand) counts as it is ("Pontevedra",
     "Germany, Braunschweig", "You are the best! ... I am from Slovenia, Maribor").
  2. Elsewhere, in every text, announcement and forum channel and thread the bot can read, at any
     time, only a clear statement counts: "I'm from ...", "I live in ...", "based in ...",
     "living in ...", "hello from ...", "greetings from ...". A passing mention ("I'm in Paris for
     a conference") does not.
  A person's latest answer in the welcome channel wins; without one, their latest statement
  elsewhere. A message that cannot be placed is left out (the run prints how many).

Placing: OpenStreetMap's Nominatim (one request per second, as its usage policy asks), first as a
town, then as a country. What it found is kept in the "geocode" section of the file and carried
over from the previous run (--previous), so a place is looked up once.

Corrections: tools/user_map_overrides.json maps a message id (Discord: right-click the message,
Copy Message ID) to the place it means ("Maribor, Slovenia"), or to null to leave it out.

The bot needs the Message Content Intent (Discord Developer Portal > the app > Bot) to read the
text of messages; without it, every message reads as empty and the map stays as it was. The token
is DISCORD_STATS_TOKEN, else DISCORD_BOT_TOKEN, as for generate_community_data.py.

    python tools/generate_user_map.py --previous previous-map.json --out out/user-map.json

Writes:
    {"version": 1, "generatedAt": "2026-11-11T03:17:00Z",
     "totals": {"people": 7, "places": 7, "countries": 5},
     "places": [{"name": "Maribor", "country": "Slovenia", "countryCode": "SI",
                 "lat": 46.56, "lon": 15.65, "people": 1}, ...],
     "countries": [{"name": "Germany", "code": "DE", "people": 3}, ...],
     "geocode": {"maribor, slovenia": {...} or null, ...}}
"""

import argparse
import json
import os
import re
import sys
import time
import unicodedata
import urllib.error
import urllib.parse
import urllib.request

sys.path.insert (0, os.path.dirname (os.path.abspath (__file__)))
import generate_community_data as community  # noqa: E402  (the Discord helpers)

NOMINATIM = 'https://nominatim.openstreetmap.org/search'
WELCOME_CHANNEL = os.environ.get ('USER_MAP_CHANNEL') or 'welcome-and-rules'
QUESTION = re.compile (r'where\s+(are\s+)?(you|everyone|people)\s+(are\s+)?(from|based|living)|where\s+are\s+the\s+users', re.I)
OVERRIDES = os.path.join (os.path.dirname (os.path.abspath (__file__)), 'user_map_overrides.json')

# A clear statement of where someone is, and the place after it (up to the end of the sentence).
STATEMENT = re.compile (
    r"\b(?:i\s*['’]?\s*a?m\s+(?:originally\s+)?from|i\s+come\s+from|i\s+(?:currently\s+)?live\s+in|"
    r"living\s+in|(?:i\s*['’]?\s*a?m\s+)?based\s+in|(?:hello|hi|hey|greetings|cheers|regards)\s+from|"
    r"here\s+from|writing\s+from)\s+(?P<place>[^.!?\n;()]{2,60})", re.I)
# In an answer, the place may also follow "from"/"in" without a verb ("From Espoo").
LOOSE = re.compile (r'\b(?:from|in)\s+(?P<place>[^.!?\n;()]{2,60})', re.I)
# Trailing words that are not part of the place ("Paris, France and I love Tapir").
TAIL = re.compile (r'\s+(?:and|but|where|which|who|working|since|for|at|so|currently|now|too|also)\b.*$', re.I)
NOISE = re.compile (r'<a?:\w+:\d+>|<[@#!&]*\d+>|https?://\S+|[\U0001F000-\U0001FFFF☀-➿️‍]')
# Words that are never a place, so "you can get from tapir" is not looked up.
NOT_A_PLACE = re.compile (r'\b(tapir|archicad|grasshopper|rhino|discord|github|claude|python|the website|bim|revit|scratch|home|work|office|today|now)\b', re.I)
PLACE_TYPES = {'city', 'town', 'village', 'municipality', 'hamlet', 'suburb', 'borough', 'city_district',
               'administrative', 'county', 'state', 'province', 'region', 'country', 'island'}


def clean (text):
    text = NOISE.sub (' ', text or '')
    return re.sub (r'\s+', ' ', text).strip (' ,.!-:')


def trim_place (place):
    place = TAIL.sub ('', place).strip (' ,.!-:')
    words = place.split ()
    return ' '.join (words[:6]) if 0 < len (words) <= 8 else ''


def candidates (text, answer):
    """The phrases of a message that may name a place, best first. answer: a reply to the question."""
    text = clean (text)
    found = [trim_place (m.group ('place')) for m in STATEMENT.finditer (text)]
    if answer:
        found += [trim_place (m.group ('place')) for m in LOOSE.finditer (text)]
        if len (text.split ()) <= 6:
            found.append (text)
    seen, result = set (), []
    for phrase in found:
        key = phrase.lower ()
        if phrase and key not in seen and not NOT_A_PLACE.search (phrase):
            seen.add (key)
            result.append (phrase)
    return result


# ---- placing ----

last_request = [0.0]


def nominatim (query, feature):
    wait = 1.1 - (time.time () - last_request[0])
    if wait > 0:
        time.sleep (wait)
    last_request[0] = time.time ()
    params = urllib.parse.urlencode ({'q': query, 'format': 'jsonv2', 'limit': 1, 'addressdetails': 1, 'namedetails': 1,
                                      'accept-language': 'en', 'featureType': feature})
    data, _ = community.request_json (NOMINATIM + '?' + params, {})
    return data[0] if data else None


def place_of (hit, feature):
    address = hit.get ('address') or {}
    kind = hit.get ('addresstype') or hit.get ('type') or ''
    if kind not in PLACE_TYPES:
        return None
    country = address.get ('country')
    code = (address.get ('country_code') or '').upper ()
    if not country or not code:
        return None
    if feature == 'country':
        name = country
    else:
        # The town by its own name (Braunschweig, not Brunswick); the country in English.
        # Only in Latin letters: Pune, not पुणे.
        local = (hit.get ('namedetails') or {}).get ('name') or ''
        if any (ch.isalpha () and 'LATIN' not in unicodedata.name (ch, '') for ch in local):
            local = ''
        name = (local if hit.get ('addresstype') in ('city', 'town', 'village', 'municipality', 'hamlet') else None) or (address.get ('city') or address.get ('town') or address.get ('village') or address.get ('municipality')
                or address.get ('county') or address.get ('state') or hit.get ('name') or country)
    # Two decimals (about 1 km): the town, never a street.
    return {'name': name, 'country': country, 'countryCode': code,
            'lat': round (float (hit['lat']), 2), 'lon': round (float (hit['lon']), 2), 'kind': feature}


def geocode (phrase, cache):
    key = unicodedata.normalize ('NFKC', phrase).lower ()
    if key in cache:
        return cache[key]
    place = None
    try:
        for feature in ('settlement', 'country'):
            hit = nominatim (phrase, feature)
            if hit:
                place = place_of (hit, feature)
                if place:
                    break
    except (urllib.error.URLError, KeyError, ValueError) as error:
        print (f'Not placed now ({error}), tried again on the next run: {phrase!r}', file=sys.stderr)
        return None  # not cached: a failed request is not "no such place"
    cache[key] = place
    return place


# ---- Discord ----

def channel_messages (channel_id, token, after=None):
    """Every message of one channel or thread, oldest first; None when the bot cannot read it."""
    messages, cursor = [], ''
    while True:
        query = f'after={after}' if after else ''
        try:
            page = community.discord_get (f'/channels/{channel_id}/messages?limit=100&{query}{cursor}', token)
        except urllib.error.HTTPError as error:
            if error.code in (401, 403):
                return None
            raise
        messages += page
        if len (page) < 100:
            break
        if after:  # with after=, Discord pages forwards: the newest of the page is the next after=
            after = max (page, key=lambda m: int (m['id']))['id']
        else:
            cursor = '&before=' + page[-1]['id']
    return sorted ((m for m in messages if not m['author'].get ('bot')), key=lambda m: int (m['id']))


def read_discord (token):
    """(answers, statements, unreadable): message dicts in the welcome channel after the question,
    and elsewhere."""
    guild = os.environ.get ('DISCORD_GUILD_ID') or ''
    if not guild:
        guilds = community.discord_get ('/users/@me/guilds', token)
        if not guilds:
            raise RuntimeError ('the bot is in no server')
        guild = guilds[0]['id']
    channels = [c for c in community.discord_get (f'/guilds/{guild}/channels', token) if c.get ('type') in (0, 5, 15)]
    threads = {t['id']: t for t in community.discord_get (f'/guilds/{guild}/threads/active', token).get ('threads', [])}
    for channel in channels:
        for thread in community.archived_threads (channel['id'], token):
            threads.setdefault (thread['id'], thread)

    welcome = next ((c for c in channels if c.get ('name') == WELCOME_CHANNEL), None)
    answers, statements, unreadable, question = [], [], 0, None
    if welcome:
        history = channel_messages (welcome['id'], token) or []
        since = os.environ.get ('USER_MAP_SINCE') or ''
        if since:
            question = since
        else:
            asked = [m for m in history if QUESTION.search (m.get ('content') or '')]
            question = asked[-1]['id'] if asked else None
        for message in history:
            if question and int (message['id']) > int (question):
                answers.append (message)
            elif int (message['id']) != int (question or 0):
                statements.append (message)
    else:
        print (f'No channel named {WELCOME_CHANNEL!r}: only statements in other channels count.', file=sys.stderr)
    for channel in [c for c in channels if c is not welcome and c['type'] != 15] + list (threads.values ()):
        history = channel_messages (channel['id'], token)
        if history is None:
            unreadable += 1
        else:
            statements += history
    return answers, statements, unreadable


# ---- assembling ----

def assemble (answers, statements, overrides, cache):
    """The place of each person, then the places and countries with their counts."""
    chosen = {}  # author id -> place; answers in the welcome channel first, then statements
    unplaced = 0
    for group, answer in ((answers, True), (statements, False)):
        for message in sorted (group, key=lambda m: int (m['id']), reverse=True):  # latest first
            author = message['author']['id']
            if author in chosen:
                continue
            if message['id'] in overrides:
                phrases = [overrides[message['id']]] if overrides[message['id']] else []
                if not phrases:
                    chosen[author] = None  # left out on purpose
                    continue
            else:
                phrases = candidates (message.get ('content'), answer)
            place = next ((p for p in (geocode (phrase, cache) for phrase in phrases) if p), None)
            if place:
                chosen[author] = place
            elif answer and phrases:
                unplaced += 1
    people = [p for p in chosen.values () if p]

    places, countries = {}, {}
    for place in people:
        key = (place['name'], place['countryCode'])
        entry = places.setdefault (key, {k: place[k] for k in ('name', 'country', 'countryCode', 'lat', 'lon')})
        entry['people'] = entry.get ('people', 0) + 1
        if place['kind'] == 'country':
            entry['countryOnly'] = True
        country = countries.setdefault (place['countryCode'], {'name': place['country'], 'code': place['countryCode'], 'people': 0})
        country['people'] += 1
    order = lambda e: (-e['people'], e['name'])  # noqa: E731
    return {
        'totals': {'people': len (people), 'places': len (places), 'countries': len (countries)},
        'places': sorted (places.values (), key=order),
        'countries': sorted (countries.values (), key=order),
    }, unplaced


def load_json (path, default):
    if path and os.path.exists (path):
        try:
            with open (path, encoding='utf-8') as file:
                return json.load (file)
        except ValueError:
            pass
    return default


def main ():
    parser = argparse.ArgumentParser (description=__doc__.split ('\n')[0])
    parser.add_argument ('--previous', help='the previous user-map.json, for its places and lookups')
    parser.add_argument ('--out', required=True)
    args = parser.parse_args ()

    previous = load_json (args.previous, {})
    cache = previous.get ('geocode') if isinstance (previous.get ('geocode'), dict) else {}
    overrides = {str (k): v for k, v in load_json (OVERRIDES, {}).items () if not str (k).startswith ('_')}

    token = community.discord_token ()
    if not token:
        print ('No Discord bot token: user-map.json not written.', file=sys.stderr)
        return
    try:
        answers, statements, unreadable = read_discord (token)
    except (urllib.error.URLError, KeyError, ValueError, RuntimeError) as error:
        print (f'Discord not read, user-map.json not written: {error}', file=sys.stderr)
        return
    if not any (m.get ('content') for m in answers + statements):
        # The Message Content Intent is off: every text reads as empty. Keep the previous map.
        print ('Discord messages have no text: turn on the Message Content Intent of the bot. '
               'user-map.json not written.', file=sys.stderr)
        return

    data, unplaced = assemble (answers, statements, overrides, cache)
    data = dict ({'version': 1, 'generatedAt': community.now ().strftime ('%Y-%m-%dT%H:%M:%SZ')}, **data,
                 geocode=dict (sorted (cache.items ())))
    os.makedirs (os.path.dirname (os.path.abspath (args.out)), exist_ok=True)
    with open (args.out, 'w', encoding='utf-8') as file:
        json.dump (data, file, ensure_ascii=False, indent=1)
        file.write ('\n')
    print (f"{data['totals']['people']} people in {data['totals']['places']} places and "
           f"{data['totals']['countries']} countries; {len (answers)} answers to the question, "
           f"{unplaced} of them not placed (see tools/user_map_overrides.json); "
           f"{unreadable} channels not readable.")


if __name__ == '__main__':
    main ()
