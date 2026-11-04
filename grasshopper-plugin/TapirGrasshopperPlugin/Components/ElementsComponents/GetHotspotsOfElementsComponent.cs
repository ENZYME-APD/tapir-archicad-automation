using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetHotspotsOfElementsComponent
        : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetHotspotsOfElements";

        public GetHotspotsOfElementsComponent()
            : base(
                "GetHotspotsOfElements",
                "Gets the 2D hotspots of the given library part instances (Objects, Lamps, Windows, Doors, " +
                "Skylights, Zones, Curtain Wall parts, etc.), e.g. to dimension to them. The outputs are " +
                "aligned with the input list of elements, one branch per element.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "hotspotsOfElements";

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the library part instances.");
        }

        protected override void AddOutputs()
        {
            OutPointTree(
                "Hotspots",
                "The 2D hotspots of each element on the XY plane (one branch per element).");

            OutErrorMessages(
                "Error message of each element (empty when successful).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var hotspots = new DataTree<object>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                hotspots.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");
                if (!(item["hotspots"] is JArray points))
                {
                    continue;
                }

                foreach (var point in points)
                {
                    hotspots.Add(
                        ElementGeometryJsonHelp.Point2D(point),
                        path);
                }
            }

            da.SetDataTree(
                0,
                hotspots);
            da.SetDataList(
                1,
                errors);
        }

        // Shares the icon of SnapPoints until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SnapPt;

        public override Guid ComponentGuid =>
            new Guid("7a934f89-729b-4c7f-83da-7d5950a3216a");
    }
}
