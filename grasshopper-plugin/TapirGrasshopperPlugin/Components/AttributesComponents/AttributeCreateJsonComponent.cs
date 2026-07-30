using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    // Shared base for the attribute creator commands. Their item schemas have
    // many optional fields that vary per attribute type (dash items, composite
    // skins, pens, surface parameters, ...), so each attribute to create is
    // passed as a JSON object matching the command's documented item schema.
    public abstract class AttributeCreateJsonComponent : ArchicadExecutorComponent
    {
        private readonly string _dataArrayKey;

        protected AttributeCreateJsonComponent(
            string name,
            string description,
            string dataArrayKey)
            : base(
                name,
                description,
                GroupNames.Attributes)
        {
            _dataArrayKey = dataArrayKey;
        }

        protected override void AddInputs()
        {
            InTexts(
                "AttributesData",
                "One JSON object per attribute to create, matching the command's item schema " +
                "(e.g. {\"name\":\"MyAttribute\", ...}).");

            InBoolean(
                "OverwriteExisting",
                "Overwrite the existing attribute with the same name.",
                false);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> attributesData))
            {
                return;
            }

            da.TryGet(
                1,
                out bool overwriteExisting);

            var items = new JArray();
            foreach (var json in attributesData)
            {
                try
                {
                    items.Add(JObject.Parse(json));
                }
                catch (Exception ex)
                {
                    this.AddError(
                        $"Invalid JSON in the AttributesData input: {ex.Message}");
                    return;
                }
            }

            var parameters = new JObject
            {
                [_dataArrayKey] = items,
                ["overwriteExisting"] = overwriteExisting
            };

            TryGetCadResponse(
                CommandName,
                parameters,
                ToAddOn,
                out _);
        }
    }
}
