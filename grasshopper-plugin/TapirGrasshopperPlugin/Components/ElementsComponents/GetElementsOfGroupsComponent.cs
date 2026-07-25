using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetElementsOfGroupsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetElementsOfGroups";

        public GetElementsOfGroupsComponent()
            : base(
                "GetElementsOfGroups",
                "Get the elements directly contained by each given group.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "GroupGuids",
                "Identifiers of the groups to query.");
        }

        protected override void AddOutputs()
        {
            OutGenericTree(
                "ElementGuids",
                "Identifiers of the elements directly contained by each group (one branch per group).");

            OutTexts(
                "ErrorMessages",
                "Error message for each queried group (empty when successful).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> groupWrappers))
            {
                return;
            }

            var groups = new JArray();
            foreach (var wrapper in groupWrappers)
            {
                var id = GuidObject<GroupGuid>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError("Invalid group identifier in the GroupGuids input.");
                    return;
                }
                groups.Add(
                    new JObject
                    {
                        ["groupId"] = new JObject { ["guid"] = id.Guid }
                    });
            }

            var parameters = new JObject { ["groups"] = groups };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var elementGuids = new DataTree<object>();
            var errors = new List<string>();

            var items = response["elementsOfGroups"] as JArray ?? new JArray();
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                elementGuids.EnsurePath(path);

                if (item?["error"] != null)
                {
                    errors.Add(item["error"]?["message"]?.ToString() ?? "");
                    continue;
                }

                errors.Add("");
                if (item["elements"] is JArray elements)
                {
                    foreach (var element in elements)
                    {
                        var guid = element["elementId"]?["guid"]?.ToString();
                        elementGuids.Add(
                            guid == null
                                ? null
                                : new ElementGuidWrapper
                                {
                                    ElementId = new ElementGuid { Guid = guid }
                                },
                            path);
                    }
                }
            }

            da.SetDataTree(0, elementGuids);
            da.SetDataList(1, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetElementsOfGroups;

        public override Guid ComponentGuid =>
            new Guid("af3feefa-89df-49b6-a899-acc65ebcdeee");
    }
}
