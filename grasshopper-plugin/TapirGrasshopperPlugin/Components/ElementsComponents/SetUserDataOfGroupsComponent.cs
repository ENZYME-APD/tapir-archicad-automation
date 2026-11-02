using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SetUserDataOfGroupsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetUserDataOfGroups";

        public SetUserDataOfGroupsComponent()
            : base(
                "SetUserDataOfGroups",
                "Store string values under keys in the user data of groups, for scripts to keep their own state " +
                "on a group. The given pairs are merged into what each group already holds: a key that exists gets " +
                "the new value, other keys are left as they are, and the keys listed in KeysToDelete are removed. " +
                "Prefix the keys with the script's name so that scripts do not overwrite each other's data. User " +
                "data in another layout is left unchanged and reported as an error. One undo step per run.",
                GroupNames.ElementGrouping)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "GroupGuids",
                "Identifiers of the groups to store the pairs on.");

            InTexts(
                "Keys",
                "Keys of the pairs to store on every group (for example myscript.databaseId). Optional.");

            InTexts(
                "Values",
                "Values of the pairs, one per key: any text. Optional, but as many as Keys.");

            InTexts(
                "KeysToDelete",
                "Keys to remove from the user data of every group; a key that is not stored is ignored. " +
                "Applied after the pairs. Optional.");

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
                "Error message of each group (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> groupWrappers))
            {
                return;
            }

            var groups = new JArray();
            foreach (var wrapper in groupWrappers)
            {
                var id = GuidObject<GroupGuid>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError("Invalid group identifier in the GroupGuids input.");
                    return;
                }
                groups.Add(
                    new JObject
                    {
                        ["groupId"] = new JObject { ["guid"] = id.Guid }
                    });
            }

            da.TryGetList(1, out List<string> keys);
            keys = keys ?? new List<string>();
            da.TryGetList(2, out List<string> values);
            values = values ?? new List<string>();
            da.TryGetList(3, out List<string> keysToDelete);
            keysToDelete = keysToDelete ?? new List<string>();

            if (keys.Count != values.Count)
            {
                this.AddError("The inputs Keys and Values must have the same size.");
                return;
            }

            var parameters = new JObject { ["groups"] = groups };

            if (keys.Count > 0)
            {
                var pairs = new JArray();
                for (var i = 0; i < keys.Count; i++)
                {
                    pairs.Add(
                        new JObject
                        {
                            ["key"] = keys[i],
                            ["value"] = values[i] ?? ""
                        });
                }
                parameters["pairs"] = pairs;
            }

            if (keysToDelete.Count > 0)
            {
                parameters["keysToDelete"] = JArray.FromObject(keysToDelete);
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                parameters,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetUserDataOfGroups;

        public override Guid ComponentGuid =>
            new Guid("7caebaa4-ce56-49a6-a0ee-6221470abc5f");
    }
}
