using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.CustomData;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.CustomDataComponents
{
    public class SetElementUserDataComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetElementUserData";

        public SetElementUserDataComponent()
            : base(
                "SetElementUserData",
                "Store a text value under a key on elements, as user data of the Tapir add-on: " +
                "saved with the project, gone with the element, shown nowhere in the Archicad UI. " +
                "The key is merged into the pairs the element already holds, so other keys stay; " +
                "KeysToDelete removes keys from the elements. " +
                "Give a Key with Values, KeysToDelete, or both.",
                GroupNames.CustomData)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Elements to store the value on.");

            InText(
                "Key",
                "Key to store the value under on every element. Prefix it (for example 'myscript.databaseId') " +
                "so that different scripts do not overwrite each other's data. " +
                "Leave empty to only delete keys.");

            InTexts(
                "Values",
                "Value to store under the key on each element (input only 1 to use the same value for all elements).");

            InTexts(
                "KeysToDelete",
                "Keys to remove from every element. A key an element does not hold is ignored.");

            SetOptionality(
                new[]
                {
                    1,
                    2,
                    3
                });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when its data was stored).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            var key = da.GetOptional<string>(
                1,
                null);
            var values = da.GetOptionals<string>(2);
            var keysToDelete = da.GetOptionals<string>(3);

            var hasPair = !string.IsNullOrEmpty(key);
            var hasKeysToDelete = keysToDelete.Count > 0;

            if (!hasPair && !hasKeysToDelete)
            {
                this.AddError(
                    "Give a Key with Values to store, KeysToDelete to remove, or both.");
                return;
            }

            if (hasPair &&
                values.Count != 1 &&
                values.Count != elements.Elements.Count)
            {
                this.AddError(
                    "The count of Values must be 1 or the same as the count of ElementGuids.");
                return;
            }

            if (!hasPair && values.Count > 0)
            {
                this.AddError("Values need a Key to be stored under.");
                return;
            }

            if (keysToDelete.Exists(string.IsNullOrEmpty))
            {
                this.AddError("Every key to delete must be a non-empty text.");
                return;
            }

            var input = new SetElementUserDataParameters
            {
                ElementUserData = new List<ElementUserDataToSet>()
            };

            for (var i = 0; i < elements.Elements.Count; i++)
            {
                var item = new ElementUserDataToSet
                {
                    ElementId = elements.Elements[i].ElementId
                };

                if (hasPair)
                {
                    item.UserData = new List<KeyValuePairObj>
                    {
                        new KeyValuePairObj
                        {
                            Key = key,
                            Value = values.Count == 1
                                ? values[0]
                                : values[i]
                        }
                    };
                }

                if (hasKeysToDelete)
                {
                    item.KeysToDelete = keysToDelete;
                }

                input.ElementUserData.Add(item);
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        // The element user data has no icon of its own yet; it shares the one
        // of the property value setter.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetPropertyValues;

        public override Guid ComponentGuid =>
            new Guid("5b7e3f90-c2a4-4d18-b6e5-8d1f0a4c7e29");
    }
}
