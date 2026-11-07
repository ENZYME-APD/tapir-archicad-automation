using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.RevisionComponents
{
    public class GetRevisionChangeCustomSchemeComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetRevisionChangeCustomScheme";

        public GetRevisionChangeCustomSchemeComponent()
            : base(
                "GetRevisionChangeCustomScheme",
                "Retrieves the custom fields of changes (the change custom scheme): the key and the name of each field. " +
                "The keys are the customSchemeKey values in the customSchemeData of changes.",
                GroupNames.Revision)
        {
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "CustomSchemeKeys",
                "The identifier of each custom field of changes.");

            OutTexts(
                "CustomSchemeNames",
                "The name of each custom field of changes.");
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
            var names = new List<string>();

            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "customScheme"))
            {
                keys.Add(item["customSchemeKey"]?.ToString());
                names.Add(item["customSchemeName"]?.ToString());
            }

            da.SetDataList(
                0,
                keys);

            da.SetDataList(
                1,
                names);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetRevisionChangeCustomScheme;

        public override Guid ComponentGuid =>
            new Guid("181b1661-7954-42b3-9692-f7e56e0f793c");
    }
}
