using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    // Sends one SetDetailsOfElements request for every branch of the inputs,
    // so a tree input makes one undo step in Archicad (#203). It has no
    // ErrorMessages output: a failed element is reported as an error.
    public class SetDetailsOfWallsComponent
        : BatchedExecutorComponent<TypedElementWithDetailsObj<SetWallDetails>>
    {
        public override string CommandName => "SetDetailsOfElements";

        public SetDetailsOfWallsComponent()
            : base(
                "SetWallDetails",
                "Set details of Wall elements.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Elements Guids to get details of.");

            InPoints("StartCoordinates");

            InPoints("EndCoordinates");

            InNumbers("Heights");
        }

        protected override int ErrorMessagesOutputIndex => -1;

        protected override bool TryGetBatchItems(
            IGH_DataAccess da,
            out List<TypedElementWithDetailsObj<SetWallDetails>> items)
        {
            items = null;

            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject inputs))
            {
                return false;
            }

            if (!da.TryGetList(
                    1,
                    out List<Point3D> startCoords))
            {
                return false;
            }

            if (!da.TryGetList(
                    2,
                    out List<Point3D> endCoords))
            {
                return false;
            }

            if (!da.TryGetList(
                    3,
                    out List<double> heights))
            {
                return false;
            }

            if (startCoords.Count != 1 &&
                inputs.Elements.Count != startCoords.Count)
            {
                this.AddError(
                    "The count of BegCoords must be 1 or the same as the count of ElementGuids.");
                return false;
            }

            if (endCoords.Count != 1 &&
                inputs.Elements.Count != endCoords.Count)
            {
                this.AddError(
                    "The count of EndCoords must be 1 or the same as the count of ElementGuids.");
                return false;
            }

            if (heights.Count != 1 && inputs.Elements.Count != heights.Count)
            {
                this.AddError(
                    "The count of Heights must be 1 or the same as the count of ElementGuids.");
                return false;
            }

            items = new List<TypedElementWithDetailsObj<SetWallDetails>>();

            for (int i = 0; i < inputs.Elements.Count; i++)
            {
                TypedElementWithDetailsObj<SetWallDetails> elementWithDetails =
                    new TypedElementWithDetailsObj<SetWallDetails>
                    {
                        ElementId = inputs.Elements[i].ElementId,
                        Details = new TypedDetails<SetWallDetails>
                        {
                            TypeSpecificDetails = new SetWallDetails
                            {
                                BegCoordinate =
                                    Point2D.Create(
                                        startCoords
                                            [i % startCoords
                                                .Count]),
                                EndCoordinate =
                                    Point2D.Create(
                                        endCoords[
                                            i % endCoords.Count]),
                                Height = heights[i % heights.Count]
                            }
                        }
                    };
                items.Add(elementWithDetails);
            }

            return true;
        }

        protected override object CreateBatchParameters(
            List<TypedElementWithDetailsObj<SetWallDetails>> items)
        {
            return new TypedElementsWithDetailsObj<SetWallDetails>
            {
                ElementsWithDetails = items
            };
        }

        protected override void ReportBatchErrors(
            List<TypedElementWithDetailsObj<SetWallDetails>> sentItems,
            List<string> messages)
        {
            for (int i = 0; i < messages.Count; i++)
            {
                if (string.IsNullOrEmpty(messages[i]))
                {
                    continue;
                }

                this.AddError(
                    messages[i] + " [ElementId " +
                    sentItems[i].ElementId?.Guid + "]");
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetWallDetails;

        public override Guid ComponentGuid =>
            new Guid("0ff1fd36-fd8b-4c9d-9db1-111cf9a9efa4");
    }

    // Sends one SetDetailsOfElements request for every branch of the inputs,
    // so a tree input makes one undo step in Archicad (#203). It has no
    // ErrorMessages output: a failed element is reported as an error.
    public class SetDetailsOfTextsComponent
        : BatchedExecutorComponent<TypedElementWithDetailsObj<SetTextDetails>>
    {
        public override string CommandName => "SetDetailsOfElements";

        public SetDetailsOfTextsComponent()
            : base(
                "SetTextDetails",
                "Set the text content of Text elements and text-type Labels.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Elements Guids of the Text elements or text-type Labels to modify.");

            InTexts(
                "Texts",
                "The new text contents. Newlines create multiple lines.");
        }

        protected override int ErrorMessagesOutputIndex => -1;

        protected override bool TryGetBatchItems(
            IGH_DataAccess da,
            out List<TypedElementWithDetailsObj<SetTextDetails>> items)
        {
            items = null;

            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject inputs))
            {
                return false;
            }

            if (!da.TryGetList(
                    1,
                    out List<string> texts))
            {
                return false;
            }

            if (texts.Count != 1 && inputs.Elements.Count != texts.Count)
            {
                this.AddError(
                    "The count of Texts must be 1 or the same as the count of ElementGuids.");
                return false;
            }

            items = new List<TypedElementWithDetailsObj<SetTextDetails>>();

            for (int i = 0; i < inputs.Elements.Count; i++)
            {
                TypedElementWithDetailsObj<SetTextDetails> elementWithDetails =
                    new TypedElementWithDetailsObj<SetTextDetails>
                    {
                        ElementId = inputs.Elements[i].ElementId,
                        Details = new TypedDetails<SetTextDetails>
                        {
                            TypeSpecificDetails = new SetTextDetails
                            {
                                Text = texts[i % texts.Count]
                            }
                        }
                    };
                items.Add(elementWithDetails);
            }

            return true;
        }

        protected override object CreateBatchParameters(
            List<TypedElementWithDetailsObj<SetTextDetails>> items)
        {
            return new TypedElementsWithDetailsObj<SetTextDetails>
            {
                ElementsWithDetails = items
            };
        }

        protected override void ReportBatchErrors(
            List<TypedElementWithDetailsObj<SetTextDetails>> sentItems,
            List<string> messages)
        {
            for (int i = 0; i < messages.Count; i++)
            {
                if (string.IsNullOrEmpty(messages[i]))
                {
                    continue;
                }

                this.AddError(
                    messages[i] + " [ElementId " +
                    sentItems[i].ElementId?.Guid + "]");
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetTextDetails;

        public override Guid ComponentGuid =>
            new Guid("6e713c87-c420-4c6e-8c6c-d5711cc81a23");
    }
}