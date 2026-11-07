using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class DeleteHotlinkNodesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteHotlinkNodes";

        public DeleteHotlinkNodesComponent()
            : base(
                "DeleteHotlinkNodes",
                "Delete hotlink nodes together with their placed instances and their " +
                "elements, as the Hotlink Manager's Delete does. Use BreakHotlinkNodes " +
                "instead to keep the elements. The nodes come from Hotlinks or " +
                "CreateHotlinkNodes.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "HotlinkNodeIds",
                "Identifiers of the hotlink nodes to delete.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each node (empty when it was deleted successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> nodeIdWrappers))
            {
                return;
            }

            var nodes = new JArray();
            foreach (var wrapper in nodeIdWrappers)
            {
                var nodeId = GuidObject<HotlinkNodeGuid>.CreateFromWrapper(wrapper);
                if (nodeId == null)
                {
                    this.AddError(
                        "Invalid hotlink node identifier in the HotlinkNodeIds input.");
                    return;
                }
                nodes.Add(
                    new JObject
                    {
                        ["hotlinkNodeId"] = new JObject { ["guid"] = nodeId.Guid }
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["hotlinkNodes"] = nodes },
                ToAddOn,
                da);
        }

        // Shares the Hotlinks icon until a badged one is drawn for it.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Hotlinks;

        public override Guid ComponentGuid =>
            new Guid("a6d41c2e-7f38-4b9d-8e15-2c9b6f0d7a48");
    }
}
