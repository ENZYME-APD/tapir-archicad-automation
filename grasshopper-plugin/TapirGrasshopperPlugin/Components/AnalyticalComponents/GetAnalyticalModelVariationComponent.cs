using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Analytical;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class GetAnalyticalModelVariationComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAnalyticalModelVariation";

        public GetAnalyticalModelVariationComponent()
            : base(
                "GetAnalyticalModelVariation",
                "Retrieves the variation of the given Structural Analytical Model, " +
                "or of the model of the current view when no model is given " +
                "(the model is required before Archicad 27). " +
                "The fields of the variation depend on the Archicad version.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "AnalyticalModelGuid",
                "Identifier of the Structural Analytical Model. " +
                "Optional; the model of the current view is used when it is omitted. " +
                "Required before Archicad 27.");

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutText(
                "VariationGuid",
                "Identifier of the analytical model variation. Archicad 27 and newer.");

            OutText(
                "RenovationFilterGuid",
                "Identifier of the renovation filter of the variation. Archicad 25 and 26.");

            OutIntegers(
                "LayerIndices",
                "Attribute index of each layer with a connection class. Archicad 26 only.");

            OutIntegers(
                "ConnectionClassIds",
                "Connection class of each layer. Archicad 25 and 26.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            if (da.TryGet(
                    0,
                    out GH_ObjectWrapper modelWrapper))
            {
                var modelId = GuidObject<AnalyticalModelGuid>.CreateFromWrapper(modelWrapper);
                if (modelId == null)
                {
                    this.AddError("Invalid identifier in the AnalyticalModelGuid input.");
                    return;
                }
                parameters["analyticalModelId"] = AnalyticalHelp.IdOf(modelId);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var variation = response["variation"];
            var layerIndices = new List<object>();
            var connectionClassIds = new List<object>();

            // Archicad 25 lists the connection classes as plain integers,
            // Archicad 26 as objects with the layer's attribute index.
            if (variation?["layerConnectionClassIds"] is JArray classIds)
            {
                foreach (var classId in classIds)
                {
                    if (classId is JObject classIdObject)
                    {
                        layerIndices.Add(
                            JsonOutputHelp.Scalar(
                                classIdObject,
                                "layerIndex"));
                        connectionClassIds.Add(
                            JsonOutputHelp.Scalar(
                                classIdObject,
                                "connectionClassId"));
                    }
                    else
                    {
                        layerIndices.Add(null);
                        connectionClassIds.Add(classId.ToObject<int?>());
                    }
                }
            }

            da.SetData(
                0,
                JsonOutputHelp.GuidOf(variation?["amVariationGuid"]));
            da.SetData(
                1,
                JsonOutputHelp.GuidOf(variation?["renovationFilterGuid"]));
            da.SetDataList(
                2,
                layerIndices);
            da.SetDataList(
                3,
                connectionClassIds);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAnalyticalModelVariation;

        public override Guid ComponentGuid =>
            new Guid("af0d9113-d270-42e9-876e-9ea0948ff4cd");
    }
}
