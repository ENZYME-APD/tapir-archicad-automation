using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class CreatePropertyDefinitionsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreatePropertyDefinitions";

        public CreatePropertyDefinitionsComponent()
            : base(
                "CreatePropertyDefinitions",
                "Create Custom Property Definitions. The default value and the possible enum values " +
                "can be given through the DefaultValues input as JSON.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Name of each new property definition.");

            InTexts(
                "Types",
                "Data type of the property (e.g. string, number, integer, boolean, length, area, volume, angle, singleEnum, multiEnum). Input only 1 to use the same value for all definitions.");

            InTexts(
                "Descriptions",
                "Description of the property definition. Input only 1 to use the same value for all definitions.");

            InTexts(
                "GroupNames",
                "Name of the property group to create the definition in. Input only 1 to use the same group for all definitions.");

            InBooleans(
                "IsEditable",
                "The property value is editable. Input only 1 to use the same value for all definitions.");

            inManager.AddGenericParameter(
                "AvailabilityGuids",
                "AvailabilityGuids",
                "Identifiers of the classification items the property is available for (one branch per definition). Optional; by default the property is not available for any classification.",
                GH_ParamAccess.tree);

            InTexts(
                "DefaultValues",
                "One JSON object per definition with the default value, e.g. " +
                "{\"basicDefaultValue\":{\"type\":\"string\",\"status\":\"normal\",\"value\":\"...\"}}. " +
                "Input only 1 to use the same value for all definitions. Optional.");

            SetOptionality(new[] { 2, 4, 5, 6 });
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "PropertyGuids",
                "Identifiers of the created property definitions (null for failed items).");

            OutTexts(
                "ErrorMessages",
                "Error message for each definition (empty when the property was created successfully).");
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

            var definitionCount = names.Count;
            if (definitionCount == 0)
            {
                this.AddError("The Names input must contain at least one item.");
                return;
            }

            if (!da.TryGetList(1, out List<string> types) ||
                !da.TryGetList(3, out List<string> groupNames))
            {
                return;
            }

            da.TryGetList(2, out List<string> descriptions);
            descriptions = descriptions ?? new List<string>();

            var isEditable = new List<bool>();
            da.GetDataList(4, isEditable);

            da.TryGetTree(
                5,
                out GH_Structure<IGH_Goo> availability);
            var availabilityBranchCount = availability?.Branches.Count ?? 0;

            da.TryGetList(6, out List<string> defaultValues);
            defaultValues = defaultValues ?? new List<string>();

            foreach (var pair in new (string Name, int Count, bool Optional)[]
                     {
                         ("Types", types.Count, false),
                         ("Descriptions", descriptions.Count, true),
                         ("GroupNames", groupNames.Count, false),
                         ("IsEditable", isEditable.Count, true),
                         ("AvailabilityGuids", availabilityBranchCount, true),
                         ("DefaultValues", defaultValues.Count, true)
                     })
            {
                var validEmpty = pair.Optional && pair.Count == 0;
                if (!validEmpty && pair.Count != 1 && pair.Count != definitionCount)
                {
                    this.AddError(
                        $"The size of the input {pair.Name} must be {(pair.Optional ? "0, " : "")}1 or equal to the size of the input Names.");
                    return;
                }
            }

            var items = new JArray();
            for (var i = 0; i < definitionCount; i++)
            {
                var availabilityArray = new JArray();
                if (availabilityBranchCount > 0)
                {
                    var branch = availability.Branches[availabilityBranchCount == 1 ? 0 : i];
                    foreach (var goo in branch)
                    {
                        var wrapper = goo as GH_ObjectWrapper ?? new GH_ObjectWrapper(goo);
                        var id = GuidObject<ClassificationGuid>.CreateFromWrapper(wrapper);
                        if (id == null)
                        {
                            this.AddError("Invalid identifier in the AvailabilityGuids input.");
                            return;
                        }
                        availabilityArray.Add(
                            new JObject
                            {
                                ["classificationItemId"] = new JObject { ["guid"] = id.Guid }
                            });
                    }
                }

                var definition = new JObject
                {
                    ["name"] = names[i],
                    ["description"] = descriptions.Count == 0
                        ? ""
                        : descriptions[descriptions.Count == 1 ? 0 : i],
                    ["type"] = types[types.Count == 1 ? 0 : i],
                    ["isEditable"] = isEditable.Count == 0 || isEditable[isEditable.Count == 1 ? 0 : i],
                    ["availability"] = availabilityArray,
                    ["group"] = new JObject
                    {
                        ["name"] = groupNames[groupNames.Count == 1 ? 0 : i]
                    }
                };

                if (defaultValues.Count > 0)
                {
                    var json = defaultValues[defaultValues.Count == 1 ? 0 : i];
                    try
                    {
                        definition["defaultValue"] = JObject.Parse(json);
                    }
                    catch (Exception ex)
                    {
                        this.AddError(
                            $"Invalid JSON in the DefaultValues input: {ex.Message}");
                        return;
                    }
                }

                items.Add(new JObject { ["propertyDefinition"] = definition });
            }

            var parameters = new JObject { ["propertyDefinitions"] = items };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var propertyGuids = new List<string>();
            var errors = new List<string>();
            if (response["propertyIds"] is JArray results)
            {
                foreach (var result in results)
                {
                    if (result?["error"] != null)
                    {
                        errors.Add(result["error"]?["message"]?.ToString() ?? "");
                        propertyGuids.Add(null);
                        continue;
                    }
                    errors.Add("");
                    propertyGuids.Add(result?["propertyId"]?["guid"]?.ToString());
                }
            }
            da.SetDataList(0, propertyGuids);
            da.SetDataList(1, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreatePropertyDefinitions;

        public override Guid ComponentGuid =>
            new Guid("9d6d8b64-9be1-4606-bb75-f64d8d2b49f3");
    }
}
