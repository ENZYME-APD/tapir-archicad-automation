using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SetElementUserDataComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetElementUserData";

        public SetElementUserDataComponent()
            : base(
                "SetElementUserData",
                "Stores string values under keys on elements, for scripts to keep their own state per element " +
                "(external database ids, sync timestamps, markers). The data is saved with the project and appears " +
                "nowhere in the Archicad UI. The given keys are merged into what the element already holds: a key that " +
                "exists gets the new value, other keys are left as they are, and the keys in KeysToDelete are removed. " +
                "Prefix the keys with the definition's name so that scripts do not overwrite each other's data. " +
                "One undo step per call; in a Teamwork project the element has to be reserved.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to store the data on.");

            inManager.AddTextParameter(
                "Keys",
                "Keys",
                "The keys to store on each element, one branch per element. Input only 1 branch to use the same keys for all elements."
                    .WithTypeName("textTree"),
                GH_ParamAccess.tree);

            inManager.AddTextParameter(
                "Values",
                "Values",
                "The value of each key, in the same structure as Keys."
                    .WithTypeName("textTree"),
                GH_ParamAccess.tree);

            inManager.AddTextParameter(
                "KeysToDelete",
                "KeysToDelete",
                "The keys to remove from each element, one branch per element. Input only 1 branch to use the same keys for all elements. Optional."
                    .WithTypeName("textTree"),
                GH_ParamAccess.tree);

            SetOptionality(new[] { 1, 2, 3 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when the data was stored).");
        }

        private static List<string> BranchFor(
            GH_Structure<GH_String> tree,
            int index)
        {
            var result = new List<string>();
            if (tree == null || tree.Branches.Count == 0)
            {
                return result;
            }

            var branch = tree.Branches[tree.Branches.Count == 1 ? 0 : index];
            foreach (var item in branch)
            {
                result.Add(item?.Value);
            }

            return result;
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> elementWrappers))
            {
                return;
            }

            da.TryGetTree(
                1,
                out GH_Structure<GH_String> keysTree);

            da.TryGetTree(
                2,
                out GH_Structure<GH_String> valuesTree);

            da.TryGetTree(
                3,
                out GH_Structure<GH_String> keysToDeleteTree);

            foreach (var tree in new[] { keysTree, valuesTree, keysToDeleteTree })
            {
                if (tree != null &&
                    tree.Branches.Count > 1 &&
                    tree.Branches.Count != elementWrappers.Count)
                {
                    this.AddError(
                        "Keys, Values and KeysToDelete must hold 0, 1 or as many branches as ElementGuids.");
                    return;
                }
            }

            var elementUserData = new JArray();
            for (var i = 0; i < elementWrappers.Count; i++)
            {
                var elementId = GuidObject<ElementGuid>.CreateFromWrapper(
                    elementWrappers[i]);
                if (elementId == null)
                {
                    this.AddError(
                        "Invalid element identifier in the ElementGuids input.");
                    return;
                }

                var item = new JObject
                {
                    ["elementId"] = new JObject { ["guid"] = elementId.Guid }
                };

                var keys = BranchFor(
                    keysTree,
                    i);
                var values = BranchFor(
                    valuesTree,
                    i);
                if (keys.Count != values.Count)
                {
                    this.AddError(
                        $"Element {i}: Keys and Values must hold the same number of items.");
                    return;
                }

                if (keys.Count > 0)
                {
                    var userData = new JArray();
                    for (var k = 0; k < keys.Count; k++)
                    {
                        userData.Add(
                            new JObject
                            {
                                ["key"] = keys[k],
                                ["value"] = values[k] ?? ""
                            });
                    }

                    item["userData"] = userData;
                }

                var keysToDelete = BranchFor(
                    keysToDeleteTree,
                    i);
                if (keysToDelete.Count > 0)
                {
                    item["keysToDelete"] = new JArray(keysToDelete);
                }

                elementUserData.Add(item);
            }

            var parameters = new JObject
            {
                ["elementUserData"] = elementUserData
            };

            SetCadValuesWithErrorMessages(
                CommandName,
                parameters,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetElementUserData;

        public override Guid ComponentGuid =>
            new Guid("02bae586-f564-4c37-9120-b97edf453c33");
    }
}
