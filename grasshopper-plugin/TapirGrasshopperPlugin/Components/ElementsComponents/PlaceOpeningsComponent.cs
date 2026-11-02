using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class PlaceOpeningsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "PlaceOpenings";

        public PlaceOpeningsComponent()
            : base(
                "PlaceOpenings",
                "Places openings with the Opening tool settings at points of their parent elements. A point off " +
                "the parent element's surface is moved to the closest point of the surface. Each opening can be " +
                "given its own extrusion and floor plan settings, which change only that opening and not the " +
                "tool's settings. Available from Archicad 29.",
                GroupNames.ElementCreation)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ParentElementGuids",
                "The element to place each opening in (input only 1 to use the same element for all).");

            InPoints(
                "Points",
                "Where to place each opening; one opening is placed per point.");

            InTexts(
                "ExtrusionParameters",
                OpeningSettingsHelp.ExtrusionParametersDescription +
                " Input only 1 to use the same settings for all. Optional.");

            InTexts(
                "FloorPlanParameters",
                OpeningSettingsHelp.FloorPlanParametersDescription +
                " Input only 1 to use the same settings for all. Optional.");

            SetOptionality(new[] { 2, 3 });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifiers of the placed openings (null where one could not be placed).");

            OutErrorMessages(
                "Error message of each opening (empty when it was placed).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> parentWrappers) ||
                !this.TryGetElementIds(
                    parentWrappers,
                    "ParentElementGuids",
                    out var parentIds))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<Point3d> points))
            {
                return;
            }

            var extrusionTexts = da.OptionalList<string>(2);
            var floorPlanTexts = da.OptionalList<string>(3);

            if (!this.CheckSizes(
                    points.Count,
                    "Points",
                    ("ParentElementGuids", parentIds.Count),
                    ("ExtrusionParameters", extrusionTexts.Count),
                    ("FloorPlanParameters", floorPlanTexts.Count)))
            {
                return;
            }

            var openingsData = new JArray();
            for (var i = 0; i < points.Count; i++)
            {
                var opening = new JObject
                {
                    ["parentElementId"] = parentIds.At(i),
                    ["point"] = new JObject
                    {
                        ["x"] = points[i].X,
                        ["y"] = points[i].Y,
                        ["z"] = points[i].Z
                    }
                };

                if (extrusionTexts.Count > 0)
                {
                    if (!OpeningSettingsHelp.TryParse(
                            this,
                            extrusionTexts.At(i),
                            "ExtrusionParameters",
                            OpeningSettingsHelp.ExtrusionKeys,
                            out var extrusionParameters))
                    {
                        return;
                    }

                    if (extrusionParameters != null)
                    {
                        opening["extrusionParameters"] = extrusionParameters;
                    }
                }

                if (floorPlanTexts.Count > 0)
                {
                    if (!OpeningSettingsHelp.TryParse(
                            this,
                            floorPlanTexts.At(i),
                            "FloorPlanParameters",
                            OpeningSettingsHelp.FloorPlanKeys,
                            out var floorPlanParameters))
                    {
                        return;
                    }

                    if (floorPlanParameters != null)
                    {
                        opening["floorPlanParameters"] = floorPlanParameters;
                    }
                }

                openingsData.Add(opening);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["openingsData"] = openingsData },
                    ToAddOn,
                    out var response))
            {
                return;
            }

            CreateElementsComponentBase.SetCreatedElementsOutputs(
                da,
                response,
                0,
                1);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.PlaceOpenings;

        public override Guid ComponentGuid =>
            new Guid("f07623cf-e556-4f97-8c8c-a4aab395b0c0");
    }
}
