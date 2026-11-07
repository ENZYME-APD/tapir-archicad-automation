using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class SetUserDataOfAttributesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetUserDataOfAttributes";

        public SetUserDataOfAttributesComponent()
            : base(
                "SetUserDataOfAttributes",
                "Stores string values under keys in the user data of attributes, for scripts to keep their own " +
                "state on an attribute. The given pairs are merged into what each attribute already holds: a key " +
                "that exists gets the new value, other keys are left as they are, and the keys listed in " +
                "KeysToDelete are removed. Prefix the keys with the script's name so that scripts do not overwrite " +
                "each other's data. User data in another layout is left unchanged and reported as an error. " +
                "Archicad does not record this change as an undo step, so Undo does not revert it.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "AttributeGuids",
                "Identifiers of the attributes to store the user data on.");

            InTexts(
                "Keys",
                "Keys of the pairs to store on every attribute, e.g. 'myscript.databaseId'. Optional.");

            InTexts(
                "Values",
                "Value of each key (one per item of Keys). Optional.");

            InTexts(
                "KeysToDelete",
                "Keys to remove from the user data, after the pairs are stored. A key that is not stored is " +
                "ignored. Optional.");

            SetOptionality(new[] { 1, 2, 3 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each attribute (empty when its user data was stored).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out AttributeGuidItemsObject input))
            {
                return;
            }

            da.TryGetList(
                1,
                out List<string> keys);

            da.TryGetList(
                2,
                out List<string> values);

            da.TryGetList(
                3,
                out List<string> keysToDelete);

            if (keys.Count != values.Count)
            {
                this.AddError(
                    "The size of the input Values must be equal to the size of the input Keys.");
                return;
            }

            var parameters = JObject.FromObject(input);

            if (keys.Count > 0)
            {
                var pairs = new JArray();
                for (var i = 0; i < keys.Count; i++)
                {
                    pairs.Add(
                        new JObject
                        {
                            ["key"] = keys[i],
                            ["value"] = values[i]
                        });
                }

                parameters["pairs"] = pairs;
            }

            if (keysToDelete.Count > 0)
            {
                parameters["keysToDelete"] = new JArray(keysToDelete);
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                parameters,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetUserDataOfAttributes;

        public override Guid ComponentGuid =>
            new Guid("77842c26-37c6-4dd1-9731-3a472d7cc968");
    }
}
