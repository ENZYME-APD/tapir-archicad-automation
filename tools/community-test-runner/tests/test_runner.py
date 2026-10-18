"""Tests for the check engine, against a small in-memory Archicad.

FakeArchicad models what was measured live on AC26/AC28 in October 2026: `fixed=True` behaves like
the verified fix builds (#750-#753), `fixed=False` like release 1.7.0 for the two unmerged fixes
(a nonexistent guid counts as deleted and rolls back a real deletion listed before it; treadDepth
is ignored).
"""
from __future__ import annotations

import itertools
import sys
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
import tapir_test_runner as r  # noqa: E402

CHECKS = sorted((HERE.parent / "checks").glob("*.json"))


class FakeArchicad:
    def __init__(self, fixed=True, version="1.7.0", untitled=True):
        self.fixed, self.version, self.untitled = fixed, version, untitled
        self.ids = (f"G-{i:04d}" for i in itertools.count())
        self.elements: dict[str, dict] = {}
        self.stories = [{"index": 0, "name": "S0", "level": 0.0}]
        self.act = 0
        self.calls: list[str] = []

    def _new(self, kind, **data):
        guid = next(self.ids)
        self.elements[guid] = dict(data, kind=kind)
        return {"elementId": {"guid": guid}}

    def __call__(self, command, params=None, timeout=None):
        p = params or {}
        self.calls.append(command)
        if command == "GetAddOnVersion":
            return {"version": self.version}
        if command == "API.GetProductInfo":
            return {"version": 28, "buildNumber": 7006}
        if command == "GetProjectInfo":
            return {"isUntitled": self.untitled}
        if command == "GetElementsByType":
            return {"elements": [{"elementId": {"guid": g}} for g, e in self.elements.items() if e["kind"] == p["elementType"]]}
        if command == "CreateWalls":
            return {"elements": [self._new("Wall") for _ in p["wallsData"]]}
        if command == "DeleteElements":
            guids = [e["elementId"]["guid"] for e in p["elements"]]
            if self.fixed:
                results = []
                for g in guids:
                    if g in self.elements:
                        del self.elements[g]
                        results.append({"success": True})
                    else:
                        results.append({"success": False, "error": {"code": -2130313115}})
                return {"executionResults": results}
            # 1.7.0: a missing guid after a real one rolls the whole call back; missing guids count as gone
            first_missing = next((i for i, g in enumerate(guids) if g not in self.elements), None)
            rolled_back = first_missing is not None and first_missing > 0
            if not rolled_back:
                for g in guids:
                    self.elements.pop(g, None)
            return {"executionResults": [{"success": g not in self.elements} if not rolled_back or g not in self.elements
                                         else {"success": False, "error": {"code": -2130313115}} for g in guids]}
        if command == "SetStories":
            self.stories = [{"index": s.get("index", i), "name": s["name"], "level": s["level"]} for i, s in enumerate(p["stories"])]
            self.act = min(self.act, len(self.stories) - 1)
            return {"success": True}
        if command == "GetStories":
            return {"actStory": self.act, "stories": self.stories}
        if command == "ChangeWindow":
            i = p.get("storyIndex")
            if i is None or not any(s["index"] == i for s in self.stories):
                return {"success": False, "error": {"code": -2130313114}}
            self.act = i
            return {"success": True}
        if command == "CreateStairs":
            out = []
            for s in p["stairsData"]:
                going = s.get("treadDepth", 0.3) if self.fixed else 0.32
                run = (s.get("stepNum", 16) - 1) * going + 0.65
                b = s["baseLinePoints"][0]
                out.append(self._new("Stair", box={"xMin": b["x"], "xMax": b["x"] + run, "yMin": b["y"] - 1, "yMax": b["y"],
                                                   "zMin": 0, "zMax": s["totalHeight"]}))
            return {"elements": out}
        if command == "Get3DBoundingBoxes":
            return {"boundingBoxes3D": [{"boundingBox3D": self.elements[e["elementId"]["guid"]]["box"]} for e in p["elements"]]}
        if command == "CreateTexts":
            return {"elements": [self._new("Text", text=t["text"], height=t["height"]) for t in p["textsData"]]}
        if command == "CreateLabels":
            return {"elements": [self._new("Label", text=lb["text"], style={"height": 2.5}) for lb in p["labelsData"]]}
        if command == "ModifyLabels":
            for item in p["labelsWithDetails"]:
                element = self.elements[item["elementId"]["guid"]]
                element["text"] = item.get("text", element["text"])
                element["style"] = dict(element["style"], **item.get("style", {}))
            return {"executionResults": [{"success": True} for _ in p["labelsWithDetails"]]}
        if command == "CreateRoofs":
            return {"elements": [self._new("Roof", thickness=rf["thickness"] if rf.get("structureType") == "Basic" else 0.345)
                                 for rf in p["roofsData"]]}
        if command == "GetDetailsOfElements":
            return {"detailsOfElements": [{"details": {k: v for k, v in self.elements[e["elementId"]["guid"]].items() if k != "kind"}}
                                          for e in p["elements"]]}
        if command == "SetDetailsOfElements":
            for item in p["elementsWithDetails"]:
                self.elements[item["elementId"]["guid"]].update(item["details"].get("typeSpecificDetails", {}))
            return {"executionResults": [{"success": True} for _ in p["elementsWithDetails"]]}
        if command == "ModifyTexts":
            for item in p["textsWithDetails"]:
                self.elements[item["elementId"]["guid"]]["text"] = item["text"]
            return {"executionResults": [{"success": True} for _ in p["textsWithDetails"]]}
        raise AssertionError(f"FakeArchicad does not know {command}")


class EngineTest(unittest.TestCase):
    def test_resolve_paths_and_wildcards(self):
        data = {"elements": [{"elementId": {"guid": "a"}}, {"elementId": {"guid": "b"}}], "x": {"y": [0, 5]}}
        self.assertEqual(r.resolve(data, "x.y[1]"), 5)
        self.assertEqual(r.resolve(data, "elements[*].elementId.guid"), ["a", "b"])
        with self.assertRaises(KeyError):
            r.resolve(data, "x.z")

    def test_substitute_keeps_types(self):
        v = {"n": 3, "g": "G-1"}
        self.assertEqual(r.substitute({"a": "${n}", "b": "id ${g}", "c": ["${g}"]}, v), {"a": 3, "b": "id G-1", "c": ["G-1"]})

    def test_expressions_allow_arithmetic_only(self):
        self.assertAlmostEqual(r.evaluate("(${a} - ${b}) * 2", {"a": 5, "b": 1.5}), 7.0)
        for hostile in ("__import__('os')", "${a}.real", "open('x')", "[1][0]"):
            with self.assertRaises(ValueError):
                r.evaluate(hostile, {"a": 1})

    def test_created_elements_are_deleted_even_after_a_failure(self):
        fake = FakeArchicad()
        spec = {"steps": [{"command": "CreateWalls", "params": {"wallsData": [{}]}},
                          {"command": "Unknown"}]}
        with self.assertRaises(AssertionError):
            r.CheckRun(fake, spec).run()
        self.assertEqual(fake.calls[-1], "DeleteElements")
        self.assertEqual(fake.elements, {})

    def test_refuses_a_real_project(self):
        rows, _ = r.run_checks(FakeArchicad(untitled=False), CHECKS[:1])
        self.assertEqual(len(rows), 1)
        self.assertFalse(rows[0]["ok"])
        self.assertIn("not untitled", rows[0]["detail"])

    def test_skips_checks_for_newer_tapir(self):
        rows, _ = r.run_checks(FakeArchicad(version="1.5.9"), [c for c in CHECKS if "story_index" in c.name])
        self.assertEqual([row["ok"] for row in rows], [None])

    def test_report_first_line_is_machine_readable(self):
        rows = [{"check": "c", "step": "s", "what": "w", "ok": True, "detail": "d"},
                {"check": "c", "step": "s", "what": "w", "ok": None, "detail": "skipped"}]
        first = r.report(rows, "28", "1.7.0", "abcdef123").splitlines()[0]
        self.assertEqual(first, "tapir-test: AC28 PASS 1/1 build abcdef1 tapir 1.7.0")


class ChecksTest(unittest.TestCase):
    def run_all(self, fake):
        rows, _ = r.run_checks(fake, CHECKS)
        return rows

    def test_every_check_passes_on_the_fixed_behaviour(self):
        rows = self.run_all(FakeArchicad(fixed=True))
        failed = [f"{x['check']} / {x['step']} / {x['what']}: {x['detail']}" for x in rows if x["ok"] is False]
        self.assertEqual(failed, [])
        self.assertGreater(len(rows), 40)

    def test_checks_catch_the_unfixed_behaviour(self):
        rows = self.run_all(FakeArchicad(fixed=False))
        failing = {x["check"] for x in rows if x["ok"] is False}
        self.assertEqual(failing, {"DeleteElements with a nonexistent guid (#749)", "CreateStairs honours treadDepth (#425)"})

    def test_checks_leave_no_elements_behind(self):
        fake = FakeArchicad(fixed=True)
        self.run_all(fake)
        self.assertEqual(fake.elements, {})


if __name__ == "__main__":
    unittest.main()
