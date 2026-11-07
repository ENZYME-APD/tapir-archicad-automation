using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class Get3DStylesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "Get3DStyles";

        public Get3DStylesComponent()
            : base(
                "Get3DStyles",
                "Get the names of the 3D styles of the project and the name of the current one, " +
                "which the 3D window uses.",
                GroupNames.Navigator)
        {
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "StyleNames",
                "The names of the 3D styles of the project.");

            OutText(
                "CurrentStyleName",
                "The name of the current 3D style, the one the 3D window uses.");
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

            if (response["styleNames"] is JArray styleNames)
            {
                da.SetDataList(
                    0,
                    styleNames.Select(name => name.Value<string>()).ToList());
            }

            da.SetData(
                1,
                response.Value<string>("currentStyleName"));
        }

        // Shares the icon of the 3D projection getter until the 3D styles get one of their own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Get3DProjectionSettings;

        public override Guid ComponentGuid =>
            new Guid("0f6d2a4e-5b1c-4e8a-9c3d-7a2b6f1e8d45");
    }
}
