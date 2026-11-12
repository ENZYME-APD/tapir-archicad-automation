# The `/triage` command

Type `/triage` in the Tapir Discord server and the Discord issue bot scans the
channels right away, instead of waiting for the next scheduled run (GitHub
drops many of those when it is busy). Options: `dry_run` (log only, no issues)
and `lookback_minutes` (1 to 10080; empty keeps the bot's default of one day).

Discord sends a slash command to an HTTPS address, and GitHub Actions cannot
listen on one, so this folder holds the small piece that does: a Cloudflare
Worker ([`worker.js`](worker.js), free tier, no dependencies). It

1. checks that the request really comes from Discord (Ed25519 signature, at
   most five minutes old);
2. checks that the person may run it: a role in `ALLOWED_ROLE_IDS` or a user in
   `ALLOWED_USER_IDS`. **With neither set nobody is allowed.** It also refuses
   other servers when `DISCORD_GUILD_ID` is set;
3. starts the **Discord Issue Bot** workflow with `workflow_dispatch`
   (`dry_run`, `lookback_minutes`), then tells the person, in a reply only they
   see, whether it started.

It never reads messages, never creates issues and never holds the Discord bot
token. The scan itself, its duplicate guards, its daily issue cap and its
checkpoint are the workflow's, unchanged; its `concurrency` group queues a scan
started while another runs, so pressing the command twice cannot overlap.

A plain @mention of the bot is not possible this way: Discord sends mentions
only over a connection that stays open, which a Worker (or GitHub Actions)
cannot hold.

## One-time setup

You need a Cloudflare account (free) and someone with write access to the
repository for the GitHub token.

1. **Discord application.** In the
   [developer portal](https://discord.com/developers/applications) open the
   bot's application, *General Information*, and copy the **Application ID**
   and the **Public Key**.
2. **GitHub token.** *Settings → Developer settings → Fine-grained tokens*:
   resource owner `ENZYME-APD`, **only** the repository
   `tapir-archicad-automation`, repository permission **Actions: Read and
   write** and nothing else. Give it an expiry and note the date to renew it.
3. **IDs.** In Discord enable *Developer Mode*, then copy the server's ID and
   the role (or user) IDs that may use `/triage`. Put them in the `[vars]` of
   [`wrangler.toml`](wrangler.toml) (they are not secrets; a deploy overwrites
   values set only in the dashboard).
4. **Deploy.**

   ```bash
   cd tools/discord-issue-bot/triage-command
   npx wrangler deploy
   npx wrangler secret put DISCORD_PUBLIC_KEY   # the Public Key from step 1
   npx wrangler secret put GITHUB_TOKEN         # the token from step 2
   ```

   Wrangler prints the Worker's address (`https://tapir-discord-triage.<you>.workers.dev`).
5. **Tell Discord the address.** In the developer portal, *General
   Information → Interactions Endpoint URL*: paste it and save. Discord sends a
   signed test request and only saves the address if the Worker answers it
   correctly, which proves the public key is right.
6. **Register the command** (the bot's own token, the same as the
   `DISCORD_BOT_TOKEN` secret):

   ```bash
   DISCORD_BOT_TOKEN=... python register_command.py --application-id <id> --guild-id <server id>
   ```

   The command starts out visible only to people with the *Manage Server*
   permission. To open it to the allowed role, use *Server Settings →
   Integrations → the bot → /triage*. The Worker checks the roles itself in any
   case, so opening it in Discord does not let anyone else through.
7. **Invite scope.** If the bot was invited with the `bot` scope only, invite
   it once more with the `applications.commands` scope added (*OAuth2 → URL
   Generator*); otherwise Discord does not show the command.

## Tests

Offline, no Discord, GitHub or Cloudflare access needed (the workflow runs them
on every scan):

```bash
cd tools/discord-issue-bot/triage-command
node --test worker.test.js && python -m unittest test_register_command
```

They cover the signature check (valid, tampered, wrong key, stale), the
allowed-role/user logic including failing closed, the dispatch request, the
option limits and the failure messages. They do **not** run against the real
Discord, GitHub or Cloudflare; the setup above is the first time those meet.

## Changing it

If the options change, change `register_command.py`, `parseOptions` in
`worker.js` and the tests together, then register again. Renew the GitHub token
before it expires (a Worker with an expired token answers "GitHub refused the
token").
