using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class UnlockElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "UnlockElements";

        public UnlockElementsComponent()
            : base(
                "UnlockElements",
                "Unlock the given elements. Manual lock, not teamwork!",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to unlock.");
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
            Properties.Resources.UnlockElements;

        public override Guid ComponentGuid =>
            new Guid("1170fa35-9fa6-4b9b-89d9-4f5243c29258");
    }
}
