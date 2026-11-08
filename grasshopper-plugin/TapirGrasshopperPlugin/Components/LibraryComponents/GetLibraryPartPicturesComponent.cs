using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class GetLibraryPartPicturesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLibraryPartPictures";

        public GetLibraryPartPicturesComponent()
            : base(
                "GetLibraryPartPictures",
                "Gets the preview picture and the GDL pictures of a loaded library part as base64 encoded PNGs, " +
                "and from Archicad 29 also their dark user interface variants. Available from Archicad 28.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Name",
                "Name of the loaded library part file with its extension, as the fileName of GetAvailableLibraryParts.");
        }

        protected override void AddOutputs()
        {
            OutText(
                "PreviewPicture",
                "The preview picture as a base64 encoded PNG (empty when the library part has none).");

            OutText(
                "DarkPreviewPicture",
                "The preview picture for the dark user interface as a base64 encoded PNG " +
                "(empty when the library part has none or before Archicad 29).");

            OutTexts(
                "GDLPictures",
                "The pictures stored in the library part for its GDL scripts, as base64 encoded PNGs, in their stored order.");

            OutTexts(
                "DarkGDLPictures",
                "The GDL pictures for the dark user interface, as base64 encoded PNGs (empty before Archicad 29).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            string name = null;
            if (!da.GetData(0, ref name))
            {
                return;
            }

            var parameters = new JObject { ["name"] = name };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(0, response["previewPicture"]?.ToString());
            da.SetData(1, response["darkPreviewPicture"]?.ToString());
            da.SetDataList(2, Texts(response, "gdlPictures"));
            da.SetDataList(3, Texts(response, "darkGdlPictures"));
        }

        private static List<string> Texts(
            JObject response,
            string arrayKey)
        {
            var texts = new List<string>();
            foreach (var item in JsonOutputHelp.Items(response, arrayKey))
            {
                texts.Add(item?.ToString());
            }
            return texts;
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetLibraryPartPictures;

        public override Guid ComponentGuid =>
            new Guid("164f915d-7e66-4e75-a73d-dbc918d611e8");
    }
}
