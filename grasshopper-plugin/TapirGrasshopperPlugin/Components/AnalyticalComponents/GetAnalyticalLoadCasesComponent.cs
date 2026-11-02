using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Analytical;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class GetAnalyticalLoadCasesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAnalyticalLoadCases";

        public GetAnalyticalLoadCasesComponent()
            : base(
                "GetAnalyticalLoadCases",
                "Retrieves the structural analysis load cases of the given load group, " +
                "or of the project when no load group is given, with all their fields.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "LoadGroupGuid",
                "Identifier of the load group to list the load cases of. " +
                "Optional; every load case of the project is listed when it is omitted.");

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "LoadCaseGuids",
                "Identifier of each load case.");

            OutTexts(
                "Names",
                "Name of each load case.");

            OutTexts(
                "Descriptions",
                "Description of each load case.");

            OutTexts(
                "ActionTypes",
                "Action type of each load case: " + AnalyticalLoadEnums.ActionTypes + ".");

            OutGenerics(
                "LoadGroupGuids",
                "Identifier of the load group of each load case.");

            OutTexts(
                "LoadTypes",
                "Load type of each load case: " + AnalyticalLoadEnums.LoadCaseLoadTypes + ".");

            OutTexts(
                "Durations",
                "Duration of each load case (null when it is not set): " + AnalyticalLoadEnums.Durations + ".");

            OutErrorMessages(
                "Error message of each load case (empty when it could be read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            if (da.TryGet(
                    0,
                    out GH_ObjectWrapper loadGroupWrapper))
            {
                var loadGroupId = GuidObject<LoadGroupGuid>.CreateFromWrapper(loadGroupWrapper);
                if (loadGroupId == null)
                {
                    this.AddError("Invalid identifier in the LoadGroupGuid input.");
                    return;
                }
                parameters["loadGroupId"] = AnalyticalHelp.IdOf(loadGroupId);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var loadCaseGuids = new List<object>();
            var names = new List<object>();
            var descriptions = new List<object>();
            var actionTypes = new List<object>();
            var loadGroupGuids = new List<object>();
            var loadTypes = new List<object>();
            var durations = new List<object>();
            var errors = new List<string>();

            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "loadCases"))
            {
                var isError = JsonOutputHelp.IsError(item);
                loadCaseGuids.Add(
                    isError ? null : AnalyticalHelp.IdFrom<LoadCaseGuid>(item["loadCaseId"]));
                names.Add(isError ? null : JsonOutputHelp.Scalar(item, "name"));
                descriptions.Add(isError ? null : JsonOutputHelp.Scalar(item, "description"));
                actionTypes.Add(isError ? null : JsonOutputHelp.Scalar(item, "actionType"));
                loadGroupGuids.Add(
                    isError ? null : AnalyticalHelp.IdFrom<LoadGroupGuid>(item["loadGroupId"]));
                loadTypes.Add(isError ? null : JsonOutputHelp.Scalar(item, "loadType"));
                durations.Add(isError ? null : JsonOutputHelp.Scalar(item, "duration"));
                errors.Add(isError ? JsonOutputHelp.ErrorMessage(item) : "");
            }

            da.SetDataList(0, loadCaseGuids);
            da.SetDataList(1, names);
            da.SetDataList(2, descriptions);
            da.SetDataList(3, actionTypes);
            da.SetDataList(4, loadGroupGuids);
            da.SetDataList(5, loadTypes);
            da.SetDataList(6, durations);
            da.SetDataList(7, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAnalyticalLoadCases;

        public override Guid ComponentGuid =>
            new Guid("2aed6361-9d88-424a-91d7-9d065a6a8f74");
    }
}
