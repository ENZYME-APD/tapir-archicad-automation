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
    public class ChangeHotlinkNodesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ChangeHotlinkNodes";

        public ChangeHotlinkNodesComponent()
            : base(
                "ChangeHotlinkNodes",
                "Repoint hotlink module nodes at other source files, as the Hotlink " +
                "Manager's Relink button does. The placed instances stay where they are; " +
                "follow it with UpdateHotlinks to re-read the content from the new file.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "HotlinkNodeIds",
                "Identifiers of the hotlink nodes to repoint.");

            InTexts(
                "SourceLocations",
                "Absolute path of the new module source file (.mod or .pln) of each node " +
                "(input only 1 to use the same value for all).");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each node (empty when it was repointed successfully).");
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

            if (!da.TryGetList(
                    1,
                    out List<string> sourceLocations))
            {
                return;
            }

            if (sourceLocations.Count != 1 &&
                sourceLocations.Count != nodeIdWrappers.Count)
            {
                this.AddError(
                    "The size of the input SourceLocations must be 1 or equal to the size of the input HotlinkNodeIds.");
                return;
            }

            var nodes = new JArray();
            for (var i = 0; i < nodeIdWrappers.Count; i++)
            {
                var nodeId = GuidObject<HotlinkNodeGuid>.CreateFromWrapper(nodeIdWrappers[i]);
                if (nodeId == null)
                {
                    this.AddError(
                        "Invalid hotlink node identifier in the HotlinkNodeIds input.");
                    return;
                }
                nodes.Add(
                    new JObject
                    {
                        ["hotlinkNodeId"] = new JObject { ["guid"] = nodeId.Guid },
                        ["sourceLocation"] = sourceLocations[sourceLocations.Count == 1 ? 0 : i]
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
            new Guid("b58c2e91-6a0d-47f3-8d2c-14e9a7f06b3a");
    }
}
