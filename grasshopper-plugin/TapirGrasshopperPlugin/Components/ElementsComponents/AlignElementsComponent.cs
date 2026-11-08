using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class AlignElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "AlignElements";

        public AlignElementsComponent()
            : base(
                "AlignElements",
                "Align elements to one edge or the middle of their plan bounding boxes, " +
                "of the whole set or of a reference element.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "ElementGuids of the elements to align.");

            InTextWithDefault(
                "AlignTo",
                "The edge to align: left, right, top, bottom, centerX (the middles in X, on one vertical line) " +
                "or centerY (the middles in Y, on one horizontal line).",
                "left");

            InGeneric(
                "Reference",
                "ElementGuid of the element to align to (it is not moved). Optional, without it the elements " +
                "are aligned to the outermost edge of the whole set, or to its middle.");

            SetOptionality(2);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when it was aligned).");
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

            var alignTo = da.GetOptional(
                1,
                "left");

            if (!AlignTo.Values.Contains(alignTo))
            {
                this.AddError(
                    "AlignTo must be one of " + string.Join(", ", AlignTo.Values) + ".");
                return;
            }

            var input = new AlignElementsParameters
            {
                Elements = elements.Elements,
                AlignTo = alignTo
            };

            if (da.TryGet(
                    2,
                    out GH_ObjectWrapper referenceWrapper) &&
                referenceWrapper?.Value != null)
            {
                input.Reference = GuidObject<ElementGuid>.CreateFromWrapper(referenceWrapper);
                if (input.Reference == null)
                {
                    this.AddError("Invalid Reference.");
                    return;
                }
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AlignElements;

        public override Guid ComponentGuid =>
            new Guid("3f8a6c21-5d9e-4b07-a2c4-8e1d6f0b9a53");
    }
}
