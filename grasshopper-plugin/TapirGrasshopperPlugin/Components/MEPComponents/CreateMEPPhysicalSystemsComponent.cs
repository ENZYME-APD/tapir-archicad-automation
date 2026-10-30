using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;
using TapirGrasshopperPlugin.Types.MEP;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class CreateMEPPhysicalSystemsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateMEPPhysicalSystems";

        public CreateMEPPhysicalSystemsComponent()
            : base(
                "CreateMEPPhysicalSystems",
                "Create MEP physical systems in the given system groups, optionally with the MEP element " +
                "each system starts from and the MEP system attribute used as its category. " +
                "Available from Archicad 29.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "SystemGroupGuids",
                "Identifier of the system group of each physical system, as handed out by GetMEPSystemGroups " +
                "or CreateMEPSystemGroups (input only 1 to put all into the same group).");

            InTexts(
                "Names",
                "Name of each physical system.");

            InGenerics(
                "RootElementGuids",
                "Identifier of the MEP element each physical system starts from " +
                "(input only 1 to use the same element for all). Optional. " +
                "Archicad keeps it only while the element is part of an MEP network.");

            InGenerics(
                "MEPSystemGuids",
                "Identifier of the MEP system attribute used as the category of each physical system " +
                "(input only 1 to use the same attribute for all). Optional.");

            SetOptionality(new[] { 2, 3 });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "PhysicalSystemGuids",
                "Identifier of each created physical system.");

            OutErrorMessages(
                "Error message of each physical system (empty when it was created successfully).");
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
            if (!fields.AddIds<MEPSystemGroupGuid>(da, 0, "systemGroupId") ||
                !fields.AddElementIds(da, 2, "rootElementId") ||
                !fields.AddIds<AttributeGuidObject>(da, 3, "mepSystemId"))
            {
                return;
            }

            var items = new JArray();
            for (var i = 0; i < names.Count; i++)
            {
                var item = new JObject
                {
                    ["name"] = names[i] ?? ""
                };
                fields.ApplyTo(item, i);
                items.Add(item);
            }

            var parameters = new JObject { ["physicalSystemsData"] = items };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            SetCreatedIdsAndErrorMessages<MEPPhysicalSystemGuid>(
                da,
                response,
                "physicalSystems",
                "physicalSystemId");
        }

        // Shares the icon of CreateMEPSystems until it gets its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateMEPSystems;

        public override Guid ComponentGuid =>
            new Guid("1e4e0b57-e5f8-450f-b255-3b7bed11293b");
    }
}
