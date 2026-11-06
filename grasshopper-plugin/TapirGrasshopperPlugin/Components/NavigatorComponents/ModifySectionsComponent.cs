using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class ModifySectionsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ModifySections";

        public ModifySectionsComponent()
            : base(
                "ModifySections",
                "Change the name and the reference ID of Section and Elevation elements.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the Sections or Elevations to change.");

            InTexts(
                "Names",
                "New name of each element (input only 1 to use the same name for all). Optional, the name is kept without it.");

            InTexts(
                "IDs",
                "New reference ID of each element, shown in its marker and in the Project Map; a '/' splits it between the start and the end marker (input only 1 to use the same ID for all). Optional, the ID is kept without it.");

            SetOptionality(new[] { 1, 2 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when it was changed).");
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

            da.TryGetList(1, out List<string> names);
            names = names ?? new List<string>();
            da.TryGetList(2, out List<string> ids);
            ids = ids ?? new List<string>();

            foreach (var pair in new (string Name, int Count)[]
                     {
                         ("Names", names.Count),
                         ("IDs", ids.Count)
                     })
            {
                if (pair.Count > 1 &&
                    pair.Count != elementWrappers.Count)
                {
                    this.AddError(
                        $"The size of the input {pair.Name} must be 0, 1 or equal to the size of the input ElementGuids.");
                    return;
                }
            }

            var items = new JArray();
            for (var i = 0; i < elementWrappers.Count; i++)
            {
                var elementId = GuidObject<ElementGuid>.CreateFromWrapper(elementWrappers[i]);
                if (elementId == null)
                {
                    this.AddError("Invalid element identifier in the ElementGuids input.");
                    return;
                }

                var item = new JObject
                {
                    ["elementId"] = new JObject { ["guid"] = elementId.Guid }
                };

                if (names.Count > 0)
                {
                    item["name"] = names[names.Count == 1 ? 0 : i];
                }

                if (ids.Count > 0)
                {
                    item["id"] = ids[ids.Count == 1 ? 0 : i];
                }

                items.Add(item);
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["sectionsWithDetails"] = items },
                ToAddOn,
                da);
        }

        // No icon of its own yet: the one of CreateSections, the other section component.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateSections;

        public override Guid ComponentGuid =>
            new Guid("5d3f8a2e-7c41-4b9e-a6d2-0f8e3c1b7a94");
    }
}
