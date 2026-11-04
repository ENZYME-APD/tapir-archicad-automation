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
    public class GetAllElementsOfGroupsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAllElementsOfGroups";

        public GetAllElementsOfGroupsComponent()
            : base(
                "GetAllElementsOfGroups",
                "Get all the elements of each given group, including the elements of its nested groups.",
                GroupNames.ElementGrouping)
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
                "Identifiers of all the elements of each group, nested groups included (one branch per group).");

            OutErrorMessages(
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

            var items = JsonOutputHelp.Items(
                response,
                "elementsOfGroups");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                elementGuids.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");
                if (item["elements"] is JArray elements)
                {
                    foreach (var element in elements)
                    {
                        elementGuids.Add(
                            ItemListHelp.ElementGuidOf(element?["elementId"]),
                            path);
                    }
                }
            }

            da.SetDataTree(0, elementGuids);
            da.SetDataList(1, errors);
        }

        // Shares the icon of GetElementsOfGroups.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetElementsOfGroups;

        public override Guid ComponentGuid =>
            new Guid("07fc89c8-4f1d-4a2d-98a8-43349efbe940");
    }
}
