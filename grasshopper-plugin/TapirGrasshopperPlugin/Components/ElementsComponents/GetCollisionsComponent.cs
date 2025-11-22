using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
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
            InGenerics(
                "ElementsGroup1",
                "The first group of Elements to check collisions with the second group.");

            InGenerics(
                "ElementsGroup2",
                "The second group of Elements to check collisions with the first group.");

            InNumber(
                "VolumeTolerance",
                "Intersection body volume greater then this value will be considered as a collision.",
                Tolerances.Main);

            InBoolean(
                "PerformSurfaceCheck",
                "Enables surface collision check. If disabled the surfaceTolerance value will be ignored.");


            InNumber(
                "SurfaceTolerance",
                "Intersection body surface area greater then this value will be considered as a collision.",
                Tolerances.Main);
        }

        protected override void AddOutputs()
        {
            OutIntegers(
                "IndexOfElementFromGroup1",
                "The index of Elements with detected collision from group1.");

            OutGenerics(
                "ElementGuidFromGroup1",
                "Elements id from the group1.");

            OutIntegers(
                "IndexOfElementFromGroup2",
                "The index of Elements with detected collision from group2.");

            OutGenerics(
                "ElementGuidFromGroup2",
                "Elements id from the group2.");

            OutBooleans(
                "HasBodyCollision",
                "The element has body collision.");

            OutBooleans(
                "HasClearenceCollision",
                "The element has clearance collision.");
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

        public override Guid ComponentGuid =>
            new Guid("6ff649b0-89a0-466a-aaa6-3b0b5eef70ee");
    }
}