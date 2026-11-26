## Script marketplace

Scripts shared by users live in [marketplace/](../marketplace/), one `<slug>/` folder each
(`listing.json`, the script, `README.md`, optional `screenshots/`). They are never added to
`builtin-scripts/`. [marketplace/README.md](../marketplace/README.md) is the format (with
[listing.schema.json](../marketplace/listing.schema.json)), the `# /// tapir` header and its
`changes` key, and the rules for adapting a shared script for public use. An issue labelled
`script-share` becomes one pull request adding its folder, adapted by those rules, keeping what
the script does, listing every change in plain words and asking the author to confirm it.
A new script *requested* in an issue also goes to `marketplace/<slug>/` (listing `author` the
person who opened the issue: `name` their GitHub profile name, else their login, and `github` their
login; README "Requested by @<login> in #<n>, written with the Tapir issue bot."; an issue a bot
opened keeps `author` `Tapir` and the README names the Discord user), always in one pull request with
the whole script, never split into parts, because it is tested in Archicad as a whole; `builtin-scripts/` gets a new script only when a maintainer
asks for it in the issue. A marketplace script may import `utilities`/`aclib` of `builtin-scripts/`,
which are copied next to it when it is installed, so keep those helpers backward compatible.
`python tools/check_marketplace.py` checks every folder against marketplace/README.md (the add-on
build check runs it on every pull request); `tools/generate_catalogue.py` lists the folders on
`main` that pass in `catalogue.json` (`site_data.yml`, `site-data` branch), which the website's
Scripts page reads, so a merged script is listed within minutes and a wrong listing is left out.
