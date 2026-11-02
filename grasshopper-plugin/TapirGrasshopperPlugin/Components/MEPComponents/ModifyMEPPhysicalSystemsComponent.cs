using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.MEP;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class ModifyMEPPhysicalSystemsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ModifyMEPPhysicalSystems";

        public ModifyMEPPhysicalSystemsComponent()
            : base(
                "ModifyMEPPhysicalSystems",
                "Modifies MEP physical systems: name, root element and the MEP system used as their category. " +
                "Only the given fields are changed. Available from Archicad 29.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "PhysicalSystemGuids",
                "Identifiers of the physical systems to modify, as handed out by GetMEPSystemGroups or CreateMEPPhysicalSystems.");

            InTexts(
                "Names",
                "New name of each physical system (input only 1 to use the same name for all). Optional.");

            InGenerics(
                "RootElementGuids",
                "Identifier of the MEP element each physical system starts from " +
                "(input only 1 to use the same element for all). Optional. " +
                "Archicad keeps it only while the element is part of an MEP network.");

            InGenerics(
                "MEPSystemGuids",
                "Identifier of the MEP system attribute used as the category of each physical system " +
                "(input only 1 to use the same attribute for all). Optional.");

            SetOptionality(new[] { 1, 2, 3 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each physical system (empty when it was modified).");
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

            var ids = new List<MEPPhysicalSystemGuid>();
            foreach (var wrapper in wrappers)
            {
                var id = GuidObject<MEPPhysicalSystemGuid>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError("Invalid identifier in the PhysicalSystemGuids input.");
                    return;
                }
                ids.Add(id);
            }

            var fields = new ElectricalItemFields(this, 0, ids.Count);
            if (!fields.AddTexts(da, 1, "name") ||
                !fields.AddElementIds(da, 2, "rootElementId") ||
                !fields.AddIds<AttributeGuidObject>(da, 3, "mepSystemId"))
            {
                return;
            }

            var items = new JArray();
            for (var i = 0; i < ids.Count; i++)
            {
                var item = new JObject
                {
                    ["physicalSystemId"] = new JObject { ["guid"] = ids[i].Guid }
                };
                fields.ApplyTo(item, i);
                items.Add(item);
            }

            var parameters = new JObject { ["physicalSystemsData"] = items };

            SetCadValuesWithErrorMessages(
                CommandName,
                parameters,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyMEPPhysicalSystems;

        public override Guid ComponentGuid =>
            new Guid("a6228d4b-1c22-40d8-805a-cede74c0fd6e");
    }
}
