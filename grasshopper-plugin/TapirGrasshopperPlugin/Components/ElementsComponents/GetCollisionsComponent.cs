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
        [JsonProperty ("elements")]
        public List<ElementIdItemObj> Elements;

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
            pManager.AddGenericParameter ("ElementGuids", "ElementGuids", "Element ids to check collisions.", GH_ParamAccess.list);
            pManager.AddNumberParameter ("VolumeTolerance", "VolumeTolerance", "Intersection body volume greater then this value will be considered as a collision.", GH_ParamAccess.item, @default: 0.001);
            pManager.AddBooleanParameter ("PerformSurfaceCheck", "PerformSurfaceCheck", "Enables surface collision check. If disabled the surfaceTolerance value will be ignored.", GH_ParamAccess.item, @default: false);
            pManager.AddNumberParameter ("SurfaceTolerance", "SurfaceTolerance", "Intersection body surface area greater then this value will be considered as a collision.", GH_ParamAccess.item, @default: 0.001);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementGuids", "ElementGuids", "Elements with detected collisions.", GH_ParamAccess.list);
            pManager.AddGenericParameter ("ElementGuids", "ElementGuids", "Elements with detected collisions.", GH_ParamAccess.list);
            pManager.AddBooleanParameter ("HasBodyCollision", "HasBodyCollision", "The element has body collision.", GH_ParamAccess.list);
            pManager.AddBooleanParameter ("HasClearenceCollision", "HasClearenceCollision", "The element has clearance collision.", GH_ParamAccess.list);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            ElementsObj inputElements = ElementsObj.Create (DA, 0);
            if (inputElements == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input ElementGuids failed to collect data.");
                return;
            }

            double volumeTolerance = 0.001;
            if (!DA.GetData (1, ref volumeTolerance)) {
                return;
            }

            bool performSurfaceCheck = false;
            if (!DA.GetData (2, ref performSurfaceCheck)) {
                return;
            }

            double surfaceTolerance = 0.001;
            if (!DA.GetData (3, ref surfaceTolerance)) {
                return;
            }

            GetCollisionsParameters parameters = new GetCollisionsParameters () {
                Elements = inputElements.Elements,
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

            List<ElementIdItemObj> elementId1s = new List<ElementIdItemObj> ();
            List<ElementIdItemObj> elementId2s = new List<ElementIdItemObj> ();
            List<bool> hasBodyCollisions = new List<bool> ();
            List<bool> hasClearenceCollisions = new List<bool> ();
            CollisionsOutput collisions = response.Result.ToObject<CollisionsOutput> ();
            foreach (Collision c in collisions.Collisions) {
                elementId1s.Add (new ElementIdItemObj { ElementId = c.ElementId1 });
                elementId2s.Add (new ElementIdItemObj { ElementId = c.ElementId2 });
                hasBodyCollisions.Add (c.HasBodyCollision);
                hasClearenceCollisions.Add (c.HasClearenceCollision);
            }
            DA.SetDataList (0, elementId1s);
            DA.SetDataList (1, elementId2s);
            DA.SetDataList (2, hasBodyCollisions);
            DA.SetDataList (3, hasClearenceCollisions);
        }

        // protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.Collisions;

        public override Guid ComponentGuid => new Guid ("6ff649b0-89a0-466a-aaa6-3b0b5eef70ee");
    }
}