using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class SetMasterLayoutContextComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetMasterLayoutContext";

        public SetMasterLayoutContextComponent()
            : base(
                "SetMasterLayoutContext",
                "Sets which layout's context the master layout of the current window is drawn in: its autotexts " +
                "then show the values of the given layout. The current window must be a master layout, otherwise " +
                "an error is returned. It does not assign a master layout to a layout. Available from Archicad 27.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "LayoutDatabaseGuid",
                "The database identifier of the layout whose context the master layout is drawn in.");
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
            if (!da.TryCreate(
                    0,
                    out DatabaseGuidObject layoutDatabaseId))
            {
                this.AddError(
                    "LayoutDatabaseGuid is not a valid database identifier.");
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    new JObject { ["layoutDatabaseId"] = JObject.FromObject(layoutDatabaseId) },
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

            da.SetData(
                0,
                response.Message());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetMasterLayoutContext;

        public override Guid ComponentGuid =>
            new Guid("d62185a5-ab00-4a55-a452-073a1720ac0f");
    }
}
