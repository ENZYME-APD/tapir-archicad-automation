using Grasshopper.Kernel;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    // Sends one MirrorElements request for every branch of the inputs, so a
    // tree input makes one undo step in Archicad (#203). The add-on edits the
    // items one after the other, so an element given in several branches is
    // still mirrored once for each of them, as with one request per branch.
    public class MirrorElementsComponent
        : BatchedExecutorComponent<ElementWithMirrorAxisParameters>
    {
        public override string CommandName => "MirrorElements";

        public MirrorElementsComponent()
            : base(
                "MirrorElements",
                "Mirror elements",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Elements Guids of the objects to mirror.");

            InPoints(
                "BeginPoints",
                "First points of the mirror axes (input only 1 point to mirror all elements across the same axis).");

            InPoints(
                "EndPoints",
                "Second points of the mirror axes (input only 1 point to mirror all elements across the same axis).");

            InBoolean(
                "Copy",
                "Mirror copies of the elements or mirror the elements themselves.",
                false);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when it succeeded).");
        }

        protected override bool TryGetBatchItems(
            IGH_DataAccess da,
            out List<ElementWithMirrorAxisParameters> items)
        {
            items = null;

            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return false;
            }

            if (!da.TryGetList(
                    1,
                    out List<Point2D> beginPoints))
            {
                return false;
            }

            if (!da.TryGetList(
                    2,
                    out List<Point2D> endPoints))
            {
                return false;
            }

            if (beginPoints.Count != 1 &&
                beginPoints.Count != elements.Elements.Count)
            {
                this.AddError(
                    "The count of BeginPoints must be 1 or equal to the count of ElementGuids.");
                return false;
            }

            if (endPoints.Count != 1 &&
                endPoints.Count != elements.Elements.Count)
            {
                this.AddError(
                    "The count of EndPoints must be 1 or equal to the count of ElementGuids.");
                return false;
            }

            if (!da.TryGet(
                    3,
                    out bool copy))
            {
                return false;
            }

            items = new List<ElementWithMirrorAxisParameters>();

            for (var i = 0; i < elements.Elements.Count; i++)
            {
                var element = elements.Elements[i];
                var beginPoint = beginPoints[beginPoints.Count == 1 ? 0 : i];
                var endPoint = endPoints[endPoints.Count == 1 ? 0 : i];

                items.Add(
                    new ElementWithMirrorAxisParameters()
                    {
                        Element = element.ElementId,
                        MirrorAxis = new MirrorAxis()
                        {
                            BeginPoint = beginPoint,
                            EndPoint = endPoint
                        },
                        Copy = copy
                    });
            }

            return true;
        }

        protected override object CreateBatchParameters(
            List<ElementWithMirrorAxisParameters> items)
        {
            return new MirrorElementsParameters
            {
                ElementsWithMirrorAxes = items
            };
        }

        // Reuses the MoveElements icon until MirrorElements gets its own
        // (this runner could not write a binary file).
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MoveElements;

        public override Guid ComponentGuid =>
            new Guid("7b3e1c52-6d94-4f0a-b8e7-2a5c9d41f6e3");
    }
}
