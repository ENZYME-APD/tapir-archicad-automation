using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CollisionDetectionSettings
    {
        [JsonProperty("volumeTolerance")]
        public double VolumeTolerance;

        [JsonProperty("performSurfaceCheck")]
        public bool PerformSurfaceCheck;

        [JsonProperty("surfaceTolerance")]
        public double SurfaceTolerance;
    }

    public class GetCollisionsParameters
    {
        [JsonProperty("elementsGroup1")]
        public List<ElementIdItemObj> ElementsGroup1;

        [JsonProperty("elementsGroup2")]
        public List<ElementIdItemObj> ElementsGroup2;

        [JsonProperty("settings")]
        public CollisionDetectionSettings Settings;
    }

    public class Collision
    {
        [JsonProperty("elementId1")]
        public ElementIdObj ElementId1;

        [JsonProperty("elementId2")]
        public ElementIdObj ElementId2;

        [JsonProperty("hasBodyCollision")]
        public bool HasBodyCollision;

        [JsonProperty("hasClearenceCollision")]
        public bool HasClearenceCollision;
    }

    public class CollisionsOutput
    {
        [JsonProperty("collisions")]
        public List<Collision> Collisions;
    }

    public class GetCollisionsComponent : ArchicadAccessorComponent
    {
        public static string Doc => "Detects collisions between elements.";
        public override string CommandName => "GetCollisions";

        public GetCollisionsComponent()
            : base(
                "Collisions",
                "Collisions",
                Doc,
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            AddGenerics(
                "ElementsGroup1",
                "The first group of Elements to check collisions with the second group.");

            AddGenerics(
                "ElementsGroup2",
                "The second group of Elements to check collisions with the first group.");

            AddNumber(
                "VolumeTolerance",
                "Intersection body volume greater then this value will be considered as a collision.",
                Tolerances.Main);

            AddBoolean(
                "PerformSurfaceCheck",
                "Enables surface collision check. If disabled the surfaceTolerance value will be ignored.");


            AddNumber(
                "SurfaceTolerance",
                "Intersection body surface area greater then this value will be considered as a collision.",
                Tolerances.Main);
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddIntegerParameter(
                "IndexOfElementFromGroup1",
                "IndexFromGroup1",
                "The index of Elements with detected collision from group1.",
                GH_ParamAccess.list);
            pManager.AddGenericParameter(
                "ElementGuidFromGroup1",
                "ElementGuid1",
                "Elements id from the group1.",
                GH_ParamAccess.list);
            pManager.AddIntegerParameter(
                "IndexOfElementFromGroup2",
                "IndexFromGroup2",
                "The index of Elements with detected collision from group2.",
                GH_ParamAccess.list);
            pManager.AddGenericParameter(
                "ElementGuidFromGroup2",
                "ElementGuid2",
                "Elements id from the group2.",
                GH_ParamAccess.list);
            pManager.AddBooleanParameter(
                "HasBodyCollision",
                "HasBodyCollision",
                "The element has body collision.",
                GH_ParamAccess.list);
            pManager.AddBooleanParameter(
                "HasClearenceCollision",
                "HasClearenceCollision",
                "The element has clearance collision.",
                GH_ParamAccess.list);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var inputElementsGroup1 = ElementsObj.Create(
                da,
                0);
            if (inputElementsGroup1 == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input ElementsGroup1 failed to collect data.");
                return;
            }

            var inputElementsGroup2 = ElementsObj.Create(
                da,
                1);
            if (inputElementsGroup2 == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input ElementsGroup2 failed to collect data.");
                return;
            }

            var volumeTolerance = 0.001;
            if (!da.GetData(
                    2,
                    ref volumeTolerance))
            {
                return;
            }

            var performSurfaceCheck = false;
            if (!da.GetData(
                    3,
                    ref performSurfaceCheck))
            {
                return;
            }

            var surfaceTolerance = 0.001;
            if (!da.GetData(
                    4,
                    ref surfaceTolerance))
            {
                return;
            }

            var parameters = new GetCollisionsParameters()
            {
                ElementsGroup1 = inputElementsGroup1.Elements,
                ElementsGroup2 = inputElementsGroup2.Elements,
                Settings = new CollisionDetectionSettings
                {
                    VolumeTolerance = volumeTolerance,
                    PerformSurfaceCheck = performSurfaceCheck,
                    SurfaceTolerance = surfaceTolerance
                }
            };

            if (!GetConvertedResponse(
                    CommandName,
                    parameters,
                    out CollisionsOutput collisions))
            {
                return;
            }

            var elementIndex1s = new List<int>();
            var elementId1s = new List<ElementIdItemObj>();
            var elementIndex2s = new List<int>();
            var elementId2s = new List<ElementIdItemObj>();
            var hasBodyCollisions = new List<bool>();
            var hasClearenceCollisions = new List<bool>();

            foreach (var c in collisions.Collisions)
            {
                elementIndex1s.Add(
                    inputElementsGroup1.Elements.FindIndex(e =>
                        e.ElementId.Equals(c.ElementId1)));
                elementId1s.Add(
                    new ElementIdItemObj { ElementId = c.ElementId1 });
                elementIndex2s.Add(
                    inputElementsGroup2.Elements.FindIndex(e =>
                        e.ElementId.Equals(c.ElementId2)));
                elementId2s.Add(
                    new ElementIdItemObj { ElementId = c.ElementId2 });
                hasBodyCollisions.Add(c.HasBodyCollision);
                hasClearenceCollisions.Add(c.HasClearenceCollision);
            }

            da.SetDataList(
                0,
                elementIndex1s);
            da.SetDataList(
                1,
                elementId1s);
            da.SetDataList(
                2,
                elementIndex2s);
            da.SetDataList(
                3,
                elementId2s);
            da.SetDataList(
                4,
                hasBodyCollisions);
            da.SetDataList(
                5,
                hasClearenceCollisions);
        }

        // protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.Collisions;

        public override Guid ComponentGuid =>
            new Guid("6ff649b0-89a0-466a-aaa6-3b0b5eef70ee");
    }
}