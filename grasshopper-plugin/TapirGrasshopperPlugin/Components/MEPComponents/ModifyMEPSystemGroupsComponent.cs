using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.MEP;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class ModifyMEPSystemGroupsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ModifyMEPSystemGroups";

        public ModifyMEPSystemGroupsComponent()
            : base(
                "ModifyMEPSystemGroups",
                "Renames MEP system groups. Available from Archicad 29.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "SystemGroupGuids",
                "Identifiers of the system groups to rename, as handed out by GetMEPSystemGroups or CreateMEPSystemGroups.");

            InTexts(
                "Names",
                "New name of each system group.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each system group (empty when it was renamed).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    1,
                    out List<string> names))
            {
                return;
            }

            var fields = new ElectricalItemFields(this, 1, names.Count);
            if (!fields.AddIds<MEPSystemGroupGuid>(da, 0, "systemGroupId"))
            {
                return;
            }

            var items = new JArray();
            for (var i = 0; i < names.Count; i++)
            {
                var item = new JObject();
                fields.ApplyTo(item, i);
                if (item.Count == 0)
                {
                    this.AddError("Give the system groups to rename in the SystemGroupGuids input.");
                    return;
                }
                item["name"] = names[i] ?? "";
                items.Add(item);
            }

            var parameters = new JObject { ["systemGroupsData"] = items };

            SetCadValuesWithErrorMessages(
                CommandName,
                parameters,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyMEPSystemGroups;

        public override Guid ComponentGuid =>
            new Guid("35dad572-5aba-4f6c-afaa-b4f13ba96cfa");
    }
}
