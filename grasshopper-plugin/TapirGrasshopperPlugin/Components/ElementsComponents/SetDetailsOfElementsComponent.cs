using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.ResponseTypes.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SetWallDetails
    {
        [JsonProperty("begCoordinate")]
        public Point2D BegCoordinate;

        [JsonProperty("endCoordinate")]
        public Point2D EndCoordinate;

        [JsonProperty("height")]
        public double Height;
    }

    public class TypedDetails<T>
    {
        [JsonProperty("typeSpecificDetails")]
        public T TypeSpecificDetails;
    }

    public class TypedElementWithDetailsObj<T>
    {
        [JsonProperty("elementId")]
        public ElementIdObj ElementId;

        [JsonProperty("details")]
        public TypedDetails<T> Details;
    }

    public class TypedElementsWithDetailsObj<T>
    {
        [JsonProperty("elementsWithDetails")]
        public List<TypedElementWithDetailsObj<T>> ElementsWithDetails;
    }

    public class SetDetailsOfWallsComponent : ArchicadExecutorComponent
    {
        public static string Doc => "Set details of wall elements.";
        public override string CommandName => "SetDetailsOfElements";

        public SetDetailsOfWallsComponent()
            : base(
                "Set Wall Details",
                "SetWallDetails",
                Doc,
                GroupNames.Elements)
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "ElementGuids",
                "ElementGuids",
                "Elements Guids to get details of.",
                GH_ParamAccess.list);
            pManager.AddPointParameter(
                "Begin coordinates",
                "BegCoords",
                "Begin coordinates.",
                GH_ParamAccess.list);
            pManager.AddPointParameter(
                "End coordinates",
                "EndCoords",
                "End coordinates.",
                GH_ParamAccess.list);
            pManager.AddNumberParameter(
                "Height",
                "Height",
                "Height.",
                GH_ParamAccess.list);
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var inputElements = ElementsObj.Create(
                da,
                0);
            if (inputElements == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input ElementGuids failed to collect data.");
                return;
            }

            var begCoords = new List<Point3d>();
            if (!da.GetDataList(
                    1,
                    begCoords))
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input BegCoords failed to collect data.");
                return;
            }

            var endCoords = new List<Point3d>();
            if (!da.GetDataList(
                    2,
                    endCoords))
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input EndCoords failed to collect data.");
                return;
            }

            var heights = new List<double>();
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

            var obj = new TypedElementsWithDetailsObj<SetWallDetails>()
            {
                ElementsWithDetails =
                    new List<TypedElementWithDetailsObj<SetWallDetails>>()
            };
            for (var i = 0; i < inputElements.Elements.Count; i++)
            {
                var elementWithDetails =
                    new TypedElementWithDetailsObj<SetWallDetails>()
                    {
                        ElementId = inputElements.Elements[i].ElementId,
                        Details = new TypedDetails<SetWallDetails>()
                        {
                            TypeSpecificDetails = new SetWallDetails()
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

            for (var i = 0; i < executionResults.ExecutionResults.Count; i++)
            {
                var eResult = executionResults.ExecutionResults[i];
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