using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetHotlinkSourceStatusComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetHotlinkSourceStatus";

        public GetHotlinkSourceStatusComponent()
            : base(
                "GetHotlinkSourceStatus",
                "Tells for each hotlink source file whether it is available, missing, not accessible, " +
                "incompatible or offline. Available from Archicad 27.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Locations",
                "Absolute paths of the hotlinks' source files, for example the Locations output of Hotlinks.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Statuses",
                "The availability of each source file: SourceAvailable, SourceMissing, SourceNotAccessible, " +
                "SourceIncompatible, SourceOffline or AuthCanceld (spelled as in the API). Null for failed items.");

            OutErrorMessages(
                "Error message of each source file (empty when its status was read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> locations))
            {
                return;
            }

            var hotlinks = new JArray();
            foreach (var location in locations)
            {
                hotlinks.Add(
                    new JObject
                    {
                        ["location"] = location ?? ""
                    });
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["hotlinks"] = hotlinks },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var statuses = new List<string>();
            var errorMessages = new List<string>();

            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "hotlinkSourceStatuses"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    statuses.Add(null);
                    errorMessages.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                statuses.Add(item["status"]?.ToString());
                errorMessages.Add("");
            }

            da.SetDataList(
                0,
                statuses);

            da.SetDataList(
                1,
                errorMessages);
        }

        // Shares the Hotlinks icon until a badged one is drawn for it.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Hotlinks;

        public override Guid ComponentGuid =>
            new Guid("27b3cf3c-dc4b-4027-bc23-82f487616b95");
    }
}
