using Grasshopper.Kernel;
using System;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class CloseProjectComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CloseProject";

        public CloseProjectComponent()
            : base(
                "CloseProject",
                "Close the currently opened project.",
                GroupNames.Project)
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
            Properties.Resources.CloseProject;

        public override Guid ComponentGuid =>
            new Guid("bfbf22c8-b810-4a84-aff3-e7e87f34d899");
    }
}
