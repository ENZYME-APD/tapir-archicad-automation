using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetHotLinksComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetHotlinks";

        public GetHotLinksComponent()
            : base(
                nameof(Hotlinks),
                "Gets the file system locations (path) of the link modules. " +
                "The hotlinks can have tree hierarchy in the project.",
                GroupNames.Project)
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Hotlinks;

        public override Guid ComponentGuid =>
            new Guid("89ae1638-8c9f-481f-8a01-5cdf2ded8071");


        protected override void AddOutputs()
        {
            OutTexts(
                nameof(Hotlink.Location) + "s",
                "File paths of the hotlink modules in the current ArchiCad instance.");

            OutText(
                "JsonHierarchy",
                "JSON object of the tree-like hierarchy of  hotlink modules.");

            OutTextTree(
                "TreeHierarchy",
                "Nested text tree object of hotlink module names.");

            // Appended after the original outputs, so saved definitions keep their wiring.
            OutGenerics(
                "HotlinkNodeIds",
                "Identifier of each hotlink node, in the order of Locations; the input of " +
                "CreateHotlinkInstances.");

            OutTexts(
                "Names",
                "Display name of each hotlink node, in the order of Locations.");

            OutTexts(
                "Types",
                "Module or XRef, for each hotlink node in the order of Locations.");

            OutGenericTree(
                "Instances",
                "Identifiers of the hotlink instance elements placed from each node " +
                "(one branch per node, in the order of Locations); the input of " +
                "ChangeHotlinkInstances and DeleteElements.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetConvertedCadValues(
                    CommandName,
                    null,
                    ToAddOn,
                    JHelp.Deserialize<HotlinksResponse>,
                    out HotlinksResponse response))
            {
                return;
            }

            da.SetDataList(
                0,
                response.Hotlinks.GetLocations());

            var nodes = response.Hotlinks.Flatten()
                .Where(link => !string.IsNullOrEmpty(link.Location))
                .ToList();

            da.SetData(
                1,
                JsonConvert.SerializeObject(
                    response,
                    Formatting.Indented));

            da.SetDataTree(
                2,
                response.GetTree());

            da.SetDataList(
                3,
                nodes.Select(link => link.HotlinkNodeId));

            da.SetDataList(
                4,
                nodes.Select(link => link.Name));

            da.SetDataList(
                5,
                nodes.Select(link => link.Type));

            // One branch per node, empty when none is placed, so the branch
            // index always matches the index in Locations.
            var instances = new DataTree<ElementGuid>();
            for (var i = 0; i < nodes.Count; i++)
            {
                var path = new GH_Path(i);
                instances.EnsurePath(path);

                if (nodes[i].Instances == null)
                {
                    continue;
                }

                foreach (var instance in nodes[i].Instances)
                {
                    if (instance?.ElementId != null)
                    {
                        instances.Add(
                            instance.ElementId,
                            path);
                    }
                }
            }

            da.SetDataTree(
                6,
                instances);
        }
    }
}