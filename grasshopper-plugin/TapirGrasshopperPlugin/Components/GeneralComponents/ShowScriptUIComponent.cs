using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class ShowScriptUIComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ShowScriptUI";

        // The optional boolean options of the palette: input name, JSON key
        // and description, in the order of the inputs after Title.
        private static readonly string[][] BooleanOptions =
        {
            new[] { "Resizable", "resizable", "Whether the user can drag-resize the palette. Archicad's default is true." },
            new[] { "ZoomEnabled", "zoomEnabled", "Whether Ctrl+scroll/pinch zooming is allowed in the page. Archicad's default is true." },
            new[] { "ScrollBarsVisible", "scrollBarsVisible", "Whether the browser's own scrollbars are shown. Archicad's default is true." },
            new[] { "ContextMenuEnabled", "contextMenuEnabled", "Whether right-click shows the browser's context menu (e.g. Inspect Element). Archicad's default is true." },
            new[] { "NavigationDisabled", "navigationDisabled", "Prevents the page from navigating away (e.g. clicking a link to another site) once it has loaded. Archicad's default is false." },
            new[] { "AllowSelfSignedCertificates", "allowSelfSignedCertificates", "Only relevant if the HTML links to a remote https:// resource with a self-signed certificate. Archicad's default is false." },
            new[] { "ClearCookies", "clearCookies", "Deletes all browser cookies before loading the HTML. Archicad's default is false." },
            new[] { "AutoHeight", "autoHeight", "Automatically resizes the palette's height to fit the page content as it changes. Archicad's default is false." }
        };

        private const int FirstBooleanOptionIndex = 5;

        // The window inputs come after the boolean options, so saved
        // definitions keep finding the older inputs by position (#1819).
        private static readonly int UiIdIndex = FirstBooleanOptionIndex + BooleanOptions.Length;
        private static readonly int ScriptIdIndex = UiIdIndex + 1;

        public ShowScriptUIComponent()
            : base(
                "ShowScriptUI",
                "Shows the given HTML content in a native Archicad palette (a tkinter alternative for Python scripts) " +
                "and outputs the UiId of its window. Read what the page submits with GetScriptUIResult and that UiId.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "HtmlContent",
                "The full HTML document to display in the Script UI palette.");

            inManager.AddIntegerParameter(
                "Width",
                "Width",
                "Palette client width in pixels. Optional; keeps the current size when not given."
                    .WithTypeName("integerItem"),
                GH_ParamAccess.item);

            inManager.AddIntegerParameter(
                "Height",
                "Height",
                "Palette client height in pixels. Optional; keeps the current size when not given."
                    .WithTypeName("integerItem"),
                GH_ParamAccess.item);

            InText(
                "Title",
                "Palette window title. Optional; keeps the current title when not given.");

            inManager.AddNumberParameter(
                "ZoomLevel",
                "ZoomLevel",
                "Initial zoom level (1.0 = 100%). Optional; keeps the current zoom when not given."
                    .WithTypeName("numberItem"),
                GH_ParamAccess.item);

            foreach (var option in BooleanOptions)
            {
                inManager.AddBooleanParameter(
                    option[0],
                    option[0],
                    (option[2] + " Optional.").WithTypeName("booleanItem"),
                    GH_ParamAccess.item);
            }

            inManager.AddIntegerParameter(
                "UiId",
                "UiId",
                ("The Script UI window to show the page in (1 to 8), as output by an earlier ShowScriptUI. " +
                 "Takes precedence over ScriptId. Optional; without UiId and ScriptId the first window is used.")
                    .WithTypeName("integerItem"),
                GH_ParamAccess.item);

            InText(
                "ScriptId",
                "Any text that identifies this definition, so it uses its own window: the first ShowScriptUI " +
                "with a new ScriptId gets a free window. Optional.");

            for (var i = 1; i <= ScriptIdIndex; i++)
            {
                SetOptionality(i);
            }
        }

        protected override void AddOutputs()
        {
            OutInteger(
                "UiId",
                "The Script UI window the page is shown in (1 to 8). Pass it to GetScriptUIResult, " +
                "or to a later ShowScriptUI to replace the page.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string htmlContent) ||
                string.IsNullOrEmpty(htmlContent))
            {
                this.AddError("HtmlContent is required.");
                return;
            }

            var parameters = new JObject
            {
                ["htmlContent"] = htmlContent
            };

            if (da.TryGet(
                    1,
                    out int width))
            {
                parameters["width"] = width;
            }

            if (da.TryGet(
                    2,
                    out int height))
            {
                parameters["height"] = height;
            }

            if (da.TryGet(
                    3,
                    out string title) &&
                !string.IsNullOrEmpty(title))
            {
                parameters["title"] = title;
            }

            if (da.TryGet(
                    4,
                    out double zoomLevel))
            {
                parameters["zoomLevel"] = zoomLevel;
            }

            for (var i = 0; i < BooleanOptions.Length; i++)
            {
                if (da.TryGet(
                        FirstBooleanOptionIndex + i,
                        out bool value))
                {
                    parameters[BooleanOptions[i][1]] = value;
                }
            }

            if (da.TryGet(
                    UiIdIndex,
                    out int uiId))
            {
                parameters["uiId"] = uiId;
            }

            if (da.TryGet(
                    ScriptIdIndex,
                    out string scriptId) &&
                !string.IsNullOrEmpty(scriptId))
            {
                parameters["scriptId"] = scriptId;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            if ((bool?)response?["success"] != true)
            {
                this.AddError(
                    response?["error"]?["message"]?.ToString() ??
                    "Failed to show the Script UI palette.");
                return;
            }

            da.SetData(
                0,
                (int?)response["uiId"]);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ShowScriptUI;

        public override Guid ComponentGuid =>
            new Guid("1504c3a6-8c70-4d2e-a693-acba0776e6c3");
    }
}
