using Grasshopper.Kernel;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class LabelElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateLabels";

        public LabelElementsComponent()
            : base(
                "LabelElements",
                "Label elements",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "IDs of Elements to create labels for.");
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
                new
                {
                    labelsData = elements
                        .Elements.Select(element =>
                            new { parentElementId = element.ElementId })
                        .ToList()
                },
                ToAddOn);
        }

        public override Guid ComponentGuid =>
            new Guid("ecdb0a59-f928-4ed3-88e1-cd9aea737b39");
    }
}