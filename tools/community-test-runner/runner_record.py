"""'record': writes a check file from the commands sent through it, so nobody has to write JSON paths.

Point a script, Grasshopper or an MCP tool at the recording port instead of Archicad's (19800 instead
of 19723, for example); every command is passed on to Archicad unchanged. On Ctrl+C the runner
writes one step per command: element guids an answer returns are saved as ${g1}, ${g2}, ... and used
in the later steps' parameters, and the answer's other values become expectations. Delete the
expectations that do not matter, and run the file with 'check' to see it pass.

Record in an empty, untitled project, as the checks run in one.
"""
from __future__ import annotations

import json
import re
import threading
import urllib.request
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any

GUID = re.compile(r"^[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}$")
SKIP = {"GetAddOnVersion", "API.GetProductInfo", "API.IsAlive", "GetProjectInfo"}  # what tools ask on connect
MAX_EXPECT = 12


def unwrap(request: dict, answer: dict) -> tuple[str, dict, Any] | None:
    """(check command, parameters, answer) of one JSON server call; None for other add-ons' commands."""
    command, params = request.get("command", ""), request.get("parameters") or {}
    if command == "API.ExecuteAddOnCommand":
        cid = params.get("addOnCommandId") or {}
        if cid.get("commandNamespace") != "TapirCommand":
            return None
        result = (answer.get("result") or {}).get("addOnCommandResponse", {})
        return cid.get("commandName", ""), params.get("addOnCommandParameters") or {}, result
    return command, params, answer.get("result") or {}


def leaves(value: Any, path: str = "") -> list[tuple[str, Any]]:
    """Every (path, scalar) of an answer, plus (path, list) for each list, in the runner's path syntax."""
    if isinstance(value, dict):
        return [item for k, v in value.items() for item in leaves(v, f"{path}.{k}" if path else k)]
    if isinstance(value, list):
        return [(path, value)] + [item for i, v in enumerate(value) for item in leaves(v, f"{path}[{i}]")]
    return [(path, value)]


def replace_guids(value: Any, names: dict[str, str]) -> Any:
    if isinstance(value, str):
        return "${" + names[value] + "}" if value in names else value
    if isinstance(value, list):
        return [replace_guids(v, names) for v in value]
    if isinstance(value, dict):
        return {k: replace_guids(v, names) for k, v in value.items()}
    return value


def expectation(path: str, value: Any) -> dict | None:
    if isinstance(value, list):
        return {"path": path, "len": len(value)}
    if isinstance(value, bool) or value is None or isinstance(value, int):
        return {"path": path, "eq": value} if value is not None else None
    if isinstance(value, float):
        return {"path": path, "approx": round(value, 6), "tol": 1e-4}
    if isinstance(value, str):
        return {"path": path, "eq": value}
    return None


def build_check(calls: list[tuple[dict, dict]], title: str = "") -> dict:
    """A check file from recorded (request, answer) pairs."""
    names: dict[str, str] = {}
    steps = []
    for request, answer in calls:
        unwrapped = unwrap(request, answer)
        if unwrapped is None or unwrapped[0] in SKIP:
            continue
        command, params, result = unwrapped
        step: dict[str, Any] = {"id": f"{len(steps) + 1} {command}", "command": command,
                                "params": replace_guids(params, names)}
        if not answer.get("succeeded", True):
            step["allowEnvelopeError"] = True
            steps.append(step)
            continue
        save, expect = {}, []
        for path, value in leaves(result):
            if isinstance(value, str) and GUID.match(value):
                if value not in names:
                    names[value] = f"g{len(names) + 1}"
                    save[names[value]] = path
            elif path and len(expect) < MAX_EXPECT and (rule := expectation(path, value)):
                expect.append(rule)
        creates = command.split(".")[-1].startswith(("Create", "Place", "Duplicate", "Copy", "Add"))
        if not creates:
            step["creates"] = False
        if save:
            step["save"] = save
        if expect:
            step["expect"] = expect
        steps.append(step)
    return {"title": title or "Recorded check", "minTapir": "1.0.0", "steps": steps}


def forwarder(target: str, calls: list, lock: threading.Lock):
    class Handler(BaseHTTPRequestHandler):
        def do_POST(self):  # noqa: N802 - the http.server name
            body = self.rfile.read(int(self.headers.get("Content-Length") or 0))
            upstream = urllib.request.Request(target, data=body, headers={"Content-Type": "application/json"}, method="POST")
            with urllib.request.urlopen(upstream, timeout=600) as response:
                payload = response.read()
            try:
                with lock:
                    calls.append((json.loads(body), json.loads(payload)))
                    print(f"  recorded {len(calls)}: {calls[-1][0].get('command')}"
                          f" {((calls[-1][0].get('parameters') or {}).get('addOnCommandId') or {}).get('commandName', '')}")
            except ValueError:
                pass
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)

        def log_message(self, *args):
            pass
    return Handler


def run(out: str, port: int | None, listen: int, title: str) -> int:
    if port is None:
        raise SystemExit("no Archicad with Tapir is open; open one with an empty, untitled project first")
    calls: list[tuple[dict, dict]] = []
    server = ThreadingHTTPServer(("127.0.0.1", listen), forwarder(f"http://127.0.0.1:{port}", calls, threading.Lock()))
    print(f"Recording: send your commands to port {listen} (Archicad is on {port}). Ctrl+C to finish.")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
    check = build_check(calls, title)
    Path(out).write_text(json.dumps(check, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {out}: {len(check['steps'])} step(s). Remove the expectations that don't matter, then:\n"
          f"  python tapir_test_runner.py check {out}")
    return 0
