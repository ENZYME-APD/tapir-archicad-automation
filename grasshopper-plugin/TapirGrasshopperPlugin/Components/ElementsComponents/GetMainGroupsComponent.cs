using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetMainGroupsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetMainGroups";

        public GetMainGroupsComponent()
            : base(
                "GetMainGroups",
                "Get the top level group of each given group. A group that is not nested in another group " +
                "is its own top level group.",
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
            OutGenerics(
                "MainGroupGuids",
                "Top level group of each given group (null when failed).");

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

            var mainGroups = new List<GroupGuidWrapper>();
            var errors = new List<string>();

            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "mainGroups"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    mainGroups.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                var guid = JsonOutputHelp.WrappedGuidOf(
                    item,
                    "groupId");
                mainGroups.Add(
                    guid == null
                        ? null
                        : new GroupGuidWrapper { GroupId = new GroupGuid { Guid = guid } });
                errors.Add("");
            }

            da.SetDataList(0, mainGroups);
            da.SetDataList(1, errors);
        }

        // Shares the icon of GetGroupsOfElements.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetGroupsOfElements;

        public override Guid ComponentGuid =>
            new Guid("17b8c896-b19b-49ce-bbad-487033b9f5a2");
    }
}
