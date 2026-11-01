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
import sys
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
        # The defaults folder is localized ("Defaults", "Grundeinstellungen", ...); the layout below it is not.
        templates = sorted(folder.glob("*/Archicad/*.tpl"), key=lambda p: ("template" not in p.name.lower(), len(str(p))))
        found.append({"version": match.group(1), "archicadExe": str(exe),
                      "blankTemplate": str(templates[0]) if templates else ""})
    return sorted(found, key=lambda a: -int(a["version"]))


def to_url(path: Path | str) -> str:
    return "lan.flat:///" + urllib.parse.quote(str(path).replace("\\", "/"), safe="/:")


def from_url(url: str) -> Path:
    return Path(urllib.parse.unquote(url.removeprefix("lan.flat:///")))


def is_tapir(path: Path | str) -> bool:
    """A Tapir add-on file, not a backup copy of one ('TapirAddOn_AC26_Win.apx.bak')."""
    name = Path(path).name.lower()
    return name.startswith("tapiraddon") and name.endswith((".apx", ".bundle"))


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
            found += [p for p in Path(actual).rglob("TapirAddOn*") if is_tapir(p) and to_url(p) not in disabled]
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
            disable += [to_url(p) for p in Path(actual).rglob("TapirAddOn*") if is_tapir(p) and to_url(p) not in disable]
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
    desktop = _open_desktop(DESKTOP)
    if desktop:  # EnumWindows only sees the desktop this process is on
        user32.EnumDesktopWindows(desktop, visit, 0)
        user32.CloseDesktop(desktop)
    return found


# --- the runner's own desktop ------------------------------------------------------------------
# Windows shows one desktop at a time. An Archicad started on another one ("hidden": true in the
# configuration) runs normally but never takes the focus or covers the tester's work.

DESKTOP = "TapirRunner"
_DESKTOP_ACCESS = 0x01FF  # every desktop right a program needs to start, enumerate and switch
_desktop_handle = None


def _open_desktop(name: str):
    if os.name != "nt":
        return None
    ctypes.windll.user32.OpenDesktopW.restype = ctypes.c_void_p
    return ctypes.windll.user32.OpenDesktopW(name, 0, False, _DESKTOP_ACCESS)


class _StartupInfo(ctypes.Structure):
    _fields_ = [("cb", ctypes.c_ulong), ("lpReserved", ctypes.c_wchar_p), ("lpDesktop", ctypes.c_wchar_p),
                ("lpTitle", ctypes.c_wchar_p), ("dwX", ctypes.c_ulong), ("dwY", ctypes.c_ulong),
                ("dwXSize", ctypes.c_ulong), ("dwYSize", ctypes.c_ulong), ("dwXCountChars", ctypes.c_ulong),
                ("dwYCountChars", ctypes.c_ulong), ("dwFillAttribute", ctypes.c_ulong), ("dwFlags", ctypes.c_ulong),
                ("wShowWindow", ctypes.c_ushort), ("cbReserved2", ctypes.c_ushort), ("lpReserved2", ctypes.c_void_p),
                ("hStdInput", ctypes.c_void_p), ("hStdOutput", ctypes.c_void_p), ("hStdError", ctypes.c_void_p)]


class _ProcessInformation(ctypes.Structure):
    _fields_ = [("hProcess", ctypes.c_void_p), ("hThread", ctypes.c_void_p),
                ("dwProcessId", ctypes.c_ulong), ("dwThreadId", ctypes.c_ulong)]


def start_hidden(args: list[str]) -> int:
    """Starts a program on the runner's desktop; returns its process id. The desktop stays while
    this process or a program on it runs."""
    global _desktop_handle
    user32, kernel32 = ctypes.windll.user32, ctypes.windll.kernel32
    if not _desktop_handle:
        user32.CreateDesktopW.restype = ctypes.c_void_p
        _desktop_handle = user32.CreateDesktopW(DESKTOP, None, None, 0, _DESKTOP_ACCESS, None)
        if not _desktop_handle:
            raise OSError(ctypes.GetLastError(), "could not create the runner's desktop")
    info = _StartupInfo(cb=ctypes.sizeof(_StartupInfo), lpDesktop=f"WinSta0\\{DESKTOP}")
    process = _ProcessInformation()
    command = ctypes.create_unicode_buffer(" ".join(f'"{a}"' for a in args))
    if not kernel32.CreateProcessW(None, command, None, None, False, 0, None, None,
                                   ctypes.byref(info), ctypes.byref(process)):
        raise OSError(ctypes.GetLastError(), f"could not start {args[0]}")
    kernel32.CloseHandle(ctypes.c_void_p(process.hProcess))
    kernel32.CloseHandle(ctypes.c_void_p(process.hThread))
    return process.dwProcessId


def show_desktop(python: str) -> bool:
    """Switches the screen to the runner's desktop, with a window there to switch back; False when
    nothing runs there."""
    desktop = _open_desktop(DESKTOP)
    if not desktop:
        return False
    user32 = ctypes.windll.user32
    try:
        if not user32.SwitchDesktop(desktop):
            return False
        if start_back_button(python):
            return True
        default = _open_desktop("Default")  # no way back from there: do not leave the tester on it
        if default:
            user32.SwitchDesktop(default)
            user32.CloseDesktop(default)
        return False
    finally:
        user32.CloseDesktop(desktop)


def start_back_button(python: str) -> bool:
    info = _StartupInfo(cb=ctypes.sizeof(_StartupInfo), lpDesktop=f"WinSta0\\{DESKTOP}")
    process = _ProcessInformation()
    command = ctypes.create_unicode_buffer(f'"{python}" "{Path(__file__).resolve()}" back')
    if not ctypes.windll.kernel32.CreateProcessW(None, command, None, None, False, 0, None, None,
                                                 ctypes.byref(info), ctypes.byref(process)):
        return False
    ctypes.windll.kernel32.CloseHandle(ctypes.c_void_p(process.hProcess))
    ctypes.windll.kernel32.CloseHandle(ctypes.c_void_p(process.hThread))
    return True


def back_button() -> None:
    """Runs on the runner's desktop: a small window that switches back to the tester's desktop."""
    import tkinter as tk

    def back():
        default = _open_desktop("Default")
        if default:
            ctypes.windll.user32.SwitchDesktop(default)
            ctypes.windll.user32.CloseDesktop(default)
        root.destroy()
    root = tk.Tk()
    root.title("Tapir test runner")
    root.attributes("-topmost", True)
    tk.Label(root, text="This is the test runner's desktop. Answer Archicad's dialog here,\n"
                        "then go back to your own desktop.", padx=16, pady=10).pack()
    tk.Button(root, text="Back to my desktop", command=back, padx=12, pady=6).pack(pady=(0, 12))
    root.protocol("WM_DELETE_WINDOW", back)  # closing the window must not leave the tester here
    root.mainloop()



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


if __name__ == "__main__" and sys.argv[1:] == ["back"]:
    back_button()
