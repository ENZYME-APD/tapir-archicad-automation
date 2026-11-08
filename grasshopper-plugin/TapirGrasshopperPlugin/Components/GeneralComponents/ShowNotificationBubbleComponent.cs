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

            InText(
                "LargeIcon",
                "The icon shown instead of Icon when the \"Use large alert cues\" option is on in Work " +
                "Environment: information, warning or error. Optional; not set by default.");

            AddOptionalInteger(
                "RedrawCountLimit",
                "Shows the bubble at most this many times (at least 1). After that it does not appear again, " +
                "for any later ShowNotificationBubble call, until the user runs the 'Enable all informational " +
                "messages' command in Archicad. Optional; not set by default.");

            inManager.AddBooleanParameter(
                "Disabled",
                "Disabled",
                ("True turns the bubble off instead of showing it: it does not appear again, for any later " +
                 "ShowNotificationBubble call, until the user runs the 'Enable all informational messages' " +
                 "command in Archicad. Optional; False (the default) changes nothing.").WithTypeName(
                    "booleanItem"),
                GH_ParamAccess.item);

            SetOptionality(new[] { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 });
        }

        protected override void AddOutputs()
        {
            OutText(
                "Text",
                "The text of the bubble after the call, read back from Archicad (null when it has none).");

            OutText(
                "Icon",
                "The icon of the bubble after the call: none, information, warning, error or other.");

            OutText(
                "LargeIcon",
                "The icon shown when the \"Use large alert cues\" option is on: none, information, warning, " +
                "error or other.");

            OutInteger(
                "OffsetH",
                "The horizontal offset of the bubble from its position, in pixels.");

            OutInteger(
                "OffsetV",
                "The vertical offset of the bubble from its position, in pixels.");

            OutInteger(
                "TimeoutMilliseconds",
                "How long the bubble stays, in milliseconds.");

            OutInteger(
                "RedrawCountLimit",
                "How many times the bubble is shown at most (null when it has no limit).");
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

            string largeIcon = null;
            if (da.GetData(8, ref largeIcon) && !string.IsNullOrEmpty(largeIcon))
            {
                parameters["largeIcon"] = largeIcon;
            }

            int redrawCountLimit = 0;
            if (da.GetData(9, ref redrawCountLimit))
            {
                parameters["redrawCountLimit"] = redrawCountLimit;
            }

            bool disabled = false;
            if (da.GetData(10, ref disabled))
            {
                parameters["disabled"] = disabled;
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
                return;
            }

            if (response == null)
            {
                return;
            }

            da.SetData(0, response["text"]?.ToString());
            da.SetData(1, response["icon"]?.ToString());
            da.SetData(2, response["largeIcon"]?.ToString());
            if (response["offset"] is JObject responseOffset)
            {
                da.SetData(3, (int?)responseOffset["h"]);
                da.SetData(4, (int?)responseOffset["v"]);
            }
            da.SetData(5, ToInteger(response["timeoutMilliseconds"]));
            da.SetData(6, ToInteger(response["redrawCountLimit"]));
        }

        // Grasshopper integers are 32-bit; the add-on returns values up to 4294967295.
        private static int? ToInteger(
            JToken token)
        {
            var value = (long?)token;
            if (value == null)
            {
                return null;
            }

            return (int)Math.Max(int.MinValue, Math.Min(int.MaxValue, value.Value));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ShowNotificationBubble;

        public override Guid ComponentGuid =>
            new Guid("4fe4a071-00bf-43f4-8540-0667e45d1d57");
    }
}
