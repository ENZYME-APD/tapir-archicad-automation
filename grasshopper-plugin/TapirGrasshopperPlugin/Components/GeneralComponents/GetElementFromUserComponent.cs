using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class GetElementFromUserComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetElementFromUser";

        public GetElementFromUserComponent()
            : base(
                "GetElementFromUser",
                "Asks the designer to click an element in the current window and returns its id, its type and the clicked point. " +
                "A click on empty space or on an element of a type not asked for asks again. Archicad waits for the click or " +
                "for Escape, and every other command queues behind this one until then; cancelling returns Cancelled.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Prompt",
                "Shown in the control box while Archicad waits for the click. Single-byte text. Optional.");

            InTexts(
                "ElementTypes",
                "Only an element of one of these types is accepted (e.g. Wall, Slab). Optional; default: any element.");

            SetOptionality(new[] { 0, 1 });
        }

        protected override void AddOutputs()
        {
            OutGeneric(
                "ElementGuid",
                "The clicked element. In a section or elevation window it is the model element the section element was cut from.");

            OutText(
                "ElementType",
                "The type of the clicked element.");

            OutPoints(
                "Position",
                "The clicked point in the project's coordinates.");

            OutPoints(
                "ElementPosition",
                "The coordinate of the clicked part of the element (e.g. its node or hotspot), when Archicad gives one.");

            OutBoolean(
                "Cancelled",
                "True when the input was cancelled with Escape.");
        }

        // The element input is modal in Archicad and blocks every other
        // command, so it is only asked for on the Refresh button, never on
        // placement or on an upstream change.
        protected override void SolveInstance(
            IGH_DataAccess da)
        {
            if (!ManualRefreshRequested)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Remark,
                    "Press Refresh to ask for an element in Archicad");
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
                    out string prompt) &&
                !string.IsNullOrEmpty(prompt))
            {
                parameters["prompt"] = prompt;
            }

            if (da.TryGetList(
                    1,
                    out List<string> elementTypes) &&
                elementTypes.Count > 0)
            {
                parameters["elementTypes"] = new JArray(elementTypes);
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
                4,
                cancelled);

            if (cancelled)
            {
                return;
            }

            var guid = JsonOutputHelp.GuidOf(response["elementId"]);
            if (guid != null)
            {
                da.SetData(
                    0,
                    new ElementGuidWrapper
                    {
                        ElementId = new ElementGuid { Guid = guid }
                    });
            }

            da.SetData(
                1,
                response["elementType"]?.ToString());

            var position = JsonOutputHelp.Point(response["position"]);
            da.SetDataList(
                2,
                position == null
                    ? new List<object>()
                    : new List<object> { position });

            var elementPosition = JsonOutputHelp.Point(response["elementPosition"]);
            da.SetDataList(
                3,
                elementPosition == null
                    ? new List<object>()
                    : new List<object> { elementPosition });
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetElementFromUser;

        public override Guid ComponentGuid =>
            new Guid("e796266b-6d13-4013-95d6-45159015ca11");
    }
}
