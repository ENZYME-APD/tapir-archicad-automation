using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class ResetLibrariesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ResetLibraries";

        public ResetLibrariesComponent()
            : base(
                "ResetLibraries",
                "Resets the loaded libraries with the Archicad API's library reset.",
                GroupNames.Library)
        {
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
            if (!TryGetConvertedCadValues(
                    CommandName,
                    new JObject(),
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

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ReloadLibraries;

        public override Guid ComponentGuid =>
            new Guid("ff2fed17-f6f2-4ef6-9593-7b6fc875aa70");
    }
}
