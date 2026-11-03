using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class SetCurrentWindowDrawingScaleComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetCurrentWindowDrawingScale";

        public SetCurrentWindowDrawingScaleComponent()
            : base(
                "SetCurrentWindowDrawingScale",
                "Changes the drawing scale of the current (active) window, the scale shown in the bottom bar, " +
                "to the given denominator: 100 for 1:100. It changes the scale of the current database (floor plan, " +
                "section, elevation, interior elevation, detail, worksheet or 3D document window, each of which has " +
                "its own). Layouts are always 1:1 and are rejected. The scale must be between 1 and 30000. " +
                "The change is undoable.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            inManager.AddNumberParameter(
                "Scale",
                "Scale",
                "The denominator of the new drawing scale: 100 for 1:100, 20 for 1:20 (between 1 and 30000)."
                    .WithTypeName("numberItem"),
                GH_ParamAccess.item);

            inManager.AddBooleanParameter(
                "Rebuild",
                "Rebuild",
                "Rebuild the window after the scale changed. Optional; Archicad's default is false."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            SetOptionality(1);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out double scale))
            {
                return;
            }

            var parameters = new JObject
            {
                ["scale"] = scale
            };

            if (da.TryGet(
                    1,
                    out bool rebuild))
            {
                parameters["rebuild"] = rebuild;
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
                    "Failed to change the drawing scale of the current window.");
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetCurrentWindowDrawingScale;

        public override Guid ComponentGuid =>
            new Guid("aebd2639-61d7-4689-8958-5c6c5057cefb");
    }
}
