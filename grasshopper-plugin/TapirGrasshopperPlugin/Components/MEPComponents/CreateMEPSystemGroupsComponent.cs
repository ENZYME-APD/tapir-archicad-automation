using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.MEP;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class CreateMEPSystemGroupsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateMEPSystemGroups";

        public CreateMEPSystemGroupsComponent()
            : base(
                "CreateMEPSystemGroups",
                "Create system groups for the MEP physical systems with the given names. " +
                "Every execution creates new groups. " +
                "Available from Archicad 29.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Name of each system group to create.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "SystemGroupGuids",
                "Identifier of each created system group.");

            OutErrorMessages(
                "Error message of each system group (empty when it was created successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> names))
            {
                return;
            }

            var items = new JArray();
            foreach (var name in names)
            {
                items.Add(
                    new JObject
                    {
                        ["name"] = name ?? ""
                    });
            }

            var parameters = new JObject { ["systemGroupsData"] = items };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            SetCreatedIdsAndErrorMessages<MEPSystemGroupGuid>(
                da,
                response,
                "systemGroups",
                "systemGroupId");
        }

        // Shares the icon of CreateGroups until it gets its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateGroups;

        public override Guid ComponentGuid =>
            new Guid("9dfaca8c-d9a1-41c4-ad1b-58d77d5d4c41");
    }
}
