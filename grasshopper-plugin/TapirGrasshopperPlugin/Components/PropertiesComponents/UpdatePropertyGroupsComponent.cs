using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class UpdatePropertyGroupsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "UpdatePropertyGroups";

        public UpdatePropertyGroupsComponent()
            : base(
                "UpdatePropertyGroups",
                "Rename or change the description of existing Custom Property Groups in place. " +
                "Only the provided fields are changed; the groups keep their identifiers.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "PropertyGroupGuids",
                "Identifiers of the custom property groups to update.");

            InTexts(
                "Names",
                "New name of each property group (input 0, 1 or the same number as PropertyGroupGuids). Optional.");

            InTexts(
                "Descriptions",
                "New description of each property group (input 0, 1 or the same number as PropertyGroupGuids). Optional.");

            SetOptionality(new[] { 1, 2 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each property group (empty when it succeeded).");
        }

        private static T GetAt<T>(
            List<T> values,
            int index)
            where T : class
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
                    out List<GH_ObjectWrapper> wrappers))
            {
                return;
            }

            da.TryGetList(1, out List<string> names);
            names = names ?? new List<string>();
            da.TryGetList(2, out List<string> descriptions);
            descriptions = descriptions ?? new List<string>();

            foreach (var pair in new (string Name, int Count)[]
                     {
                         ("Names", names.Count),
                         ("Descriptions", descriptions.Count)
                     })
            {
                if (pair.Count > 1 &&
                    pair.Count != wrappers.Count)
                {
                    this.AddError(
                        $"The size of the input {pair.Name} must be 0, 1 or equal to the size of the input PropertyGroupGuids.");
                    return;
                }
            }

            var input = new UpdatePropertyGroupsParameters
            {
                PropertyGroups = new List<PropertyGroupUpdate>()
            };

            for (var i = 0; i < wrappers.Count; i++)
            {
                var id = GuidObject<PropertyGroupGuidObject>.CreateFromWrapper(wrappers[i]);
                if (id == null)
                {
                    this.AddError(
                        "Invalid property group identifier in the PropertyGroupGuids input.");
                    return;
                }

                input.PropertyGroups.Add(
                    new PropertyGroupUpdate
                    {
                        PropertyGroupId = id,
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
            Properties.Resources.UpdatePropertyGroups;

        public override Guid ComponentGuid =>
            new Guid("b8c97528-6467-477f-8feb-699ea7d8a19f");
    }
}
