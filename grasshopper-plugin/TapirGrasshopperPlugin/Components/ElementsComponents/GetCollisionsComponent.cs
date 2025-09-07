using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CollisionDetectionSettings
    {
        [JsonProperty ("volumeTolerance")]
        public double VolumeTolerance;

        [JsonProperty ("performSurfaceCheck")]
        public bool PerformSurfaceCheck;

        [JsonProperty ("surfaceTolerance")]
        public double SurfaceTolerance;
    }

    public class GetCollisionsParameters
    {
        [JsonProperty ("elementsGroup1")]
        public List<ElementIdItemObj> ElementsGroup1;

        [JsonProperty ("elementsGroup2")]
        public List<ElementIdItemObj> ElementsGroup2;

        [JsonProperty ("settings")]
        public CollisionDetectionSettings Settings;
    }

    public class Collision
    {
        [JsonProperty ("elementId1")]
        public ElementIdObj ElementId1;

        [JsonProperty ("elementId2")]
        public ElementIdObj ElementId2;

        [JsonProperty ("hasBodyCollision")]
        public bool HasBodyCollision;

        [JsonProperty ("hasClearenceCollision")]
        public bool HasClearenceCollision;
    }

    public class CollisionsOutput
    {

        [JsonProperty ("collisions")]
        public List<Collision> Collisions;
    }

    public class GetCollisionsComponent : ArchicadAccessorComponent
    {
        public GetCollisionsComponent ()
          : base (
                "Collisions",
                "Collisions",
                "Detects collisions between elements.",
                "Elements"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementsGroup1", "ElementsGroup1", "The first group of Elements to check collisions with the second group.", GH_ParamAccess.list);
            pManager.AddGenericParameter ("ElementsGroup2", "ElementsGroup2", "The second group of Elements to check collisions with the first group.", GH_ParamAccess.list);
            pManager.AddNumberParameter ("VolumeTolerance", "VolumeTolerance", "Intersection body volume greater then this value will be considered as a collision.", GH_ParamAccess.item, @default: 0.001);
            pManager.AddBooleanParameter ("PerformSurfaceCheck", "PerformSurfaceCheck", "Enables surface collision check. If disabled the surfaceTolerance value will be ignored.", GH_ParamAccess.item, @default: false);
            pManager.AddNumberParameter ("SurfaceTolerance", "SurfaceTolerance", "Intersection body surface area greater then this value will be considered as a collision.", GH_ParamAccess.item, @default: 0.001);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddIntegerParameter ("IndexOfElementFromGroup1", "IndexFromGroup1", "The index of Element with detected collision from group1.", GH_ParamAccess.list);
            pManager.AddGenericParameter ("ElementGuidFromGroup1", "ElementGuid1", "Element id from the group1.", GH_ParamAccess.list);
            pManager.AddIntegerParameter ("IndexOfElementFromGroup2", "IndexFromGroup2", "The index of Element with detected collision from group2.", GH_ParamAccess.list);
            pManager.AddGenericParameter ("ElementGuidFromGroup2", "ElementGuid2", "Element id from the group2.", GH_ParamAccess.list);
            pManager.AddBooleanParameter ("HasBodyCollision", "HasBodyCollision", "The element has body collision.", GH_ParamAccess.list);
            pManager.AddBooleanParameter ("HasClearenceCollision", "HasClearenceCollision", "The element has clearance collision.", GH_ParamAccess.list);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            ElementsObj inputElementsGroup1 = ElementsObj.Create (DA, 0);
            if (inputElementsGroup1 == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input ElementsGroup1 failed to collect data.");
                return;
            }

            ElementsObj inputElementsGroup2 = ElementsObj.Create (DA, 1);
            if (inputElementsGroup2 == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input ElementsGroup2 failed to collect data.");
                return;
            }

            double volumeTolerance = 0.001;
            if (!DA.GetData (2, ref volumeTolerance)) {
                return;
            }

            bool performSurfaceCheck = false;
            if (!DA.GetData (3, ref performSurfaceCheck)) {
                return;
            }

            double surfaceTolerance = 0.001;
            if (!DA.GetData (4, ref surfaceTolerance)) {
                return;
            }

            GetCollisionsParameters parameters = new GetCollisionsParameters () {
                ElementsGroup1 = inputElementsGroup1.Elements,
                ElementsGroup2 = inputElementsGroup2.Elements,
                Settings = new CollisionDetectionSettings {
                    VolumeTolerance = volumeTolerance,
                    PerformSurfaceCheck = performSurfaceCheck,
                    SurfaceTolerance = surfaceTolerance
                }
            };
            JObject parametersObj = JObject.FromObject (parameters);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "GetCollisions", parametersObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }

            List<int> elementIndex1s = new List<int> ();
            List<ElementIdItemObj> elementId1s = new List<ElementIdItemObj> ();
            List<int> elementIndex2s = new List<int> ();
            List<ElementIdItemObj> elementId2s = new List<ElementIdItemObj> ();
            List<bool> hasBodyCollisions = new List<bool> ();
            List<bool> hasClearenceCollisions = new List<bool> ();
            CollisionsOutput collisions = response.Result.ToObject<CollisionsOutput> ();
            foreach (Collision c in collisions.Collisions) {
                elementIndex1s.Add (inputElementsGroup1.Elements.FindIndex (e => e.ElementId.Equals (c.ElementId1)));
                elementId1s.Add (new ElementIdItemObj { ElementId = c.ElementId1 });
                elementIndex2s.Add (inputElementsGroup2.Elements.FindIndex (e => e.ElementId.Equals (c.ElementId2)));
                elementId2s.Add (new ElementIdItemObj { ElementId = c.ElementId2 });
                hasBodyCollisions.Add (c.HasBodyCollision);
                hasClearenceCollisions.Add (c.HasClearenceCollision);
            }
            DA.SetDataList (0, elementIndex1s);
            DA.SetDataList (1, elementId1s);
            DA.SetDataList (2, elementIndex2s);
            DA.SetDataList (3, elementId2s);
            DA.SetDataList (4, hasBodyCollisions);
            DA.SetDataList (5, hasClearenceCollisions);
        }

        // protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.Collisions;

        public override Guid ComponentGuid => new Guid ("6ff649b0-89a0-466a-aaa6-3b0b5eef70ee");
    }
}