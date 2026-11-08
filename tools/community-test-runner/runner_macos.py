"""macOS side of the community test runner: finding Archicads, Archicad's add-on list, starting Archicad,
the token store.

Archicad on macOS keeps the add-ons added in the Add-On Manager in its preferences domain
('com.graphisoft.AC-ARM 29.0.0 INT v1' in ~/Library/Preferences), under 'Add-On Manager': 'Include' and
'Disable' are the same numbered lists as in the registry on Windows (see runner_windows), with an integer
count and lan.null2:/// URLs. The runner reads and writes them with 'defaults', which goes through the
preferences daemon: a plist file edited behind its back can be overwritten by the daemon's cached copy.
"""
from __future__ import annotations

import json
import plistlib
import re
import shutil
import subprocess
import urllib.parse
import zipfile
from pathlib import Path

import runner_windows as rw  # the rules both share: which files are Tapirs, the lists' names, the token's name

GRAPHISOFT_FOLDER = "/Applications/Graphisoft"
ARCHICAD_SEARCH = GRAPHISOFT_FOLDER
PLATFORM = "Mac"  # in the build check's artifact names and the add-on's name
TOKEN_STORE = "your login keychain"
CREDENTIAL_TARGET = rw.CREDENTIAL_TARGET  # the keychain item's service name
MANAGER = "Add-On Manager"
SCHEME = "lan.null2://"  # what Archicad writes for a path on the startup disk


def state_dir() -> Path:
    folder = Path.home() / ".local" / "share" / "TapirRunner"
    folder.mkdir(parents=True, exist_ok=True)
    return folder


# --- finding Archicads ---------------------------------------------------------------------------

def executable(app: Path) -> Path | None:
    """The program inside an Archicad .app, the path its process runs as; None for any other app."""
    try:
        name = plistlib.loads((app / "Contents" / "Info.plist").read_bytes()).get("CFBundleExecutable", "")
    except (OSError, plistlib.InvalidFileException):
        return None
    exe = app / "Contents" / "MacOS" / name
    return exe if name.lower() == "archicad" and exe.is_file() else None


def find_archicads(root: str = GRAPHISOFT_FOLDER) -> list[dict]:
    """Installed Archicads, newest first: version, executable and a stock template."""
    found = []
    for folder in Path(root).glob("Archicad *"):
        match = re.fullmatch(r"Archicad (\d+)(?: .+)?", folder.name)  # "Archicad 29 INT": the language is in the name
        exe = next(filter(None, map(executable, sorted(folder.glob("*.app")))), None)
        if not match or exe is None:
            continue
        templates = sorted(folder.glob("*/Archicad/*.tpl"), key=lambda p: ("template" not in p.name.lower(), len(str(p))))
        found.append({"version": match.group(1), "archicadExe": str(exe),
                      "blankTemplate": str(templates[0]) if templates else ""})
    return sorted(found, key=lambda a: -int(a["version"]))


def app_of(exe: str | Path) -> Path:
    """The .app of an Archicad executable (…/Archicad 29.app/Contents/MacOS/Archicad)."""
    return Path(exe).parents[2]


def to_url(path: Path | str) -> str:
    return SCHEME + urllib.parse.quote(str(path), safe="/")


def from_url(url: str) -> Path:
    return Path(urllib.parse.unquote(re.sub(r"^lan\.\w+://", "", url)))


# --- the add-on list ----------------------------------------------------------------------------

class Preferences:
    """Archicad's preferences domains, through 'defaults'; tests replace it with a dict."""

    def __init__(self, folder: Path | None = None):
        self.folder = folder or Path.home() / "Library" / "Preferences"

    def domains(self) -> list[str]:
        return sorted(p.stem for p in self.folder.glob("com.graphisoft.*.plist"))

    def read(self, domain: str) -> dict:
        """The whole domain; {} when it does not exist."""
        out = subprocess.run(["defaults", "export", domain, "-"], capture_output=True, timeout=60)
        return plistlib.loads(out.stdout) if out.returncode == 0 and out.stdout else {}

    def write(self, domain: str, key: str, value: dict) -> None:
        """Replaces one top-level value of the domain."""
        subprocess.run(["defaults", "write", domain, key, plistlib.dumps(value).decode("utf-8")],
                       check=True, capture_output=True, timeout=60)


def manager_domain(prefs: Preferences, version: str) -> str | None:
    """The preferences domain with this Archicad version's Add-On Manager (the one with the longest add-on
    list, if several). Not 'com.graphisoft.startup-ARM.AC-ARM 29.0.0 INT v1', which the start-up writes."""
    names = [d for d in prefs.domains() if re.fullmatch(rf"com\.graphisoft\.AC[^ .]* {version}\..*", d)]
    managers = {d: prefs.read(d).get(MANAGER) for d in names}
    managers = {d: m for d, m in managers.items() if isinstance(m, dict)}
    return max(managers, key=lambda d: len(managers[d].get("Include", {})), default=None)


def entries(values: dict) -> list[str]:
    numbered_names = [n for n in values if re.fullmatch(r"#\d+\.", n)]
    return [values[n] for n in sorted(numbered_names, key=lambda n: int(n[1:-1]))]


def numbered(kind: str, urls: list[str]) -> dict:
    return {f"{kind} Number": len(urls), **{f"#{i}.": url for i, url in enumerate(urls, 1)}}


class AddOnList:
    """Loads one add-on in place of every Tapir in an Archicad's add-on list, and puts the list back: the
    same as runner_windows.AddOnList, with the preferences in place of the registry. The backup is a file,
    so a list left changed by a crash or a power cut is put back the next time the runner starts."""

    def __init__(self, version: str, addons_folder: Path, prefs: Preferences | None = None, folder: Path | None = None):
        self.version = str(version)
        self.addons_folder = Path(addons_folder)  # the Archicad's own Add-Ons folder, which it loads everything from
        self.prefs = prefs or Preferences()
        self.backup_file = (folder or state_dir()) / f"addon-list-backup-AC{self.version}.json"

    def manager(self) -> tuple[str, dict]:
        """The preferences domain and its Add-On Manager settings."""
        domain = manager_domain(self.prefs, self.version)
        if domain is None:
            raise RuntimeError(f"Archicad {self.version} has no Add-On Manager settings yet; start it once, then run the runner again")
        return domain, self.prefs.read(domain)[MANAGER]

    def folder_tapirs(self, disabled: set[Path]) -> list[Path]:
        """The Tapirs in the Add-Ons folder that are not switched off."""
        if not self.addons_folder.is_dir():
            return []
        return [p for p in self.addons_folder.rglob("TapirAddOn*") if rw.is_tapir(p) and p not in disabled]

    def tapirs(self) -> list[Path]:
        """Every Tapir this Archicad loads now: the add-on list, plus the Add-Ons folder."""
        _, manager = self.manager()
        disabled = {from_url(u) for u in entries(manager.get("Disable", {}))}
        listed = [from_url(u) for u in entries(manager.get("Include", {}))]
        return [p for p in listed if rw.is_tapir(p) and p not in disabled] + self.folder_tapirs(disabled)

    def swap_files(self) -> list[Path]:
        """The Tapirs to swap instead of changing the list. On macOS a Tapir in the Add-Ons folder that is
        switched off keeps the build from loading too, as up to Archicad 28 on Windows (#1150): Archicad 29
        maps the switched-off Tapir, registers no Tapir command and never loads the listed build. So every
        version swaps. A list a test left changed counts as it was before that test."""
        _, manager = self.manager()
        if self.backup_file.exists():
            manager = json.loads(self.backup_file.read_text(encoding="utf-8"))["lists"]
        return self.folder_tapirs({from_url(u) for u in entries(manager.get("Disable", {}))})

    def install(self, addon: Path) -> None:
        if self.backup_file.exists():
            raise RuntimeError(f"{self.backup_file} exists: a previous test did not finish; run 'restore' first")
        swap = ", ".join(str(p) for p in self.swap_files())
        if swap:
            raise RuntimeError(
                f"Archicad {self.version} cannot load a test build while your Tapir is in its Add-Ons folder "
                f"({swap}): switched off there, it keeps the build from loading too (#1150). Run setup again, "
                "which sets \"addonFile\" for this Archicad")
        domain, manager = self.manager()
        saved = {kind: manager[kind] for kind in rw.LISTS if kind in manager}
        self.backup_file.write_text(json.dumps({"domain": domain, "lists": saved}, indent=1), encoding="utf-8")
        include = [u for u in entries(saved.get("Include", {})) if not rw.is_tapir(from_url(u))] + [to_url(addon)]
        disable = [u for u in entries(saved.get("Disable", {})) if from_url(u) != addon]
        disable += [to_url(p) for p in self.folder_tapirs({from_url(u) for u in disable})]
        self.prefs.write(domain, MANAGER, dict(manager, Include=numbered("Include", include),
                                               Disable=numbered("Disable", disable)))

    def restore(self) -> bool:
        """Puts the add-on list back as it was before `install`; False when there was nothing to restore."""
        if not self.backup_file.exists():
            return False
        backup = json.loads(self.backup_file.read_text(encoding="utf-8"))
        manager = {k: v for k, v in self.prefs.read(backup["domain"]).get(MANAGER, {}).items() if k not in rw.LISTS}
        self.prefs.write(backup["domain"], MANAGER, dict(manager, **backup["lists"]))
        self.backup_file.unlink()
        return True


def addon_list(ac: dict) -> AddOnList:
    return AddOnList(str(ac["version"]), app_of(ac["archicadExe"]).parent / "Add-Ons")


def kept_original(addon: Path, version: str) -> Path:
    """Where the tester's own add-on waits while a test build takes its place (with 'addonFile'): outside
    the Add-Ons folder, where Archicad would load it next to the build."""
    return state_dir() / "originals" / f"AC{version}" / addon.name


# --- builds --------------------------------------------------------------------------------------

def copy_bundle(source: Path, target: Path) -> None:
    """Replaces target with a copy of the .bundle folder; ditto keeps its signature and extended attributes."""
    target.parent.mkdir(parents=True, exist_ok=True)
    if target.exists():
        shutil.rmtree(target)
    subprocess.run(["ditto", str(source), str(target)], check=True, capture_output=True, timeout=300)


def unpack_build(archive: zipfile.ZipFile, dest: Path) -> Path:
    """The .bundle from the build check's artifact, which holds a zip of it made by ditto."""
    member = next(n for n in archive.namelist() if n.endswith(".zip"))
    inner = dest / Path(member).name
    inner.write_bytes(archive.read(member))
    subprocess.run(["ditto", "-x", "-k", str(inner), str(dest)], check=True, capture_output=True, timeout=300)
    return next(dest.glob("TapirAddOn*.bundle"))


def stage_build(build: Path, version: str) -> Path:
    """Copies a build to the runner's own folder, under the name Archicad expects."""
    target = state_dir() / "builds" / f"AC{version}" / f"TapirAddOn_AC{version}_{PLATFORM}.bundle"
    copy_bundle(build, target)
    return target


# --- processes -----------------------------------------------------------------------------------

def processes() -> dict[int, str]:
    """pid -> executable path of the running processes."""
    out = subprocess.run(["ps", "-axo", "pid=,comm="], capture_output=True, text=True, timeout=60).stdout
    found = {}
    for line in out.splitlines():
        pid, _, path = line.strip().partition(" ")
        if pid.isdigit() and path:
            found[int(pid)] = path.strip()
    return found


def listening_ports(pids: set[int]) -> set[int]:
    """The TCP ports these processes listen on, from lsof."""
    if not pids:
        return set()
    out = subprocess.run(["lsof", "-nP", "-a", "-iTCP", "-sTCP:LISTEN", "-p", ",".join(str(p) for p in sorted(pids)), "-Fn"],
                         capture_output=True, text=True, timeout=60).stdout
    return {int(port) for port in re.findall(r"^n.*:(\d+)$", out, re.MULTILINE)}


def launch(exe: str, document: str) -> None:
    """Starts a new instance of this Archicad with the document; a template opens as a new, untitled project."""
    subprocess.run(["open", "-n", "-a", str(app_of(exe)), document], check=True, capture_output=True, timeout=60)


def activate(pids: set[int]) -> None:
    """Brings these processes to the front. An Archicad started in the background can wait on its splash
    screen, without an API port, until it is activated."""
    for pid in pids:
        try:  # the first time, macOS asks the tester to let the terminal control System Events
            subprocess.run(["osascript", "-e", 'tell application "System Events" to set frontmost of '
                            f"(first process whose unix id is {pid}) to true"], capture_output=True, timeout=30)
        except subprocess.TimeoutExpired:
            pass


def dialogs(pids: set[int]) -> list[str]:
    """Titles of the dialogs these processes show. Not known on macOS yet: a start that waits at one runs
    into 'startTimeout'."""
    return []


# --- token store (the login keychain) ------------------------------------------------------------

def save_token(token: str) -> None:
    """Keeps the GitHub token in the login keychain. 'security -i' reads the command from its input, so the
    token never shows in the list of processes."""
    if not re.fullmatch(r"\w+", token):
        raise ValueError("a GitHub token has only letters, digits and underscores")
    done = subprocess.run(["security", "-i"], input=f"add-generic-password -U -a github -s {CREDENTIAL_TARGET} -w {token}\n",
                          capture_output=True, text=True, timeout=60)
    if done.returncode or done.stderr.strip():
        raise OSError(f"could not save the token in the keychain: {done.stderr.strip()}")


def load_token() -> str:
    found = subprocess.run(["security", "find-generic-password", "-a", "github", "-s", CREDENTIAL_TARGET, "-w"],
                           capture_output=True, text=True, timeout=60)
    return found.stdout.strip() if found.returncode == 0 else ""


def delete_token() -> bool:
    return subprocess.run(["security", "delete-generic-password", "-a", "github", "-s", CREDENTIAL_TARGET],
                          capture_output=True, timeout=60).returncode == 0
