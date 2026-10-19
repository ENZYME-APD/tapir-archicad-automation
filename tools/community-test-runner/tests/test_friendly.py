"""Tests for setup-free installation (Archicad's add-on list), the recorder and the short output."""
from __future__ import annotations

import itertools
import json
import sys
import tempfile
import unittest
import unittest.mock
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

    def collision_list(self, version: str, in_folder: bool) -> rw.AddOnList:
        key = KEY.replace("30.0.0 INT", f"{version}.0.0 GER")
        folder = Path(self.tmp.name) / f"AC{version}"
        (folder / "Sonstiges").mkdir(parents=True)
        if in_folder:
            (folder / "Sonstiges" / f"TapirAddOn_AC{version}_Win.apx").write_bytes(b"release")
        (folder / "Sonstiges" / f"TapirAddOn_AC{version}_Win.apx.bak-1").write_bytes(b"old")
        self.registry.keys[key] = {"Actual Path": [str(folder), 1]}
        self.registry.keys[key + "\\Include"] = rw.numbered("Include", [] if in_folder else [rw.to_url(self.own)])
        self.registry.keys[key + "\\Disable"] = rw.numbered("Disable", [])
        return rw.AddOnList(version, self.registry, Path(self.tmp.name))

    def test_up_to_ac28_a_tapir_in_the_addons_folder_is_swapped_not_listed(self):
        ac28 = self.collision_list("28", in_folder=True)
        self.assertEqual([p.name for p in ac28.swap_files()], ["TapirAddOn_AC28_Win.apx"])
        before = json.dumps(self.registry.keys, sort_keys=True)
        with self.assertRaisesRegex(RuntimeError, "addonFile"):
            ac28.install(Path(self.tmp.name) / "build.apx")
        self.assertEqual(json.dumps(self.registry.keys, sort_keys=True), before)
        self.assertFalse(ac28.backup_file.exists())
        self.assertEqual(self.list.swap_files(), [])  # the add-on list worked on AC30 (#1088)

    def test_up_to_ac28_a_tapir_only_in_the_list_still_uses_the_list(self):
        ac26 = self.collision_list("26", in_folder=False)
        self.assertEqual(ac26.swap_files(), [])
        ac26.install(Path(self.tmp.name) / "build.apx")
        self.assertTrue(ac26.restore())

    def test_a_list_left_changed_by_1150_still_shows_the_file_to_swap(self):
        ac28 = self.collision_list("28", in_folder=True)
        tapir = ac28.swap_files()[0]
        key = KEY.replace("30.0.0 INT", "28.0.0 GER")
        # what the earlier runner left after the dialog: the folder Tapir on Disable, the backup pending
        ac28.backup_file.write_text(json.dumps({"key": key, "lists": {
            "Include": self.registry.read(key + "\\Include"), "Disable": self.registry.read(key + "\\Disable")}}))
        self.registry.write(key + "\\Disable", rw.numbered("Disable", [rw.to_url(tapir)]))
        self.assertEqual(ac28.swap_files(), [tapir])

    def test_every_tapir_in_the_addons_folder_is_found(self):
        ac28 = self.collision_list("28", in_folder=True)
        (Path(self.tmp.name) / "AC28" / "Tapir").mkdir()
        (Path(self.tmp.name) / "AC28" / "Tapir" / "TapirAddOn_AC28_Win.apx").write_bytes(b"second")
        self.assertEqual(len(ac28.swap_files()), 2)

    def test_restore_works_from_a_new_process(self):
        self.list.install(Path(self.tmp.name) / "b.apx")
        fresh = rw.AddOnList("30", self.registry, Path(self.tmp.name))
        self.assertTrue(fresh.restore())
        self.assertEqual(json.dumps(self.registry.keys, sort_keys=True), self.original)

    def test_urls_round_trip(self):
        path = Path(r"C:\Users\A B\Tapir #1\TapirAddOn_AC30_Win.apx")
        self.assertEqual(rw.to_url(path), "lan.flat:///C:/Users/A%20B/Tapir%20%231/TapirAddOn_AC30_Win.apx")
        self.assertEqual(rw.from_url(rw.to_url(path)), path)


class InstallationTest(unittest.TestCase):
    def setUp(self):
        self.patches = [unittest.mock.patch.object(r, "archicad_running", return_value=False),
                        unittest.mock.patch.object(r, "archicad_pids", return_value={1}),
                        unittest.mock.patch.object(r.subprocess, "Popen"),
                        unittest.mock.patch.object(r.time, "sleep")]
        for p in self.patches:
            p.start()
        self.ac = {"version": "26", "archicadExe": "Archicad.exe", "blankTemplate": "t.tpl", "startTimeout": 30,
                   "addonFile": str(Path(tempfile.gettempdir()) / "TapirAddOn_AC26_Win.apx")}

    def tearDown(self):
        for p in self.patches:
            p.stop()

    def test_a_dialog_stops_the_start_and_is_named(self):
        with unittest.mock.patch.object(r, "find_port", return_value=None), \
                unittest.mock.patch.object(r.os, "name", "nt"), \
                unittest.mock.patch.object(rw, "dialogs", return_value=["Archicad-Umgebung"]) as dialogs, \
                self.assertRaisesRegex(RuntimeError, "Archicad-Umgebung.*1150"):
            r.Installation(self.ac).start()
        self.assertEqual(dialogs.call_count, 3)  # three polls in a row, not the whole startTimeout

    def test_no_administrator_rights_is_said_plainly(self):
        with unittest.mock.patch.object(r.shutil, "copy2", side_effect=PermissionError(13, "Access is denied")), \
                self.assertRaisesRegex(RuntimeError, "administrator"):
            r.Installation(self.ac).install(Path("build.apx"))


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
