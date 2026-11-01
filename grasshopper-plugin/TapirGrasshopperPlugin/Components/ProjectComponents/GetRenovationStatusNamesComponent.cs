using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetRenovationStatusNamesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetRenovationStatusNames";

        public GetRenovationStatusNamesComponent()
            : base(
                "GetRenovationStatusNames",
                "Gets the names the running Archicad shows for the renovation statuses Existing, New and Demolished " +
                "(localized). Available from Archicad 27.",
                GroupNames.Project)
        {
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Statuses",
                "The renovation statuses, in the order Existing, New, Demolished (null where the name could not be got).");

            OutTexts(
                "Names",
                "The name of each status as the running Archicad shows it (localized).");

            OutErrorMessages(
                "Error message of each status (empty when its name was got).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out var response))
            {
                return;
            }

            var statuses = new List<string>();
            var names = new List<string>();
            var errors = new List<string>();
            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "renovationStatuses"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    statuses.Add(null);
                    names.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                statuses.Add(item["status"]?.ToString());
                names.Add(item["name"]?.ToString());
                errors.Add("");
            }

            da.SetDataList(0, statuses);
            da.SetDataList(1, names);
            da.SetDataList(2, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetRenovationStatusNames;

        public override Guid ComponentGuid =>
            new Guid("cab8d087-14bb-4e12-951b-737583a37417");
    }
}
