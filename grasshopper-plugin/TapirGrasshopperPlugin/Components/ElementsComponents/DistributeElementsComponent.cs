using Grasshopper.Kernel;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class DistributeElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DistributeElements";

        public DistributeElementsComponent()
            : base(
                "DistributeElements",
                "Distribute 3 or more elements evenly along X or Y, by the middles of their plan bounding boxes " +
                "or by the space between them. The two outermost elements stay where they are.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "ElementGuids of the elements to distribute (at least 3).");

            InTextWithDefault(
                "Direction",
                "horizontal (along X) or vertical (along Y).",
                "horizontal");

            InTextWithDefault(
                "Mode",
                "centers (equal distances between the middles of the bounding boxes) " +
                "or gaps (equal space between the bounding boxes).",
                "centers");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when it was distributed).");
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

            var direction = da.GetOptional(
                1,
                "horizontal");

            if (!DistributeDirection.Values.Contains(direction))
            {
                this.AddError(
                    "Direction must be one of " + string.Join(", ", DistributeDirection.Values) + ".");
                return;
            }

            var mode = da.GetOptional(
                2,
                "centers");

            if (!DistributeMode.Values.Contains(mode))
            {
                this.AddError(
                    "Mode must be one of " + string.Join(", ", DistributeMode.Values) + ".");
                return;
            }

            var input = new DistributeElementsParameters
            {
                Elements = elements.Elements,
                Direction = direction,
                Mode = mode
            };

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DistributeElements;

        public override Guid ComponentGuid =>
            new Guid("b7d4e912-6a3f-4c58-9e0b-2f71a5c8d364");
    }
}
