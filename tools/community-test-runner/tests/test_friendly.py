"""Tests for setup-free installation (Archicad's add-on list), the recorder and the short output."""
from __future__ import annotations

import itertools
import json
import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
sys.path.insert(0, str(HERE))
import runner_record as rec  # noqa: E402
import runner_windows as rw  # noqa: E402
import tapir_test_runner as r  # noqa: E402
from test_runner import FakeArchicad  # noqa: E402

KEY = r"Software\GRAPHISOFT\Archicad\Archicad 30.0.0 INT R1\Add-On Manager"


class FakeRegistry(rw.Registry):
    def __init__(self, keys: dict[str, dict[str, list]]):
        self.keys = {k: {n: list(v) for n, v in values.items()} for k, values in keys.items()}

    def subkeys(self, path):
        prefix = path + "\\"
        return sorted({k[len(prefix):].split("\\")[0] for k in self.keys if k.startswith(prefix)})

    def read(self, path):
        return {n: list(v) for n, v in self.keys.get(path, {}).items()}

    def write(self, path, values):
        self.keys[path] = {n: list(v) for n, v in values.items()}


class AddOnListTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        root = Path(self.tmp.name)
        self.addons = root / "Add-Ons"
        (self.addons / "Tapir").mkdir(parents=True)
        (self.addons / "Tapir" / "TapirAddOn_AC30_Win.apx").write_bytes(b"release")
        self.own = root / "dev" / "TapirAddOn_AC30_Win.apx"
        self.other = root / "Other.apx"
        self.registry = FakeRegistry({
            KEY: {"Actual Path": [str(self.addons), 1]},
            KEY + "\\Include": rw.numbered("Include", [rw.to_url(self.other), rw.to_url(self.own)]),
            KEY + "\\Disable": rw.numbered("Disable", []),
            r"Software\GRAPHISOFT\Archicad\Archicad 29.0.0 INT R1\Add-On Manager\Include": rw.numbered("Include", [])})
        self.original = json.dumps(self.registry.keys, sort_keys=True)
        self.list = rw.AddOnList("30", self.registry, root)

    def tearDown(self):
        self.tmp.cleanup()

    def test_finds_the_version_key_and_its_tapirs(self):
        self.assertEqual(rw.manager_key(self.registry, "30"), KEY)
        self.assertEqual(set(self.list.tapirs()), {self.own, self.addons / "Tapir" / "TapirAddOn_AC30_Win.apx"})

    def test_install_loads_only_the_build_and_restore_puts_everything_back(self):
        build = Path(self.tmp.name) / "runner" / "TapirAddOn_AC30_Win.apx"
        self.list.install(build)
        include = rw.entries(self.registry.read(KEY + "\\Include"))
        disable = rw.entries(self.registry.read(KEY + "\\Disable"))
        self.assertEqual(include, [rw.to_url(self.other), rw.to_url(build)])
        self.assertEqual(disable, [rw.to_url(self.addons / "Tapir" / "TapirAddOn_AC30_Win.apx")])
        self.assertEqual(self.registry.read(KEY + "\\Include")["Include Number"][0], "2")
        with self.assertRaises(RuntimeError):  # a second install before restoring would lose the backup
            self.list.install(build)
        self.assertTrue(self.list.restore())
        self.assertEqual(json.dumps(self.registry.keys, sort_keys=True), self.original)
        self.assertFalse(self.list.restore())

    def test_restore_works_from_a_new_process(self):
        self.list.install(Path(self.tmp.name) / "b.apx")
        fresh = rw.AddOnList("30", self.registry, Path(self.tmp.name))
        self.assertTrue(fresh.restore())
        self.assertEqual(json.dumps(self.registry.keys, sort_keys=True), self.original)

    def test_urls_round_trip(self):
        path = Path(r"C:\Users\A B\Tapir #1\TapirAddOn_AC30_Win.apx")
        self.assertEqual(rw.to_url(path), "lan.flat:///C:/Users/A%20B/Tapir%20%231/TapirAddOn_AC30_Win.apx")
        self.assertEqual(rw.from_url(rw.to_url(path)), path)


def tapir_call(command, params):
    return {"command": "API.ExecuteAddOnCommand", "parameters": {
        "addOnCommandId": {"commandNamespace": "TapirCommand", "commandName": command}, "addOnCommandParameters": params}}


class UuidArchicad(FakeArchicad):
    def __init__(self, **kw):
        super().__init__(**kw)
        self.ids = (f"00000000-0000-0000-0000-{i:012d}" for i in itertools.count(1))


class RecorderTest(unittest.TestCase):
    def recorded(self):
        g = "AAAAAAAA-0000-0000-0000-000000000001"
        wall = {"wallsData": [{"begCoordinate": {"x": 0, "y": 0}, "endCoordinate": {"x": 3, "y": 0}}]}
        return [
            (tapir_call("GetAddOnVersion", {}), {"succeeded": True, "result": {"addOnCommandResponse": {"version": "1.7.2"}}}),
            (tapir_call("CreateWalls", wall), {"succeeded": True, "result": {"addOnCommandResponse": {"elements": [{"elementId": {"guid": g}}]}}}),
            (tapir_call("GetElementsByType", {"elementType": "Wall"}),
             {"succeeded": True, "result": {"addOnCommandResponse": {"elements": [{"elementId": {"guid": g}}]}}}),
            (tapir_call("DeleteElements", {"elements": [{"elementId": {"guid": g}}]}),
             {"succeeded": True, "result": {"addOnCommandResponse": {"executionResults": [{"success": True}]}}}),
            ({"command": "API.ExecuteAddOnCommand", "parameters": {"addOnCommandId": {"commandNamespace": "Other", "commandName": "X"}}},
             {"succeeded": True, "result": {}}),
        ]

    def test_builds_steps_with_saved_guids_and_expectations(self):
        check = rec.build_check(self.recorded(), "walls")
        steps = check["steps"]
        self.assertEqual([s["command"] for s in steps], ["CreateWalls", "GetElementsByType", "DeleteElements"])
        self.assertEqual(steps[0]["save"], {"g1": "elements[0].elementId.guid"})
        self.assertNotIn("creates", steps[0])
        self.assertFalse(steps[1]["creates"])
        self.assertIn({"path": "elements", "len": 1}, steps[1]["expect"])
        self.assertEqual(steps[2]["params"], {"elements": [{"elementId": {"guid": "${g1}"}}]})
        self.assertIn({"path": "executionResults[0].success", "eq": True}, steps[2]["expect"])

    def test_a_recorded_check_replays(self):
        rows = r.CheckRun(UuidArchicad(), rec.build_check(self.recorded(), "walls")).run()
        self.assertTrue(rows)
        self.assertTrue(all(row["ok"] for row in rows), rows)


class OutputTest(unittest.TestCase):
    rows = [{"check": "c", "step": "s", "what": "w", "ok": True, "detail": "d"},
            {"check": "c", "step": "s2", "what": "x.y", "ok": False, "detail": "1 (want == 2)"}]

    def test_brief_is_the_verdict_and_failures(self):
        self.assertEqual(r.brief(self.rows, "30", "1.7.2", "abcdef123"),
                         "AC30 FAIL 1/2 build abcdef1 tapir 1.7.2\n  x c / s2 / x.y: 1 (want == 2)")

    def test_brief_caps_the_failures(self):
        many = [dict(self.rows[1], step=str(i)) for i in range(8)]
        self.assertTrue(r.brief(many, "30", "1", "s", max_failures=3).endswith("... 5 more failures"))

    def test_forks_need_trust(self):
        cfg = dict(r.DEFAULTS, trustedForks=["me"])
        pull = {"number": 1, "head": {"sha": "abc", "repo": {"full_name": "someone/tapir"}}}
        result = r.test_pull(None, cfg, pull, {"version": "30"}, "me", post=False)
        self.assertEqual(result["state"], "skipped")
        self.assertIn("someone's fork", result["text"])
        self.assertEqual(r.fork_owner(cfg, {"head": {"repo": None}}), "deleted fork")
        self.assertIsNone(r.fork_owner(cfg, {"head": {"repo": {"full_name": cfg["repo"]}}}))


if __name__ == "__main__":
    unittest.main()
