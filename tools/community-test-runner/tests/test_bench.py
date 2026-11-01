"""Tests for the benchmark (runner_bench) against a fake Archicad: seeding, cases, tokens, comparing."""
from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
sys.path.insert(0, str(HERE))
import runner_bench as b  # noqa: E402
from test_runner import FakeArchicad  # noqa: E402

GUID = "6A0F5F6E-27B4-4A2C-9E3B-2B2C3C4D5E6F"


class BenchArchicad(FakeArchicad):
    """Knows the commands of the standard suite; everything it does not know answers with an error."""

    def __call__(self, command, params=None, timeout=None):
        p = params or {}
        for kind, key in (("Column", "columnsData"), ("Slab", "slabsData")):
            if command == f"Create{kind}s":
                self.calls.append(command)
                return {"elements": [self._new(kind) for _ in p[key]]}
        if command == "GetAllElements":
            self.calls.append(command)
            return {"elements": [{"elementId": {"guid": g}} for g in self.elements]}
        if command in ("API.IsAlive", "GetStories", "GetLibraries", "GetAttributesByType", "GetFavoritesByType", "GetAllProperties"):
            self.calls.append(command)
            return {"items": [{"guid": GUID, "name": "x"}] * 3}
        if command == "API.GetPropertyIds":
            return {"properties": [{"propertyId": {"guid": GUID}}] * len(p["properties"])}
        if command in ("API.GetTypesOfElements", "GetDetailsOfElements", "GetPropertyValuesOfElements", "Get3DBoundingBoxes"):
            self.calls.append(command)
            return {"items": [{"elementId": e["elementId"], "value": 1.5} for e in p["elements"]]}
        if command == "MoveElements":
            return {"executionResults": [{"success": True} for _ in p["elementsWithMoveVectors"]]}
        if command == "GetProjectInfo":
            return {"isUntitled": self.untitled}
        if command in ("GetAddOnVersion", "API.GetProductInfo", "GetElementsByType", "CreateWalls", "DeleteElements"):
            return super().__call__(command, params, timeout)
        return {"__envelopeError__": {"message": f"unknown command {command}"}}


def suite(**changes) -> dict:
    data = json.loads(b.DEFAULT_SUITE.read_text(encoding="utf-8"))
    data.update(repeat=2, warmup=0, scales=[10, 40], **changes)
    for spec in data["stress"]:
        spec.update(calls=6, concurrency=[1, 3])
    return data


class BenchTest(unittest.TestCase):
    def run_bench(self, archicad=None, **kwargs):
        archicad = archicad or BenchArchicad()
        logs: list[str] = []
        results = b.Bench(archicad, suite(), logs.append).run(**kwargs)
        return archicad, results, logs

    def test_seeds_each_scale_measures_every_case_and_cleans_up(self):
        archicad, results, _ = self.run_bench()
        self.assertEqual(archicad.elements, {}, "everything made is deleted again")
        made = [s for s in results["seed"] if s["command"] != "DeleteElements"]
        self.assertEqual(sum(s["created"] for s in made), 40)
        self.assertEqual({s["command"] for s in made}, {"CreateColumns", "CreateWalls", "CreateSlabs"})
        details = {c["scale"]: c for c in results["cases"] if c["id"] == "details-full"}
        self.assertEqual((details[10]["elements"], details[40]["elements"]), (10, 40))
        self.assertGreater(details[40]["tokens"], details[10]["tokens"])
        self.assertEqual(details[40]["runs"], 2)
        self.assertEqual(len(results["stress"]), 4)

    def test_quantities_unknown_to_the_fake_is_an_error_row_not_a_stop(self):
        _, results, logs = self.run_bench()
        errors = [c for c in results["cases"] if c.get("error")]
        self.assertEqual({c["id"] for c in errors}, {"quantities"})
        self.assertTrue(any("quantities@10: ERROR" in line for line in logs))

    def test_refuses_a_project_that_is_not_empty(self):
        archicad = BenchArchicad(untitled=False)
        with self.assertRaises(RuntimeError):
            b.Bench(archicad, suite()).run()
        self.assertNotIn("CreateColumns", archicad.calls)

    def test_read_only_creates_nothing_and_skips_writes(self):
        archicad = BenchArchicad(untitled=False)
        for _ in range(25):
            archicad._new("Wall")
        _, results, _ = self.run_bench(archicad, read_only=True)
        self.assertEqual(len(archicad.elements), 25)
        self.assertNotIn("CreateColumns", archicad.calls)
        self.assertEqual(results["meta"]["scales"], [10, 25])
        self.assertFalse([c for c in results["cases"] if c["id"] == "move"])

    def test_stop_still_deletes_what_was_made(self):
        archicad = BenchArchicad()
        calls = iter(range(10_000))
        b.Bench(archicad, suite(), lambda _t: None, stop=lambda: next(calls) > 12).run()
        self.assertEqual(archicad.elements, {})

    def test_mcp_page_is_the_first_hundred_items(self):
        result = {"elements": [{"elementId": {"guid": GUID}}] * 250}
        page = json.loads(b.mcp_text(result, True))
        self.assertEqual(len(page["elements"]), 100)
        self.assertIn("next_page_token", page)
        self.assertLess(b.estimate_tokens(b.mcp_text(result, True)), b.estimate_tokens(b.mcp_text(result, False)) / 2)

    def test_fill_expressions_and_each(self):
        v = {"x": 2.0, "y": 4.0, "elements": [{"elementId": {"guid": "a"}}]}
        self.assertEqual(b.fill({"x": "$x+0.8", "y": "$y"}, v), {"x": 2.8, "y": 4.0})
        self.assertEqual(b.fill({"$each": "elements", "as": {"id": "$item.elementId"}}, v), [{"id": {"guid": "a"}}])

    def test_save_report_and_compare_with_the_last_run(self):
        with tempfile.TemporaryDirectory() as tmp:
            logs: list[str] = []
            first = b.run_and_save(BenchArchicad(), log=logs.append, scales=[10], repeat=1, out=Path(tmp))
            data = json.loads(first.read_text(encoding="utf-8"))
            self.assertTrue(first.with_suffix(".md").read_text(encoding="utf-8").startswith("# tapir-bench: AC28 GER"))
            slower = json.loads(json.dumps(data))
            slower["meta"]["started"] = "2999-01-01T00:00:00"
            for c in slower["cases"]:
                if c["id"] == "details-full":
                    c["ms"]["p50"] += 100
                    c["tokens"] *= 2
            lines, worse = b.compare(data, slower)
            self.assertEqual(worse, 2)
            self.assertTrue(any("details-full@10 tokens" in line for line in lines))
            second = b.save(slower, Path(tmp))
            self.assertEqual(Path(b.previous(second)["_path"]).name, first.name)


if __name__ == "__main__":
    unittest.main()
