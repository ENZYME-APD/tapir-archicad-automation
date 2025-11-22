using Grasshopper.Kernel;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.ResponseTypes.Element;
using TapirGrasshopperPlugin.ResponseTypes.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SetDetailsOfWallsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetDetailsOfElements";

        public SetDetailsOfWallsComponent()
            : base(
                "Set Wall Details",
                "Set details of wall elements.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Elements Guids to get details of.");

            InPoints(
                "Begin coordinates",
                "Begin coordinates.");

            InPoints(
                "End coordinates",
                "End coordinates.");

            InNumbers(
                "Height",
                "Height.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            ElementsObj inputElements = ElementsObj.Create(
                da,
                0);
            if (inputElements == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input ElementGuids failed to collect data.");
                return;
            }

            List<Point3d> begCoords = new List<Point3d>();
            if (!da.GetDataList(
                    1,
                    begCoords))
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input BegCoords failed to collect data.");
                return;
            }

            List<Point3d> endCoords = new List<Point3d>();
            if (!da.GetDataList(
                    2,
                    endCoords))
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input EndCoords failed to collect data.");
                return;
            }

            List<double> heights = new List<double>();
            if (!da.GetDataList(
                    3,
                    heights))
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input Heights failed to collect data.");
                return;
            }

            if (begCoords.Count != 1 &&
                inputElements.Elements.Count != begCoords.Count)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "The count of BegCoords must be 1 or the same as the count of ElementGuids.");
                return;
            }

            if (endCoords.Count != 1 &&
                inputElements.Elements.Count != endCoords.Count)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "The count of EndCoords must be 1 or the same as the count of ElementGuids.");
                return;
            }

            if (heights.Count != 1 &&
                inputElements.Elements.Count != heights.Count)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "The count of Heights must be 1 or the same as the count of ElementGuids.");
                return;
            }

            if (inputElements.Elements.Count == 0)
            {
                return;
            }

            TypedElementsWithDetailsObj<SetWallDetails> obj =
                new TypedElementsWithDetailsObj<SetWallDetails>
                {
                    ElementsWithDetails =
                        new List<TypedElementWithDetailsObj<
                            SetWallDetails>>()
                };
            for (int i = 0; i < inputElements.Elements.Count; i++)
            {
                TypedElementWithDetailsObj<SetWallDetails> elementWithDetails =
                    new TypedElementWithDetailsObj<SetWallDetails>
                    {
                        ElementId = inputElements.Elements[i].ElementId,
                        Details = new TypedDetails<SetWallDetails>
                        {
                            TypeSpecificDetails = new SetWallDetails
                            {
                                BegCoordinate =
                                    Point2D.Create(
                                        begCoords
                                            [i % begCoords.Count]),
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

            if (!GetConvertedResponse(
                    CommandName,
                    obj,
                    out ExecutionResultsResponse executionResults))
            {
                return;
            }

            for (int i = 0; i < executionResults.ExecutionResults.Count; i++)
            {
                ExecutionResultBase eResult =
                    executionResults.ExecutionResults[i];
                if (eResult.Success)
                {
                    continue;
                }

                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    eResult.Message() + " [ElementId " +
                    inputElements.Elements[i].ToString() + "]");
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetWallDetails;

        public override Guid ComponentGuid =>
            new Guid("0ff1fd36-fd8b-4c9d-9db1-111cf9a9efa4");
    }
}