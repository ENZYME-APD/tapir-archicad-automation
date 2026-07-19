using Grasshopper.Kernel;
using System;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class QuitArchicadComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "QuitArchicad";

        public QuitArchicadComponent()
            : base(
                "QuitArchicad",
                "Perform a quit operation on the currently running Archicad instance.",
                GroupNames.General)
        {
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            SetCadValues(
                CommandName,
                new { },
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.QuitArchicad;

        public override Guid ComponentGuid =>
            new Guid("74511b98-f3e1-40d0-b34a-f0d55195139c");
    }
}
