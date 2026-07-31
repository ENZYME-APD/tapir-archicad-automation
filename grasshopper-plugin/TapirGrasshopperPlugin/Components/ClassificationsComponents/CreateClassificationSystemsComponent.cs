using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class CreateClassificationSystemsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateClassificationSystems";

        public CreateClassificationSystemsComponent()
            : base(
                "CreateClassificationSystems",
                "Create Classification Systems. The classification item hierarchy of each system " +
                "can be given through the ClassificationItems input as JSON.",
                GroupNames.Classifications)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Display name of each new classification system.");

            InTexts(
                "Versions",
                "Version of each new classification system. Input only 1 to use the same value for all systems.");

            InTexts(
                "Descriptions",
                "Description of each new classification system. Input only 1 to use the same value for all systems.");

            InTexts(
                "Sources",
                "Source of each new classification system (e.g. URL of the standard). Input only 1 to use the same value for all systems.");

            InTexts(
                "Dates",
                "Release date of each classification system's current version (YYYY-MM-DD). Input only 1 to use the same value for all systems.");

            InTexts(
                "ClassificationItems",
                "One JSON array per system with the classification item hierarchy, e.g. " +
                "[{\"id\":\"01\",\"name\":\"...\",\"description\":\"...\",\"children\":[...]}]. " +
                "Input only 1 to use the same items for all systems. Optional.");

            SetOptionality(5);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "ErrorMessages",
                "Error message for each system (empty when the classification system was created successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> names))
            {
                return;
            }

            var systemCount = names.Count;
            if (systemCount == 0)
            {
                this.AddError("The Names input must contain at least one item.");
                return;
            }

            if (!da.TryGetList(1, out List<string> versions) ||
                !da.TryGetList(2, out List<string> descriptions) ||
                !da.TryGetList(3, out List<string> sources) ||
                !da.TryGetList(4, out List<string> dates))
            {
                return;
            }

            da.TryGetList(
                5,
                out List<string> classificationItems);
            classificationItems = classificationItems ?? new List<string>();

            foreach (var pair in new (string Name, int Count, bool Optional)[]
                     {
                         ("Versions", versions.Count, false),
                         ("Descriptions", descriptions.Count, false),
                         ("Sources", sources.Count, false),
                         ("Dates", dates.Count, false),
                         ("ClassificationItems", classificationItems.Count, true)
                     })
            {
                var validEmpty = pair.Optional && pair.Count == 0;
                if (!validEmpty && pair.Count != 1 && pair.Count != systemCount)
                {
                    this.AddError(
                        $"The size of the input {pair.Name} must be {(pair.Optional ? "0, " : "")}1 or equal to the size of the input Names.");
                    return;
                }
            }

            var items = new JArray();
            for (var i = 0; i < systemCount; i++)
            {
                var itemsOfSystem = new JArray();
                if (classificationItems.Count > 0)
                {
                    var json = classificationItems[classificationItems.Count == 1 ? 0 : i];
                    try
                    {
                        itemsOfSystem = JArray.Parse(json);
                    }
                    catch (Exception ex)
                    {
                        this.AddError(
                            $"Invalid JSON in the ClassificationItems input: {ex.Message}");
                        return;
                    }
                }

                items.Add(
                    new JObject
                    {
                        ["classificationSystem"] = new JObject
                        {
                            ["name"] = names[i],
                            ["version"] = versions[versions.Count == 1 ? 0 : i],
                            ["description"] = descriptions[descriptions.Count == 1 ? 0 : i],
                            ["source"] = sources[sources.Count == 1 ? 0 : i],
                            ["date"] = dates[dates.Count == 1 ? 0 : i]
                        },
                        ["classificationItems"] = itemsOfSystem
                    });
            }

            var parameters = new JObject { ["classificationSystemsWithItems"] = items };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var errors = new List<string>();
            if (response["executionResults"] is JArray results)
            {
                foreach (var result in results)
                {
                    errors.Add(
                        (bool?)result["success"] == true
                            ? ""
                            : result["error"]?["message"]?.ToString() ?? "Unknown error.");
                }
            }
            da.SetDataList(0, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateClassificationSystems;

        public override Guid ComponentGuid =>
            new Guid("061c6a1b-9f40-41db-befc-639018a6344f");
    }
}
