using Grasshopper.Kernel;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    // Sends one MoveElements request for every branch of the inputs, so a
    // tree input makes one undo step in Archicad (#203). The add-on edits the
    // items one after the other, so an element given in several branches is
    // still moved once for each of them, as with one request per branch.
    public class MoveElementsComponent
        : BatchedExecutorComponent<ElementWithMoveParameters>
    {
        public override string CommandName => "MoveElements";

        public MoveElementsComponent()
            : base(
                "MoveElements",
                "Move elements",
                GroupNames.ElementModification)
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
                "Move copies of the elements or move the elements themselves.",
                false);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when it succeeded).");
        }

        protected override bool TryGetBatchItems(
            IGH_DataAccess da,
            out List<ElementWithMoveParameters> items)
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
                    out List<Vector3d> moveVectors))
            {
                return false;
            }

            if (moveVectors.Count != 1 &&
                moveVectors.Count != elements.Elements.Count)
            {
                this.AddError(
                    "The size of the input MoveVectors must be 1 or equal to the size of the input ElementGuids.");
                return false;
            }

            if (!da.TryGet(
                    2,
                    out bool copy))
            {
                return false;
            }

            items = new List<ElementWithMoveParameters>();

            for (var i = 0; i < elements.Elements.Count; i++)
            {
                var element = elements.Elements[i];
                var moveVector = moveVectors[moveVectors.Count == 1 ? 0 : i];

                items.Add(
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

            return true;
        }

        protected override object CreateBatchParameters(
            List<ElementWithMoveParameters> items)
        {
            return new MoveElementsParameters
            {
                ElementsWithMoveParameters = items
            };
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MoveElements;

        public override Guid ComponentGuid =>
            new Guid("e37db5dc-9ac6-42e6-836c-3c2b04b15d95");
    }
}