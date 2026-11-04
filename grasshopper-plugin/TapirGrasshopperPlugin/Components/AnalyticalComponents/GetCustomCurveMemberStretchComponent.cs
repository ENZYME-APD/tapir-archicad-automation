using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class GetCustomCurveMemberStretchComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetCustomCurveMemberStretch";

        public GetCustomCurveMemberStretchComponent()
            : base(
                "GetCustomCurveMemberStretch",
                "Retrieves the lengths added to the start and end of the given curve members. " +
                "Available from Archicad 29.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements of the curve members.");
        }

        protected override void AddOutputs()
        {
            OutNumberList(
                "BegAdditionalLengths",
                "The length added to the curve member of each element at its start " +
                "(null when it could not be got).");

            OutNumberList(
                "EndAdditionalLengths",
                "The length added to the curve member of each element at its end " +
                "(null when it could not be got).");

            OutErrorMessages(
                "Error message of each element (empty when its stretch was got).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();
            if (!AnalyticalJsonHelp.TryAddElements(da, 0, parameters))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var items = AnalyticalJsonHelp.SplitErrors(
                response,
                "stretches",
                out List<string> errors);

            var begLengths = new List<object>();
            var endLengths = new List<object>();
            foreach (var item in items)
            {
                begLengths.Add(JsonOutputHelp.Scalar(item, "begAdditionalLength"));
                endLengths.Add(JsonOutputHelp.Scalar(item, "endAdditionalLength"));
            }

            da.SetDataList(0, begLengths);
            da.SetDataList(1, endLengths);
            da.SetDataList(2, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAnalyticalMemberTypesOfElements;

        public override Guid ComponentGuid =>
            new Guid("6645bf55-9215-44d3-bcc6-f7f0f04a87d3");
    }
}
