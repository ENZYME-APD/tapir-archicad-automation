#!/usr/bin/env python3
"""Community test runner for Tapir pull requests.

Picks up open pull requests labelled for testing, installs their CI build into a local Archicad,
runs the pull request's checks against an empty project, restores the previous add-on and posts
the result as a pull request comment. Standard library only; no Claude or other model is needed.

    python tapir_test_runner.py setup                   # once: finds your Archicads, stores the token, writes the config
    python tapir_test_runner.py gui                     # a window: pending pull requests, Test, Pause, results
    python tapir_test_runner.py test --all --brief      # test every testable pull request, one line each
    python tapir_test_runner.py prs                     # what is open and what can be tested, one line each
    python tapir_test_runner.py check checks/*.json     # run checks against an Archicad that is already open
    python tapir_test_runner.py record --out my.json    # write a check from the commands you send through it
    python tapir_test_runner.py once / poll             # one pass / a pass every pollMinutes over labelled pull requests

Checks are JSON data, not code: a pull request can only make the runner send Tapir commands and
compare their answers, never run anything else on the tester's machine. See README.md.
"""
from __future__ import annotations

import argparse
import ast
import base64
import glob
import io
import json
import operator
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request
import zipfile
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path
from typing import Any, Callable

sys.path.insert(0, str(Path(__file__).resolve().parent))
import runner_windows as rw  # noqa: E402

RESULT_MARKER = "tapir-test:"
DEFAULT_TIMEOUT = 60.0
PORTS = range(19723, 19744)  # Archicad's JSON server takes the first free one, in start order
REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULTS = {"repo": "ENZYME-APD/tapir-archicad-automation", "label": "needs-test",
            "checkFolder": "archicad-addon/Test/checks/", "tokenEnv": "TAPIR_RUNNER_TOKEN",
            "pollMinutes": 15, "allowForks": False, "trustedForks": [], "archicads": []}


# --- Tapir connection ---------------------------------------------------------------------

class TapirError(Exception):
    pass


class Tapir:
    """Sends one command to the Archicad JSON server; 'API.X' is official, anything else Tapir."""

    def __init__(self, port: int, host: str = "127.0.0.1"):
        self.url = f"http://{host}:{port}"

    def __call__(self, command: str, params: dict | None = None, timeout: float = DEFAULT_TIMEOUT) -> dict:
        if command.startswith("API."):
            body = {"command": command, "parameters": params or {}}
        else:
            body = {"command": "API.ExecuteAddOnCommand", "parameters": {
                "addOnCommandId": {"commandNamespace": "TapirCommand", "commandName": command},
                "addOnCommandParameters": params or {}}}
        request = urllib.request.Request(self.url, data=json.dumps(body).encode("utf-8"),
                                         headers={"Content-Type": "application/json"}, method="POST")
        try:
            with urllib.request.urlopen(request, timeout=timeout) as response:
                answer = json.loads(response.read().decode("utf-8"))
        except (urllib.error.URLError, OSError) as exc:
            raise TapirError(f"no answer from {self.url} for {command}: {exc}") from exc
        if not answer.get("succeeded"):
            return {"__envelopeError__": answer.get("error") or {}}
        result = answer.get("result") or {}
        return result if command.startswith("API.") else result.get("addOnCommandResponse", {})

    def alive(self) -> bool:
        try:
            return "version" in self("GetAddOnVersion", timeout=3)
        except TapirError:
            return False


# --- check engine ----------------------------------------------------------------------------

_PATH_TOKEN = re.compile(r"([^.\[\]]+)|\[(\d+|\*)\]")


def resolve(data: Any, path: str) -> Any:
    """'a.b[0].c' or 'elements[*].elementId.guid' (a list); raises KeyError when a part is missing."""
    values = [data]
    for name, index in _PATH_TOKEN.findall(path):
        nxt = []
        for value in values:
            if name:
                if not isinstance(value, dict) or name not in value:
                    raise KeyError(path)
                nxt.append(value[name])
            elif index == "*":
                if not isinstance(value, list):
                    raise KeyError(path)
                nxt.extend(value)
            else:
                if not isinstance(value, list) or int(index) >= len(value):
                    raise KeyError(path)
                nxt.append(value[int(index)])
        values = nxt
    return values if "[*]" in path else values[0]


_VAR = re.compile(r"\$\{([A-Za-z_][A-Za-z0-9_]*)\}")


def substitute(value: Any, variables: dict) -> Any:
    """Replace ${name}; a string that is only '${name}' takes the variable's own type."""
    if isinstance(value, str):
        whole = _VAR.fullmatch(value)
        if whole:
            return variables[whole.group(1)]
        return _VAR.sub(lambda m: str(variables[m.group(1)]), value)
    if isinstance(value, list):
        return [substitute(v, variables) for v in value]
    if isinstance(value, dict):
        return {k: substitute(v, variables) for k, v in value.items()}
    return value


_ARITH = {ast.Add: operator.add, ast.Sub: operator.sub, ast.Mult: operator.mul, ast.Div: operator.truediv}


def evaluate(expression: str, variables: dict) -> float:
    """Arithmetic over numbers and ${variables} only: + - * / and parentheses, nothing else."""
    tree = ast.parse(_VAR.sub(lambda m: f"v_{m.group(1)}", expression), mode="eval")

    def ev(node: ast.AST) -> float:
        if isinstance(node, ast.Expression):
            return ev(node.body)
        if isinstance(node, ast.Constant) and isinstance(node.value, (int, float)):
            return node.value
        if isinstance(node, ast.Name) and node.id.startswith("v_"):
            return float(variables[node.id[2:]])
        if isinstance(node, ast.BinOp) and type(node.op) in _ARITH:
            return _ARITH[type(node.op)](ev(node.left), ev(node.right))
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.USub):
            return -ev(node.operand)
        raise ValueError(f"not allowed in expressions: {ast.dump(node)}")
    return ev(tree)


def compare(actual: Any, rule: dict) -> tuple[bool, str]:
    """One expectation; returns (ok, readable expectation)."""
    if "eq" in rule:
        return actual == rule["eq"], f"== {json.dumps(rule['eq'])}"
    if "ne" in rule:
        return actual != rule["ne"], f"!= {json.dumps(rule['ne'])}"
    if "approx" in rule:
        tol = rule.get("tol", 1e-3)
        ok = isinstance(actual, (int, float)) and abs(actual - rule["approx"]) <= tol
        return ok, f"~= {rule['approx']} +/- {tol}"
    if "lt" in rule:
        return isinstance(actual, (int, float)) and actual < rule["lt"], f"< {rule['lt']}"
    if "gt" in rule:
        return isinstance(actual, (int, float)) and actual > rule["gt"], f"> {rule['gt']}"
    if "len" in rule:
        return isinstance(actual, list) and len(actual) == rule["len"], f"has {rule['len']} items"
    if "contains" in rule:
        return isinstance(actual, str) and rule["contains"] in actual, f"contains {json.dumps(rule['contains'])}"
    if "exists" in rule:
        return (actual is not None) == bool(rule["exists"]), "exists" if rule["exists"] else "missing"
    raise ValueError(f"unknown expectation: {rule}")


class CheckRun:
    """Runs one check file. Every element a step creates is deleted at the end, whatever happens."""

    def __init__(self, send: Callable[..., dict], spec: dict, name: str = ""):
        self.send, self.spec, self.name = send, spec, name or spec.get("title", "check")
        self.variables: dict[str, Any] = {}
        self.created: list[str] = []
        self.rows: list[dict] = []

    def _row(self, step: str, what: str, ok: bool, detail: str) -> None:
        self.rows.append({"check": self.name, "step": step, "what": what, "ok": ok, "detail": detail})

    def run(self) -> list[dict]:
        try:
            for i, step in enumerate(self.spec.get("steps", []), 1):
                if not self._step(i, step):
                    break
        except TapirError as exc:
            self._row("-", "connection", False, str(exc))
        finally:
            self._cleanup()
        return self.rows

    def _step(self, index: int, step: dict) -> bool:
        label = step.get("id", f"#{index}")
        params = substitute(step.get("params", {}), self.variables)
        answer = self.send(step["command"], params, step.get("timeout", DEFAULT_TIMEOUT))
        if "__envelopeError__" in answer and not step.get("allowEnvelopeError"):
            self._row(label, step["command"], False, f"rejected: {json.dumps(answer['__envelopeError__'])[:200]}")
            return False
        try:
            guids = resolve(answer, "elements[*].elementId.guid") if step.get("creates", True) else []
        except KeyError:
            guids = []
        self.created.extend(g for g in guids if isinstance(g, str))
        for name, path in step.get("save", {}).items():
            try:
                self.variables[name] = resolve(answer, path)
            except KeyError:
                self._row(label, f"save {name}", False, f"{path} not in answer: {json.dumps(answer)[:200]}")
                return False
        for rule in step.get("expect", []):
            if "expr" in rule:
                try:
                    actual: Any = evaluate(rule["expr"], self.variables)
                except (KeyError, ValueError, ZeroDivisionError) as exc:
                    self._row(label, rule["expr"], False, f"cannot evaluate: {exc}")
                    continue
                what = rule["expr"]
            else:
                try:
                    actual = resolve(answer, rule["path"])
                except KeyError:
                    actual = None
                what = rule["path"]
            ok, expectation = compare(actual, rule)
            self._row(label, rule.get("name", what), ok, f"{json.dumps(actual)[:120]} (want {expectation})")
            if not ok and step.get("stopOnFail"):
                return False
        return True

    def _cleanup(self) -> None:
        # One call per element: in a batch, a guid that is already gone can roll back the whole delete (#749).
        for guid in self.created:
            self.send("DeleteElements", {"elements": [{"elementId": {"guid": guid}}]}, DEFAULT_TIMEOUT)
        for step in self.spec.get("teardown", []):
            try:
                params = substitute(step.get("params", {}), self.variables)
            except KeyError:
                continue  # it cleans up something an earlier step failed to make
            self.send(step["command"], params, DEFAULT_TIMEOUT)


def project_is_scratch(send: Callable[..., dict], spec: dict) -> tuple[bool, str]:
    """Checks may only run in an untitled project without the element types they could disturb."""
    info = send("GetProjectInfo", {}, DEFAULT_TIMEOUT)
    if not info.get("isUntitled"):
        return False, "the open project is not untitled; checks only run in an empty, unsaved project"
    for element_type in spec.get("requiresEmpty", ["Wall", "Slab", "Stair", "Roof", "Zone"]):
        found = send("GetElementsByType", {"elementType": element_type}, DEFAULT_TIMEOUT).get("elements", [])
        if found:
            return False, f"the project already contains {len(found)} {element_type} element(s)"
    return True, ""


def version_tuple(text: str) -> tuple[int, ...]:
    return tuple(int(p) for p in re.findall(r"\d+", text)[:3])


def archicad_version(send: Callable[..., dict]) -> str:
    """The Archicad major version behind this connection. Ports follow the start order, so never infer it from the port."""
    return str(send("API.GetProductInfo", {}, DEFAULT_TIMEOUT).get("version", "?"))


def archicad_label(send: Callable[..., dict]) -> str:
    """Version and language, e.g. "28 GER": the language brings its own template and defaults,
    which can change a result, so reports name it."""
    info = send("API.GetProductInfo", {}, DEFAULT_TIMEOUT)
    return " ".join(str(v) for v in (info.get("version", "?"), info.get("languageCode")) if v)


def run_checks(send: Callable[..., dict], spec_files: list[Path]) -> tuple[list[dict], str]:
    version = send("GetAddOnVersion", {}, DEFAULT_TIMEOUT).get("version", "?")
    ac = archicad_version(send)
    rows: list[dict] = []
    for path in spec_files:
        spec = json.loads(path.read_text(encoding="utf-8"))
        name = spec.get("title", path.stem)
        needed = spec.get("minTapir")
        if needed and version_tuple(version) < version_tuple(needed):
            rows.append({"check": name, "step": "-", "what": "minTapir", "ok": None,
                         "detail": f"skipped: needs Tapir {needed}, running {version}"})
            continue
        needed = spec.get("minArchicad")
        if needed and version_tuple(ac) < version_tuple(str(needed)):
            rows.append({"check": name, "step": "-", "what": "minArchicad", "ok": None,
                         "detail": f"skipped: needs Archicad {needed}, running {ac}"})
            continue
        ok, reason = project_is_scratch(send, spec)
        if not ok:
            rows.append({"check": name, "step": "-", "what": "scratch project", "ok": False, "detail": reason})
            continue
        rows.extend(CheckRun(send, spec, name).run())
    return rows, version


def report(rows: list[dict], ac_version: str, tapir_version: str, sha: str) -> str:
    """ac_version: the version, or version and language ("28 GER") from archicad_label."""
    scored = [r for r in rows if r["ok"] is not None]
    passed = sum(1 for r in scored if r["ok"])
    verdict = "PASS" if scored and passed == len(scored) else "FAIL"
    lines = [f"{RESULT_MARKER} AC{ac_version} {verdict} {passed}/{len(scored)} build {sha[:7]} tapir {tapir_version}", "",
             "| check | step | expectation | result | detail |", "|---|---|---|---|---|"]
    for r in rows:
        mark = "skipped" if r["ok"] is None else ("pass" if r["ok"] else "**FAIL**")
        detail = str(r["detail"]).replace("|", "\\|")
        lines.append(f"| {r['check']} | {r['step']} | `{r['what']}` | {mark} | {detail} |")
    lines += ["", "<sub>Posted by the Tapir community test runner (checks are JSON data from this pull request).</sub>"]
    return "\n".join(lines)


def brief(rows: list[dict], ac_version: str, tapir_version: str, sha: str, max_failures: int = 5) -> str:
    """The report's first line plus only the failures, shortened: for scripts, logs and Claude."""
    first = report(rows, ac_version, tapir_version, sha).splitlines()[0].removeprefix(RESULT_MARKER).strip()
    failures = [r for r in rows if r["ok"] is False]
    lines = [first] + [f"  x {r['check']} / {r['step']} / {r['what']}: {str(r['detail'])[:140]}" for r in failures[:max_failures]]
    if len(failures) > max_failures:
        lines.append(f"  ... {len(failures) - max_failures} more failures")
    return "\n".join(lines)


def find_port(version: str | None = None, ports=PORTS, exe: str | None = None) -> int | None:
    """The port of the Archicad of this version (any Archicad with Tapir when None). Ports follow the
    start order, so the version is always asked, never assumed. With exe (Windows), only the ports
    that this Archicad's own process listens on are asked: another Archicad is never sent anything,
    not even while it is busy quitting."""
    if exe is not None:
        ports = [p for p in ports if p in ports_of(exe)]
    for port in ports:
        tapir = Tapir(port)
        try:
            running = archicad_version(lambda c, p, t: tapir(c, p, 2))
        except TapirError:
            continue
        if (version is None or running == str(version)) and tapir.alive():
            return port
    return None


# --- configuration ---------------------------------------------------------------------------

def default_config_path() -> Path:
    return rw.state_dir() / "runner.json"


def load_config(path: str | Path | None) -> dict:
    path = Path(path) if path else default_config_path()
    if not path.exists():
        sys.exit(f"no configuration at {path}; run 'python tapir_test_runner.py setup' first")
    cfg = dict(DEFAULTS, **json.loads(path.read_text(encoding="utf-8")))
    cfg["_path"] = str(path)
    return cfg


def get_token(cfg: dict) -> str:
    """The GitHub token: the environment variable, else the one 'setup' stored in Credential Manager."""
    return os.environ.get(cfg.get("tokenEnv", "TAPIR_RUNNER_TOKEN"), "") or rw.load_token()


def pick_archicads(cfg: dict, versions: list[str] | None) -> list[dict]:
    chosen = [a for a in cfg["archicads"] if not versions or str(a["version"]) in versions]
    if not chosen:
        sys.exit(f"no Archicad {', '.join(versions or [])} in {cfg.get('_path')}; run 'setup' again to add it")
    return chosen


# --- Archicad lifecycle ----------------------------------------------------------------------

def processes() -> dict[int, str]:
    """pid -> executable path of the running processes (Windows)."""
    if os.name != "nt":
        raise NotImplementedError("macOS support is not written yet")
    out = subprocess.run(["powershell", "-NoProfile", "-Command",
                          "Get-Process -ErrorAction SilentlyContinue | ForEach-Object { \"$($_.Id)|$($_.Path)\" }"],
                         capture_output=True, text=True, timeout=60).stdout
    found = {}
    for line in out.splitlines():
        pid, _, path = line.strip().partition("|")
        if pid.isdigit() and path:
            found[int(pid)] = path
    return found


def archicad_pids(exe: str) -> set[int]:
    """The processes of this Archicad itself; not its crash reporter (GSReport) or other helpers in its folder."""
    target = str(Path(exe)).lower()
    return {pid for pid, path in processes().items() if path.lower() == target}


def archicad_running(exe: str) -> bool:
    """True while this Archicad runs (Windows); the add-on may only change while it does not."""
    return bool(archicad_pids(exe))


def crash_reporter_running(exe: str) -> bool:
    folder = str(Path(exe).parent).lower()
    return any(Path(path).name.lower() == "gsreport.exe" and path.lower().startswith(folder) for path in processes().values())


def ports_of(exe: str) -> set[int]:
    """The TCP ports this Archicad's own process listens on (Windows, from netstat)."""
    pids = archicad_pids(exe)
    out = subprocess.run(["netstat", "-ano", "-p", "TCP"], capture_output=True, text=True, timeout=60).stdout
    return listening_ports(out, pids)


def listening_ports(netstat: str, pids: set[int]) -> set[int]:
    """The ports these processes listen on, from 'netstat -ano'. The state column is localized
    ("LISTENING", "ABHÖREN", ...); a listening socket is the one without a foreign address."""
    ports = set()
    for line in netstat.splitlines():
        parts = line.split()
        if (len(parts) >= 5 and parts[2] in ("0.0.0.0:0", "[::]:0") and parts[-1].isdigit()
                and int(parts[-1]) in pids):
            port = parts[1].rsplit(":", 1)[-1]
            if port.isdigit():
                ports.add(int(port))
    return ports


def wait_for(predicate: Callable[[], bool], timeout: float, step: float = 2.0) -> bool:
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        if predicate():
            return True
        time.sleep(step)
    return predicate()


class Installation:
    """Loads a build into one Archicad and puts the tester's own Tapir back afterwards.

    Without 'addonFile' in the configuration (what 'setup' writes) the runner changes Archicad's
    add-on list: the build is added, every other Tapir is left out, and the list is written back
    exactly as it was afterwards, so no file of the tester's own installation is touched and no
    administrator rights are needed. With 'addonFile' it swaps that file, as the first version did."""

    def __init__(self, ac: dict, addon_list: rw.AddOnList | None = None):
        self.ac = ac
        self.addon = Path(ac["addonFile"]) if ac.get("addonFile") else None
        self.keep = self.addon.with_name(self.addon.name + ".runner-original") if self.addon else None
        self.addon_list = addon_list if addon_list or self.addon else rw.AddOnList(str(ac["version"]))

    def install(self, build: Path) -> None:
        if archicad_running(self.ac["archicadExe"]):
            raise RuntimeError(f"Archicad {self.ac['version']} is running; not touching the add-on")
        if self.addon is None:
            self.addon_list.install(rw.stage_build(build, str(self.ac["version"])))
            return
        try:
            if not self.keep.exists():
                shutil.copy2(self.addon, self.keep)
            shutil.copy2(build, self.addon)
        except PermissionError as exc:
            raise RuntimeError(f"cannot replace {self.addon} ({exc.strerror}): the Add-Ons folder of Archicad "
                               f"{self.ac['version']} needs administrator rights; run the runner as administrator") from exc

    def pending(self) -> bool:
        """A test build is still in place (a test did not finish)."""
        return self.addon_list.backup_file.exists() if self.addon is None else self.keep.exists()

    def restore(self) -> None:
        if not self.pending():
            return
        if archicad_running(self.ac["archicadExe"]):
            raise RuntimeError(f"Archicad {self.ac['version']} still running; original add-on not restored")
        if self.addon is None:
            self.addon_list.restore()
        else:
            shutil.copy2(self.keep, self.addon)
            self.keep.unlink()

    def hidden(self) -> bool:
        return os.name == "nt" and bool(self.ac.get("hidden"))

    def where(self) -> str:
        """Where to answer a dialog: the runner's desktop is never shown on its own."""
        return " It is on the runner's own desktop: 'show' takes you there." if self.hidden() else ""

    def start(self) -> Tapir:
        if self.hidden():
            rw.start_hidden([self.ac["archicadExe"], self.ac["blankTemplate"]])
        else:
            subprocess.Popen([self.ac["archicadExe"], self.ac["blankTemplate"]])
        found: dict = {}

        def ready() -> bool:
            port = find_port(str(self.ac["version"]), [self.ac["port"]] if self.ac.get("port") else PORTS,
                             self.ac["archicadExe"] if os.name == "nt" else None)
            if port and Tapir(port).alive():
                found["port"] = port
                return True
            # A modal dialog seen in three polls in a row (10 s and more, not a window that comes and goes
            # during the start) is waiting for a human. A normal start shows none: its splash and palettes
            # are other window classes.
            shown = rw.dialogs(archicad_pids(self.ac["archicadExe"])) if os.name == "nt" else []
            found["dialog"] = found.get("dialog", 0) + 1 if shown else 0
            found["title"] = shown[0] if shown else ""
            return found["dialog"] >= 3
        if not wait_for(ready, self.ac.get("startTimeout", 300), 5):
            raise RuntimeError(f"Archicad {self.ac['version']} did not answer with Tapir loaded within "
                               f"{self.ac.get('startTimeout', 300)} s; it may be waiting at a dialog (project recovery, "
                               "licence, update). Answer it, quit Archicad, then 'restore' puts your add-on back." + self.where())
        if "port" not in found:
            raise RuntimeError(f"Archicad {self.ac['version']} shows a dialog ('{found['title']}') instead of answering "
                               "with Tapir loaded. Answer it, quit Archicad, then 'restore' puts your add-on back. If it "
                               "says some add-ons could not be loaded, see #1150 (set \"addonFile\")." + self.where())
        tapir = Tapir(found["port"])
        running = archicad_version(tapir)
        if running != str(self.ac["version"]):
            raise RuntimeError(f"port {found['port']} answers as Archicad {running}, not {self.ac['version']}; "
                               "ports follow the start order - start only one Archicad while the runner works")
        return tapir

    def quit(self, tapir: Tapir) -> None:
        """Asks Archicad to quit and waits; never kills it (a dialog may need a human)."""
        try:
            tapir("QuitArchicad", {}, 30)
        except TapirError:
            pass
        if not wait_for(lambda: not archicad_running(self.ac["archicadExe"]), self.ac.get("quitTimeout", 180), 5):
            raise RuntimeError(f"Archicad {self.ac['version']} did not quit; close it by hand, then run 'restore'." + self.where())


def restore_all(cfg: dict, log: Callable[[str], None] = print) -> int:
    """Puts back an add-on a previous test left in place (a crash, a power cut, a dialog); returns how many."""
    restored = 0
    for ac in cfg["archicads"]:
        install = Installation(ac)
        if install.pending():
            if archicad_running(ac["archicadExe"]):
                log(f"AC{ac['version']}: a test build is still loaded; close Archicad {ac['version']} to get your add-on back")
            else:
                install.restore()
                restored += 1
                log(f"AC{ac['version']}: your add-on is back")
    return restored


def run_installed(ac: dict, build: Path, checks: list[Path]) -> tuple[list[dict], str, str]:
    """Load, start, check, quit, restore; returns the rows, the Tapir version and the Archicad label
    ("28 GER"). A crash, or an Archicad that does not quit, becomes a failed row of the report (a
    crash is a result too); only a failure to start or to restore raises."""
    install = Installation(ac)
    install.install(build)
    exe = ac["archicadExe"]
    try:
        tapir = install.start()
        label = archicad_label(tapir)
        rows: list[dict] = []
        version = "?"
        try:
            rows, version = run_checks(tapir, checks)
        except TapirError as exc:
            rows.append({"check": "Archicad", "step": "-", "what": "connection", "ok": False, "detail": str(exc)})
        if not archicad_running(exe):
            rows.append({"check": "Archicad", "step": "-", "what": "crash", "ok": False,
                         "detail": "Archicad closed during the checks" + (" (crash report open)" if crash_reporter_running(exe) else "")})
        else:
            try:
                install.quit(tapir)
                if crash_reporter_running(exe):  # reported, not scored: some setups crash on every quit
                    rows.append({"check": "Archicad", "step": "-", "what": "quit", "ok": None,
                                 "detail": "warning: Archicad crashed while quitting (crash report open)"})
            except RuntimeError as exc:
                rows.append({"check": "Archicad", "step": "-", "what": "quit", "ok": False, "detail": str(exc)})
                raise RuntimeError(report(rows, label, version, "local")) from exc
        return rows, version, label
    finally:
        if not archicad_running(exe):  # otherwise the test build stays until Archicad is closed and 'restore' runs
            install.restore()


# --- GitHub ----------------------------------------------------------------------------------

class GitHub:
    def __init__(self, repo: str, token: str):
        self.repo, self.token = repo, token

    class _NoRedirect(urllib.request.HTTPRedirectHandler):
        def redirect_request(self, *args, **kwargs):
            return None

    def _request(self, url: str, data: dict | None = None, raw: bool = False) -> Any:
        if not url.startswith("http"):
            url = f"https://api.github.com/repos/{self.repo}/{url}"
        headers = {"Accept": "application/vnd.github+json", "User-Agent": "tapir-community-test-runner"}
        if self.token:
            headers["Authorization"] = f"Bearer {self.token}"
        request = urllib.request.Request(url, data=None if data is None else json.dumps(data).encode("utf-8"),
                                         method="GET" if data is None else "POST", headers=headers)
        try:
            with urllib.request.build_opener(self._NoRedirect).open(request, timeout=120) as response:
                payload = response.read()
        except urllib.error.HTTPError as exc:
            if exc.code in (403, 429) and exc.headers.get("X-RateLimit-Remaining") == "0":
                reset = time.strftime("%H:%M", time.localtime(int(exc.headers.get("X-RateLimit-Reset", "0"))))
                raise RuntimeError(f"GitHub's request limit is used up until {reset}"
                                   + ("" if self.token else "; without a token it is 60 an hour, run 'setup' to add one")) from exc
            if exc.code not in (301, 302, 307):
                raise
            # artifact downloads redirect to blob storage, which rejects the GitHub token
            with urllib.request.urlopen(exc.headers["Location"], timeout=300) as response:
                payload = response.read()
        return payload if raw else json.loads(payload.decode("utf-8"))

    def open_pulls(self) -> list[dict]:
        return self._request("pulls?state=open&per_page=100")

    def pulls_to_test(self, label: str) -> list[dict]:
        return [p for p in self.open_pulls() if any(lb["name"] == label for lb in p.get("labels", []))]

    def pull(self, number: int) -> dict:
        return self._request(f"pulls/{number}")

    def reports(self, number: int) -> list[dict]:
        return [c for c in self._request(f"issues/{number}/comments?per_page=100") if c["body"].startswith(RESULT_MARKER)]

    def already_reported(self, number: int, sha: str, ac_version: str, login: str) -> bool:
        needle = f"{RESULT_MARKER} AC{ac_version} "
        return any(c["user"]["login"] == login and c["body"].startswith(needle) and f" build {sha[:7]} " in c["body"]
                   for c in self.reports(number))

    def login(self) -> str:
        return self._request("https://api.github.com/user")["login"]

    def build_run(self, sha: str) -> dict | None:
        runs = self._request(f"actions/runs?head_sha={sha}&status=success&per_page=50")["workflow_runs"]
        return next((r for r in runs if r.get("name") == "Archicad Add-On Build Check"
                     and r["repository"]["full_name"] == self.repo), None)

    def built_versions(self, sha: str) -> list[str]:
        """The Archicad versions the build check built for this commit (Windows)."""
        run = self.build_run(sha)
        if run is None:
            return []
        names = [a["name"] for a in self._request(f"actions/runs/{run['id']}/artifacts")["artifacts"] if not a["expired"]]
        return sorted({m.group(1) for n in names if (m := re.fullmatch(r"Tapir Add-On AC(\d+) Win", n))}, key=int)

    def build_artifact(self, sha: str, ac_version: str, dest: Path) -> Path | None:
        """The add-on that the repository's own build check made for exactly this commit."""
        run = self.build_run(sha)
        if run is None:
            return None
        artifacts = self._request(f"actions/runs/{run['id']}/artifacts")["artifacts"]
        wanted = next((a for a in artifacts if a["name"] == f"Tapir Add-On AC{ac_version} Win" and not a["expired"]), None)
        if wanted is None:
            return None
        with zipfile.ZipFile(io.BytesIO(self._request(wanted["archive_download_url"], raw=True))) as zf:
            member = next(n for n in zf.namelist() if n.endswith(".apx"))
            target = dest / Path(member).name
            target.write_bytes(zf.read(member))
            return target

    def check_paths(self, number: int, folder: str, every: bool = False, sha: str = "") -> list[str]:
        """The check files this pull request adds or changes; with every, also the rest of the folder."""
        paths = [f["filename"] for f in self._request(f"pulls/{number}/files?per_page=100")
                 if f["status"] != "removed" and f["filename"].startswith(folder) and f["filename"].endswith(".json")]
        if every:
            try:
                listed = self._request(f"contents/{folder.rstrip('/')}?ref={sha}")
            except urllib.error.HTTPError:
                listed = []
            paths += [f["path"] for f in listed if f["name"].endswith(".json") and f["path"] not in paths]
        return paths

    def check_files(self, number: int, sha: str, folder: str, dest: Path, every: bool = False) -> list[Path]:
        """The check files, fetched at the pull request's head commit."""
        found = []
        for name in self.check_paths(number, folder, every, sha):
            content = self._request(f"contents/{name}?ref={sha}")
            path = dest / Path(name).name
            path.write_bytes(base64.b64decode(content["content"]))
            found.append(path)
        return found

    def comment(self, number: int, body: str) -> None:
        self._request(f"issues/{number}/comments", {"body": body})


# --- orchestration -------------------------------------------------------------------------

def fork_owner(cfg: dict, pull: dict) -> str | None:
    """The owner of the pull request's fork, or None when its branch is in this repository."""
    head = (pull["head"].get("repo") or {}).get("full_name") or "deleted fork/"
    return None if head == cfg["repo"] else head.split("/")[0]


def outcome(pull: dict, ac: dict, state: str, text: str, rows: list[dict] | None = None, body: str = "",
            label: str = "") -> dict:
    """state: tested (text is the verdict line), skipped, or error."""
    return {"number": pull["number"], "title": pull.get("title", ""), "ac": str(ac["version"]),
            "label": label or str(ac["version"]), "sha": pull["head"]["sha"], "state": state, "text": text,
            "rows": rows or [], "body": body}


def test_pull(gh: GitHub, cfg: dict, pull: dict, ac: dict, login: str, post: bool,
              every: bool = False, retest: bool = False) -> dict:
    number, sha = pull["number"], pull["head"]["sha"]
    owner = fork_owner(cfg, pull)
    if owner and not cfg.get("allowForks", False) and owner not in cfg.get("trustedForks", []):
        return outcome(pull, ac, "skipped", f"branch is in {owner}'s fork (add it to trustedForks to test it)")
    if not retest and login and gh.already_reported(number, sha, ac["version"], login):
        return outcome(pull, ac, "skipped", f"already reported for {sha[:7]}")
    with tempfile.TemporaryDirectory() as tmp:
        work = Path(tmp)
        checks = gh.check_files(number, sha, cfg.get("checkFolder", DEFAULTS["checkFolder"]), work, every)
        if not checks:
            return outcome(pull, ac, "skipped", "no check files" + ("" if every else " (--every-check runs the whole check folder)"))
        build = gh.build_artifact(sha, ac["version"], work)
        if build is None:
            return outcome(pull, ac, "skipped", f"no finished AC{ac['version']} build for {sha[:7]} yet")
        rows, version, label = run_installed(ac, build, checks)
    body = report(rows, label, version, sha)
    if post:
        gh.comment(number, body)
    return outcome(pull, ac, "tested", body.splitlines()[0], rows, body, label)


def format_outcome(result: dict, as_brief: bool) -> str:
    head = f"#{result['number']} AC{result['ac']}"
    if result["state"] != "tested":
        return f"{head} {result['state']}: {result['text']}"
    if as_brief:
        version = result["text"].rsplit(" tapir ", 1)[-1]
        return f"#{result['number']} " + brief(result["rows"], result["label"], version, result["sha"])
    return f"{head}\n{result['body']}"


def trial(ac: dict, build: Path, checks: list[Path], as_brief: bool = False) -> str:
    """The full install-test-restore cycle for a local build, without GitHub."""
    rows, version, label = run_installed(ac, build, checks)
    return brief(rows, label, version, "local") if as_brief else report(rows, label, version, "local")


def select_pulls(gh: GitHub, cfg: dict, numbers: list[int] | None, every_open: bool) -> list[dict]:
    if numbers:
        return [gh.pull(n) for n in numbers]
    if every_open:
        return [p for p in gh.open_pulls() if not p.get("draft")]
    return gh.pulls_to_test(cfg.get("label", "needs-test"))


def run_pass(cfg: dict, post: bool, numbers: list[int] | None = None, every_open: bool = False,
             versions: list[str] | None = None, every: bool = False, retest: bool = False,
             log: Callable[[str], None] = print, as_brief: bool = False,
             stop: Callable[[], bool] = lambda: False) -> list[dict]:
    """Tests the chosen pull requests on the chosen Archicads; logs one entry per pull request and version."""
    token = get_token(cfg)
    if not token:
        raise RuntimeError("no GitHub token: run 'setup', or set " + cfg.get("tokenEnv", "TAPIR_RUNNER_TOKEN")
                           + " (classic token with the public_repo scope: download builds, post comments)")
    gh = GitHub(cfg["repo"], token)
    login = gh.login()
    restore_all(cfg, log)
    pulls = select_pulls(gh, cfg, numbers, every_open)
    if not as_brief:
        log(f"{len(pulls)} pull request(s) to look at")
    results: list[dict] = []
    for ac in pick_archicads(cfg, versions):
        if archicad_running(ac["archicadExe"]):
            log(f"AC{ac['version']} is open - skipped (the runner only works while it is closed; "
                "'check' runs against an open one)")
            continue
        for pull in pulls:
            if stop():
                return results
            try:
                result = test_pull(gh, cfg, pull, ac, login, post, every, retest)
            except (RuntimeError, TapirError, urllib.error.URLError) as exc:
                result = outcome(pull, ac, "error", f"stopped - {exc}")
                results.append(result)
                log(format_outcome(result, as_brief))
                return results
            results.append(result)
            log(format_outcome(result, as_brief))
    return results


def pull_rows(cfg: dict, every_open: bool = True,
              on_row: Callable[[dict, int, int], None] | None = None) -> list[dict]:
    """Per open pull request: where its branch is, its checks, its builds and the last reports."""
    token = get_token(cfg)
    if not token:
        raise RuntimeError("no GitHub token yet: run 'python tapir_test_runner.py setup' (GitHub allows only 60 requests an hour without one)")
    gh = GitHub(cfg["repo"], token)
    pulls = select_pulls(gh, cfg, None, every_open)

    def basic(pull: dict) -> dict:
        owner = fork_owner(cfg, pull)
        trusted = not owner or cfg.get("allowForks") or owner in cfg.get("trustedForks", [])
        return {"number": pull["number"], "title": pull["title"], "sha": pull["head"]["sha"][:7],
                "branch": "own" if not owner else owner + (" (trusted)" if trusted else " fork"),
                "testable": bool(trusted), "labels": ",".join(lb["name"] for lb in pull.get("labels", [])) or "-",
                "checks": None, "builds": None, "last": None}

    def detail(pull: dict) -> dict:
        row = basic(pull)
        row.update(checks=len(gh.check_paths(pull["number"], cfg.get("checkFolder", DEFAULTS["checkFolder"]))),
                   builds=gh.built_versions(pull["head"]["sha"]),
                   last=[c["body"].splitlines()[0].removeprefix(RESULT_MARKER).strip()
                         for c in gh.reports(pull["number"])][-2:])
        return row

    rows = {p["number"]: basic(p) for p in pulls}
    if on_row:
        for row in rows.values():
            on_row(row, 0, len(rows))
    # three or four requests per pull request: in parallel, or a hundred open ones take minutes
    with ThreadPoolExecutor(max_workers=8) as pool:
        for done, future in enumerate(as_completed([pool.submit(detail, p) for p in pulls]), 1):
            row = future.result()
            rows[row["number"]] = row
            if on_row:
                on_row(row, done, len(pulls))
    return [rows[p["number"]] for p in pulls]


def list_pulls(cfg: dict, every_open: bool = True) -> list[str]:
    return [f"#{p['number']} {p['branch']} | {p['sha']} | labels {p['labels']} | checks {p['checks']}"
            f" | builds {'AC' + ','.join(p['builds']) if p['builds'] else 'none'}"
            f" | last {'; '.join(p['last']) if p['last'] else '-'} | {p['title'][:60]}"
            for p in pull_rows(cfg, every_open)]


def uninstall(config_path: Path) -> int:
    """Everything the runner put on this computer goes, after the tester's own add-on is back."""
    if config_path.exists():
        cfg = dict(DEFAULTS, **json.loads(config_path.read_text(encoding="utf-8")))
        if any(Installation(ac).pending() and archicad_running(ac["archicadExe"]) for ac in cfg["archicads"]):
            raise RuntimeError("a test build is still loaded in an open Archicad; close Archicad, then uninstall again")
        restore_all(cfg)
    print("GitHub token removed from Credential Manager" if rw.delete_token() else "no stored GitHub token")
    if os.name == "nt":
        task = subprocess.run(["schtasks", "/delete", "/f", "/tn", "TapirTestRunner"], capture_output=True, text=True)
        print("logon task removed" if task.returncode == 0 else "no logon task")
    folder = rw.state_dir()
    shutil.rmtree(folder, ignore_errors=True)
    print(f"removed {folder}")
    if config_path.exists() and folder not in config_path.parents:
        config_path.unlink()
        print(f"removed {config_path}")
    print("Done. You can delete the runner's folder too. Revoke the token on https://github.com/settings/tokens")
    return 0


def expand(patterns: list[str]) -> list[Path]:
    return [Path(f) for pattern in patterns for f in sorted(glob.glob(pattern))]


def local_checks() -> list[str]:
    """The check folder of the checkout this runner is in, for 'check' and 'trial' without files."""
    return [str(REPO_ROOT / DEFAULTS["checkFolder"] / "*.json")]


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    sub = parser.add_subparsers(dest="mode", required=True)

    def with_config(p: argparse.ArgumentParser) -> argparse.ArgumentParser:
        p.add_argument("--config", help=f"default: {default_config_path()}")
        return p

    s = sub.add_parser("setup", help="once: find Archicads, store the GitHub token, write the configuration")
    s.add_argument("--config", help=f"where to write it (default: {default_config_path()})")
    s.add_argument("--yes", action="store_true", help="accept every default, ask nothing (the token comes from the environment)")
    s.add_argument("--install-claude-skill", action="store_true", help="also copy the Claude Code skill to ~/.claude/skills")
    with_config(sub.add_parser("gui", help="a window: pending pull requests, Test, Pause, results"))
    rec = sub.add_parser("record", help="write a check from the commands sent through a recording port")
    rec.add_argument("--out", required=True)
    rec.add_argument("--port", type=int, help="Archicad's port (default: the first Archicad with Tapir)")
    rec.add_argument("--listen", type=int, default=19800, help="the port your script or tool sends to instead")
    rec.add_argument("--title", default="")
    c = sub.add_parser("check", help="run check files against an Archicad that is already open (an empty, untitled project)")
    c.add_argument("files", nargs="*", help="default: this checkout's archicad-addon/Test/checks")
    c.add_argument("--port", type=int, help="default: the first Archicad that answers with Tapir")
    c.add_argument("--brief", action="store_true", help="the verdict line and the failures only")
    t = with_config(sub.add_parser("test", help="test chosen pull requests (default: those labelled for testing)"))
    t.add_argument("numbers", nargs="*", type=int)
    t.add_argument("--all", action="store_true", help="every open, non-draft pull request")
    t.add_argument("--ac", action="append", help="only this Archicad version (repeatable)")
    t.add_argument("--every-check", action="store_true", help="run the whole check folder, not only the pull request's checks")
    t.add_argument("--no-post", action="store_true", help="print the report instead of commenting")
    t.add_argument("--retest", action="store_true", help="test again even if you already reported on this commit")
    t.add_argument("--brief", action="store_true", help="one line per pull request and version, plus failures")
    p = with_config(sub.add_parser("prs", help="one line per open pull request: checks, builds, last results"))
    p.add_argument("--labelled", action="store_true", help="only those labelled for testing")
    with_config(sub.add_parser("restore", help="put your own add-on back after a test that did not finish"))
    sub.add_parser("show", help="switch to the runner's own desktop, where a hidden Archicad shows its dialogs")
    with_config(sub.add_parser("uninstall", help="put your add-on back, then remove the token, the logon task and the runner's files"))
    for name in ("once", "poll"):
        o = with_config(sub.add_parser(name))
        o.add_argument("--no-post", action="store_true", help="print the report instead of commenting")
        o.add_argument("--brief", action="store_true")
    tr = with_config(sub.add_parser("trial", help="install a local build, run checks, quit Archicad and restore - no GitHub"))
    tr.add_argument("--ac", required=True, help="Archicad version from the config, e.g. 26")
    tr.add_argument("--build", required=True, help="the .apx to test")
    tr.add_argument("--brief", action="store_true")
    tr.add_argument("files", nargs="*", help="default: this checkout's archicad-addon/Test/checks")
    args = parser.parse_args(argv)
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")  # Windows consoles default to cp1252

    if args.mode == "setup":
        import runner_setup
        return runner_setup.run(args.config or default_config_path(), args.yes, args.install_claude_skill)
    if args.mode == "record":
        import runner_record
        return runner_record.run(args.out, args.port or find_port(), args.listen, args.title)
    if args.mode == "check":
        port = args.port or find_port()
        if port is None:
            sys.exit(f"no Archicad with Tapir answers on ports {PORTS.start}-{PORTS.stop - 1}")
        tapir = Tapir(port)
        rows, version = run_checks(tapir, expand(args.files or local_checks()))
        print((brief if args.brief else report)(rows, archicad_label(tapir), version, "local"))
        return 0 if all(r["ok"] is not False for r in rows) else 1

    if args.mode == "show":
        if not rw.show_desktop(sys.executable):
            sys.exit("could not switch to the runner's desktop (nothing runs there?)")
        return 0
    if args.mode == "uninstall":
        return uninstall(Path(args.config) if args.config else default_config_path())
    cfg = load_config(args.config)
    if args.mode == "gui":
        import runner_gui
        return runner_gui.run(cfg)
    if args.mode == "prs":
        print("\n".join(list_pulls(cfg, not args.labelled)) or "no open pull requests")
        return 0
    if args.mode == "restore":
        restore_all(cfg)
        return 0
    if args.mode == "trial":
        ac = pick_archicads(cfg, [args.ac])[0]
        body = trial(ac, Path(args.build), expand(args.files or local_checks()), args.brief)
        print(body)
        return 0 if " PASS " in body.splitlines()[0] else 1
    try:
        if args.mode == "test":
            results = run_pass(cfg, not args.no_post, args.numbers, args.all, args.ac, args.every_check,
                               args.retest, as_brief=args.brief)
            return 2 if any(r["state"] == "error" for r in results) else (
                1 if any(r["state"] == "tested" and " FAIL " in r["text"] for r in results) else 0)
        while True:
            run_pass(cfg, not args.no_post, as_brief=args.brief)
            if args.mode == "once":
                return 0
            time.sleep(cfg.get("pollMinutes", 15) * 60)
    except RuntimeError as exc:
        sys.exit(str(exc))


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (RuntimeError, TapirError) as error:  # a message for the tester, not a traceback
        print(f"stopped: {error}", file=sys.stderr)
        sys.exit(2)
