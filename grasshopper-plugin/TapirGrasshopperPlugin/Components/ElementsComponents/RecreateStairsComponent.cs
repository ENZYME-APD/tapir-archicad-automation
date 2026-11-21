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
    public class RecreateStairsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "RecreateStairs";

        public RecreateStairsComponent()
            : base(
                "RecreateStairs",
                "Recreate Stair elements on a new baseline, because the Archicad API cannot change the baseline " +
                "of an existing stair. The new baseline is given as points (BaseLines) or as a curve with arcs " +
                "for curved stairs (BaseLineCurves). The new stair gets the old one's settings, Element ID, " +
                "layer, story, classifications, categories, property values, solid element operations and " +
                "trims, and the old stair is deleted; the new stair has a new GUID. Each stair is recreated in " +
                "its own undo step, and when a step fails nothing is changed for that stair. Associative " +
                "labels, dimensions that measured the old stair and its group are not carried over; they are " +
                "listed in NotCarriedOver.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the stairs to recreate.");

            inManager.AddPointParameter(
                "BaseLines",
                "BaseLines",
                ("The new baseline points of each stair (one branch per stair, at least 2 points; only X and " +
                 "Y are used). Every edge is straight. Leave it empty when the baselines are given as curves " +
                 "in BaseLineCurves.")
                    .WithTypeName("pointTree"),
                GH_ParamAccess.tree);

            inManager.AddCurveParameter(
                "BaseLineCurves",
                "BaseLineCurves",
                "The new baseline of each stair as a curve, one curve per stair, instead of the BaseLines " +
                "points: a polyline, or a polycurve of lines and arcs for a curved or spiral stair (for example " +
                "the BaseLines output of SolveStairBaseline). Its corners become baseLinePoints and its arcs " +
                "baseLineArcs; any other curve is approximated with a polyline. Only X and Y are used. Archicad " +
                "keeps the circle of an arc but may shorten the flight to its risers, so the stair can be " +
                "shorter than the curve. Optional.",
                GH_ParamAccess.list);

            InTexts(
                "AdditionalSettings",
                "One JSON object per stair with further fields of the RecreateStairs item, merged into it: " +
                "baseLineEdges and baseLineVertices keep the landings and winders of an L or U stair, e.g. " +
                "{\"baseLineVertices\": [{\"turnType\": \"None\"}, {\"turnType\": \"Landing\"}, " +
                "{\"turnType\": \"None\"}]}. Input only 1 to use the same settings for all. Optional.");

            SetOptionality(new[] { 1, 2, 3 });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "The new stair of each input stair (null when it was not recreated).");

            OutGenerics(
                "OldElementGuids",
                "The stair that was deleted for each input stair (null when it was not recreated).");

            OutTextTree(
                "NotCarriedOver",
                "What was linked to the old stair and could not be moved to the new one, one branch per input " +
                "stair: Label (an associative label, deleted with the stair), Dimension or LevelDimension (a " +
                "dimension of the current database that measured it), each followed by the GUID it had, or " +
                "Group (the old stair was in a group; the new one is not).");

            OutErrorMessages(
                "Error message of each stair (empty when it was recreated). After an error nothing was changed " +
                "for that stair.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> elementWrappers) ||
                !this.TryGetElementIds(
                    elementWrappers,
                    "ElementGuids",
                    out List<JObject> elementIds))
            {
                return;
            }

            if (!da.TryGetTree(
                    1,
                    out GH_Structure<GH_Point> baseLines))
            {
                return;
            }

            var baseLineCurves = da.OptionalList<Curve>(2);
            if (baseLineCurves.Count > 0 && baseLines.DataCount > 0)
            {
                this.AddError("Give either the BaseLines or the BaseLineCurves input, not both.");
                return;
            }

            if (!TryReadBaseLines(
                    baseLines,
                    baseLineCurves,
                    out List<JArray> baseLinePoints,
                    out List<JArray> baseLineArcs))
            {
                return;
            }

            var count = elementIds.Count;
            if (count == 0)
            {
                return;
            }

            var baseLinesInputName = baseLineCurves.Count > 0 ? "BaseLineCurves" : "BaseLines (branch count)";
            if (baseLinePoints.Count == 0)
            {
                this.AddError("Give the BaseLines or the BaseLineCurves input.");
                return;
            }

            if (baseLinePoints.Count != count)
            {
                this.AddError(
                    $"The size of the input {baseLinesInputName} must be equal to the size of the input ElementGuids.");
                return;
            }

            var additionalSettings = da.OptionalList<string>(3);
            if (!this.CheckSizes(
                    count,
                    "ElementGuids",
                    ("AdditionalSettings", additionalSettings.Count)))
            {
                return;
            }

            var stairs = new JArray();
            for (var i = 0; i < count; i++)
            {
                var stair = new JObject
                {
                    ["elementId"] = elementIds[i],
                    ["baseLinePoints"] = baseLinePoints[i]
                };

                if (baseLineArcs[i].Count > 0)
                {
                    stair["baseLineArcs"] = baseLineArcs[i];
                }

                if (additionalSettings.Count > 0)
                {
                    try
                    {
                        stair.Merge(
                            JObject.Parse(additionalSettings.At(i)),
                            new JsonMergeSettings
                            {
                                MergeArrayHandling = MergeArrayHandling.Replace
                            });
                    }
                    catch (Exception ex)
                    {
                        this.AddError($"Invalid JSON in the AdditionalSettings input: {ex.Message}");
                        return;
                    }
                }

                stairs.Add(stair);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["stairs"] = stairs },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var newElements = new List<object>();
            var oldElements = new List<object>();
            var notCarriedOver = new DataTree<string>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "results");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                notCarriedOver.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    newElements.Add(null);
                    oldElements.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                newElements.Add(ItemListHelp.ElementGuidOf(item["elementId"]));
                oldElements.Add(ItemListHelp.ElementGuidOf(item["oldElementId"]));
                if (item["notCarriedOver"] is JArray lost)
                {
                    foreach (var entry in lost)
                    {
                        var relation = entry["relation"]?.ToString() ?? "";
                        var guid = entry["elementId"]?["guid"]?.ToString();
                        notCarriedOver.Add(
                            guid == null ? relation : $"{relation} {guid}",
                            path);
                    }
                }

                errors.Add("");
            }

            da.SetDataList(0, newElements);
            da.SetDataList(1, oldElements);
            da.SetDataTree(2, notCarriedOver);
            da.SetDataList(3, errors);
        }

        // The baseline of each stair from the BaseLines points or the
        // BaseLineCurves: its points and, from a curve, its arcs.
        private bool TryReadBaseLines(
            GH_Structure<GH_Point> baseLines,
            List<Curve> baseLineCurves,
            out List<JArray> baseLinePoints,
            out List<JArray> baseLineArcs)
        {
            baseLinePoints = new List<JArray>();
            baseLineArcs = new List<JArray>();

            if (baseLineCurves.Count > 0)
            {
                foreach (var curve in baseLineCurves)
                {
                    if (!PolygonHelp.TryToBaseLine(
                            curve,
                            out JArray points,
                            out JArray arcs,
                            out string error))
                    {
                        this.AddError($"The input BaseLineCurves is invalid: {error}.");
                        return false;
                    }

                    baseLinePoints.Add(points);
                    baseLineArcs.Add(arcs);
                }

                return true;
            }

            if (baseLines.DataCount == 0)
            {
                return true;
            }

            for (var i = 0; i < baseLines.Branches.Count; i++)
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
                    return false;
                }

                baseLinePoints.Add(points);
                baseLineArcs.Add(new JArray());
            }

            return true;
        }

        // There is no RecreateStairs bitmap yet; the component shows the
        // ModifyStairs one.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyStairs;

        public override Guid ComponentGuid =>
            new Guid("6e2b7c41-93d5-4a8f-b1e0-5c7d2f9a3b68");
    }
}
