using Grasshopper.Kernel;
using System;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class ReloadLibrariesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ReloadLibraries";

        public ReloadLibrariesComponent()
            : base(
                "ReloadLibraries",
                "Execute the reload libraries command.",
                GroupNames.Library)
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
            Properties.Resources.ReloadLibraries;

        public override Guid ComponentGuid =>
            new Guid("1702904b-e3f8-4575-a8c9-b809ccdb280e");
    }
}
