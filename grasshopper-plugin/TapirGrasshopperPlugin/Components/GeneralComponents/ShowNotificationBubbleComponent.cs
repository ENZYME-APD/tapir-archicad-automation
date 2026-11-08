using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class ShowNotificationBubbleComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ShowNotificationBubble";

        public ShowNotificationBubbleComponent()
            : base(
                "ShowNotificationBubble",
                "Shows a non-blocking notification bubble with a message in the current Archicad window, e.g. to " +
                "report progress or a result without stopping the user as ShowAlert does. The bubble disappears " +
                "by itself after the timeout. It is shown at Coordinate or at the pixel position ScreenH/ScreenV " +
                "of the current view, by default at the center of the current 2D view. Requires Archicad 29 or later.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Message",
                "The text of the notification bubble.");

            InText(
                "Icon",
                "The icon shown in the bubble: none, information, warning or error. Optional; default: information.");

            AddOptionalInteger(
                "TimeoutMilliseconds",
                "How long the bubble stays, in milliseconds. Optional; default: Archicad's default, 1000 (1 second).");

            InPoint(
                "Coordinate",
                "The model coordinate in the current database (meters) where the bubble is shown, converted like " +
                "ModelToScreen does: in a 3D window it is projected with the current camera, in other windows z is " +
                "disregarded. Do not give it together with ScreenH/ScreenV. Optional.");

            AddOptionalInteger(
                "ScreenH",
                "Horizontal pixel position from the left edge of the current view window (not the desktop) where " +
                "the bubble is shown, e.g. an H output of ModelToScreen. Give it with ScreenV, not with Coordinate. " +
                "Optional.");

            AddOptionalInteger(
                "ScreenV",
                "Vertical pixel position from the top edge of the current view window where the bubble is shown. " +
                "Give it with ScreenH. Optional.");

            AddOptionalInteger(
                "OffsetH",
                "Moves the bubble from its position by this many pixels, positive to the right. Optional.");

            AddOptionalInteger(
                "OffsetV",
                "Moves the bubble from its position by this many pixels, positive downwards. Optional.");

            SetOptionality(new[] { 1, 2, 3, 4, 5, 6, 7 });
        }

        private void AddOptionalInteger(
            string name,
            string description)
        {
            inManager.AddIntegerParameter(
                name,
                name,
                description.WithTypeName("integerItem"),
                GH_ParamAccess.item);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            string message = null;
            if (!da.GetData(0, ref message) || string.IsNullOrEmpty(message))
            {
                this.AddError("Message must not be empty.");
                return;
            }

            var parameters = new JObject { ["message"] = message };

            string icon = null;
            if (da.GetData(1, ref icon) && !string.IsNullOrEmpty(icon))
            {
                parameters["icon"] = icon;
            }

            int timeout = 0;
            if (da.GetData(2, ref timeout))
            {
                parameters["timeoutMilliseconds"] = timeout;
            }

            var coordinate = Point3d.Unset;
            var hasCoordinate = da.GetData(3, ref coordinate);

            int screenH = 0;
            int screenV = 0;
            var hasScreenH = da.GetData(4, ref screenH);
            var hasScreenV = da.GetData(5, ref screenV);

            if (hasScreenH != hasScreenV)
            {
                this.AddError("Give both ScreenH and ScreenV, or neither.");
                return;
            }

            if (hasCoordinate && hasScreenH)
            {
                this.AddError("Give either Coordinate or ScreenH/ScreenV, not both.");
                return;
            }

            if (hasCoordinate)
            {
                parameters["coordinate"] = new JObject
                {
                    ["x"] = coordinate.X,
                    ["y"] = coordinate.Y,
                    ["z"] = coordinate.Z
                };
            }

            if (hasScreenH)
            {
                parameters["screenPoint"] = new JObject
                {
                    ["h"] = screenH,
                    ["v"] = screenV
                };
            }

            int offsetH = 0;
            int offsetV = 0;
            var hasOffsetH = da.GetData(6, ref offsetH);
            var hasOffsetV = da.GetData(7, ref offsetV);
            if (hasOffsetH || hasOffsetV)
            {
                parameters["offset"] = new JObject
                {
                    ["h"] = offsetH,
                    ["v"] = offsetV
                };
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            if ((bool?)response?["success"] == false)
            {
                this.AddError(
                    response["error"]?["message"]?.ToString() ??
                    "Failed to show the notification bubble.");
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ShowNotificationBubble;

        public override Guid ComponentGuid =>
            new Guid("4fe4a071-00bf-43f4-8540-0667e45d1d57");
    }
}
