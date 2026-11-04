using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class CreateSectionsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateSections";

        public CreateSectionsComponent()
            : base(
                "CreateSections",
                "Create Section elements on the floor plan.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InPoints(
                "StartPoints",
                "Start point of each section line (only X and Y are used).");

            InPoints(
                "EndPoints",
                "End point of each section line (only X and Y are used).");

            InNumbers(
                "Depths",
                "Depth of each section (input only 1 to use the same depth for all). Optional.");

            InTexts(
                "Names",
                "Name of each section (input only 1 to use the same name for all). Optional.");

            InIntegers(
                "FloorIndices",
                "Floor index of each section (input only 1 to use the same floor for all). Optional.");

            InTexts(
                "IDs",
                "Reference ID of each section, shown in its marker and in the Project Map; a '/' splits it between the start and the end marker (input only 1 to use the same ID for all). Optional.");

            InTexts(
                "FavoriteNames",
                "Name of the Section favorite to base each section on (input only 1 to use the same favorite for all). Optional.");

            SetOptionality(new[] { 2, 3, 4, 5, 6 });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifier of each created section.");

            OutErrorMessages(
                "Error message of each section (empty when it was created successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<Point3d> startPoints))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<Point3d> endPoints))
            {
                return;
            }

            if (startPoints.Count != endPoints.Count)
            {
                this.AddError(
                    "The size of the inputs StartPoints and EndPoints must be equal.");
                return;
            }

            da.TryGetList(2, out List<double> depths);
            depths = depths ?? new List<double>();
            da.TryGetList(3, out List<string> names);
            names = names ?? new List<string>();
            da.TryGetList(4, out List<int> floorIndices);
            floorIndices = floorIndices ?? new List<int>();
            da.TryGetList(5, out List<string> ids);
            ids = ids ?? new List<string>();
            da.TryGetList(6, out List<string> favoriteNames);
            favoriteNames = favoriteNames ?? new List<string>();

            foreach (var pair in new (string Name, int Count)[]
                     {
                         ("Depths", depths.Count),
                         ("Names", names.Count),
                         ("FloorIndices", floorIndices.Count),
                         ("IDs", ids.Count),
                         ("FavoriteNames", favoriteNames.Count)
                     })
            {
                if (pair.Count > 1 &&
                    pair.Count != startPoints.Count)
                {
                    this.AddError(
                        $"The size of the input {pair.Name} must be 0, 1 or equal to the size of the input StartPoints.");
                    return;
                }
            }

            var items = new JArray();
            for (var i = 0; i < startPoints.Count; i++)
            {
                var item = new JObject
                {
                    ["startCoordinate"] = new JObject
                    {
                        ["x"] = startPoints[i].X,
                        ["y"] = startPoints[i].Y
                    },
                    ["endCoordinate"] = new JObject
                    {
                        ["x"] = endPoints[i].X,
                        ["y"] = endPoints[i].Y
                    }
                };

                if (depths.Count > 0)
                {
                    item["depth"] = depths[depths.Count == 1 ? 0 : i];
                }

                if (names.Count > 0)
                {
                    item["name"] = names[names.Count == 1 ? 0 : i];
                }

                if (floorIndices.Count > 0)
                {
                    item["floorIndex"] = floorIndices[floorIndices.Count == 1 ? 0 : i];
                }

                if (ids.Count > 0)
                {
                    item["id"] = ids[ids.Count == 1 ? 0 : i];
                }

                if (favoriteNames.Count > 0)
                {
                    item["favoriteName"] = favoriteNames[favoriteNames.Count == 1 ? 0 : i];
                }

                items.Add(item);
            }

            var parameters = new JObject { ["sectionsData"] = items };

            SetCadValuesWithCreatedIds<ElementGuid>(
                CommandName,
                parameters,
                ToAddOn,
                da,
                "elements",
                "elementId");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateSections;

        public override Guid ComponentGuid =>
            new Guid("e0dc09fb-dfe6-4379-9119-cf1688d02ddc");
    }
}
