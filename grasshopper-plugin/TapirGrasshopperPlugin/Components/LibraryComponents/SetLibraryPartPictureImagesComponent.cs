using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class SetLibraryPartPictureImagesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetLibraryPartPictureImages";

        public SetLibraryPartPictureImagesComponent()
            : base(
                "SetLibraryPartPictureImages",
                "Sets the preview picture and the GDL pictures of a loaded library part from base64 encoded PNGs, " +
                "and from Archicad 29 also their dark user interface variants. All pictures are set in one step; an " +
                "invalid picture changes nothing. A library part that is not editable is refused and left " +
                "unchanged. Available from Archicad 28.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Name",
                "Name of the loaded library part file with its extension, as the fileName of GetAvailableLibraryParts.");

            InText(
                "PreviewPicture",
                "The new preview picture as a base64 encoded PNG, e.g. the PreviewPicture output of " +
                "GetLibraryPartPictures. Optional.");

            InTexts(
                "GDLPictures",
                "The new GDL pictures, each a base64 encoded PNG. They replace all GDL pictures of the library " +
                "part. Optional.");

            InText(
                "DarkPreviewPicture",
                "The new preview picture for the dark user interface as a base64 encoded PNG (Archicad 29 or " +
                "newer). Optional.");

            InTexts(
                "DarkGDLPictures",
                "The new GDL pictures for the dark user interface, each a base64 encoded PNG. They replace all dark " +
                "GDL pictures of the library part (Archicad 29 or newer). Optional.");

            SetOptionality(new[] { 1, 2, 3, 4 });
        }

        protected override void AddOutputs()
        {
            OutText(
                "ErrorMessage",
                "Error message of the command (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            string name = null;
            if (!da.GetData(0, ref name) ||
                string.IsNullOrEmpty(name))
            {
                return;
            }

            var parameters = new JObject { ["name"] = name };

            string previewPicture = null;
            if (da.GetData(1, ref previewPicture) &&
                !string.IsNullOrEmpty(previewPicture))
            {
                parameters["previewPicture"] = previewPicture;
            }

            var gdlPictures = da.OptionalList<string>(2);
            if (gdlPictures.Count > 0)
            {
                parameters["gdlPictures"] = new JArray(gdlPictures);
            }

            string darkPreviewPicture = null;
            if (da.GetData(3, ref darkPreviewPicture) &&
                !string.IsNullOrEmpty(darkPreviewPicture))
            {
                parameters["darkPreviewPicture"] = darkPreviewPicture;
            }

            var darkGdlPictures = da.OptionalList<string>(4);
            if (darkGdlPictures.Count > 0)
            {
                parameters["darkGdlPictures"] = new JArray(darkGdlPictures);
            }

            if (parameters.Count == 1)
            {
                this.AddError("Give at least one picture to set.");
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                LibraryComponentHelp.ExecutionResultMessage(response));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetLibraryPartPictures;

        public override Guid ComponentGuid =>
            new Guid("d2ea66f1-fcfd-4ddb-bfb9-a92c88be5f92");
    }
}
