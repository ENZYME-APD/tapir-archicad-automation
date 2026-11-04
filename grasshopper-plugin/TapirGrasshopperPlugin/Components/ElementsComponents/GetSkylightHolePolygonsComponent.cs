using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetSkylightHolePolygonsComponent
        : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetSkylightHolePolygons";

        public GetSkylightHolePolygonsComponent()
            : base(
                "GetSkylightHolePolygons",
                "Gets the polygon of the hole each given Skylight cuts into its host Roof or Shell: the " +
                "outline and any holes of it as 2D coordinates. The outputs are aligned with the input list " +
                "of elements; an element that is not a Skylight gets an error.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "skylightHolePolygons";

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the Skylight elements.");
        }

        protected override void AddOutputs()
        {
            OutCurves(
                "Outlines",
                "The outline of the hole of each skylight as a closed polyline on the XY plane (null for an " +
                "error). A curved edge is given by its end points only.");

            OutCurveTree(
                "Holes",
                "The holes of each outline as closed curves on the XY plane (one branch per skylight).");

            OutErrorMessages(
                "Error message of each element (empty when successful), e.g. when it is not a skylight.");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var outlines = new List<object>();
            var holes = new DataTree<object>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                holes.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    outlines.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                outlines.Add(
                    ElementGeometryJsonHelp.Polyline2D(
                        item["polygonOutline"],
                        true));
                errors.Add("");

                if (item["holes"] is JArray itemHoles)
                {
                    foreach (var hole in itemHoles)
                    {
                        holes.Add(
                            PolygonHelp.ToCurve(
                                hole["polygonOutline"] ?? hole["polygonCoordinates"],
                                hole["polygonArcs"],
                                true),
                            path);
                    }
                }
            }

            da.SetDataList(
                0,
                outlines);
            da.SetDataTree(
                1,
                holes);
            da.SetDataList(
                2,
                errors);
        }

        // Shares the icon of GetCutPolygonsOfElements until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetCutPolygonsOfElements;

        public override Guid ComponentGuid =>
            new Guid("7104029c-266d-4990-becd-dfac714c79db");
    }
}
