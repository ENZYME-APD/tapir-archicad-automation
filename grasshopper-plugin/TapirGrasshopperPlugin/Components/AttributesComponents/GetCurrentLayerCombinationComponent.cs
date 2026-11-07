using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetCurrentLayerCombinationComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetCurrentLayerCombination";

        public GetCurrentLayerCombinationComponent()
            : base(
                "GetCurrentLayerCombination",
                "Returns the layer combination in effect in the current window. No layer combination is returned " +
                "when the current state of the layers matches none.",
                GroupNames.Attributes)
        {
        }

        protected override void AddOutputs()
        {
            OutGeneric(
                "AttributeGuid",
                "Identifier of the layer combination in effect in the current window. Null when the current state " +
                "of the layers matches no layer combination.");

            OutText(
                "Name",
                "Name of the layer combination. Null together with AttributeGuid.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var guid = JsonOutputHelp.GuidOf(response["layerCombinationId"]);
            da.SetData(
                0,
                guid == null
                    ? null
                    : new AttributeGuidWrapper
                    {
                        AttributeId = new AttributeGuidObject { Guid = guid }
                    });

            da.SetData(
                1,
                JsonOutputHelp.Scalar(response, "name"));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetCurrentLayerCombination;

        public override Guid ComponentGuid =>
            new Guid("4c610dbc-04f3-4674-960a-37b17d7b0434");
    }
}
