# Security policy

## What the parts are

| Part | Runs as | Listens |
|---|---|---|
| `TapirAddOn_AC<version>_<Win\|Mac>` — the Archicad Add-On | inside Archicad, with your rights | nothing of its own |
| Tapir Grasshopper plugin | inside Rhino/Grasshopper | nothing |
| `TapirInstaller_<Win\|Mac>` — the installer | you, elevated when it writes into the Archicad folders | nothing |
| `tools/community-test-runner`, `tools/*` — the scripts | you | nothing |

The Add-On registers commands on **Archicad's own JSON interface**, which Archicad opens on
`http://127.0.0.1:19723` (the next Archicad instance on the port after it). Tapir runs no server of
its own and adds no authentication: the port, its binding and its access rules are Graphisoft's.

## The trust model, stated plainly

Anything that can open a connection to that port can call every command, with the rights of the user
running Archicad — reading the open project and calling the commands that write files, create and
delete elements and change attributes. That is Archicad's design, not something Tapir can change from
inside the Add-On, and it is why the port must stay on loopback.

Consequences worth knowing:

- **Do not expose the port** beyond `127.0.0.1` (no port forwarding, no reverse proxy, no tunnel).
  Tapir's commands would be reachable by whoever can reach that address.
- **A local process running as you need not be trusted.** It can already do what the commands do; the
  commands add convenience, not privilege.
- **A web page reaching the interface is the interesting case.** Browsers can be coaxed to send
  requests to loopback addresses (DNS rebinding, and simple-request CSRF). If you can show a page
  driving the JSON interface — Tapir's commands or Graphisoft's — that is worth reporting to both
  Graphisoft and us.

## Reporting a vulnerability

Please report privately through GitHub's
[private vulnerability reporting](https://github.com/ENZYME-APD/tapir-archicad-automation/security/advisories/new)
(Security tab → *Report a vulnerability*). Please do not open a public issue, and please do not
include project data you would not publish.

Useful in a report:

- what the attacker gains, and what they must already be able to do to get it;
- the Archicad version, the Tapir version (`GetAddOnVersion`), and the platform;
- the command and the JSON you sent, and the answer you got;
- for the installer or the release artefacts: the exact file, its URL, and its size or digest.

The maintainers will confirm the report, work on a fix, and agree with you on when the fix and the
advisory are published. This is a volunteer project with a small team: a fix may take a while, and
reports that come with a test or a patch are much faster to act on. Credit is given in the advisory
unless you prefer otherwise.

## In scope

- The Add-On: a command that writes outside the path or the element the caller asked for, a
  `dryRun` that changes the project, a path that escapes the folder it names, a crash triggered by a
  JSON input that a caller could be induced to send, resource exhaustion that a script cannot avoid.
- The installer: writing anywhere other than the Add-Ons folders it reports, elevation it does not
  need, accepting a download it should refuse, leaving files or registry entries behind that change
  what Archicad loads.
- The release pipeline and the artefacts: anything that lets someone else's file be built, signed or
  installed as a Tapir release.
- The Grasshopper plugin, the Python client and the tools: command execution, secrets written to
  disk or logs, unsafe deserialization, path traversal in the file-writing helpers.
- Anything in `docs/` that runs on a visitor's machine.

## Out of scope

- **Archicad's JSON interface having no authentication.** That is Graphisoft's design and is
  documented above; a report that a local program can call the interface is not a vulnerability in
  Tapir. The same request sent from outside the machine, or from a web page, is interesting — see
  above.
- Bugs and crashes in Archicad, the Archicad API or the DevKits themselves. Report those to
  Graphisoft; if Tapir can avoid triggering one, tell us as well and we will guard it.
- The Windows `.apx` files not being code signed. The installers are signed (Windows through
  [SignPath](https://about.signpath.io), macOS notarized with the project's Developer ID) and the
  README's [code signing policy](README.md) is the statement of what is signed and what is not.
- Reports about the repository's automated assistant, its workflows, or issues and pull requests
  opened by the bots, unless they lead to code or artefacts being changed by someone who should not
  be able to.
- Findings that require an attacker to already have the user's account, or to be able to modify the
  machine's files as that user.

## Supported versions

The latest release, and `main` for problems that are already fixed in the next release. Older
releases are not patched; the Add-On updates itself and the installer always installs the latest
release.

Thank you for telling us rather than exploiting it.
