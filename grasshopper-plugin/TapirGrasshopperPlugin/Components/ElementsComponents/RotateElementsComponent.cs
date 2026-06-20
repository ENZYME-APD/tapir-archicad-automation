using Grasshopper.Kernel;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class RotateElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "RotateElements";

        public RotateElementsComponent()
            : base(
                "RotateElements",
                "Rotate elements",
                GroupNames.Elements)
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
                    out List<Point2D> beginPoints))
            {
                return;
            }

            if (!da.TryGetList(
                    2,
                    out List<Point2D> endPoints))
            {
                return;
            }

            if (!da.TryGetList(
                    3,
                    out List<Point2D> origins))
            {
                return;
            }

            if (beginPoints.Count != 1 &&
                beginPoints.Count != elements.Elements.Count)
            {
                this.AddError(
                    "The count of BeginPoints must be 1 or equal to the count of ElementGuids.");
                return;
            }

            if (endPoints.Count != 1 &&
                endPoints.Count != elements.Elements.Count)
            {
                this.AddError(
                    "The count of EndPoints must be 1 or equal to the count of ElementGuids.");
                return;
            }

            if (origins.Count != 1 &&
                origins.Count != elements.Elements.Count)
            {
                this.AddError(
                    "The count of Origins must be 1 or equal to the count of ElementGuids.");
                return;
            }

            if (!da.TryGet(
                    4,
                    out bool copy))
            {
                return;
            }

            var input = new RotateElementsParameters
            {
                ElementsWithRotations = new List<ElementWithRotationParameters>()
            };

            for (var i = 0; i < elements.Elements.Count; i++)
            {
                var element = elements.Elements[i];
                var beginPoint = beginPoints[beginPoints.Count == 1 ? 0 : i];
                var endPoint = endPoints[endPoints.Count == 1 ? 0 : i];
                var origin = origins[origins.Count == 1 ? 0 : i];

                input.ElementsWithRotations.Add(
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

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        public override Guid ComponentGuid =>
            new Guid("f2d7c9a3-4f85-4a86-9ac5-1cfb8b88d8dd");
    }
}
