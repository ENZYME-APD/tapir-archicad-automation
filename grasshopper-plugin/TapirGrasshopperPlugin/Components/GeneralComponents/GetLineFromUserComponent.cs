using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class GetLineFromUserComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLineFromUser";

        public GetLineFromUserComponent()
            : base(
                "GetLineFromUser",
                "Waits for the user to input a line in the active Archicad window and returns start and end coordinates. " +
                "Archicad waits for the input or for Escape, and every other command queues behind this one until then.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "StartPrompt",
                "Prompt for selecting the line start point. Optional.");

            InText(
                "EndPrompt",
                "Prompt for selecting the line end point. Optional.");

            inManager.AddBooleanParameter(
                "ShowRubberLineFeedback",
                "ShowRubberLineFeedback",
                "If true, draws a rubber feedback around the cursor while the input is active. Optional; Archicad's default is true."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            SetOptionality(new[] { 0, 1, 2 });
        }

        protected override void AddOutputs()
        {
            OutCurves(
                "Line",
                "The line the user input (empty when cancelled).");

            OutPoints(
                "StartPoint",
                "The start point of the line.");

            OutPoints(
                "EndPoint",
                "The end point of the line.");

            OutBoolean(
                "Cancelled",
                "True when the user cancelled the input with Escape.");
        }

        // The line input is modal in Archicad and blocks every other command,
        // so it is only asked for on the Refresh button, never on placement or
        // on an upstream change.
        protected override void SolveInstance(
            IGH_DataAccess da)
        {
            if (!ManualRefreshRequested)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Remark,
                    "Press Refresh to ask for a line in Archicad");
                return;
            }

            Solve(da);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            if (da.TryGet(
                    0,
                    out string startPrompt) &&
                !string.IsNullOrEmpty(startPrompt))
            {
                parameters["startPrompt"] = startPrompt;
            }

            if (da.TryGet(
                    1,
                    out string endPrompt) &&
                !string.IsNullOrEmpty(endPrompt))
            {
                parameters["endPrompt"] = endPrompt;
            }

            if (da.TryGet(
                    2,
                    out bool showRubberLineFeedback))
            {
                parameters["showRubberLineFeedback"] = showRubberLineFeedback;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var cancelled = (bool?)response["cancelled"] == true;
            da.SetData(
                3,
                cancelled);

            if (cancelled)
            {
                return;
            }

            var startPoint = JsonOutputHelp.Point(response["startPoint"]);
            var endPoint = JsonOutputHelp.Point(response["endPoint"]);

            da.SetDataList(
                0,
                startPoint is Point3d start && endPoint is Point3d end
                    ? new List<Curve> { new LineCurve(start, end) }
                    : new List<Curve>());

            da.SetDataList(
                1,
                startPoint == null
                    ? new List<object>()
                    : new List<object> { startPoint });

            da.SetDataList(
                2,
                endPoint == null
                    ? new List<object>()
                    : new List<object> { endPoint });
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetLineFromUser;

        public override Guid ComponentGuid =>
            new Guid("9a7facc6-9828-48bc-ba23-43893e48cde3");
    }
}
