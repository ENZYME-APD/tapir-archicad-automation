using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SetWallDetails
    {
        [JsonProperty ("begCoordinate")]
        public Point2D BegCoordinate;

        [JsonProperty ("endCoordinate")]
        public Point2D EndCoordinate;

        [JsonProperty ("height")]
        public double Height;
    }

    public class TypedDetails<T>
    {
        [JsonProperty ("typeSpecificDetails")]
        public T TypeSpecificDetails;
    }

    public class TypedElementWithDetailsObj<T>
    {
        [JsonProperty ("elementId")]
        public ElementIdObj ElementId;

        [JsonProperty ("details")]
        public TypedDetails<T> Details;
    }

    public class TypedElementsWithDetailsObj<T>
    {
        [JsonProperty ("elementsWithDetails")]
        public List<TypedElementWithDetailsObj<T>> ElementsWithDetails;
    }

    public class SetDetailsOfWallsComponent : ArchicadAccessorComponent
    {
        public SetDetailsOfWallsComponent ()
          : base (
                "Set Wall Details",
                "SetWallDetails",
                "Set details of wall elements.",
                "Elements"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementGuids", "ElementGuids", "Element Guids to get details of.", GH_ParamAccess.list);
            pManager.AddPointParameter ("Begin coordinates", "BegCoords", "Begin coordinates.", GH_ParamAccess.list);
            pManager.AddPointParameter ("End coordinates", "EndCoords", "End coordinates.", GH_ParamAccess.list);
            pManager.AddNumberParameter ("Height", "Height", "Height.", GH_ParamAccess.list);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            ElementsObj inputElements = ElementsObj.Create (DA, 0);
            if (inputElements == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input ElementGuids failed to collect data.");
                return;
            }

            List<Point3d> begCoords = new List<Point3d> ();
            if (!DA.GetDataList (1, begCoords)) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input BegCoords failed to collect data.");
                return;
            }

            List<Point3d> endCoords = new List<Point3d> ();
            if (!DA.GetDataList (2, endCoords)) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input EndCoords failed to collect data.");
                return;
            }

            List<double> heights = new List<double> ();
            if (!DA.GetDataList (3, heights)) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input Heights failed to collect data.");
                return;
            }

            if (begCoords.Count != 1 && inputElements.Elements.Count != begCoords.Count) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "The count of BegCoords must be 1 or the same as the count of ElementGuids.");
                return;
            }

            if (endCoords.Count != 1 && inputElements.Elements.Count != endCoords.Count) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "The count of EndCoords must be 1 or the same as the count of ElementGuids.");
                return;
            }

            if (heights.Count != 1 && inputElements.Elements.Count != heights.Count) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "The count of Heights must be 1 or the same as the count of ElementGuids.");
                return;
            }

            if (inputElements.Elements.Count == 0) {
                return;
            }

            TypedElementsWithDetailsObj<SetWallDetails> obj = new TypedElementsWithDetailsObj<SetWallDetails> () {
                ElementsWithDetails = new List<TypedElementWithDetailsObj<SetWallDetails>> ()
            };
            for (int i = 0; i < inputElements.Elements.Count; i++) {
                TypedElementWithDetailsObj<SetWallDetails> elementWithDetails = new TypedElementWithDetailsObj<SetWallDetails> () {
                    ElementId = inputElements.Elements[i].ElementId,
                    Details = new TypedDetails<SetWallDetails> () {
                        TypeSpecificDetails = new SetWallDetails () {
                            BegCoordinate = Point2D.Create (begCoords[i % begCoords.Count]),
                            EndCoordinate = Point2D.Create (endCoords[i % endCoords.Count]),
                            Height = heights[i % heights.Count]
                        }
                    }
                };
                obj.ElementsWithDetails.Add (elementWithDetails);
            }

            JObject elementsWithDetailsObj = JObject.FromObject (obj);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "SetDetailsOfElements", elementsWithDetailsObj);
            ExecutionResultsObj executionResults = response.Result.ToObject<ExecutionResultsObj> ();
            for (int i = 0; i < executionResults.ExecutionResults.Count; i++) {
                ExecutionResultObj executionResult = executionResults.ExecutionResults[i];
                if (executionResult.Success) {
                    continue;
                }
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, executionResult.Error.Message + " [ElementId " + inputElements.Elements[i].ToString () + "]");
            }
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.SetWallDetails;

        public override Guid ComponentGuid => new Guid ("0ff1fd36-fd8b-4c9d-9db1-111cf9a9efa4");
    }
}
