using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class Change3DStylesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "Change3DStyles";

        public Change3DStylesComponent()
            : base(
                "Change3DStyles",
                "Changes the given fields of existing 3D styles, identified by name. Available from Archicad 27.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            Style3DHelp.AddInputs(
                this,
                "The name of each 3D style to change.",
                "The settings to change; only the given keys change.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each 3D style (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var styles = Style3DHelp.ReadStyles(
                this,
                da);

            if (styles == null)
            {
                return;
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["styles"] = styles },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Change3DStyles;

        public override Guid ComponentGuid =>
            new Guid("f4a3c90d-75c4-4a12-aab1-b545995594a9");
    }
}
