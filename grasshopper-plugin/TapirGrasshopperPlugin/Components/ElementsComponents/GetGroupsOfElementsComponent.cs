using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetGroupsOfElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetGroupsOfElements";

        public GetGroupsOfElementsComponent()
            : base(
                "GetGroupsOfElements",
                "Get the identifier of the group that directly contains each given element. " +
                "The GroupGuids output holds null for elements that are not part of any group.",
                GroupNames.ElementGrouping)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to query.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "GroupGuids",
                "Identifier of the group of each element (null for ungrouped elements).");

            OutTexts(
                "ErrorMessages",
                "Error message for each queried element (empty when the element is part of a group).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(elements),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var groupGuids = new List<object>();
            var errors = new List<string>();

            if (response["groupGuids"] is JArray items)
            {
                foreach (var item in items)
                {
                    if (item?["error"] != null)
                    {
                        errors.Add(item["error"]?["message"]?.ToString() ?? "");
                        groupGuids.Add(null);
                        continue;
                    }

                    errors.Add("");
                    var guid = item["groupId"]?["guid"]?.ToString();
                    groupGuids.Add(
                        guid == null
                            ? null
                            : new GroupGuidWrapper
                            {
                                GroupId = new GroupGuid { Guid = guid }
                            });
                }
            }

            da.SetDataList(0, groupGuids);
            da.SetDataList(1, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetGroupsOfElements;

        public override Guid ComponentGuid =>
            new Guid("d9c1f1ef-4b1e-477e-9426-5627e31736b5");
    }
}
