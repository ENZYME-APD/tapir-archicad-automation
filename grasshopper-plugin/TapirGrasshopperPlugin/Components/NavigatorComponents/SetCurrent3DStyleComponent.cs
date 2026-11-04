using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class SetCurrent3DStyleComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetCurrent3DStyle";

        public SetCurrent3DStyleComponent()
            : base(
                "SetCurrent3DStyle",
                "Make the given 3D style, one of the names Get3DStyles returns, the current one, which the 3D " +
                "window uses.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "StyleName",
                "The name of an existing 3D style.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string styleName) ||
                string.IsNullOrEmpty(styleName))
            {
                this.AddError("StyleName is required.");
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    new JObject { ["styleName"] = styleName },
                    ToAddOn,
                    ExecutionResult.Deserialize,
                    out ExecutionResult response))
            {
                return;
            }

            if (!response.Success)
            {
                this.AddError(response.Message());
            }
        }

        // Shares the icon of the 3D projection setter until the 3D styles get one of their own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Set3DProjectionSettings;

        public override Guid ComponentGuid =>
            new Guid("a616a3e3-fff8-4e6e-b6b8-73dc8f53c9b9");
    }
}
