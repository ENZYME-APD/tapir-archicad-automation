using Grasshopper.Kernel;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    // Sends one RotateElements request for every branch of the inputs, so a
    // tree input makes one undo step in Archicad (#203). The add-on edits the
    // items one after the other, so an element given in several branches is
    // still rotated once for each of them, as with one request per branch.
    public class RotateElementsComponent
        : BatchedExecutorComponent<ElementWithRotationParameters>
    {
        public override string CommandName => "RotateElements";

        public RotateElementsComponent()
            : base(
                "RotateElements",
                "Rotate elements",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Elements Guids of the objects to rotate.");

            InPoints(
                "BeginPoints",
                "Begin points for the rotation arc (input only 1 point to rotate all elements with the same begin point).");

            InPoints(
                "EndPoints",
                "End points for the rotation arc (input only 1 point to rotate all elements with the same end point).");

            InPoints(
                "Origins",
                "Origin points for the rotation (input only 1 point to rotate all elements around the same origin).");

            InBoolean(
                "Copy",
                "Rotate copies of the elements or rotate the elements themselves.",
                false);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when it succeeded).");
        }

        protected override bool TryGetBatchItems(
            IGH_DataAccess da,
            out List<ElementWithRotationParameters> items)
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

            if (!da.TryGetList(
                    3,
                    out List<Point2D> origins))
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

            if (origins.Count != 1 &&
                origins.Count != elements.Elements.Count)
            {
                this.AddError(
                    "The count of Origins must be 1 or equal to the count of ElementGuids.");
                return false;
            }

            if (!da.TryGet(
                    4,
                    out bool copy))
            {
                return false;
            }

            items = new List<ElementWithRotationParameters>();

            for (var i = 0; i < elements.Elements.Count; i++)
            {
                var element = elements.Elements[i];
                var beginPoint = beginPoints[beginPoints.Count == 1 ? 0 : i];
                var endPoint = endPoints[endPoints.Count == 1 ? 0 : i];
                var origin = origins[origins.Count == 1 ? 0 : i];

                items.Add(
                    new ElementWithRotationParameters()
                    {
                        Element = element.ElementId,
                        Rotation = new Rotation()
                        {
                            BeginPoint = beginPoint,
                            EndPoint = endPoint,
                            Origin = origin
                        },
                        Copy = copy
                    });
            }

            return true;
        }

        protected override object CreateBatchParameters(
            List<ElementWithRotationParameters> items)
        {
            return new RotateElementsParameters
            {
                ElementsWithRotations = items
            };
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RotateElements;

        public override Guid ComponentGuid =>
            new Guid("f2d7c9a3-4f85-4a86-9ac5-1cfb8b88d8dd");
    }
}
