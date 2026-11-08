"""Tests for the macOS side: Archicad's add-on list in its preferences, finding Archicads, bundles."""
from __future__ import annotations

import json
import plistlib
import shutil
import subprocess
import sys
import tempfile
import unittest
import unittest.mock
import zipfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
import runner_macos as rm  # noqa: E402
import tapir_test_runner as r  # noqa: E402

DOMAIN = "com.graphisoft.AC-ARM 29.0.0 INT v1"
OTHER = "lan.null2:///Users/ben/Downloads/PointCloudDensity26.bundle"  # as Archicad 29 wrote it


class FakePreferences(rm.Preferences):
    def __init__(self, domains: dict[str, dict]):
        self.data = json.loads(json.dumps(domains))

    def domains(self):
        return sorted(self.data)

    def read(self, domain):
        return json.loads(json.dumps(self.data.get(domain, {})))

    def write(self, domain, key, value):
        self.data.setdefault(domain, {})[key] = json.loads(json.dumps(value))


class AddOnListTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        root = Path(self.tmp.name)
        self.addons = root / "Archicad 29 INT" / "Add-Ons"
        self.folder_tapir = self.addons / "TapirAddOn_AC29_Mac.bundle"
        (self.folder_tapir / "Contents" / "MacOS").mkdir(parents=True)
        (self.folder_tapir / "Contents" / "MacOS" / "TapirAddOn_AC29_Mac").write_bytes(b"release")
        self.prefs = FakePreferences({
            DOMAIN: {"Add-On Manager": {"Use Default Location": True, "Default Location": "%DEFAULT%",
                                        "Include": {"#1.": OTHER, "Include Number": 1},
                                        "Manager Dialog": {"SortListBy": 3}},
                     "Other Setting": 1},
            "com.graphisoft.startup-ARM.AC-ARM 29.0.0 INT v1": {"Last": 1},
            "com.graphisoft.AC-ARM 28.0.0 INT v1": {"Add-On Manager": {"Include": {"Include Number": 0}}}})
        self.original = json.dumps(self.prefs.data, sort_keys=True)
        self.list = rm.AddOnList("29", self.addons, self.prefs, root)
        self.build = root / "runner" / "TapirAddOn_AC29_Mac.bundle"

    def tearDown(self):
        self.tmp.cleanup()

    def manager(self) -> dict:
        return self.prefs.data[DOMAIN]["Add-On Manager"]

    def test_finds_the_version_domain_and_its_tapirs(self):
        self.assertEqual(rm.manager_domain(self.prefs, "29"), DOMAIN)
        self.assertIsNone(rm.manager_domain(self.prefs, "30"))
        self.assertEqual(self.list.tapirs(), [self.folder_tapir])

    def without_folder_tapir(self) -> Path:
        """The tester's Tapir only in the add-on list (a dev build), the case the list method serves."""
        shutil.rmtree(self.folder_tapir)
        own = Path("/Users/me/dev/TapirAddOn_AC29_Mac.bundle")
        self.manager()["Include"] = rm.numbered("Include", [OTHER, rm.to_url(own)])
        self.original = json.dumps(self.prefs.data, sort_keys=True)
        return own

    def test_install_loads_only_the_build_and_restore_puts_everything_back(self):
        self.without_folder_tapir()
        self.list.install(self.build)
        self.assertEqual(rm.entries(self.manager()["Include"]), [OTHER, rm.to_url(self.build)])
        self.assertEqual(self.manager()["Include"]["Include Number"], 2)
        self.assertEqual(self.manager()["Manager Dialog"], {"SortListBy": 3})
        self.assertEqual(self.list.tapirs(), [self.build])
        with self.assertRaises(RuntimeError):  # a second install before restoring would lose the backup
            self.list.install(self.build)
        self.manager()["Manager Dialog"]["SortListBy"] = 1  # what Archicad changes meanwhile stays
        self.assertTrue(self.list.restore())
        self.manager()["Manager Dialog"]["SortListBy"] = 3
        self.assertEqual(json.dumps(self.prefs.data, sort_keys=True), self.original)  # no 'Disable' added
        self.assertFalse(self.list.restore())

    def test_restore_works_from_a_new_process(self):
        self.without_folder_tapir()
        self.list.install(self.build)
        fresh = rm.AddOnList("29", self.addons, self.prefs, Path(self.tmp.name))
        self.assertTrue(fresh.restore())
        self.assertEqual(json.dumps(self.prefs.data, sort_keys=True), self.original)

    def test_a_tapir_in_the_list_is_left_out_and_a_switched_off_one_stays_off(self):
        own = Path("/Users/me/dev/TapirAddOn_AC29_Mac.bundle")
        self.manager()["Include"] = rm.numbered("Include", [rm.to_url(own), OTHER])
        self.manager()["Disable"] = rm.numbered("Disable", [rm.to_url(self.folder_tapir)])
        self.assertEqual(self.list.tapirs(), [own])
        self.list.install(self.build)
        self.assertEqual(rm.entries(self.manager()["Include"]), [OTHER, rm.to_url(self.build)])
        self.assertEqual(rm.entries(self.manager()["Disable"]), [rm.to_url(self.folder_tapir)])
        self.list.restore()

    def test_a_tapir_in_the_addons_folder_is_swapped_not_listed_on_every_version(self):
        folder = Path(self.tmp.name) / "Archicad 28 INT" / "Add-Ons"
        (folder / "TapirAddOn_AC28_Mac.bundle").mkdir(parents=True)
        ac28 = rm.AddOnList("28", folder, self.prefs, Path(self.tmp.name))
        self.assertEqual([p.name for p in ac28.swap_files()], ["TapirAddOn_AC28_Mac.bundle"])
        before = json.dumps(self.prefs.data, sort_keys=True)
        with self.assertRaisesRegex(RuntimeError, "addonFile"):
            ac28.install(self.build)
        self.assertEqual(json.dumps(self.prefs.data, sort_keys=True), before)
        self.assertEqual(self.list.swap_files(), [self.folder_tapir])  # Archicad 29 on macOS too

    def test_an_archicad_never_started_is_said_plainly(self):
        with self.assertRaisesRegex(RuntimeError, "start it once"):
            rm.AddOnList("30", self.addons, self.prefs, Path(self.tmp.name)).install(self.build)

    def test_urls_round_trip(self):
        self.assertEqual(rm.to_url(self.folder_tapir.relative_to(self.tmp.name).as_posix()),
                         "lan.null2://Archicad%2029%20INT/Add-Ons/TapirAddOn_AC29_Mac.bundle")
        self.assertEqual(rm.from_url(rm.to_url(self.folder_tapir)), self.folder_tapir)
        self.assertEqual(rm.from_url(OTHER), Path("/Users/ben/Downloads/PointCloudDensity26.bundle"))

    def test_entries_are_in_number_order(self):
        urls = [f"lan.null2:///a/{i}.bundle" for i in range(1, 12)]
        self.assertEqual(rm.entries(rm.numbered("Include", urls)), urls)


class FindArchicadsTest(unittest.TestCase):
    def test_finds_the_app_and_its_template(self):
        with tempfile.TemporaryDirectory() as tmp:
            folder = Path(tmp) / "Archicad 29 INT"
            app = folder / "Archicad 29.app" / "Contents"
            (app / "MacOS").mkdir(parents=True)
            (app / "Info.plist").write_bytes(plistlib.dumps({"CFBundleExecutable": "Archicad"}))
            (app / "MacOS" / "Archicad").write_bytes(b"")
            (folder / "Defaults" / "Archicad").mkdir(parents=True)
            (folder / "Defaults" / "Archicad" / "Archicad 29 Template.tpl").write_bytes(b"")
            (Path(tmp) / "Archicad Monolith Migration Libraries INT").mkdir()
            found = rm.find_archicads(tmp)
            self.assertEqual(found, [{"version": "29", "archicadExe": str(app / "MacOS" / "Archicad"),
                                      "blankTemplate": str(folder / "Defaults" / "Archicad" / "Archicad 29 Template.tpl")}])
            self.assertEqual(rm.addon_list(found[0]).addons_folder, folder / "Add-Ons")


@unittest.skipUnless(sys.platform == "darwin", "ditto and xattr are macOS tools")
class BundleTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.own = self.root / "Add-Ons" / "TapirAddOn_AC29_Mac.bundle"
        (self.own / "Contents" / "MacOS").mkdir(parents=True)
        (self.own / "Contents" / "MacOS" / "TapirAddOn_AC29_Mac").write_bytes(b"own")
        subprocess.run(["xattr", "-w", "net.tristem.tapir.commit", "abc123", str(self.own)], check=True)
        self.build = self.root / "build" / "TapirAddOn_AC29_Mac.bundle"
        (self.build / "Contents" / "MacOS").mkdir(parents=True)
        (self.build / "Contents" / "MacOS" / "TapirAddOn_AC29_Mac").write_bytes(b"build")

    def tearDown(self):
        self.tmp.cleanup()

    def test_swapping_a_bundle_keeps_the_original_outside_addons_with_its_attributes(self):
        ac = {"version": "29", "archicadExe": "/x/Archicad 29.app/Contents/MacOS/Archicad", "addonFile": str(self.own)}
        with unittest.mock.patch.object(r, "host", rm), unittest.mock.patch.object(rm, "state_dir", return_value=self.root / "state"), \
                unittest.mock.patch.object(r, "archicad_running", return_value=False):
            install = r.Installation(ac)
            install.install(self.build)
            self.assertEqual(install.keep, self.root / "state" / "originals" / "AC29" / "TapirAddOn_AC29_Mac.bundle")
            self.assertEqual((self.own / "Contents" / "MacOS" / "TapirAddOn_AC29_Mac").read_bytes(), b"build")
            self.assertEqual(list((self.root / "Add-Ons").iterdir()), [self.own])
            self.assertTrue(install.pending())
            install.restore()
        self.assertFalse(install.pending())
        self.assertEqual((self.own / "Contents" / "MacOS" / "TapirAddOn_AC29_Mac").read_bytes(), b"own")
        attribute = subprocess.run(["xattr", "-p", "net.tristem.tapir.commit", str(self.own)], capture_output=True, text=True)
        self.assertEqual(attribute.stdout.strip(), "abc123")

    def test_unpacks_the_build_checks_zip_in_a_zip(self):
        inner = self.root / "TapirAddOn_AC29_Mac.zip"
        subprocess.run(["ditto", "-c", "-k", "--keepParent", str(self.build), str(inner)], check=True)
        artifact = self.root / "artifact.zip"
        with zipfile.ZipFile(artifact, "w") as zf:
            zf.write(inner, inner.name)
        dest = self.root / "dest"
        dest.mkdir()
        with zipfile.ZipFile(artifact) as zf:
            bundle = rm.unpack_build(zf, dest)
        self.assertEqual(bundle, dest / "TapirAddOn_AC29_Mac.bundle")
        self.assertEqual((bundle / "Contents" / "MacOS" / "TapirAddOn_AC29_Mac").read_bytes(), b"build")


if __name__ == "__main__":
    unittest.main()
