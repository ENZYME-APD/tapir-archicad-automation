using Grasshopper.Kernel;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class MoveElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "MoveElements";

        public MoveElementsComponent()
            : base(
                "MoveElements",
                "Move elements",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "ElementsGuids of the objects to move.");

            InVectors(
                "Vectors",
                "The 3D vectors to move elements (input only 1 vector to move all elements with the same vector).");

            InBoolean(
                "Copy",
                "Move copies of the elements or move the elements themselves.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!ElementsObj.TryCreate(
                    da,
                    0,
                    out ElementsObj elements))
            {
                return;
            }

            if (!da.TryGetItems(
                    1,
                    out List<Vector3d> moveVectors))
            {
                return;
            }

            if (moveVectors.Count != 1 &&
                moveVectors.Count != elements.Elements.Count)
            {
                this.AddError(
                    "The size of the input MoveVectors must be 1 or equal to the size of the input ElementGuids.");
                return;
            }

            if (!da.TryGetItem(
                    2,
                    out bool copy))
            {
                return;
            }

            var input = new MoveElementsParameters()
            {
                ElementsWithMoveParameters =
                    new List<ElementWithMoveParameters>()
            };
            for (var i = 0; i < elements.Elements.Count; i++)
            {
                var element = elements.Elements[i];
                var moveVector = moveVectors[moveVectors.Count == 1 ? 0 : i];
                input.ElementsWithMoveParameters.Add(
                    new ElementWithMoveParameters()
                    {
                        Element = element.ElementId,
                        MoveVector = new Vector3D()
                        {
                            X = moveVector.X,
                            Y = moveVector.Y,
                            Z = moveVector.Z
                        },
                        Copy = copy
                    });
            }

            SetArchiCadValues(
                CommandName,
                input);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MoveElements;

        public override Guid ComponentGuid =>
            new Guid("e37db5dc-9ac6-42e6-836c-3c2b04b15d95");
    }
}