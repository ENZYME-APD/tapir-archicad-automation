"""'setup': the one-time setup of the community test runner, as a few questions.

Finds the installed Archicads and their stock templates, stores the GitHub token in Windows
Credential Manager, writes the configuration, and optionally starts the runner's window at logon
and installs the Claude Code skill. Nothing in Program Files is changed: tests load the pull
request's build through Archicad's add-on list (see runner_windows.AddOnList).
"""
from __future__ import annotations

import getpass
import json
import os
import shutil
import subprocess
import sys
import webbrowser
from pathlib import Path

import runner_windows as rw
import tapir_test_runner as r

TOKEN_URL = "https://github.com/settings/tokens/new?scopes=public_repo&description=Tapir%20community%20test%20runner"
SKILL = Path(__file__).resolve().parent / "claude-skill" / "tapir-pr-test" / "SKILL.md"


def ask(question: str, default: str, yes: bool) -> str:
    if yes:
        return default
    answer = input(f"{question} [{default}]: ").strip()
    return answer or default


def ask_yes(question: str, default: bool, yes: bool) -> bool:
    return ask(question + (" (Y/n)" if default else " (y/N)"), "y" if default else "n", yes).lower().startswith("y")


def describe(ac: dict) -> str:
    try:
        tapirs = rw.AddOnList(ac["version"]).tapirs()
        loaded = ", ".join(str(p) for p in tapirs) or "no Tapir loaded yet (that is fine)"
    except RuntimeError as exc:
        loaded = str(exc)
    template = ac["blankTemplate"] or "NO TEMPLATE FOUND"
    return f"  Archicad {ac['version']}: {ac['archicadExe']}\n    template: {template}\n    your Tapir: {loaded}"


def token_step(cfg: dict, yes: bool) -> str:
    token = r.get_token(cfg)
    if token:
        try:
            login = r.GitHub(cfg["repo"], token).login()
            print(f"GitHub: signed in as {login} (stored token)")
            if not ask_yes("Keep this token?", True, yes):
                token = ""
            else:
                return login
        except Exception as exc:  # noqa: BLE001 - any failure means the token must be replaced
            print(f"GitHub: the stored token does not work ({exc})")
            token = ""
    if yes:
        print(f"GitHub: no token; set {cfg['tokenEnv']} or run setup without --yes")
        return ""
    print("\nThe runner needs a GitHub token to download builds and post results in your name.")
    print("Create a classic token with only the 'public_repo' scope:\n  " + TOKEN_URL)
    if ask_yes("Open that page in your browser?", True, yes):
        webbrowser.open(TOKEN_URL)
    while True:
        token = getpass.getpass("Paste the token (it is not shown; Enter to skip): ").strip()
        if not token:
            return ""
        try:
            login = r.GitHub(cfg["repo"], token).login()
        except Exception as exc:  # noqa: BLE001
            print(f"  that token does not work ({exc}); try again")
            continue
        rw.save_token(token)
        print(f"  signed in as {login}; the token is kept in Windows Credential Manager ('{rw.CREDENTIAL_TARGET}')")
        return login


def logon_task(script: Path, config: Path) -> None:
    pythonw = Path(sys.executable).with_name("pythonw.exe")
    command = f'"{pythonw if pythonw.exists() else sys.executable}" "{script}" gui --config "{config}"'
    subprocess.run(["schtasks", "/create", "/f", "/tn", "TapirTestRunner", "/sc", "onlogon", "/tr", command], check=True)


def install_skill() -> Path:
    target = Path.home() / ".claude" / "skills" / "tapir-pr-test"
    target.mkdir(parents=True, exist_ok=True)
    shutil.copy2(SKILL, target / "SKILL.md")
    return target


def run(config_path: Path | str, yes: bool, claude_skill: bool) -> int:
    if os.name != "nt":
        sys.exit("setup is Windows only for now; on macOS write runner.json by hand (see README.md)")
    config_path = Path(config_path)
    cfg = dict(r.DEFAULTS)
    if config_path.exists():
        cfg.update(json.loads(config_path.read_text(encoding="utf-8")))
        print(f"Updating {config_path}")

    found = rw.find_archicads()
    if not found:
        sys.exit("no Archicad found under C:\\Program Files\\Graphisoft")
    print("Archicads on this computer:")
    for ac in found:
        print(describe(ac))
    usable = [a for a in found if a["blankTemplate"]]
    chosen = ask("Which versions should the runner test? (comma-separated)",
                 ",".join(a["version"] for a in usable), yes).replace(" ", "").split(",")
    previous = {str(a["version"]): a for a in cfg.get("archicads", [])}
    archicads = []
    for ac in usable:
        if ac["version"] in chosen:
            entry = dict(ac, startTimeout=300, quitTimeout=180)
            entry.update({k: v for k, v in previous.get(ac["version"], {}).items() if k not in ("archicadExe",)})
            try:
                swap = [] if entry.get("addonFile") else rw.AddOnList(ac["version"]).swap_files()
            except RuntimeError:
                swap = []
            if len(swap) == 1:
                entry["addonFile"] = str(swap[0])
                print(f"  Archicad {ac['version']}: your Tapir is in its Add-Ons folder, which Archicad up to 28 does not "
                      "allow next to a test build in the add-on list (#1150). The runner swaps that file instead "
                      "and needs administrator rights for it.")
            elif swap:
                print(f"  Archicad {ac['version']}: there are {len(swap)} Tapirs in its Add-Ons folder "
                      f"({', '.join(str(p) for p in swap)}); keep one and run setup again, or the others load next "
                      "to every test build.")
            archicads.append(entry)
    hidden = ask_yes("Start Archicad on a desktop of its own while testing, so it never takes the focus "
                     "or covers your work?", all(a.get("hidden", True) for a in archicads), yes)
    for entry in archicads:
        entry["hidden"] = hidden
    cfg["archicads"] = archicads

    login = token_step(cfg, yes)
    if login and not yes:
        forks = ask("Also test pull requests from these forks (GitHub names, comma-separated; your own is suggested)",
                    ",".join(cfg.get("trustedForks") or [login]), yes)
        cfg["trustedForks"] = [f for f in forks.replace(" ", "").split(",") if f]

    config_path.parent.mkdir(parents=True, exist_ok=True)
    config_path.write_text(json.dumps({k: v for k, v in cfg.items() if not k.startswith("_")}, indent=2), encoding="utf-8")
    print(f"\nSaved {config_path}")

    script = Path(r.__file__).resolve()
    if ask_yes("Open the runner's window every time you log on to Windows?", False, yes):
        logon_task(script, config_path)
        print("  added the 'TapirTestRunner' task (remove it with: schtasks /delete /tn TapirTestRunner)")
    if claude_skill or ask_yes("Install the Claude Code skill, so Claude can test pull requests with it?", False, yes):
        print(f"  installed {install_skill()}")

    print("\nDone. Next:")
    print(f"  window:            python \"{script}\" gui")
    print(f"  test what is due:  python \"{script}\" test --brief")
    print("Archicad must be closed while the runner tests; your own Tapir is put back after every test.")
    return 0
