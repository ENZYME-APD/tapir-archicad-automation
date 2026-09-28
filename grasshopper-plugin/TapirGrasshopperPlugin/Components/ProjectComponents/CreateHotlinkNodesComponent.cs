using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class CreateHotlinkNodesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateHotlinkNodes";

        public CreateHotlinkNodesComponent()
            : base(
                "CreateHotlinkNodes",
                "Create hotlink module nodes from module source files. A node that already " +
                "points at the same file is returned as it is, with Existing true, and the " +
                "name and story settings asked for are ignored.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "SourceLocations",
                "Absolute path of each module source file (.mod or .pln).");

            InTexts(
                "Names",
                "Display name of each node, defaulting to the file name " +
                "(input only 1 to use the same value for all). Optional.");

            InTexts(
                "StoryRangeTypes",
                "Which stories of the source are placed: AllStories or SingleStory " +
                "(input only 1 to use the same value for all). Optional.");

            InIntegers(
                "RefFloorIndices",
                "Index of the reference story in the source file, defaulting to 0 " +
                "(input only 1 to use the same value for all). Optional.");

            SetOptionality(new[] { 1, 2, 3 });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "HotlinkNodeIds",
                "Identifier of each created or already existing node (null for failed items).");

            OutBooleans(
                "Existing",
                "True where a node for the same source file was already there and was " +
                "returned instead of created.");

            OutErrorMessages(
                "Error message of each node (empty when it was created successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> sourceLocations))
            {
                return;
            }

            da.TryGetList(
                1,
                out List<string> names);

            da.TryGetList(
                2,
                out List<string> storyRangeTypes);

            da.TryGetList(
                3,
                out List<int> refFloorIndices);

            foreach (var pair in new (string Name, int Count)[]
                     {
                         ("Names", names.Count),
                         ("StoryRangeTypes", storyRangeTypes.Count),
                         ("RefFloorIndices", refFloorIndices.Count)
                     })
            {
                if (pair.Count > 1 &&
                    pair.Count != sourceLocations.Count)
                {
                    this.AddError(
                        $"The size of the input {pair.Name} must be 0, 1 or equal to the size of the input SourceLocations.");
                    return;
                }
            }

            var nodes = new JArray();
            for (var i = 0; i < sourceLocations.Count; i++)
            {
                var node = new JObject { ["sourceLocation"] = sourceLocations[i] };

                if (names.Count > 0)
                {
                    var name = names[names.Count == 1 ? 0 : i];
                    if (!string.IsNullOrEmpty(name))
                    {
                        node["name"] = name;
                    }
                }

                if (storyRangeTypes.Count > 0)
                {
                    var storyRangeType =
                        storyRangeTypes[storyRangeTypes.Count == 1 ? 0 : i];
                    if (!string.IsNullOrEmpty(storyRangeType))
                    {
                        node["storyRangeType"] = storyRangeType;
                    }
                }

                if (refFloorIndices.Count > 0)
                {
                    node["refFloorIndex"] =
                        refFloorIndices[refFloorIndices.Count == 1 ? 0 : i];
                }

                nodes.Add(node);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["hotlinkNodes"] = nodes },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var ids = new List<HotlinkNodeGuid>();
            var existing = new List<object>();
            var errorMessages = new List<string>();

            foreach (var item in JsonOutputHelp.Items(response, "hotlinkNodes"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    ids.Add(null);
                    existing.Add(null);
                    errorMessages.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                ids.Add(
                    HotlinkNodeGuid.FromString(
                        JsonOutputHelp.GuidOf(item["hotlinkNodeId"])));
                existing.Add(JsonOutputHelp.Scalar(item, "existing"));
                errorMessages.Add("");
            }

            da.SetDataList(0, ids);
            da.SetDataList(1, existing);
            da.SetDataList(2, errorMessages);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateHotlinkNodes;

        public override Guid ComponentGuid =>
            new Guid("1509d2ac-0ddc-3860-3a02-55fcac73cfb2");
    }
}
