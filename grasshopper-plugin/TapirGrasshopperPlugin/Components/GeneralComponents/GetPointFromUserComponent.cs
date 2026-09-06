using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class GetPointFromUserComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetPointFromUser";

        public GetPointFromUserComponent()
            : base(
                "GetPointFromUser",
                "Ask the user to click a point in Archicad and return it. Archicad waits for " +
                "the click or for Escape, and every other command queues behind this one " +
                "until then.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Prompt",
                "Shown in the control box while Archicad waits for the click. " +
                "Single-byte text. Optional.");

            SetOptionality(new[] { 0 });
        }

        protected override void AddOutputs()
        {
            OutPoints(
                "Point",
                "The point the user clicked. Empty when the user pressed Escape.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            string prompt = null;
            if (da.GetData(0, ref prompt) &&
                !string.IsNullOrEmpty(prompt))
            {
                parameters["prompt"] = prompt;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var point = JsonOutputHelp.Point(response["position"]);

            da.SetDataList(
                0,
                point == null
                    ? new List<object>()
                    : new List<object> { point });
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetPointFromUser;

        public override Guid ComponentGuid =>
            new Guid("f986a134-b3b8-a932-6abf-e7689ee87129");
    }
}
