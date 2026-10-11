using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class UpdateClassificationSystemsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "UpdateClassificationSystems";

        public UpdateClassificationSystemsComponent()
            : base(
                "UpdateClassificationSystems",
                "Edit the name, description, source, version or date of existing Classification Systems in place. " +
                "Only the provided fields are changed; the systems keep their identifiers.",
                GroupNames.Classifications)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ClassificationSystemGuids",
                "Identifiers of the classification systems to update.");

            InTexts(
                "Names",
                "New display name of each classification system (input only 1 to use the same value for all). Optional.");

            InTexts(
                "Descriptions",
                "New description of each classification system (input only 1 to use the same value for all). Optional.");

            InTexts(
                "Sources",
                "New source of each classification system (input only 1 to use the same value for all). Optional.");

            InTexts(
                "Versions",
                "New version of each classification system (input only 1 to use the same value for all). Optional.");

            InTexts(
                "Dates",
                "New release date of each classification system (YYYY-MM-DD, input only 1 to use the same value for all). Optional.");

            SetOptionality(new[] { 1, 2, 3, 4, 5 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each classification system (empty when it succeeded).");
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
                    out List<GH_ObjectWrapper> systemWrappers))
            {
                return;
            }

            da.TryGetList(1, out List<string> names);
            names = names ?? new List<string>();
            da.TryGetList(2, out List<string> descriptions);
            descriptions = descriptions ?? new List<string>();
            da.TryGetList(3, out List<string> sources);
            sources = sources ?? new List<string>();
            da.TryGetList(4, out List<string> versions);
            versions = versions ?? new List<string>();
            da.TryGetList(5, out List<string> dates);
            dates = dates ?? new List<string>();

            foreach (var pair in new (string Name, int Count)[]
                     {
                         ("Names", names.Count),
                         ("Descriptions", descriptions.Count),
                         ("Sources", sources.Count),
                         ("Versions", versions.Count),
                         ("Dates", dates.Count)
                     })
            {
                if (pair.Count > 1 &&
                    pair.Count != systemWrappers.Count)
                {
                    this.AddError(
                        $"The size of the input {pair.Name} must be 0, 1 or equal to the size of the input ClassificationSystemGuids.");
                    return;
                }
            }

            var input = new UpdateClassificationSystemsParameters
            {
                ClassificationSystems = new List<ClassificationSystemUpdate>()
            };

            for (var i = 0; i < systemWrappers.Count; i++)
            {
                var id = GuidObject<ClassificationGuid>.CreateFromWrapper(systemWrappers[i]);
                if (id == null)
                {
                    this.AddError(
                        "Invalid identifier in the ClassificationSystemGuids input.");
                    return;
                }

                input.ClassificationSystems.Add(
                    new ClassificationSystemUpdate
                    {
                        ClassificationSystemId = id,
                        Name = GetAt(names, i),
                        Description = GetAt(descriptions, i),
                        Source = GetAt(sources, i),
                        Version = GetAt(versions, i),
                        Date = GetAt(dates, i)
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.UpdateClassificationSystems;

        public override Guid ComponentGuid =>
            new Guid("93984ead-7f8d-4ac6-b509-b959d75eac90");
    }
}
