using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetDrawingContentBoxesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetDrawingContentBoxes";

        public GetDrawingContentBoxesComponent()
            : base(
                "GetDrawingContentBoxes",
                "Returns the full content box of the given Drawing elements, independent of their frames, " +
                "as ACAPI_Drawing_GetFullDrawingContentBox reports it. Available from Archicad 27.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "DrawingGuids",
                "Identifiers of the Drawing elements.");
        }

        protected override void AddOutputs()
        {
            outManager.AddRectangleParameter(
                "ContentBoxes",
                "ContentBoxes",
                "The full content box of each Drawing, independent of its frame (null for a failed item)."
                    .WithTypeName("rectangleList"),
                GH_ParamAccess.list);

            OutErrorMessages(
                "Error message of each Drawing (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(elements),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var contentBoxes = new List<object>();
            var errorMessages = new List<string>();

            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "contentBoxes"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    contentBoxes.Add(null);
                    errorMessages.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                contentBoxes.Add(NavigatorComponentHelp.Rectangle(item["contentBox"]));
                errorMessages.Add("");
            }

            da.SetDataList(
                0,
                contentBoxes);

            da.SetDataList(
                1,
                errorMessages);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetDrawingContentBoxes;

        public override Guid ComponentGuid =>
            new Guid("6a179dca-d02d-4370-bad4-8ec3d39dbe0e");
    }
}
