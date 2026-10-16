#!/usr/bin/env python3
"""Community test runner for Tapir pull requests.

Picks up open pull requests labelled for testing, installs their CI build into a local Archicad,
runs the pull request's checks against an empty project, restores the previous add-on and posts
the result as a pull request comment. Standard library only; no Claude or other model is needed.

    python tapir_test_runner.py check checks/*.json --port 19723     # run checks against a running Archicad
    python tapir_test_runner.py once --config runner.json              # one pass over all pending pull requests
    python tapir_test_runner.py poll --config runner.json              # repeat 'once' every poll_minutes

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
from pathlib import Path
from typing import Any, Callable

RESULT_MARKER = "tapir-test:"
DEFAULT_TIMEOUT = 60.0


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
            self.send(step["command"], substitute(step.get("params", {}), self.variables), DEFAULT_TIMEOUT)


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


def run_checks(send: Callable[..., dict], spec_files: list[Path]) -> tuple[list[dict], str]:
    version = send("GetAddOnVersion", {}, DEFAULT_TIMEOUT).get("version", "?")
    rows: list[dict] = []
    for path in spec_files:
        spec = json.loads(path.read_text(encoding="utf-8"))
        name = spec.get("title", path.stem)
        needed = spec.get("minTapir")
        if needed and version_tuple(version) < version_tuple(needed):
            rows.append({"check": name, "step": "-", "what": "minTapir", "ok": None,
                         "detail": f"skipped: needs Tapir {needed}, running {version}"})
            continue
        ok, reason = project_is_scratch(send, spec)
        if not ok:
            rows.append({"check": name, "step": "-", "what": "scratch project", "ok": False, "detail": reason})
            continue
        rows.extend(CheckRun(send, spec, name).run())
    return rows, version


def report(rows: list[dict], ac_version: str, tapir_version: str, sha: str) -> str:
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


# --- Archicad lifecycle ----------------------------------------------------------------------

def archicad_running(exe: str) -> bool:
    """True while a process runs from this Archicad's folder (Windows); the add-on may only change then."""
    folder = str(Path(exe).parent).lower()
    if os.name != "nt":
        raise NotImplementedError("macOS support is not written yet")
    out = subprocess.run(["powershell", "-NoProfile", "-Command",
                          "Get-Process -ErrorAction SilentlyContinue | ForEach-Object { $_.Path }"],
                         capture_output=True, text=True, timeout=60).stdout
    return any(line.strip().lower().startswith(folder) for line in out.splitlines() if line.strip())


def wait_for(predicate: Callable[[], bool], timeout: float, step: float = 2.0) -> bool:
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        if predicate():
            return True
        time.sleep(step)
    return predicate()


class Installation:
    """Swaps the add-on file of one Archicad and puts the original back afterwards."""

    def __init__(self, ac: dict):
        self.ac = ac
        self.addon = Path(ac["addonFile"])
        self.keep = self.addon.with_name(self.addon.name + ".runner-original")

    def install(self, build: Path) -> None:
        if archicad_running(self.ac["archicadExe"]):
            raise RuntimeError(f"Archicad {self.ac['version']} is running; not touching the add-on")
        if not self.keep.exists():
            shutil.copy2(self.addon, self.keep)
        shutil.copy2(build, self.addon)

    def restore(self) -> None:
        if self.keep.exists():
            if archicad_running(self.ac["archicadExe"]):
                raise RuntimeError(f"Archicad {self.ac['version']} still running; original add-on not restored")
            shutil.copy2(self.keep, self.addon)
            self.keep.unlink()

    def start(self) -> Tapir:
        subprocess.Popen([self.ac["archicadExe"], self.ac["blankTemplate"]])
        tapir = Tapir(self.ac["port"])
        if not wait_for(tapir.alive, self.ac.get("startTimeout", 300), 5):
            raise RuntimeError(f"Archicad {self.ac['version']} did not answer on port {self.ac['port']}")
        return tapir

    def quit(self, tapir: Tapir) -> None:
        """Asks Archicad to quit and waits; never kills it (a dialog may need a human)."""
        try:
            tapir("QuitArchicad", {}, 30)
        except TapirError:
            pass
        if not wait_for(lambda: not archicad_running(self.ac["archicadExe"]), self.ac.get("quitTimeout", 180), 5):
            raise RuntimeError(f"Archicad {self.ac['version']} did not quit; close it by hand, then run again")


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
        request = urllib.request.Request(url, data=None if data is None else json.dumps(data).encode("utf-8"),
                                         method="GET" if data is None else "POST",
                                         headers={"Authorization": f"Bearer {self.token}",
                                                  "Accept": "application/vnd.github+json",
                                                  "User-Agent": "tapir-community-test-runner"})
        try:
            with urllib.request.build_opener(self._NoRedirect).open(request, timeout=120) as response:
                payload = response.read()
        except urllib.error.HTTPError as exc:
            if exc.code not in (301, 302, 307):
                raise
            # artifact downloads redirect to blob storage, which rejects the GitHub token
            with urllib.request.urlopen(exc.headers["Location"], timeout=300) as response:
                payload = response.read()
        return payload if raw else json.loads(payload.decode("utf-8"))

    def pulls_to_test(self, label: str) -> list[dict]:
        pulls = self._request("pulls?state=open&per_page=100")
        return [p for p in pulls if any(lb["name"] == label for lb in p.get("labels", []))]

    def already_reported(self, number: int, sha: str, ac_version: str, login: str) -> bool:
        comments = self._request(f"issues/{number}/comments?per_page=100")
        needle = f"{RESULT_MARKER} AC{ac_version} "
        return any(c["user"]["login"] == login and c["body"].startswith(needle) and f" build {sha[:7]} " in c["body"]
                   for c in comments)

    def login(self) -> str:
        return self._request("https://api.github.com/user")["login"]

    def build_artifact(self, sha: str, ac_version: str, dest: Path) -> Path | None:
        """The add-on that the repository's own build check made for exactly this commit."""
        runs = self._request(f"actions/runs?head_sha={sha}&status=success&per_page=50")["workflow_runs"]
        for run in runs:
            if run.get("name") != "Archicad Add-On Build Check" or run["repository"]["full_name"] != self.repo:
                continue
            artifacts = self._request(f"actions/runs/{run['id']}/artifacts")["artifacts"]
            wanted = next((a for a in artifacts if a["name"] == f"Tapir Add-On AC{ac_version} Win" and not a["expired"]), None)
            if wanted:
                with zipfile.ZipFile(io.BytesIO(self._request(wanted["archive_download_url"], raw=True))) as zf:
                    member = next(n for n in zf.namelist() if n.endswith(".apx"))
                    target = dest / Path(member).name
                    target.write_bytes(zf.read(member))
                    return target
        return None

    def check_files(self, number: int, sha: str, folder: str, dest: Path) -> list[Path]:
        """The check files this pull request adds or changes, fetched at its head commit."""
        files = self._request(f"pulls/{number}/files?per_page=100")
        found = []
        for f in files:
            if f["status"] != "removed" and f["filename"].startswith(folder) and f["filename"].endswith(".json"):
                content = self._request(f"contents/{f['filename']}?ref={sha}")
                path = dest / Path(f["filename"]).name
                path.write_bytes(base64.b64decode(content["content"]))
                found.append(path)
        return found

    def comment(self, number: int, body: str) -> None:
        self._request(f"issues/{number}/comments", {"body": body})


# --- orchestration -------------------------------------------------------------------------

def test_pull(gh: GitHub, cfg: dict, pull: dict, ac: dict, login: str, post: bool) -> str:
    number, sha = pull["number"], pull["head"]["sha"]
    if pull["head"]["repo"]["full_name"] != cfg["repo"] and not cfg.get("allowForks", False):
        return f"#{number}: skipped, the branch is in a fork (only the project's own branches are auto-tested)"
    if gh.already_reported(number, sha, ac["version"], login):
        return f"#{number}: already reported for {sha[:7]} on AC{ac['version']}"
    with tempfile.TemporaryDirectory() as tmp:
        work = Path(tmp)
        checks = gh.check_files(number, sha, cfg.get("checkFolder", "archicad-addon/Test/checks/"), work)
        if not checks:
            return f"#{number}: no check files in this pull request"
        build = gh.build_artifact(sha, ac["version"], work)
        if build is None:
            return f"#{number}: no finished build for {sha[:7]} on AC{ac['version']} yet"
        install = Installation(ac)
        install.install(build)
        try:
            tapir = install.start()
            try:
                rows, version = run_checks(tapir, checks)
            finally:
                install.quit(tapir)
        finally:
            install.restore()
    body = report(rows, ac["version"], version, sha)
    if post:
        gh.comment(number, body)
    return f"#{number}: {body.splitlines()[0]}"


def once(cfg: dict, post: bool) -> None:
    token = os.environ.get(cfg.get("tokenEnv", "TAPIR_RUNNER_TOKEN"), "")
    if not token:
        sys.exit(f"set {cfg.get('tokenEnv', 'TAPIR_RUNNER_TOKEN')} to a GitHub token (classic token with the public_repo scope: download builds, post comments)")
    gh = GitHub(cfg["repo"], token)
    login = gh.login()
    pulls = gh.pulls_to_test(cfg.get("label", "needs-test"))
    print(f"{len(pulls)} pull request(s) labelled {cfg.get('label', 'needs-test')!r}")
    for ac in cfg["archicads"]:
        if archicad_running(ac["archicadExe"]):
            print(f"AC{ac['version']} is running - skipped this round (the runner only works while it is closed)")
            continue
        for pull in pulls:
            try:
                print(test_pull(gh, cfg, pull, ac, login, post))
            except (RuntimeError, TapirError, urllib.error.URLError) as exc:
                print(f"#{pull['number']} on AC{ac['version']}: stopped - {exc}")
                return


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    sub = parser.add_subparsers(dest="mode", required=True)
    c = sub.add_parser("check", help="run check files against an already running Archicad")
    c.add_argument("files", nargs="+")
    c.add_argument("--port", type=int, default=19723)
    c.add_argument("--ac", default="?", help="Archicad version for the report line")
    for name in ("once", "poll"):
        p = sub.add_parser(name)
        p.add_argument("--config", required=True)
        p.add_argument("--no-post", action="store_true", help="print the report instead of commenting")
    args = parser.parse_args(argv)
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")  # Windows consoles default to cp1252

    if args.mode == "check":
        files = [Path(f) for pattern in args.files for f in sorted(glob.glob(pattern))]
        tapir = Tapir(args.port)
        rows, version = run_checks(tapir, files)
        print(report(rows, args.ac, version, "local"))
        return 0 if all(r["ok"] is not False for r in rows) else 1

    cfg = json.loads(Path(args.config).read_text(encoding="utf-8"))
    if args.mode == "once":
        once(cfg, not args.no_post)
        return 0
    while True:
        once(cfg, not args.no_post)
        time.sleep(cfg.get("pollMinutes", 15) * 60)


if __name__ == "__main__":
    sys.exit(main())
