using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SetOpeningDetailsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetOpeningDetails";

        public SetOpeningDetailsComponent()
            : base(
                "SetOpeningDetails",
                "Changes the details of Opening elements: the parent element, the cut elements, the polygon, a " +
                "transformation of the geometry and the extrusion parameters (constraint, limit type, linked " +
                "status, shape type, size, anchor, offsets, surface overrides) returned by GetOpeningDetails. " +
                "A detail that is not given stays as it is. Available from Archicad 29.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the Opening elements (made with the Opening tool) to change.");

            InGenerics(
                "ParentElementGuids",
                "The new element each opening belongs to (input only 1 to use the same element for all). Optional.");

            InGenericTree(
                "AddCutElementGuids",
                "Elements each opening should cut as well (one branch per opening, or one branch for all). Optional.");

            InGenericTree(
                "RemoveCutElementGuids",
                "Elements each opening should no longer cut (one branch per opening, or one branch for all). Optional.");

            inManager.AddPointParameter(
                "Polygons",
                "Polygons",
                ("The new polygon of each opening, at least 3 points in the plane of the X and Y axes returned by " +
                 "GetOpeningDetails (one branch per opening, or one branch for all; Z is ignored). Optional.")
                    .WithTypeName("pointTree"),
                GH_ParamAccess.tree);

            inManager.AddNumberParameter(
                "Transformations",
                "Transformations",
                ("A transformation applied to the geometry of each opening: 12 numbers, a 3x4 matrix row by row, " +
                 "the 4th, 8th and 12th numbers being the translation along X, Y and Z (one branch per opening, or " +
                 "one branch for all). Optional.")
                    .WithTypeName("numberTree"),
                GH_ParamAccess.tree);

            InTexts(
                "ExtrusionParameters",
                OpeningSettingsHelp.ExtrusionParametersDescription +
                " Input only 1 to use the same settings for all. Optional.");

            SetOptionality(new[] { 1, 2, 3, 4, 5, 6 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each opening (empty when it was changed).");
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
                    out var elementIds) ||
                !this.TryGetElementIds(
                    da.OptionalList<GH_ObjectWrapper>(1),
                    "ParentElementGuids",
                    out var parentIds))
            {
                return;
            }

            da.GetDataTree(2, out GH_Structure<IGH_Goo> addCutTree);
            da.GetDataTree(3, out GH_Structure<IGH_Goo> removeCutTree);
            da.GetDataTree(4, out GH_Structure<GH_Point> polygonTree);
            da.GetDataTree(5, out GH_Structure<GH_Number> transformationTree);
            var extrusionTexts = da.OptionalList<string>(6);

            var count = elementIds.Count;
            if (!this.CheckSizes(
                    count,
                    "ElementGuids",
                    ("ParentElementGuids", parentIds.Count),
                    ("AddCutElementGuids (branch count)", BranchCount(addCutTree)),
                    ("RemoveCutElementGuids (branch count)", BranchCount(removeCutTree)),
                    ("Polygons (branch count)", BranchCount(polygonTree)),
                    ("Transformations (branch count)", BranchCount(transformationTree)),
                    ("ExtrusionParameters", extrusionTexts.Count)))
            {
                return;
            }

            var items = new JArray();
            for (var i = 0; i < count; i++)
            {
                var item = new JObject { ["elementId"] = elementIds[i] };

                if (parentIds.Count > 0)
                {
                    item["parentElementId"] = parentIds.At(i);
                }

                if (BranchCount(addCutTree) > 0)
                {
                    if (!TryGetElementIdArray(Branch(addCutTree, i), "AddCutElementGuids", out var addCutElements))
                    {
                        return;
                    }

                    item["addCutElements"] = addCutElements;
                }

                if (BranchCount(removeCutTree) > 0)
                {
                    if (!TryGetElementIdArray(Branch(removeCutTree, i), "RemoveCutElementGuids", out var removeCutElements))
                    {
                        return;
                    }

                    item["removeCutElements"] = removeCutElements;
                }

                if (BranchCount(polygonTree) > 0)
                {
                    var polygon = new JArray();
                    foreach (var point in Branch(polygonTree, i))
                    {
                        if (point != null)
                        {
                            polygon.Add(new JObject { ["x"] = point.Value.X, ["y"] = point.Value.Y });
                        }
                    }

                    if (polygon.Count < 3)
                    {
                        this.AddError("A branch of Polygons has fewer than 3 points.");
                        return;
                    }

                    item["polygon"] = polygon;
                }

                if (BranchCount(transformationTree) > 0)
                {
                    var transformation = new JArray();
                    foreach (var number in Branch(transformationTree, i))
                    {
                        if (number != null)
                        {
                            transformation.Add(number.Value);
                        }
                    }

                    if (transformation.Count != 12)
                    {
                        this.AddError("Every branch of Transformations needs exactly 12 numbers.");
                        return;
                    }

                    item["transformation"] = transformation;
                }

                if (extrusionTexts.Count > 0)
                {
                    if (!OpeningSettingsHelp.TryParse(
                            this,
                            extrusionTexts.At(i),
                            "ExtrusionParameters",
                            OpeningSettingsHelp.ExtrusionKeys,
                            out var extrusionParameters))
                    {
                        return;
                    }

                    if (extrusionParameters != null)
                    {
                        item["extrusionParameters"] = extrusionParameters;
                    }
                }

                items.Add(item);
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["openingsWithDetails"] = items },
                ToAddOn,
                da);
        }

        private bool TryGetElementIdArray(
            IEnumerable<IGH_Goo> branch,
            string inputName,
            out JArray elements)
        {
            elements = new JArray();
            foreach (var goo in branch)
            {
                if (goo == null)
                {
                    continue;
                }

                var id = GuidObject<ElementGuid>.CreateFromWrapper(goo as GH_ObjectWrapper ?? new GH_ObjectWrapper(goo));
                if (id == null)
                {
                    this.AddError($"Invalid element identifier in the {inputName} input.");
                    return false;
                }

                elements.Add(new JObject { ["elementId"] = new JObject { ["guid"] = id.Guid } });
            }

            return true;
        }

        // The branch count of an optional tree input; 0 when it holds nothing.
        private static int BranchCount<T>(
            GH_Structure<T> tree)
            where T : IGH_Goo
        {
            return tree == null || tree.DataCount == 0 ? 0 : tree.Branches.Count;
        }

        private static List<T> Branch<T>(
            GH_Structure<T> tree,
            int i)
            where T : IGH_Goo
        {
            return tree.Branches.Count == 1 ? tree.Branches[0] : tree.Branches[i];
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetOpeningDetails;

        public override Guid ComponentGuid =>
            new Guid("741833b9-f5f0-4151-94d6-419d6666258b");
    }
}
