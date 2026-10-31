using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetFontsByNameComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetFontsByName";

        public GetFontsByNameComponent()
            : base(
                "GetFontsByName",
                "Finds loaded fonts by name through Archicad's font search and returns their index (as Text and " +
                "Label fields reference fonts in fontIndex) and name, or an error for a name no loaded font has.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "FontNames",
                "Names of the fonts to find, as the Text tool's font list shows them, e.g. 'Arial'.");
        }

        protected override void AddOutputs()
        {
            OutIntegers(
                "FontIndices",
                "Index of each font (from 1), as Text and Label fields reference it in fontIndex. " +
                "Null for a name no loaded font has.");

            OutTexts(
                "FontNames",
                "Name of each font as Archicad carries it. Null for a name no loaded font has.");

            OutErrorMessages(
                "Error message of each name (empty when a font was found).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> fontNames))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["fontNames"] = new JArray(fontNames) },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var indices = new List<object>();
            var names = new List<object>();
            var errors = new List<string>();

            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "fonts"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    indices.Add(null);
                    names.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                indices.Add(JsonOutputHelp.Scalar(item, "fontIndex"));
                names.Add(JsonOutputHelp.Scalar(item, "fontName"));
                errors.Add("");
            }

            da.SetDataList(0, indices);
            da.SetDataList(1, names);
            da.SetDataList(2, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetFontsByName;

        public override Guid ComponentGuid =>
            new Guid("f7e9cecd-a5cf-4119-b51b-7c16ad97231c");
    }
}
