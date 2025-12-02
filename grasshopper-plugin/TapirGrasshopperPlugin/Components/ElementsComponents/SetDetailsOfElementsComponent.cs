using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;
using TapirGrasshopperPlugin.ResponseTypes.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SetDetailsOfWallsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetDetailsOfElements";

        public SetDetailsOfWallsComponent()
            : base(
                "SetWallDetails",
                "Set details of Wall elements.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Elements Guids to get details of.");

            InPoints("StartCoordinates");
            InPoints("EndCoordinates");
            InNumbers("Height");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObj inputs))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<Point3D> startCoords))
            {
                return;
            }

            if (!da.TryGetList(
                    2,
                    out List<Point3D> endCoords))
            {
                return;
            }

            if (!da.TryGetList(
                    3,
                    out List<double> heights))
            {
                return;
            }

            if (startCoords.Count != 1 &&
                inputs.Elements.Count != startCoords.Count)
            {
                this.AddError(
                    "The count of BegCoords must be 1 or the same as the count of ElementGuids.");
                return;
            }

            if (endCoords.Count != 1 &&
                inputs.Elements.Count != endCoords.Count)
            {
                this.AddError(
                    "The count of EndCoords must be 1 or the same as the count of ElementGuids.");
                return;
            }

            if (heights.Count != 1 && inputs.Elements.Count != heights.Count)
            {
                this.AddError(
                    "The count of Heights must be 1 or the same as the count of ElementGuids.");
                return;
            }

            if (inputs.Elements.Count == 0)
            {
                return;
            }

            var obj = new TypedElementsWithDetailsObj<SetWallDetails>
            {
                ElementsWithDetails =
                    new List<TypedElementWithDetailsObj<SetWallDetails>>()
            };

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
                obj.ElementsWithDetails.Add(elementWithDetails);
            }

            if (!TryGetConvertedValues(
                    CommandName,
                    obj,
                    ToAddOn,
                    ExecutionResultsResponse.Deserialize,
                    out ExecutionResultsResponse response))
            {
                return;
            }

            for (int i = 0; i < response.ExecutionResults.Count; i++)
            {
                ExecutionResult eResult = response.ExecutionResults[i];
                if (eResult.Success)
                {
                    continue;
                }

                this.AddError(
                    eResult.Message() + " [NewElementId " +
                    inputs.Elements[i].ToString() + "]");
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetWallDetails;

        public override Guid ComponentGuid =>
            new Guid("0ff1fd36-fd8b-4c9d-9db1-111cf9a9efa4");
    }
}