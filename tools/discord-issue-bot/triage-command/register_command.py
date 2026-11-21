"""Registers the /triage slash command on the Tapir Discord server.

Run once (and again whenever the command's options change):

    DISCORD_BOT_TOKEN=... python register_command.py --application-id <id> --guild-id <id>

`--print` shows the command definition without calling Discord. A guild
command appears at once; the command is hidden from everyone without the
Manage Server permission by default (Discord's per-command permissions in
Server Settings > Integrations can open it to a role), and the Worker checks
the allowed roles and users itself in any case.
"""
import argparse
import json
import os
import sys
import urllib.error
import urllib.request

MANAGE_GUILD = 1 << 5
MAX_LOOKBACK_MINUTES = 10080

BOOLEAN = 5
INTEGER = 4

TRIAGE_COMMAND = {
    'name': 'triage',
    'type': 1,
    'description': 'Scan the Discord channels for bug reports and feature requests now',
    'default_member_permissions': str(MANAGE_GUILD),
    'dm_permission': False,
    'options': [
        {
            'type': BOOLEAN,
            'name': 'dry_run',
            'description': 'Only log what it would file, without creating issues',
            'required': False,
        },
        {
            'type': INTEGER,
            'name': 'lookback_minutes',
            'description': 'How far back to scan (default: the last day)',
            'required': False,
            'min_value': 1,
            'max_value': MAX_LOOKBACK_MINUTES,
        },
    ],
}


def register(application_id, guild_id, token, opener=urllib.request.urlopen):
    """PUT the command list of the guild (this bot's only command) and return Discord's answer."""
    request = urllib.request.Request(
        f'https://discord.com/api/v10/applications/{application_id}/guilds/{guild_id}/commands',
        data=json.dumps([TRIAGE_COMMAND]).encode(),
        method='PUT',
        headers={
            'Authorization': f'Bot {token}',
            'Content-Type': 'application/json',
            'User-Agent': 'tapir-discord-triage-command (register_command.py)',
        },
    )
    with opener(request, timeout=30) as response:
        return json.loads(response.read())


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--application-id')
    parser.add_argument('--guild-id')
    parser.add_argument('--print', action='store_true', help='print the command definition and exit')
    args = parser.parse_args(argv)
    if args.print:
        print(json.dumps(TRIAGE_COMMAND, indent=2))
        return 0
    token = os.environ.get('DISCORD_BOT_TOKEN', '')
    if not (args.application_id and args.guild_id and token):
        parser.error('--application-id, --guild-id and the DISCORD_BOT_TOKEN variable are required')
    try:
        commands = register(args.application_id, args.guild_id, token)
    except urllib.error.HTTPError as error:
        print(f'Discord answered {error.code}: {error.read().decode(errors="replace")[:300]}', file=sys.stderr)
        return 1
    print('Registered:', ', '.join('/' + command['name'] for command in commands))
    return 0


if __name__ == '__main__':
    sys.exit(main())
