using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.MEP;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class DeleteMEPPhysicalSystemsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteMEPPhysicalSystems";

        public DeleteMEPPhysicalSystemsComponent()
            : base(
                "DeleteMEPPhysicalSystems",
                "Removes MEP physical systems from their system groups. Their member elements are not deleted. " +
                "Available from Archicad 29.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "PhysicalSystemGuids",
                "Identifiers of the physical systems to remove, as handed out by GetMEPSystemGroups or CreateMEPPhysicalSystems.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each physical system (empty when it was removed).");
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

            var items = new JArray();
            foreach (var wrapper in wrappers)
            {
                var id = GuidObject<MEPPhysicalSystemGuid>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError("Invalid identifier in the PhysicalSystemGuids input.");
                    return;
                }
                items.Add(
                    new JObject
                    {
                        ["physicalSystemId"] = new JObject { ["guid"] = id.Guid }
                    });
            }

            var parameters = new JObject { ["physicalSystems"] = items };

            SetCadValuesWithErrorMessages(
                CommandName,
                parameters,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteMEPPhysicalSystems;

        public override Guid ComponentGuid =>
            new Guid("3247529c-bd47-416a-8593-8a24b7545a0d");
    }
}
