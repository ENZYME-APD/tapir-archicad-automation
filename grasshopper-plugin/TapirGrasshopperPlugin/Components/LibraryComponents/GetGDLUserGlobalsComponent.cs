using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class GetGDLUserGlobalsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetGDLUserGlobals";

        public GetGDLUserGlobalsComponent()
            : base(
                "GetGDLUserGlobals",
                "Returns the current GDL global variables the GDL interpreter gets, with their names, types and " +
                "values. Available from Archicad 28.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Names",
                "Name of each GDL global variable.");

            OutTexts(
                "Types",
                "Type of each GDL global variable, as GetGDLParametersOfElements names parameter types.");

            OutTexts(
                "Values",
                "Value of each GDL global variable as text; an array as JSON.");
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

            var names = new List<string>();
            var types = new List<string>();
            var values = new List<string>();
            foreach (var gdlGlobal in JsonOutputHelp.Items(response, "globals"))
            {
                names.Add(gdlGlobal["name"]?.ToString());
                types.Add(gdlGlobal["type"]?.ToString());
                var value = gdlGlobal["value"];
                values.Add(
                    value is JArray
                        ? value.ToString(Newtonsoft.Json.Formatting.None)
                        : value?.ToString());
            }

            da.SetDataList(0, names);
            da.SetDataList(1, types);
            da.SetDataList(2, values);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetGDLUserGlobals;

        public override Guid ComponentGuid =>
            new Guid("2bca1402-1a98-4d86-8a88-1a7f2168bb0b");
    }
}
