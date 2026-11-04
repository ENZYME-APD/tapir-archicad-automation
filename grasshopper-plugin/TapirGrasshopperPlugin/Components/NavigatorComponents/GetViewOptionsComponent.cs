using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetViewOptionsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetViewOptions";

        public GetViewOptionsComponent()
            : base(
                "GetViewOptions",
                "Gets the model view options and on-screen view options of the current plan window, and the index of " +
                "the model view option combination it applies.",
                GroupNames.Navigator)
        {
        }

        protected override void AddOutputs()
        {
            OutInteger(
                "ModelViewOptionIndex",
                "The index of the model view option combination the window applies, in the order of " +
                "GetModelViewOptions, starting from 1.");

            OutText(
                "ModelViewOptions_JSON",
                "The model view options as a JSON object, every field by its API_ModelViewOptions name, with the " +
                "curtainWall, stair and railing parts as nested objects. Values are the numeric API constants; " +
                "attribute indices are plain integers.");

            OutText(
                "OnScreenViewOptions_JSON",
                "The on-screen view options as a JSON object, every field by its API_OnScreenViewOptions name.");
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

            da.SetData(0, (int?)response["modelViewOptionIndex"]);
            da.SetData(1, NavigatorComponentHelp.JsonText(response["viewOptions"]?["modelViewOptions"]));
            da.SetData(2, NavigatorComponentHelp.JsonText(response["viewOptions"]?["onScreenViewOptions"]));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModelViewOptions;

        public override Guid ComponentGuid =>
            new Guid("0e162c12-cbc9-4a1b-a040-53986810e240");
    }
}
