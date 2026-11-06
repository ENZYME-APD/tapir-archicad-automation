using Grasshopper.Kernel;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class MirrorElementsComponent : ArchicadExecutorComponent
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

            if (!da.TryGet(
                    3,
                    out bool copy))
            {
                return;
            }

            var input = new MirrorElementsParameters
            {
                ElementsWithMirrorAxes = new List<ElementWithMirrorAxisParameters>()
            };

            for (var i = 0; i < elements.Elements.Count; i++)
            {
                var element = elements.Elements[i];
                var beginPoint = beginPoints[beginPoints.Count == 1 ? 0 : i];
                var endPoint = endPoints[endPoints.Count == 1 ? 0 : i];

                input.ElementsWithMirrorAxes.Add(
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

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        // Reuses the MoveElements icon until MirrorElements gets its own
        // (this runner could not write a binary file).
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MoveElements;

        public override Guid ComponentGuid =>
            new Guid("7b3e1c52-6d94-4f0a-b8e7-2a5c9d41f6e3");
    }
}
