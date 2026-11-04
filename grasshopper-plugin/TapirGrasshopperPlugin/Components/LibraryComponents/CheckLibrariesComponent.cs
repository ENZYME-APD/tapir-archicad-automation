using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class CheckLibrariesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CheckLibraries";

        public CheckLibrariesComponent()
            : base(
                "CheckLibraries",
                "Checks the loaded libraries with the Archicad API's library check. Returns the error the API " +
                "reports, if any.",
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
            Properties.Resources.GetLibraries;

        public override Guid ComponentGuid =>
            new Guid("10689f7d-8da9-4942-955f-ee0b3602c7bc");
    }
}
