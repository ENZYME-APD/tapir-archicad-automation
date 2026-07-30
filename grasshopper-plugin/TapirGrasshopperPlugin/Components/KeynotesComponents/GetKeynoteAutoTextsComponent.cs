using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Keynotes;

namespace TapirGrasshopperPlugin.Components.KeynotesComponents
{
    public class GetKeynoteAutoTextsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetKeynoteAutoTexts";

        public GetKeynoteAutoTextsComponent()
            : base(
                "GetKeynoteAutoTexts",
                "Get the autotext tokens of the given keynote items. " +
                "The tokens can be used as label text content to reference the fields of a keynote item. " +
                "Available from Archicad 28.",
                GroupNames.Keynotes)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ItemGuids",
                "Identifiers of the keynote items.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "KeyTokens",
                "Autotext token referencing the key of each keynote item.");

            OutTexts(
                "TitleTokens",
                "Autotext token referencing the title of each keynote item.");

            OutTexts(
                "DescriptionTokens",
                "Autotext token referencing the description of each keynote item.");

            OutTexts(
                "ReferenceTokens",
                "Autotext token referencing the reference of each keynote item.");

            OutTexts(
                "ErrorMessages",
                "Error message for each queried keynote item (empty when successful).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out KeynoteItemsObject input))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(input),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var keyTokens = new List<object>();
            var titleTokens = new List<object>();
            var descriptionTokens = new List<object>();
            var referenceTokens = new List<object>();
            var errors = new List<string>();

            if (response["autoTexts"] is JArray autoTexts)
            {
                foreach (var autoText in autoTexts)
                {
                    if (autoText?["error"] != null)
                    {
                        errors.Add(autoText["error"]?["message"]?.ToString() ?? "");
                        keyTokens.Add(null);
                        titleTokens.Add(null);
                        descriptionTokens.Add(null);
                        referenceTokens.Add(null);
                        continue;
                    }

                    errors.Add("");
                    keyTokens.Add(autoText["keyToken"]?.ToString());
                    titleTokens.Add(autoText["titleToken"]?.ToString());
                    descriptionTokens.Add(autoText["descriptionToken"]?.ToString());
                    referenceTokens.Add(autoText["referenceToken"]?.ToString());
                }
            }

            da.SetDataList(0, keyTokens);
            da.SetDataList(1, titleTokens);
            da.SetDataList(2, descriptionTokens);
            da.SetDataList(3, referenceTokens);
            da.SetDataList(4, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetKeynoteAutoTexts;

        public override Guid ComponentGuid =>
            new Guid("d97c44ec-c12a-44b9-90ab-fcb77d332e98");
    }
}
