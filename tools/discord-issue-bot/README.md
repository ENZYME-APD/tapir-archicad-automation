# Tapir Discord issue bot

Watches the project's Discord channels for messages that read like a **bug
report** or a **feature request**, files each one as a GitHub issue in this
repository, and replies to the Discord message with the number and link of
the created issue.

It runs as a scheduled GitHub Actions workflow
([`.github/workflows/discord_issue_bot.yml`](../../.github/workflows/discord_issue_bot.yml)),
so no server hosting is needed. Every run:

1. Fetches the recent messages (default: last 3 hours) of each configured
   channel over the Discord REST API.
2. Asks the Claude API whether a message is an actionable Tapir bug report or
   feature request. Casual chat, questions and help requests are ignored.
3. Creates a GitHub issue labelled `bug` or `enhancement` plus `discord`,
   containing a summary, the quoted original message and a link back to it.
4. Adds a ✅ reaction to the Discord message (this is how the bot remembers it
   has already handled a message) and replies to it with the issue number.
   Messages classified as *not* actionable get a 👀 reaction instead, so
   later runs skip them rather than paying to classify them again.

Duplicates are prevented twice over: a message carrying the bot's own ✅ or
👀 reaction is skipped, and before creating an issue the bot searches
existing issues for the Discord message ID (which is embedded in every
issue body).

## One-time setup

### 1. Create the Discord bot

1. In the [Discord developer portal](https://discord.com/developers/applications),
   create an application and add a **Bot** to it.
2. Under *Bot*, enable the **Message Content Intent** (without it the API
   returns messages with empty text).
3. Copy the **bot token**.
4. Invite the bot to the server via *OAuth2 → URL Generator*: scope `bot`,
   permissions **View Channels**, **Read Message History**, **Send Messages**
   and **Add Reactions**.
5. Collect the IDs of the channels to watch: enable *Developer Mode* in your
   Discord settings, then right-click a channel → *Copy Channel ID*.

### 2. Configure the repository

Under *Settings → Secrets and variables → Actions* add:

| Kind     | Name                  | Value                                        |
| -------- | --------------------- | -------------------------------------------- |
| Secret   | `DISCORD_BOT_TOKEN`   | the bot token from step 1                    |
| Secret   | `ANTHROPIC_API_KEY`   | a Claude API key (used to classify messages) |
| Variable | `DISCORD_CHANNEL_IDS` | comma-separated channel IDs to watch         |

The workflow uses the built-in `GITHUB_TOKEN` to create issues; no extra
GitHub credential is needed. If any of the three values above is missing the
scheduled run exits without doing anything, so enabling the workflow before
finishing the setup is harmless.

### 3. Try it

Run the workflow by hand from the *Actions* tab (*Discord Issue Bot → Run
workflow*) with **dry run** checked: the log shows how each recent message
was classified, without creating issues or posting to Discord. When the
result looks right, run it again without dry run, or just wait for the
schedule.

## Tuning

All knobs are environment variables of the script
([`discord_issue_bot.py`](discord_issue_bot.py)); the workflow sets the
important ones and the rest fall back to defaults:

| Variable             | Default         | Meaning                                          |
| -------------------- | --------------- | ------------------------------------------------ |
| `CLAUDE_MODEL`       | `claude-opus-5` | model used to classify messages                  |
| `LOOKBACK_MINUTES`   | `180`           | how far back each run scans (keep it much larger than the schedule interval; scheduled runs can be delayed) |
| `MAX_ISSUES_PER_RUN` | `5`             | safety cap on issues created per run             |
| `MIN_CONFIDENCE`     | `0.7`           | classifier confidence required to file an issue  |
| `MIN_MESSAGE_LENGTH` | `25`            | messages shorter than this are never considered  |
| `DRY_RUN`            | `false`         | classify and log only, no issues/replies         |

Running it locally works too:

```bash
pip install -r tools/discord-issue-bot/requirements.txt
export DISCORD_BOT_TOKEN=... DISCORD_CHANNEL_IDS=... \
       GITHUB_TOKEN=... GITHUB_REPOSITORY=ENZYME-APD/tapir-archicad-automation \
       ANTHROPIC_API_KEY=... DRY_RUN=true
python tools/discord-issue-bot/discord_issue_bot.py
```
