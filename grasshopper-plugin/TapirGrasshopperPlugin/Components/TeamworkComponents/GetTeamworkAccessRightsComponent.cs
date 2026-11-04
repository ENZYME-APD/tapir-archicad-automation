using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.TeamworkComponents
{
    public class GetTeamworkAccessRightsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetTeamworkAccessRights";

        public GetTeamworkAccessRightsComponent()
            : base(
                "GetTeamworkAccessRights",
                "Returns whether the role of the current user has the given Teamwork access rights " +
                "(API_TWAccessRights names without the API prefix, e.g. LayersCreate), all of them when no " +
                "names are given.",
                GroupNames.Teamwork)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "AccessRights",
                "Optional. The names of the access rights to check, the API_TWAccessRights enumerators " +
                "without the API prefix, e.g. LayersCreate or LibraryPartDeleteModify. Unconnected checks " +
                "all rights available in this Archicad version.");

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Names",
                "The name of each checked access right, in the requested order.");

            OutBooleans(
                "HasRights",
                "Whether the role of the current user has each access right.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            var accessRights = new List<string>();
            foreach (var accessRight in da.GetOptionals<string>(0))
            {
                if (!string.IsNullOrWhiteSpace(accessRight))
                {
                    accessRights.Add(accessRight.Trim());
                }
            }

            if (accessRights.Count > 0)
            {
                parameters["accessRights"] = JArray.FromObject(accessRights);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var names = new List<string>();
            var hasRights = new List<object>();

            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "accessRights"))
            {
                names.Add(item["name"]?.ToString());
                hasRights.Add(item.Value<bool?>("hasRight"));
            }

            da.SetDataList(
                0,
                names);

            da.SetDataList(
                1,
                hasRights);
        }

        // Shares the icon of GetTeamworkStatus until it gets its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetTeamworkStatus;

        public override Guid ComponentGuid =>
            new Guid("94f627e3-cb43-4fc4-be44-3eaeccce58e9");
    }
}
