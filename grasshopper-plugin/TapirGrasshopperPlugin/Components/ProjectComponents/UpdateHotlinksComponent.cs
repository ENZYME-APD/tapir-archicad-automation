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
    public class UpdateHotlinksComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "UpdateHotlinks";

        public UpdateHotlinksComponent()
            : base(
                "UpdateHotlinks",
                "Re-read the source files of hotlink module nodes and refresh their " +
                "cached content in the project, as the Hotlink Manager's Update button " +
                "does. The nodes come from Hotlinks or CreateHotlinkNodes.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "HotlinkNodeIds",
                "Identifiers of the hotlink nodes to update.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each node (empty when it was updated successfully).");
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
            new Guid("7d3a6f1e-2b84-4c5a-9e07-3f61b2c8d495");
    }
}
