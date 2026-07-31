using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateAssociativeDimensionsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateAssociativeDimensions";

        public CreateAssociativeDimensionsComponent()
            : base(
                "CreateAssociativeDimensions",
                "Create associative linear dimensions measuring the given witness elements. " +
                "The witness elements are given as a tree with one branch per dimension. " +
                "Advanced witness point references (inIndex, nodeType, etc.) can be given " +
                "through the AdditionalSettings input.",
                GroupNames.ElementCreation)
        {
        }

        protected override void AddInputs()
        {
            InPoints(
                "ReferencePoints",
                "Point the dimension line goes through (only X and Y are used).");

            InPoints(
                "Directions",
                "Direction of the dimension line (only X and Y are used). Input only 1 to use the same direction for all dimensions.");

            inManager.AddGenericParameter(
                "WitnessElementGuids",
                "WitnessElementGuids",
                "Identifiers of the elements to measure (one branch per dimension, at least 2 elements).",
                GH_ParamAccess.tree);

            InTexts(
                "AdditionalSettings",
                "One JSON object per dimension with further optional settings matching the " +
                "command's documented item schema. Input only 1 to use the same settings for all. Optional.");

            SetOptionality(3);
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifiers of the created dimensions (null for failed items).");

            OutTexts(
                "ErrorMessages",
                "Error message for each item (empty when the dimension was created successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<Point3d> referencePoints))
            {
                return;
            }

            var dimensionCount = referencePoints.Count;
            if (dimensionCount == 0)
            {
                this.AddError("The ReferencePoints input must contain at least one item.");
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<Point3d> directions))
            {
                return;
            }

            if (directions.Count != 1 && directions.Count != dimensionCount)
            {
                this.AddError(
                    "The size of the input Directions must be 1 or equal to the size of the input ReferencePoints.");
                return;
            }

            if (!da.TryGetTree(
                    2,
                    out GH_Structure<IGH_Goo> witnessElements))
            {
                return;
            }

            var witnessBranchCount = witnessElements.Branches.Count;
            if (witnessBranchCount != 1 && witnessBranchCount != dimensionCount)
            {
                this.AddError(
                    "The number of branches in the WitnessElementGuids input must be 1 or equal to the size of the input ReferencePoints.");
                return;
            }

            da.TryGetList(
                3,
                out List<string> additionalSettings);
            additionalSettings = additionalSettings ?? new List<string>();
            if (additionalSettings.Count > 1 &&
                additionalSettings.Count != dimensionCount)
            {
                this.AddError(
                    "The size of the input AdditionalSettings must be 0, 1 or equal to the size of the input ReferencePoints.");
                return;
            }

            var items = new JArray();
            for (var i = 0; i < dimensionCount; i++)
            {
                var branch = witnessElements.Branches[witnessBranchCount == 1 ? 0 : i];
                var witnessPoints = new JArray();
                foreach (var goo in branch)
                {
                    var wrapper = goo as GH_ObjectWrapper ?? new GH_ObjectWrapper(goo);
                    var id = GuidObject<ElementGuid>.CreateFromWrapper(wrapper);
                    if (id == null)
                    {
                        this.AddError(
                            "Invalid element identifier in the WitnessElementGuids input.");
                        return;
                    }
                    witnessPoints.Add(
                        new JObject
                        {
                            ["elementId"] = new JObject { ["guid"] = id.Guid }
                        });
                }

                if (witnessPoints.Count < 2)
                {
                    this.AddError(
                        "Each branch of the WitnessElementGuids input must contain at least 2 elements.");
                    return;
                }

                var direction = directions[directions.Count == 1 ? 0 : i];
                var item = new JObject
                {
                    ["referencePoint"] = new JObject
                    {
                        ["x"] = referencePoints[i].X,
                        ["y"] = referencePoints[i].Y
                    },
                    ["direction"] = new JObject
                    {
                        ["x"] = direction.X,
                        ["y"] = direction.Y
                    },
                    ["witnessPoints"] = witnessPoints
                };

                if (additionalSettings.Count > 0)
                {
                    var json = additionalSettings[additionalSettings.Count == 1 ? 0 : i];
                    try
                    {
                        item.Merge(
                            JObject.Parse(json),
                            new JsonMergeSettings
                            {
                                MergeArrayHandling = MergeArrayHandling.Replace
                            });
                    }
                    catch (Exception ex)
                    {
                        this.AddError(
                            $"Invalid JSON in the AdditionalSettings input: {ex.Message}");
                        return;
                    }
                }

                items.Add(item);
            }

            var parameters = new JObject { ["dimensionsData"] = items };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            CreateElementsComponentBase.SetCreatedElementsOutputs(da, response, 0, 1);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateAssociativeDimensions;

        public override Guid ComponentGuid =>
            new Guid("24a2fb04-121e-408a-9f20-bace552ec1b5");
    }
}
