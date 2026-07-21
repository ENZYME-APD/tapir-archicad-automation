using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class LockElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "LockElements";

        public LockElementsComponent()
            : base(
                "LockElements",
                "Lock the given elements. Manual lock, not teamwork!",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to lock.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            SetCadValues(
                CommandName,
                elements,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.LockElements;

        public override Guid ComponentGuid =>
            new Guid("583d1938-045f-4f32-9904-b56e3b285b26");
    }
}
