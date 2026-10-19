"""Windows side of the community test runner: finding Archicads, Archicad's add-on list, the token store.

Archicad keeps the add-ons added in the Add-On Manager in the registry, under
HKCU\\Software\\GRAPHISOFT\\Archicad\\Archicad <version> <language> <release>\\Add-On Manager:
'Include' lists the add-ons loaded from anywhere, 'Disable' the ones switched off, each as
'#1.', '#2.', ... plus an '<Include|Disable> Number' count, with lan.flat:/// URLs. The runner uses
this to load a pull request's build next to (and instead of) the tester's own Tapir without
touching any file in Program Files, and writes both lists back exactly as they were afterwards.
"""
from __future__ import annotations

import ctypes
import json
import os
import re
import shutil
import urllib.parse
from pathlib import Path

try:
    import winreg
except ImportError:  # not Windows: the module still imports, so the tests and the rest of the runner run anywhere
    winreg = None  # type: ignore[assignment]

ARCHICAD_KEY = r"Software\GRAPHISOFT\Archicad"
LISTS = ("Include", "Disable")
# Up to this version Archicad loads no add-on from 'Include' while the same add-on is on 'Disable'
# at another path: it says some add-ons could not be loaded and loads neither (#1150).
COLLIDES_UP_TO = 28
CREDENTIAL_TARGET = "tapir-community-test-runner"


def state_dir() -> Path:
    base = os.environ.get("LOCALAPPDATA") or str(Path.home() / ".local" / "share")
    folder = Path(base) / "TapirRunner"
    folder.mkdir(parents=True, exist_ok=True)
    return folder


# --- finding Archicads ---------------------------------------------------------------------------

def find_archicads(root: str = r"C:\Program Files\Graphisoft") -> list[dict]:
    """Installed Archicads, newest first: version, executable and a stock template."""
    found = []
    for folder in Path(root).glob("Archicad *"):
        match = re.fullmatch(r"Archicad (\d+)", folder.name)
        exe = folder / "Archicad.exe"
        if not match or not exe.exists():
            continue
        templates = sorted(folder.glob("Defaults/**/*.tpl"), key=lambda p: ("template" not in p.name.lower(), len(str(p))))
        found.append({"version": match.group(1), "archicadExe": str(exe),
                      "blankTemplate": str(templates[0]) if templates else ""})
    return sorted(found, key=lambda a: -int(a["version"]))


def to_url(path: Path | str) -> str:
    return "lan.flat:///" + urllib.parse.quote(str(path).replace("\\", "/"), safe="/:")


def from_url(url: str) -> Path:
    return Path(urllib.parse.unquote(url.removeprefix("lan.flat:///")))


def is_tapir(path: Path | str) -> bool:
    return Path(path).name.lower().startswith("tapiraddon")


# --- the add-on list ----------------------------------------------------------------------------

class Registry:
    """The few registry operations the add-on list needs; tests replace it with a dict."""

    def subkeys(self, path: str) -> list[str]:
        try:
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, path) as key:
                return [winreg.EnumKey(key, i) for i in range(winreg.QueryInfoKey(key)[0])]
        except OSError:
            return []

    def read(self, path: str) -> dict[str, list]:
        """All values of a key as {name: [value, kind]}; {} when the key is missing."""
        try:
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, path) as key:
                values = {}
                for i in range(winreg.QueryInfoKey(key)[1]):
                    name, value, kind = winreg.EnumValue(key, i)
                    values[name] = [value, kind]
                return values
        except OSError:
            return {}

    def write(self, path: str, values: dict[str, list]) -> None:
        """Replaces every value of the key with exactly these."""
        with winreg.CreateKeyEx(winreg.HKEY_CURRENT_USER, path, 0, winreg.KEY_ALL_ACCESS) as key:
            for name in list(self.read(path)):
                winreg.DeleteValue(key, name)
            for name, (value, kind) in values.items():
                winreg.SetValueEx(key, name, 0, kind, value)


REG_SZ = 1  # winreg.REG_SZ, also when winreg is missing


def manager_key(registry: Registry, version: str) -> str | None:
    """The Add-On Manager key of this Archicad version (the one with the longest add-on list, if several)."""
    names = [n for n in registry.subkeys(ARCHICAD_KEY) if n.startswith(f"Archicad {version}.")]
    keys = [f"{ARCHICAD_KEY}\\{n}\\Add-On Manager" for n in names]
    keys = [k for k in keys if registry.read(k + "\\Include") or registry.read(k)]
    return max(keys, key=lambda k: len(registry.read(k + "\\Include")), default=None)


def entries(values: dict[str, list]) -> list[str]:
    return [v for n, (v, _) in sorted(values.items(), key=lambda kv: _number(kv[0])) if n.startswith("#")]


def _number(name: str) -> int:
    match = re.match(r"#(\d+)\.", name)
    return int(match.group(1)) if match else 0


def numbered(kind: str, urls: list[str]) -> dict[str, list]:
    values = {f"{kind} Number": [str(len(urls)), REG_SZ]}
    values.update({f"#{i}.": [url, REG_SZ] for i, url in enumerate(urls, 1)})
    return values


class AddOnList:
    """Loads one add-on in place of every Tapir in an Archicad's add-on list, and puts the list back.

    The backup is a file, so a list left changed by a crash or a power cut is put back the next time
    the runner starts (`restore_pending`)."""

    def __init__(self, version: str, registry: Registry | None = None, folder: Path | None = None):
        self.version = str(version)
        self.registry = registry or Registry()
        self.backup_file = (folder or state_dir()) / f"addon-list-backup-AC{self.version}.json"

    def key(self) -> str:
        key = manager_key(self.registry, self.version)
        if key is None:
            raise RuntimeError(f"Archicad {self.version} has no Add-On Manager settings yet; start it once, then run the runner again")
        return key

    def tapirs(self) -> list[Path]:
        """Every Tapir this Archicad loads now: the add-on list, plus the Add-Ons folder it scans."""
        key = self.key()
        include = self.registry.read(key + "\\Include")
        disabled = set(entries(self.registry.read(key + "\\Disable")))
        found = [from_url(u) for u in entries(include) if is_tapir(from_url(u)) and u not in disabled]
        actual = self.registry.read(key).get("Actual Path", [""])[0]
        if actual and Path(actual).is_dir():
            found += [p for p in Path(actual).rglob("TapirAddOn*") if to_url(p) not in disabled]
        return found

    def swap_files(self) -> list[Path]:
        """The Tapirs to swap instead of changing the list. Up to Archicad 28 a Tapir in the Add-Ons
        folder can only be left out by putting it on 'Disable', and then the build does not load
        either (#1150). A list a test left changed counts as it was before that test."""
        if int(self.version) > COLLIDES_UP_TO:
            return []
        key = self.key()
        if self.backup_file.exists():
            disable = json.loads(self.backup_file.read_text(encoding="utf-8"))["lists"]["Disable"]
        else:
            disable = self.registry.read(key + "\\Disable")
        disabled = set(entries(disable))
        actual = self.registry.read(key).get("Actual Path", [""])[0]
        if not actual or not Path(actual).is_dir():
            return []
        return [p for p in Path(actual).rglob("TapirAddOn*")
                if p.suffix.lower() in (".apx", ".bundle") and to_url(p) not in disabled]

    def install(self, addon: Path) -> None:
        if self.backup_file.exists():
            raise RuntimeError(f"{self.backup_file} exists: a previous test did not finish; run 'restore' first")
        swap = ", ".join(str(p) for p in self.swap_files())
        if swap:
            raise RuntimeError(
                f"Archicad {self.version} cannot load a test build while your Tapir is in its Add-Ons folder "
                f"({swap}): switched off there, it keeps the build from loading too (#1150). Run setup again, "
                "which sets \"addonFile\" for this Archicad, then run the runner as administrator")
        key = self.key()
        saved = {kind: self.registry.read(f"{key}\\{kind}") for kind in LISTS}
        self.backup_file.write_text(json.dumps({"key": key, "lists": saved}, indent=1), encoding="utf-8")
        include = [u for u in entries(saved["Include"]) if not is_tapir(from_url(u))] + [to_url(addon)]
        disable = [u for u in entries(saved["Disable"]) if from_url(u) != addon]
        actual = self.registry.read(key).get("Actual Path", [""])[0]
        if actual and Path(actual).is_dir():
            disable += [to_url(p) for p in Path(actual).rglob("TapirAddOn*") if to_url(p) not in disable]
        self.registry.write(f"{key}\\Include", numbered("Include", include))
        self.registry.write(f"{key}\\Disable", numbered("Disable", disable))

    def restore(self) -> bool:
        """Puts the add-on list back as it was before `install`; False when there was nothing to restore."""
        if not self.backup_file.exists():
            return False
        backup = json.loads(self.backup_file.read_text(encoding="utf-8"))
        for kind in LISTS:
            self.registry.write(f"{backup['key']}\\{kind}", {n: list(v) for n, v in backup["lists"][kind].items()})
        self.backup_file.unlink()
        return True


def dialogs(pids: set[int]) -> list[str]:
    """Titles of the modal dialogs these processes show: Archicad's own (DGModalDlgClass) and the
    system's (#32770). Found by window class, not title, so it works in every language."""
    if os.name != "nt" or not pids:
        return []
    from ctypes import wintypes
    user32 = ctypes.windll.user32
    found = []

    @ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    def visit(hwnd, _):
        pid = wintypes.DWORD()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
        if pid.value in pids and user32.IsWindowVisible(hwnd):
            kind = ctypes.create_unicode_buffer(256)
            user32.GetClassNameW(hwnd, kind, 256)
            if kind.value in ("DGModalDlgClass", "#32770"):
                title = ctypes.create_unicode_buffer(512)
                user32.GetWindowTextW(hwnd, title, 512)
                found.append(title.value or kind.value)
        return True
    user32.EnumWindows(visit, 0)
    return found


def stage_build(build: Path, version: str) -> Path:
    """Copies a build to the runner's own folder, under the name Archicad expects."""
    folder = state_dir() / "builds" / f"AC{version}"
    folder.mkdir(parents=True, exist_ok=True)
    target = folder / f"TapirAddOn_AC{version}_Win.apx"
    shutil.copy2(build, target)
    return target


# --- token store (Windows Credential Manager) ------------------------------------------------

if os.name == "nt":
    from ctypes import wintypes

    class _CREDENTIAL(ctypes.Structure):
        _fields_ = [("Flags", wintypes.DWORD), ("Type", wintypes.DWORD), ("TargetName", wintypes.LPWSTR),
                    ("Comment", wintypes.LPWSTR), ("LastWritten", wintypes.FILETIME),
                    ("CredentialBlobSize", wintypes.DWORD), ("CredentialBlob", ctypes.POINTER(ctypes.c_char)),
                    ("Persist", wintypes.DWORD), ("AttributeCount", wintypes.DWORD), ("Attributes", ctypes.c_void_p),
                    ("TargetAlias", wintypes.LPWSTR), ("UserName", wintypes.LPWSTR)]

    _advapi = ctypes.WinDLL("advapi32", use_last_error=True)
    _advapi.CredReadW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD, ctypes.POINTER(ctypes.POINTER(_CREDENTIAL))]
    _advapi.CredWriteW.argtypes = [ctypes.POINTER(_CREDENTIAL), wintypes.DWORD]
    _advapi.CredDeleteW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD]
    _advapi.CredFree.argtypes = [ctypes.c_void_p]


def save_token(token: str) -> None:
    """Keeps the GitHub token in Windows Credential Manager, readable only by this Windows user."""
    blob = token.encode("utf-8")
    cred = _CREDENTIAL(Type=1, TargetName=CREDENTIAL_TARGET, CredentialBlobSize=len(blob),
                       CredentialBlob=ctypes.cast(ctypes.create_string_buffer(blob, len(blob)), ctypes.POINTER(ctypes.c_char)),
                       Persist=2, UserName="github")
    if not _advapi.CredWriteW(ctypes.byref(cred), 0):
        raise OSError(ctypes.get_last_error(), "could not save the token in Credential Manager")


def load_token() -> str:
    if os.name != "nt":
        return ""
    pcred = ctypes.POINTER(_CREDENTIAL)()
    if not _advapi.CredReadW(CREDENTIAL_TARGET, 1, 0, ctypes.byref(pcred)):
        return ""
    try:
        return ctypes.string_at(pcred.contents.CredentialBlob, pcred.contents.CredentialBlobSize).decode("utf-8")
    finally:
        _advapi.CredFree(pcred)


def delete_token() -> bool:
    return os.name == "nt" and bool(_advapi.CredDeleteW(CREDENTIAL_TARGET, 1, 0))
