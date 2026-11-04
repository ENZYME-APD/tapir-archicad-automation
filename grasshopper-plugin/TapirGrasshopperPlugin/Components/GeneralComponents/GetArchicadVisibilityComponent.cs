using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class GetArchicadVisibilityComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetArchicadVisibility";

        public GetArchicadVisibilityComponent()
            : base(
                "GetArchicadVisibility",
                "Returns whether the Archicad application is shown or hidden. Supported on Windows only; " +
                "on macOS it returns an error.",
                GroupNames.General)
        {
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "IsVisible",
                "True when the Archicad application is shown, false when it is hidden.");
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

            da.SetData(
                0,
                response.Value<bool?>("isVisible"));
        }

        // Shares the icon of ArchicadLocation until it gets its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ArchicadLocation;

        public override Guid ComponentGuid =>
            new Guid("5c01721d-0694-4c15-864b-a0bdac271ca3");
    }
}
