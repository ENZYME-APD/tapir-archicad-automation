using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components
{
    // Shared base for executor commands whose input is an array of deeply
    // nested objects. Each item is passed as a JSON object matching the
    // command's documented item schema.
    //
    // The configuration is provided through overridable members instead of
    // constructor parameters on purpose: GH_Component's constructor calls
    // RegisterInputParams (and thus AddInputs) before the derived
    // constructor bodies run, so constructor-assigned fields would still be
    // null at that point. Overrides must not depend on instance state.
    public abstract class JsonItemsExecutorComponent : ArchicadExecutorComponent
    {
        // The name of the command's array parameter (e.g. "subsetsData").
        protected abstract string ArrayKey { get; }

        // When set, each parsed item is wrapped as { ItemWrapKey: item }.
        protected virtual string ItemWrapKey => null;

        // Name and description of the JSON text input.
        protected abstract string InputName { get; }
        protected abstract string InputDescription { get; }

        protected JsonItemsExecutorComponent(
            string name,
            string description,
            string subCategory)
            : base(
                name,
                description,
                subCategory)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                InputName,
                InputDescription);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> jsonItems))
            {
                return;
            }

            var items = new JArray();
            foreach (var json in jsonItems)
            {
                JObject item;
                try
                {
                    item = JObject.Parse(json);
                }
                catch (Exception ex)
                {
                    this.AddError(
                        $"Invalid JSON in the {InputName} input: {ex.Message}");
                    return;
                }

                if (ItemWrapKey != null)
                {
                    items.Add(new JObject { [ItemWrapKey] = item });
                }
                else
                {
                    items.Add(item);
                }
            }

            var parameters = new JObject { [ArrayKey] = items };

            TryGetCadResponse(
                CommandName,
                parameters,
                ToAddOn,
                out _);
        }
    }
}
