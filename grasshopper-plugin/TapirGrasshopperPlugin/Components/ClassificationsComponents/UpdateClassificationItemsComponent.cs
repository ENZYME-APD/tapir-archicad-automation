using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class UpdateClassificationItemsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "UpdateClassificationItems";

        public UpdateClassificationItemsComponent()
            : base(
                "UpdateClassificationItems",
                "Edit the id (code), name or description of existing Classification Items in place. " +
                "Only the provided fields are changed; the items keep their identifiers and their parents.",
                GroupNames.Classifications)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ClassificationItemGuids",
                "Identifiers of the classification items to update.");

            InTexts(
                "Ids",
                "New id (code, e.g. 21.10) of each classification item (input only 1 to use the same value for all). Optional.");

            InTexts(
                "Names",
                "New display name of each classification item (input only 1 to use the same value for all). Optional.");

            InTexts(
                "Descriptions",
                "New description of each classification item (input only 1 to use the same value for all). Optional.");

            SetOptionality(new[] { 1, 2, 3 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each classification item (empty when it succeeded).");
        }

        private static string GetAt(
            List<string> values,
            int index)
        {
            if (values.Count == 0)
            {
                return null;
            }

            return values[values.Count == 1 ? 0 : index];
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> itemWrappers))
            {
                return;
            }

            da.TryGetList(1, out List<string> ids);
            ids = ids ?? new List<string>();
            da.TryGetList(2, out List<string> names);
            names = names ?? new List<string>();
            da.TryGetList(3, out List<string> descriptions);
            descriptions = descriptions ?? new List<string>();

            foreach (var pair in new (string Name, int Count)[]
                     {
                         ("Ids", ids.Count),
                         ("Names", names.Count),
                         ("Descriptions", descriptions.Count)
                     })
            {
                if (pair.Count > 1 &&
                    pair.Count != itemWrappers.Count)
                {
                    this.AddError(
                        $"The size of the input {pair.Name} must be 0, 1 or equal to the size of the input ClassificationItemGuids.");
                    return;
                }
            }

            var input = new UpdateClassificationItemsParameters
            {
                ClassificationItems = new List<ClassificationItemUpdate>()
            };

            for (var i = 0; i < itemWrappers.Count; i++)
            {
                var id = GuidObject<ClassificationGuid>.CreateFromWrapper(itemWrappers[i]);
                if (id == null)
                {
                    this.AddError(
                        "Invalid identifier in the ClassificationItemGuids input.");
                    return;
                }

                input.ClassificationItems.Add(
                    new ClassificationItemUpdate
                    {
                        ClassificationItemId = id,
                        Id = GetAt(ids, i),
                        Name = GetAt(names, i),
                        Description = GetAt(descriptions, i)
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.UpdateClassificationItems;

        public override Guid ComponentGuid =>
            new Guid("7a46780e-6757-4464-92de-56b0b42b8312");
    }
}
