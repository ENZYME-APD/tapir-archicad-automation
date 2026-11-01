using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ElevateElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ElevateElements";

        public ElevateElementsComponent()
            : base(
                "ElevateElements",
                "Elevate elements",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "ElementsGuids of the objects to elevate.");

            InNumbers(
                "Elevations",
                "The vertical offsets in meters to move the elements by, positive up and negative down (input only 1 value to elevate all elements by the same offset).");

            InBoolean(
                "Copy",
                "Elevate copies of the elements or elevate the elements themselves.",
                false);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when it succeeded).");
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

            if (!da.TryGetList(
                    1,
                    out List<double> elevations))
            {
                return;
            }

            if (elevations.Count != 1 &&
                elevations.Count != elements.Elements.Count)
            {
                this.AddError(
                    "The size of the input Elevations must be 1 or equal to the size of the input ElementGuids.");
                return;
            }

            if (!da.TryGet(
                    2,
                    out bool copy))
            {
                return;
            }

            var input = new ElevateElementsParameters
            {
                ElementsWithElevations =
                    new List<ElementWithElevationParameters>()
            };

            for (var i = 0; i < elements.Elements.Count; i++)
            {
                var element = elements.Elements[i];
                var elevation = elevations[elevations.Count == 1 ? 0 : i];

                input.ElementsWithElevations.Add(
                    new ElementWithElevationParameters()
                    {
                        Element = element.ElementId,
                        Elevation = elevation,
                        Copy = copy
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MoveElements;

        public override Guid ComponentGuid =>
            new Guid("7c4e1b2a-9d53-4f6e-8a1b-3e5c7d9f2a64");
    }
}
