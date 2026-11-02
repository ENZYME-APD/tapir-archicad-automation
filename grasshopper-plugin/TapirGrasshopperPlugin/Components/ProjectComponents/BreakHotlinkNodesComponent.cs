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
    public class BreakHotlinkNodesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "BreakHotlinkNodes";

        public BreakHotlinkNodesComponent()
            : base(
                "BreakHotlinkNodes",
                "Break hotlink nodes, as the Hotlink Manager's Break Hotlink does: the " +
                "elements of each node's placed instances stay in the project as ordinary " +
                "elements, no longer tied to the source file. The nodes come from Hotlinks " +
                "or CreateHotlinkNodes.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "HotlinkNodeIds",
                "Identifiers of the hotlink nodes to break.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each node (empty when it was broken successfully).");
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
            new Guid("3f9e5a7c-1b2d-4e86-9a4f-7c0d2e815b63");
    }
}
