using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class Create3DStylesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "Create3DStyles";

        public Create3DStylesComponent()
            : base(
                "Create3DStyles",
                "Creates 3D styles. Each new style starts from the current 3D style and differs from it only in the " +
                "given fields. Available from Archicad 27.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            Style3DHelp.AddInputs(
                this,
                "The name of each new 3D style, unique within the project.",
                "The settings in which each new style differs from the current 3D style.");
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
            Properties.Resources.Create3DStyles;

        public override Guid ComponentGuid =>
            new Guid("328189af-8406-4f5a-8f6d-63551664d34a");
    }
}
