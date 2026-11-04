using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class ScreenToModelComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "ScreenToModel";

        public ScreenToModelComponent()
            : base(
                "ScreenToModel",
                "Converts pixel positions local to the current Archicad view window to 2D model coordinates " +
                "of the current database, using the current window's size, drawing scale and zoom. It is a " +
                "2D mapping: it does not reconstruct a 3D point, and several model coordinates can map to " +
                "the same pixel.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InIntegers(
                "H",
                "Horizontal pixel position of each point from the left edge of the view window " +
                "(-32768 to 32767).");

            InIntegers(
                "V",
                "Vertical pixel position of each point from the top edge of the view window " +
                "(-32768 to 32767), parallel to H.");
        }

        protected override void AddOutputs()
        {
            OutPoints(
                "Coordinates",
                "The 2D model coordinate of each screen point in the current database (meters, z is 0; " +
                "null for failed items).");

            OutErrorMessages(
                "Error message of each screen point (empty when it was converted).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<int> hs) ||
                !da.TryGetList(
                    1,
                    out List<int> vs))
            {
                return;
            }

            if (hs.Count != vs.Count)
            {
                this.AddError(
                    "The size of the inputs H and V must be equal.");
                return;
            }

            var screenPoints = new JArray();
            for (var i = 0; i < hs.Count; i++)
            {
                screenPoints.Add(
                    new JObject
                    {
                        ["h"] = hs[i],
                        ["v"] = vs[i]
                    });
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["screenPoints"] = screenPoints },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var coordinates = new List<object>();
            var errorMessages = new List<string>();

            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "coordinates"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    coordinates.Add(null);
                    errorMessages.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                coordinates.Add(JsonOutputHelp.Point(item));
                errorMessages.Add("");
            }

            da.SetDataList(
                0,
                coordinates);

            da.SetDataList(
                1,
                errorMessages);
        }

        // Shares the icon of GetPointFromUser until it gets its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetPointFromUser;

        public override Guid ComponentGuid =>
            new Guid("bfb50da1-4a08-4e01-8c54-3514ed0bc0c1");
    }
}
