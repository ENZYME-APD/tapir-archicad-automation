using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class ModelToScreenComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "ModelToScreen";

        public ModelToScreenComponent()
            : base(
                "ModelToScreen",
                "Converts model coordinates to pixel positions local to the current Archicad view window " +
                "(not global desktop coordinates), using the current window's size, drawing scale and zoom. " +
                "In a 3D window the coordinates are projected with the current camera; in other windows z is " +
                "disregarded. Available from Archicad 29.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InPoints(
                "Coordinates",
                "Model coordinates in the current database (meters).");
        }

        protected override void AddOutputs()
        {
            OutIntegers(
                "H",
                "Horizontal pixel position of each coordinate from the left edge of the view window " +
                "(null for failed items).");

            OutIntegers(
                "V",
                "Vertical pixel position of each coordinate from the top edge of the view window " +
                "(null for failed items).");

            OutErrorMessages(
                "Error message of each coordinate (empty when it was converted).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<Point3d> points))
            {
                return;
            }

            var coordinates = new JArray();
            foreach (var point in points)
            {
                coordinates.Add(
                    new JObject
                    {
                        ["x"] = point.X,
                        ["y"] = point.Y,
                        ["z"] = point.Z
                    });
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["coordinates"] = coordinates },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var hs = new List<object>();
            var vs = new List<object>();
            var errorMessages = new List<string>();

            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "screenPoints"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    hs.Add(null);
                    vs.Add(null);
                    errorMessages.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                hs.Add(item.Value<int?>("h"));
                vs.Add(item.Value<int?>("v"));
                errorMessages.Add("");
            }

            da.SetDataList(
                0,
                hs);

            da.SetDataList(
                1,
                vs);

            da.SetDataList(
                2,
                errorMessages);
        }

        // Shares the icon of GetPointFromUser until it gets its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetPointFromUser;

        public override Guid ComponentGuid =>
            new Guid("19bfd49d-7855-4cdf-a25b-eaafab97008a");
    }
}
