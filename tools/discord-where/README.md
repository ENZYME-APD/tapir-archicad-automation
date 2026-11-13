# /where: put your town on the Tapir map

`/where place: Maribor, Slovenia` in the Tapir Discord answers in the channel
("📍 @someone is in **Maribor**, Slovenia…"), and the town appears on the Community page's map
(<https://archicad-api.com/community/#user-map>) after the nightly Site Data run.
`/where place: remove` takes the person off the map. A person's latest `/where` wins over anything
else they said.

How it fits together:

- `api/interactions.js` is a Vercel function (no dependencies) that Discord calls for each use of
  the command. It stores nothing: its public answer is the record.
- `tools/generate_user_map.py` (Site Data job) reads those answers every night, by the command's
  name and the person who used it, and places the town in bold.
- `.github/workflows/discord_commands.yml`, run by hand, registers the command with Discord.

Setup, once:

1. Deploy this folder to Vercel (project `tapir-discord-where`, root directory
   `tools/discord-where`), with the environment variable `DISCORD_PUBLIC_KEY` = the app's
   **Public Key** (Discord Developer Portal > the app > General Information).
2. In the same page, set **Interactions Endpoint URL** to
   `https://<the deployment's domain>/api/interactions` and save (Discord checks it at once).
3. Run the **Discord Commands** workflow (Actions tab). Its log names the app; it must be the app
   of step 1 and 2.
