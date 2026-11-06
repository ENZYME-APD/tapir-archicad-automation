using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetAllAutoTextKeysComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAllAutoTextKeys";

        public GetAllAutoTextKeysComponent()
            : base(
                "GetAllAutoTextKeys",
                "Gets every autotext key Archicad knows (not only the property based ones of GetAutoTextKeys) " +
                "with its current value, resolved without an element context.",
                GroupNames.Project)
        {
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Keys",
                "Key of each autotext, e.g. LAYOUTID.");

            OutTexts(
                "EmbeddableKeys",
                "Each key surrounded by < and >, ready to be put into the content of a Text or Label.");

            OutTexts(
                "Values",
                "The current value of each autotext, as Archicad resolves it without an element context.");
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

            var keys = new List<string>();
            var embeddableKeys = new List<string>();
            var values = new List<string>();

            foreach (var item in JsonOutputHelp.Items(response, "autoTexts"))
            {
                var key = item["key"]?.ToString() ?? "";
                keys.Add(key);
                embeddableKeys.Add("<" + key + ">");
                values.Add(item["value"]?.ToString() ?? "");
            }

            da.SetDataList(0, keys);
            da.SetDataList(1, embeddableKeys);
            da.SetDataList(2, values);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAutoTextKeys;

        public override Guid ComponentGuid =>
            new Guid("dab6e2ea-a217-44a3-8289-d68cbbd72b94");
    }
}
