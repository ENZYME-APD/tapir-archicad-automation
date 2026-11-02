using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetNoteAutoTextKeywordsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetNoteAutoTextKeywords";

        public GetNoteAutoTextKeywordsComponent()
            : base(
                "GetNoteAutoTextKeywords",
                "Retrieves the keywords of note autotexts (the autotexts of dimension, label and other element notes) " +
                "by their numeric API_NoteAutotextID values. Available from Archicad 26.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InIntegers(
                "AutoTextIds",
                "Numeric values of the API_NoteAutotextID enumeration of the Archicad API.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Keywords",
                "Keyword of each note autotext, aligned with AutoTextIds (null where it failed).");

            OutErrorMessages(
                "Error message of each id (empty when the keyword was found).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<int> autoTextIds))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["autoTextIds"] = JArray.FromObject(autoTextIds) },
                    ToAddOn,
                    out var response))
            {
                return;
            }

            var keywords = new List<string>();
            var errors = new List<string>();
            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "keywords"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    keywords.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                keywords.Add(item["keyword"]?.ToString());
                errors.Add("");
            }

            da.SetDataList(0, keywords);
            da.SetDataList(1, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetNoteAutoTextKeywords;

        public override Guid ComponentGuid =>
            new Guid("ea2c1fd0-8a93-455b-93cb-45e707ce5fd7");
    }
}
