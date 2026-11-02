using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class SetLibraryPartPicturesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetLibraryPartPictures";

        public SetLibraryPartPicturesComponent()
            : base(
                "SetLibraryPartPictures",
                "Deletes the preview picture, the GDL pictures (and from Archicad 29 their dark variants) or the 2D symbol " +
                "of a loaded library part, or copies the 2D symbol from another library part. A library part that is not " +
                "editable is refused and left unchanged. Available from Archicad 28.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Name",
                "Name of the loaded library part file with its extension, as the fileName of GetAvailableLibraryParts.");

            InBoolean(
                "DeletePreviewPicture",
                "Delete the preview picture.",
                false);

            InBoolean(
                "DeleteDarkPreviewPicture",
                "Delete the preview picture for the dark user interface (Archicad 29 or newer).",
                false);

            InBoolean(
                "DeleteGDLPictures",
                "Delete all GDL pictures.",
                false);

            InBoolean(
                "DeleteDarkGDLPictures",
                "Delete all GDL pictures for the dark user interface (Archicad 29 or newer).",
                false);

            InText(
                "Copy2DSymbolFrom",
                "Name of another loaded library part (fileName with extension) whose 2D symbol (the drawn 2D vector image) " +
                "replaces the 2D symbol of this part. Cannot be combined with Delete2DSymbol. Optional.");

            InBoolean(
                "Delete2DSymbol",
                "Delete the 2D symbol (the drawn 2D vector image).",
                false);

            SetOptionality(new[] { 1, 2, 3, 4, 5, 6 });
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
            if (!da.GetData(0, ref name))
            {
                return;
            }

            var parameters = new JObject { ["name"] = name };

            // Only the requested operations are sent.
            foreach (var (index, key) in new[]
                     {
                         (1, "deletePreviewPicture"),
                         (2, "deleteDarkPreviewPicture"),
                         (3, "deleteGdlPictures"),
                         (4, "deleteDarkGdlPictures"),
                         (6, "delete2DSymbol")
                     })
            {
                if (da.GetOptional(index, false))
                {
                    parameters[key] = true;
                }
            }

            string copyFrom = null;
            if (da.GetData(5, ref copyFrom) &&
                !string.IsNullOrEmpty(copyFrom))
            {
                parameters["copy2DSymbolFrom"] = copyFrom;
            }

            if (parameters.Count == 1)
            {
                this.AddError("Give at least one operation to do.");
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
            new Guid("5f7eecaf-7fc5-4c59-b635-6d51066b33e4");
    }
}
