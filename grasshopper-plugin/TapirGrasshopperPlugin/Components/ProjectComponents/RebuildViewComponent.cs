using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class RebuildViewComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "RebuildView";

        public RebuildViewComponent()
            : base(
                "RebuildView",
                "Rebuild the current view.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InBoolean(
                "Regenerate",
                "Regenerate the view. The default is false, meaning the view will be rebuilt but not regenerated.",
                false);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            da.TryGet(
                0,
                out bool regenerate);

            SetCadValues(
                CommandName,
                new RebuildViewParameters { Regenerate = regenerate },
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RebuildView;

        public override Guid ComponentGuid =>
            new Guid("75f631e8-b980-4f13-b860-830fa0fb6acf");
    }
}
