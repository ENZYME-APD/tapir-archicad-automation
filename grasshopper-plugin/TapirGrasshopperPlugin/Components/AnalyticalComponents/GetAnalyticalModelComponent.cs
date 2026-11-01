using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Analytical;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class GetAnalyticalModelComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAnalyticalModel";

        public GetAnalyticalModelComponent()
            : base(
                "GetAnalyticalModel",
                "Retrieves the Structural Analytical Model of the given variation, " +
                "as returned by GetAnalyticalModelVariation.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "VariationGuid",
                "Identifier of the analytical model variation. Required from Archicad 27.");

            InText(
                "RenovationFilterGuid",
                "Identifier of the renovation filter of the variation. Required in Archicad 25 and 26.");

            InIntegers(
                "LayerIndices",
                "Attribute index of each layer with a connection class. Archicad 26 only; " +
                "leave it empty in Archicad 25.");

            InIntegers(
                "ConnectionClassIds",
                "Connection class of each layer. Archicad 25 and 26.");

            SetOptionality(
                new[]
                {
                    0,
                    1,
                    2,
                    3
                });
        }

        protected override void AddOutputs()
        {
            OutGeneric(
                "AnalyticalModelGuid",
                "Identifier of the Structural Analytical Model of the variation.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            da.TryGet(
                0,
                out string variationGuid);
            da.TryGet(
                1,
                out string renovationFilterGuid);
            da.TryGetList(
                2,
                out List<int> layerIndices);
            da.TryGetList(
                3,
                out List<int> connectionClassIds);

            if (string.IsNullOrEmpty(variationGuid) &&
                string.IsNullOrEmpty(renovationFilterGuid))
            {
                this.AddError(
                    "Give VariationGuid (Archicad 27 and newer) or RenovationFilterGuid (Archicad 25 and 26).");
                return;
            }

            if (layerIndices.Count > 0 &&
                layerIndices.Count != connectionClassIds.Count)
            {
                this.AddError(
                    "The size of the input LayerIndices must be 0 or equal to the size of the input ConnectionClassIds.");
                return;
            }

            var variation = new JObject();
            if (!string.IsNullOrEmpty(variationGuid))
            {
                variation["amVariationGuid"] = new JObject { ["guid"] = variationGuid };
            }
            if (!string.IsNullOrEmpty(renovationFilterGuid))
            {
                variation["renovationFilterGuid"] = new JObject { ["guid"] = renovationFilterGuid };
            }
            if (connectionClassIds.Count > 0)
            {
                var classIds = new JArray();
                for (var i = 0; i < connectionClassIds.Count; i++)
                {
                    // Archicad 26 pairs each connection class with its layer,
                    // Archicad 25 takes the plain integers.
                    if (layerIndices.Count > 0)
                    {
                        classIds.Add(
                            new JObject
                            {
                                ["layerIndex"] = layerIndices[i],
                                ["connectionClassId"] = connectionClassIds[i]
                            });
                    }
                    else
                    {
                        classIds.Add(connectionClassIds[i]);
                    }
                }
                variation["layerConnectionClassIds"] = classIds;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["variation"] = variation },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                AnalyticalHelp.IdFrom<AnalyticalModelGuid>(
                    response["analyticalModelId"]));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAnalyticalModel;

        public override Guid ComponentGuid =>
            new Guid("f525f937-6434-4eb5-815a-f660adefe2f2");
    }
}
