using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    // Shared base for the library-part based element creator commands
    // (Objects and Lamps): a library part name plus a 3D insertion point,
    // with the remaining optional fields provided as JSON.
    public abstract class LibraryPartCreatorComponent : ArchicadExecutorComponent
    {
        private readonly string _dataArrayKey;

        protected LibraryPartCreatorComponent(
            string name,
            string description,
            string dataArrayKey)
            : base(
                name,
                description,
                GroupNames.Elements)
        {
            _dataArrayKey = dataArrayKey;
        }

        protected override void AddInputs()
        {
            InTexts(
                "LibraryPartNames",
                "Names of the library parts to place (input only 1 to use the same library part for all points).");

            InPoints(
                "Points",
                "Insertion points of the elements.");

            InTexts(
                "AdditionalSettings",
                "One JSON object per element with further optional settings " +
                "(e.g. {\"dimensions\":{\"x\":1.0,\"y\":1.0,\"z\":1.0},\"floorIndex\":0}). " +
                "Input only 1 to use the same settings for all. Optional.");

            SetOptionality(2);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> libraryPartNames))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<Point3d> points))
            {
                return;
            }

            if (libraryPartNames.Count != 1 &&
                libraryPartNames.Count != points.Count)
            {
                this.AddError(
                    "The size of the input LibraryPartNames must be 1 or equal to the size of the input Points.");
                return;
            }

            da.TryGetList(
                2,
                out List<string> additionalSettings);
            additionalSettings = additionalSettings ?? new List<string>();
            if (additionalSettings.Count > 1 &&
                additionalSettings.Count != points.Count)
            {
                this.AddError(
                    "The size of the input AdditionalSettings must be 0, 1 or equal to the size of the input Points.");
                return;
            }

            var items = new JArray();
            for (var i = 0; i < points.Count; i++)
            {
                var item = new JObject
                {
                    ["libraryPartName"] =
                        libraryPartNames[libraryPartNames.Count == 1 ? 0 : i],
                    ["coordinates"] = new JObject
                    {
                        ["x"] = points[i].X,
                        ["y"] = points[i].Y,
                        ["z"] = points[i].Z
                    }
                };

                if (additionalSettings.Count > 0)
                {
                    var json = additionalSettings[additionalSettings.Count == 1 ? 0 : i];
                    try
                    {
                        item.Merge(
                            JObject.Parse(json),
                            new JsonMergeSettings
                            {
                                MergeArrayHandling = MergeArrayHandling.Replace
                            });
                    }
                    catch (Exception ex)
                    {
                        this.AddError(
                            $"Invalid JSON in the AdditionalSettings input: {ex.Message}");
                        return;
                    }
                }

                items.Add(item);
            }

            var parameters = new JObject { [_dataArrayKey] = items };

            TryGetCadResponse(
                CommandName,
                parameters,
                ToAddOn,
                out _);
        }
    }
}
