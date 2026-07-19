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
    public abstract class JsonItemsExecutorComponent : ArchicadExecutorComponent
    {
        private readonly string _arrayKey;
        private readonly string _itemWrapKey;
        private readonly string _inputName;
        private readonly string _inputDescription;

        protected JsonItemsExecutorComponent(
            string name,
            string description,
            string subCategory,
            string arrayKey,
            string inputName,
            string inputDescription,
            string itemWrapKey = null)
            : base(
                name,
                description,
                subCategory)
        {
            _arrayKey = arrayKey;
            _itemWrapKey = itemWrapKey;
            _inputName = inputName;
            _inputDescription = inputDescription;
        }

        protected override void AddInputs()
        {
            InTexts(
                _inputName,
                _inputDescription);
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
                        $"Invalid JSON in the {_inputName} input: {ex.Message}");
                    return;
                }

                if (_itemWrapKey != null)
                {
                    items.Add(new JObject { [_itemWrapKey] = item });
                }
                else
                {
                    items.Add(item);
                }
            }

            var parameters = new JObject { [_arrayKey] = items };

            TryGetCadResponse(
                CommandName,
                parameters,
                ToAddOn,
                out _);
        }
    }
}
