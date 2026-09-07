using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetAutoTextNameComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAutoTextName";

        public GetAutoTextNameComponent()
            : base(
                "GetAutoTextName",
                "Get the display name of autotext keys, as returned by GetAutoTextKeys or " +
                "GetProjectInfoFields, or as found between < and > in the content of a Text or Label.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Keys",
                "Autotext keys without the surrounding < and >, e.g. PROJECTNAME or " +
                "PROPERTY-69A58F6F-DD3B-478D-B5EF-09A16BD0C548.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Names",
                "Display name of each key (null for a key that could not be resolved).");

            OutErrorMessages(
                "Error message of each key (empty when it was resolved).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> keys) ||
                keys.Count == 0)
            {
                return;
            }

            var parameters = new JObject { ["keys"] = new JArray(keys) };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var names = new List<string>();
            var errorMessages = new List<string>();

            foreach (var item in JsonOutputHelp.Items(response, "autoTextNames"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    names.Add(null);
                    errorMessages.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                names.Add(item["name"]?.ToString() ?? "");
                errorMessages.Add("");
            }

            da.SetDataList(0, names);
            da.SetDataList(1, errorMessages);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAutoTextName;

        public override Guid ComponentGuid =>
            new Guid("d8fef9b3-6d83-440f-b56d-e0f969143200");
    }
}
