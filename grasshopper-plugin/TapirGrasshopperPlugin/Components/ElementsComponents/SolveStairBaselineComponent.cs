using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SolveStairBaselineComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "SolveStairBaseline";

        public SolveStairBaselineComponent()
            : base(
                "SolveStairBaseline",
                "Runs the Archicad stair solver on the given Stair baselines and parameters without creating any " +
                "element. Returns for each one whether the baseline was already correct, could be solved or could " +
                "not be solved, with the baseline and the main stair settings after solving. The inputs are the " +
                "same as those of CreateStairs; the settings not given come from the Stair tool defaults. " +
                "Available from Archicad 27.",
                GroupNames.ElementDetails)
        {
        }

        protected override void AddInputs()
        {
            inManager.AddPointParameter(
                "BaseLines",
                "BaseLines",
                "Base line points of each stair (one branch per stair, at least 2 points; only X and Y are used)."
                    .WithTypeName("pointTree"),
                GH_ParamAccess.tree);

            InNumbers(
                "ZCoordinates",
                "Z coordinate (absolute elevation) of the stair base (input only 1 to use the same value for all).");

            InIntegers(
                "FloorIndices",
                "Home story index of the stair; derived from the Z coordinate when not given " +
                "(input only 1 to use the same value for all). Optional.");

            InNumbers(
                "TotalHeights",
                "Total height of the stair (input only 1 to use the same value for all). Optional.");

            InNumbers(
                "FlightWidths",
                "Width of the stair flight (input only 1 to use the same value for all). Optional.");

            InIntegers(
                "StepNums",
                "Number of risers (input only 1 to use the same value for all). Optional.");

            InNumbers(
                "RiserHeights",
                "Height of each riser (input only 1 to use the same value for all). Optional.");

            InNumbers(
                "TreadDepths",
                "Depth (going) of each tread (input only 1 to use the same value for all). Optional.");

            InBooleans(
                "FinishVisible",
                "If false, the tread/riser finishes are hidden and only the stair structure is modeled " +
                "(input only 1 to use the same value for all). Optional.");

            SetOptionality(new[] { 2, 3, 4, 5, 6, 7, 8 });
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Solutions",
                "Result of the solver for each stair: Correct (the baseline was already correct), Solved or NotSolved.");

            OutCurves(
                "BaseLines",
                "Base line of each stair after solving, at its Z coordinate.");

            OutNumberList(
                "TotalHeights",
                "Total height of each stair after solving.");

            OutNumberList(
                "FlightWidths",
                "Flight width of each stair after solving.");

            OutIntegers(
                "StepNums",
                "Number of risers of each stair after solving.");

            OutNumberList(
                "RiserHeights",
                "Riser height of each stair after solving.");

            OutNumberList(
                "TreadDepths",
                "Tread depth of each stair after solving.");

            OutErrorMessages(
                "Error message of each stair (empty when the solver ran).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetTree(
                    0,
                    out GH_Structure<GH_Point> baseLines))
            {
                return;
            }

            var zCoordinates = da.OptionalList<double>(1);
            var floorIndices = da.OptionalList<int>(2);
            var totalHeights = da.OptionalList<double>(3);
            var flightWidths = da.OptionalList<double>(4);
            var stepNums = da.OptionalList<int>(5);
            var riserHeights = da.OptionalList<double>(6);
            var treadDepths = da.OptionalList<double>(7);
            var finishVisible = da.OptionalList<bool>(8);

            var count = baseLines.Branches.Count;
            if (count == 0)
            {
                return;
            }

            if (zCoordinates.Count == 0)
            {
                this.AddError("ZCoordinates is required.");
                return;
            }

            if (!this.CheckSizes(
                    count,
                    "BaseLines (branch count)",
                    ("ZCoordinates", zCoordinates.Count),
                    ("FloorIndices", floorIndices.Count),
                    ("TotalHeights", totalHeights.Count),
                    ("FlightWidths", flightWidths.Count),
                    ("StepNums", stepNums.Count),
                    ("RiserHeights", riserHeights.Count),
                    ("TreadDepths", treadDepths.Count),
                    ("FinishVisible", finishVisible.Count)))
            {
                return;
            }

            var stairsData = new JArray();
            for (var i = 0; i < count; i++)
            {
                var points = new JArray();
                foreach (var point in baseLines.Branches[i])
                {
                    if (point == null)
                    {
                        continue;
                    }

                    points.Add(new JObject { ["x"] = point.Value.X, ["y"] = point.Value.Y });
                }

                if (points.Count < 2)
                {
                    this.AddError($"Branch {i} of BaseLines has fewer than 2 points.");
                    return;
                }

                var stair = new JObject
                {
                    ["baseLinePoints"] = points,
                    ["zCoordinate"] = zCoordinates.At(i)
                };

                if (floorIndices.Count > 0)
                {
                    stair["floorIndex"] = floorIndices.At(i);
                }

                if (totalHeights.Count > 0)
                {
                    stair["totalHeight"] = totalHeights.At(i);
                }

                if (flightWidths.Count > 0)
                {
                    stair["flightWidth"] = flightWidths.At(i);
                }

                if (stepNums.Count > 0)
                {
                    stair["stepNum"] = stepNums.At(i);
                }

                if (riserHeights.Count > 0)
                {
                    stair["riserHeight"] = riserHeights.At(i);
                }

                if (treadDepths.Count > 0)
                {
                    stair["treadDepth"] = treadDepths.At(i);
                }

                if (finishVisible.Count > 0)
                {
                    stair["finishVisible"] = finishVisible.At(i);
                }

                stairsData.Add(stair);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["stairsData"] = stairsData },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var solutions = new List<string>();
            var curves = new List<Curve>();
            var solvedTotalHeights = new List<double?>();
            var solvedFlightWidths = new List<double?>();
            var solvedStepNums = new List<int?>();
            var solvedRiserHeights = new List<double?>();
            var solvedTreadDepths = new List<double?>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "solutions");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                if (JsonOutputHelp.IsError(item))
                {
                    solutions.Add(null);
                    curves.Add(null);
                    solvedTotalHeights.Add(null);
                    solvedFlightWidths.Add(null);
                    solvedStepNums.Add(null);
                    solvedRiserHeights.Add(null);
                    solvedTreadDepths.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                solutions.Add(item["solution"]?.ToString());
                curves.Add(
                    PolygonHelp.ToCurve(
                        item["baseLinePoints"],
                        item["baseLineArcs"],
                        false,
                        zCoordinates.At(i)));
                solvedTotalHeights.Add(item.Value<double?>("totalHeight"));
                solvedFlightWidths.Add(item.Value<double?>("flightWidth"));
                solvedStepNums.Add(item.Value<int?>("stepNum"));
                solvedRiserHeights.Add(item.Value<double?>("riserHeight"));
                solvedTreadDepths.Add(item.Value<double?>("treadDepth"));
                errors.Add("");
            }

            da.SetDataList(0, solutions);
            da.SetDataList(1, curves);
            da.SetDataList(2, solvedTotalHeights);
            da.SetDataList(3, solvedFlightWidths);
            da.SetDataList(4, solvedStepNums);
            da.SetDataList(5, solvedRiserHeights);
            da.SetDataList(6, solvedTreadDepths);
            da.SetDataList(7, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SolveStairBaseline;

        public override Guid ComponentGuid =>
            new Guid("3a904b89-5905-4fb4-8309-153775f8460b");
    }
}
