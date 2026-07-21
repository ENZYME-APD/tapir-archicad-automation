using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetLayoutCustomSchemeComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLayoutCustomScheme";

        public GetLayoutCustomSchemeComponent()
            : base(
                "GetLayoutCustomScheme",
                "Get the Layout Info Panel custom field definitions (name and key) from Book Settings.",
                GroupNames.Navigator)
        {
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Keys",
                "Key of each custom scheme field.");

            OutTexts(
                "Names",
                "Name of each custom scheme field.");
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

            var keys = new List<object>();
            var names = new List<object>();

            foreach (var item in JsonOutputHelp.Items(response, "customScheme"))
            {
                keys.Add(JsonOutputHelp.Scalar(item, "customSchemeKey"));
                names.Add(JsonOutputHelp.Scalar(item, "customSchemeName"));
            }

            da.SetDataList(0, keys);
            da.SetDataList(1, names);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetLayoutCustomScheme;

        public override Guid ComponentGuid =>
            new Guid("fb8d59b6-15b1-4f89-9e6c-64cb07d0ef3f");
    }
}
