# Tapir Discord issue bot

Watches the project's Discord channels for messages that read like a **bug
report** or a **feature request**, files each one as a GitHub issue in this
repository, and replies to the Discord message with the number and link of
the created issue.

It runs as a scheduled GitHub Actions workflow
([`.github/workflows/discord_issue_bot.yml`](../../.github/workflows/discord_issue_bot.yml)),
so no server hosting is needed. Every run:

1. Fetches the recent messages (default: last 24 hours) of each configured
   channel over the Discord REST API.
2. Asks Claude whether each message is an actionable Tapir bug report or
   feature request. Casual chat, questions and help requests are ignored.
   Classification runs through the Claude Code CLI on a Pro/Max
   subscription, authorized by `CLAUDE_CODE_OAUTH_TOKEN` — the same
   credential the repository's other Claude workflows use. No Anthropic
   API key is needed.
3. Creates a GitHub issue labelled `bug` or `enhancement` plus `discord`,
   containing a summary, the quoted original message and a link back to it.
4. Adds a ✅ reaction to the Discord message (this is how the bot remembers it
   has already handled a message) and replies to it with the issue number.
   Messages classified as *not* actionable get a 👀 reaction instead, so
   later runs skip them rather than paying to classify them again.
   Actionable-looking messages that fall just short of the confidence
   threshold also get the 👀 reaction, but are recorded on a rolling
   **Borderline Discord reports** issue (and in the workflow run's
   summary) so a maintainer can file them by hand if the classifier was
   too cautious. Closing that issue archives it; the bot opens a fresh
   one when needed.

Duplicates are prevented twice over: a message carrying the bot's own ✅ or
👀 reaction is skipped, and before creating an issue the bot searches
existing issues for the Discord message ID (which is embedded in every
issue body).

Only regular channel messages are scanned — threads and forum posts are
not read, so reports posted there will not become issues.

Messages older than the lookback window are never revisited. Note that
GitHub disables scheduled workflows after 60 days without repository
activity; if the schedule was ever paused for longer than a day,
re-enable it and run the workflow once manually with the *lookback
minutes* input raised to cover the gap.

Because the issues are created with the workflow's own `GITHUB_TOKEN`,
they do not trigger other workflows: the automatic Claude issue triage
never runs on them (the reporter is on Discord and would not see its
answer anyway). A maintainer can still apply the `claude-fix` label to
have a fix attempted, exactly as for hand-written issues.

## One-time setup

### 1. Create the Discord bot

1. In the [Discord developer portal](https://discord.com/developers/applications),
   create an application and add a **Bot** to it.
2. Under *Bot*, enable the **Message Content Intent** (without it the API
   returns messages with empty text; the bot fails the run when every
   recent human message comes back empty, so the mistake cannot stay
   silent).
3. Copy the **bot token**.
4. Invite the bot to the server via *OAuth2 → URL Generator*: scope `bot`,
   permissions **View Channels**, **Read Message History**, **Send Messages**
   and **Add Reactions**.
5. Collect the IDs of the channels to watch: enable *Developer Mode* in your
   Discord settings, then right-click a channel → *Copy Channel ID*.

### 2. Configure the repository

Under *Settings → Secrets and variables → Actions* add:

| Kind     | Name                      | Value                                    |
| -------- | ------------------------- | ---------------------------------------- |
| Secret   | `DISCORD_BOT_TOKEN`       | the bot token from step 1                |
| Secret   | `CLAUDE_CODE_OAUTH_TOKEN` | Claude Code OAuth token (run `claude setup-token` locally) — classification then draws on the Pro/Max subscription, like the repo's other Claude workflows |
| Variable | `DISCORD_CHANNEL_IDS`     | comma-separated channel IDs to watch     |

Also create a **`discord` label** in the repository (*Issues → Labels*):
the bot tags every issue it files with it, next to `bug` or `enhancement`.
A missing label is dropped from the issue rather than failing the run,
so this step is recommended, not required.

The workflow uses the built-in `GITHUB_TOKEN` to create issues; no extra
GitHub credential is needed. While none of the values above are configured,
the scheduled run exits without doing anything, so enabling the workflow
before finishing the setup is harmless (a *partially* configured bot fails
loudly instead, so a renamed secret cannot go unnoticed).

### 3. Try it

Run the workflow by hand from the *Actions* tab (*Discord Issue Bot → Run
workflow*) with **dry run** checked: the log shows how each recent message
was classified, without creating issues or posting to Discord. When the
result looks right, run it again without dry run, or just wait for the
schedule.

After the first real issue appears, verify the duplicate backstop once:
search the repository's issues for `discord-message-id:` and confirm the
new issue is found. The ✅ reaction is the primary duplicate guard; the
search is the fallback for a lost reaction, and it relies on GitHub
indexing the marker line in the issue body.

## Tuning

All knobs are environment variables of the script
([`discord_issue_bot.py`](discord_issue_bot.py)); the workflow sets the
important ones and the rest fall back to defaults:

| Variable             | Default         | Meaning                                          |
| -------------------- | --------------- | ------------------------------------------------ |
| `CLAUDE_MODEL`       | (CLI default)   | model used to classify messages; defaults to the Claude Code CLI's own default model |
| `LOOKBACK_MINUTES`   | `1440`          | how far back each run scans; the wide overlap is nearly free because marked messages are skipped without a model call |
| `MAX_ISSUES_PER_RUN` | `5`             | safety cap on issues created per run             |
| `MIN_CONFIDENCE`     | `0.7`           | classifier confidence required to file an issue  |
| `MIN_MESSAGE_LENGTH` | `25`            | messages shorter than this are never considered  |
| `DRY_RUN`            | `false`         | classify and log only, no issues/replies         |

Running it locally works too:

```bash
pip install -r tools/discord-issue-bot/requirements.txt
export DISCORD_BOT_TOKEN=... DISCORD_CHANNEL_IDS=... \
       GITHUB_TOKEN=... GITHUB_REPOSITORY=ENZYME-APD/tapir-archicad-automation \
       CLAUDE_CODE_OAUTH_TOKEN=... DRY_RUN=true
python tools/discord-issue-bot/discord_issue_bot.py
```
