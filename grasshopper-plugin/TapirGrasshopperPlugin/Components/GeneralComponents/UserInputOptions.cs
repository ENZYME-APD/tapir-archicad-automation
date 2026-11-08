using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    // The optional snap filter and constraint inputs shared by the
    // Get...FromUser components (the snapFilter and constraint parameters of
    // their commands). They are appended after a component's own inputs, so
    // definitions saved before they existed keep their wires.
    public static class UserInputOptions
    {
        public const int InputCount = 6;

        public static void AddInputs(
            Component component)
        {
            var first = component.Params.Input.Count;

            component.InText(
                "SnapBase",
                "What the cursor snaps to before SnapInclude and SnapExclude are applied: All (everything), " +
                "None (nothing), Edges (only the lines and edges of the elements) or Nodes (only their points: " +
                "end points, vertices, hotspots). Optional; default: None when SnapInclude is given, otherwise All.");

            component.InTexts(
                "SnapInclude",
                "The cursor also snaps to these: an element type (e.g. Wall, Zone, Line) for its points, " +
                "or the type with On (e.g. WallOn) for its lines and edges; also Hotspot, Text, Label, " +
                "Dimension and DimensionOn. Optional.");

            component.InTexts(
                "SnapExclude",
                "The cursor does not snap to these (the same names as SnapInclude). Applied after SnapInclude. Optional.");

            component.InText(
                "ConstraintType",
                "Constrains the clicks like a value locked in the Tracker: X, Y or Z fixes that coordinate, " +
                "Angle the direction from the reference point (the cursor moves along a line), Distance the " +
                "distance from it (the cursor moves along a circle). Optional; default: no constraint.");

            component.InNumber(
                "ConstraintValue",
                "The fixed value of ConstraintType: a coordinate or distance in meters, or for Angle an angle " +
                "in radians, counterclockwise from the X axis. Used only with ConstraintType.",
                0.0);

            component.InBoolean(
                "ConstraintFromLastPoint",
                "If true, ConstraintValue is measured from the last clicked point, otherwise from the project " +
                "origin. Used only with ConstraintType.",
                false);

            component.SetOptionality(
                Enumerable.Range(
                        first,
                        InputCount)
                    .ToArray());
        }

        // Adds snapFilter and constraint to the parameters when their inputs
        // are given. The add-on checks the names and reports a wrong one.
        public static void AddParameters(
            IGH_DataAccess da,
            int first,
            JObject parameters)
        {
            var snapFilter = new JObject();

            if (da.TryGet(
                    first,
                    out string snapBase) &&
                !string.IsNullOrEmpty(snapBase))
            {
                snapFilter["base"] = snapBase;
            }

            if (da.TryGetList(
                    first + 1,
                    out List<string> include))
            {
                include = include.Where(name => !string.IsNullOrEmpty(name)).ToList();
                if (include.Count > 0)
                {
                    snapFilter["include"] = new JArray(include);
                }
            }

            if (da.TryGetList(
                    first + 2,
                    out List<string> exclude))
            {
                exclude = exclude.Where(name => !string.IsNullOrEmpty(name)).ToList();
                if (exclude.Count > 0)
                {
                    snapFilter["exclude"] = new JArray(exclude);
                }
            }

            if (snapFilter.Count > 0)
            {
                parameters["snapFilter"] = snapFilter;
            }

            if (da.TryGet(
                    first + 3,
                    out string constraintType) &&
                !string.IsNullOrEmpty(constraintType))
            {
                da.TryGet(
                    first + 4,
                    out double constraintValue);
                da.TryGet(
                    first + 5,
                    out bool fromLastPoint);

                parameters["constraint"] = new JObject
                {
                    ["type"] = constraintType,
                    ["value"] = constraintValue,
                    ["fromLastPoint"] = fromLastPoint
                };
            }
        }
    }
}
