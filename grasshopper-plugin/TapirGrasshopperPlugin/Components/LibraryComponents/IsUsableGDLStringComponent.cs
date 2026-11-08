using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class IsUsableGDLStringComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "IsUsableGDLString";

        public IsUsableGDLStringComponent()
            : base(
                "IsUsableGDLString",
                "Checks whether each of the given strings is usable in GDL, " +
                "e.g. before writing it into a GDL parameter or script. Returns one boolean for each string.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Strings",
                "The strings to check.");
        }

        protected override void AddOutputs()
        {
            OutBooleans(
                "IsUsable",
                "True for each string Archicad accepts for use in GDL, in the order of the strings.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> strings))
            {
                return;
            }

            var items = new JArray();
            foreach (var text in strings)
            {
                items.Add(text ?? "");
            }

            var parameters = new JObject { ["strings"] = items };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var results = new List<object>();
            foreach (var result in JsonOutputHelp.Items(response, "results"))
            {
                results.Add((bool?)result);
            }

            da.SetDataList(0, results);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.IsUsableGDLString;

        public override Guid ComponentGuid =>
            new Guid("943a5a3c-0258-474f-94f3-4320775bf658");
    }
}
