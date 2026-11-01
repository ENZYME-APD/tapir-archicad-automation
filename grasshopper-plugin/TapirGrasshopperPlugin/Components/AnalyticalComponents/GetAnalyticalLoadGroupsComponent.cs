using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Analytical;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class GetAnalyticalLoadGroupsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAnalyticalLoadGroups";

        public GetAnalyticalLoadGroupsComponent()
            : base(
                "GetAnalyticalLoadGroups",
                "Retrieves the structural analysis load groups of the project with all their fields.",
                GroupNames.Analytical)
        {
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "LoadGroupGuids",
                "Identifier of each load group.");

            OutTexts(
                "Names",
                "Name of each load group.");

            OutTexts(
                "GroupTypes",
                "Type of each load group: " + AnalyticalLoadEnums.GroupTypes + ".");

            OutTexts(
                "Relations",
                "How the load cases of each load group are combined: " + AnalyticalLoadEnums.Relations + ".");

            OutTexts(
                "LoadTypes",
                "Load type of each load group as free text.");

            OutErrorMessages(
                "Error message of each load group (empty when it could be read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var loadGroupGuids = new List<object>();
            var names = new List<object>();
            var groupTypes = new List<object>();
            var relations = new List<object>();
            var loadTypes = new List<object>();
            var errors = new List<string>();

            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "loadGroups"))
            {
                var isError = JsonOutputHelp.IsError(item);
                loadGroupGuids.Add(
                    isError ? null : AnalyticalHelp.IdFrom<LoadGroupGuid>(item["loadGroupId"]));
                names.Add(isError ? null : JsonOutputHelp.Scalar(item, "name"));
                groupTypes.Add(isError ? null : JsonOutputHelp.Scalar(item, "groupType"));
                relations.Add(isError ? null : JsonOutputHelp.Scalar(item, "relation"));
                loadTypes.Add(isError ? null : JsonOutputHelp.Scalar(item, "loadType"));
                errors.Add(isError ? JsonOutputHelp.ErrorMessage(item) : "");
            }

            da.SetDataList(0, loadGroupGuids);
            da.SetDataList(1, names);
            da.SetDataList(2, groupTypes);
            da.SetDataList(3, relations);
            da.SetDataList(4, loadTypes);
            da.SetDataList(5, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAnalyticalLoadGroups;

        public override Guid ComponentGuid =>
            new Guid("ac7dd7e7-6867-4e15-8bc2-209b4d1745b3");
    }
}
