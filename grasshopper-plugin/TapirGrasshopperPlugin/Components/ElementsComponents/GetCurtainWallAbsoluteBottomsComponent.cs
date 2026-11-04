using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetCurtainWallAbsoluteBottomsComponent
        : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetCurtainWallAbsoluteBottoms";

        public GetCurtainWallAbsoluteBottomsComponent()
            : base(
                "GetCurtainWallAbsoluteBottoms",
                "Gets the absolute bottom level of curtain walls or their subelements (segments, frames, " +
                "panels, junctions, accessories). The outputs are aligned with the input list of elements.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "absoluteBottoms";

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the Curtain Walls or their subelements.");
        }

        protected override void AddOutputs()
        {
            OutNumberList(
                "AbsoluteBottoms",
                "The absolute bottom level of each element (null for an error).");

            OutErrorMessages(
                "Error message of each element (empty when successful).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var bottoms = new List<double?>();
            var errors = new List<string>();

            foreach (var item in items)
            {
                if (JsonOutputHelp.IsError(item))
                {
                    bottoms.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                bottoms.Add(item?.Value<double?>("absoluteBottom"));
                errors.Add("");
            }

            da.SetDataList(
                0,
                bottoms);
            da.SetDataList(
                1,
                errors);
        }

        // Shares the icon of ElementDetails until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ElemDetails;

        public override Guid ComponentGuid =>
            new Guid("1641ceaa-7878-481d-ba43-fa24f93a33f7");
    }
}
