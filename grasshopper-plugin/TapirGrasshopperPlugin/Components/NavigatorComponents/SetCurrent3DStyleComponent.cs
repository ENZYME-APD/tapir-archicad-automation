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
                "Set the current 3D style of the 3D window by name, one of the names Get3DStyles returns.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Name",
                "The name of the 3D style to make current.");
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult.Message),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(0, out string name))
            {
                return;
            }

            var parameters = new JObject
            {
                ["name"] = name
            };

            if (!TryGetConvertedCadValues(
                    CommandName,
                    parameters,
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

            da.SetData(0, response.Message());
        }

        // Shares the icon of the 3D projection setter until the 3D styles get one of their own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Set3DProjectionSettings;

        public override Guid ComponentGuid =>
            new Guid("3c9e7b21-84d6-4f0a-b5e2-1d6a9c4f7e83");
    }
}
