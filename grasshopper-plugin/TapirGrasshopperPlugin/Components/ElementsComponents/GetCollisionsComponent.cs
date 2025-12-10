using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetCollisionsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetCollisions";

        public GetCollisionsComponent()
            : base(
                "Collisions",
                "Detects collisions between elements.",
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

            SetOptionality(
                new[]
                {
                    2,
                    4
                });
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
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject inputGroup1))
            {
                return;
            }

            if (!da.TryCreateFromList(
                    1,
                    out ElementsObject inputGroup2))
            {
                return;
            }

            var volumeTolerance = da.GetOptional(
                2,
                Tolerances.Main);

            if (!da.TryGet(
                    3,
                    out bool performSurfaceCheck))
            {
                return;
            }

            var surfaceTolerance = da.GetOptional(
                4,
                Tolerances.Main);

            var parameters = new GetCollisionsParameters()
            {
                ElementsGroup1 = inputGroup1.Elements,
                ElementsGroup2 = inputGroup2.Elements,
                Settings = new CollisionDetectionSettings
                {
                    VolumeTolerance = volumeTolerance,
                    PerformSurfaceCheck = performSurfaceCheck,
                    SurfaceTolerance = surfaceTolerance
                }
            };

            if (!TryGetConvertedValues(
                    CommandName,
                    parameters,
                    ToAddOn,
                    JHelp.Deserialize<CollisionsOutput>,
                    out CollisionsOutput response))
            {
                return;
            }

            var elementIndex1s = new List<int>();
            var elementId1s = new List<ElementGuidWrapper>();
            var elementIndex2s = new List<int>();
            var elementId2s = new List<ElementGuidWrapper>();
            var hasBodyCollisions = new List<bool>();
            var hasClearenceCollisions = new List<bool>();

            foreach (var c in response.Collisions)
            {
                elementIndex1s.Add(
                    inputGroup1.Elements.FindIndex(e =>
                        e.ElementId.Equals(c.ElementId1)));
                elementId1s.Add(
                    new ElementGuidWrapper { ElementId = c.ElementId1 });
                elementIndex2s.Add(
                    inputGroup2.Elements.FindIndex(e =>
                        e.ElementId.Equals(c.ElementId2)));
                elementId2s.Add(
                    new ElementGuidWrapper { ElementId = c.ElementId2 });
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