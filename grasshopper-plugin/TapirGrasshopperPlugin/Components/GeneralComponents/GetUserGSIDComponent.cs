using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class GetUserGSIDComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetUserGSID";

        public GetUserGSIDComponent()
            : base(
                "GetUserGSID",
                "Get the GSID and organization identifiers of the signed-in Graphisoft user. Available from Archicad 27.",
                GroupNames.General)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "UserId",
                "The stable GSID user identifier of the signed-in user.");

            OutTexts(
                "OrganizationIds",
                "The identifiers of the organizations the user belongs to.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    null,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(0, response["userId"]?.ToString());

            var organizationIds = new List<string>();
            if (response["organizationIds"] is JArray items)
            {
                foreach (var item in items)
                {
                    organizationIds.Add(item?.ToString());
                }
            }
            da.SetDataList(1, organizationIds);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetUserGSID;

        public override Guid ComponentGuid =>
            new Guid("7518ab05-07e9-436e-a506-b6c4ad24764e");
    }
}
