using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetElementConnectionsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetElementConnections";

        public GetElementConnectionsComponent()
            : base(
                "GetElementConnections",
                "Gets the connections among the given elements: each pair of touching elements once, with the " +
                "polygons of the surface where they touch, in world coordinates. The 3D model of the " +
                "elements is generated with separate components in a temporary sight, so the 3D window is " +
                "not changed. Also returns, aligned with the input list of elements, whether each element " +
                "was found. Available from Archicad 28.",
                GroupNames.ElementDetails)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements whose connections among each other are looked up.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids1",
                "The first element of each connection.");

            OutGenerics(
                "ElementGuids2",
                "The second element of each connection.");

            OutCurveTree(
                "Polygons",
                "The polygons of the surface where the two elements touch, as closed polylines in world " +
                "coordinates (one branch per connection).");

            OutErrorMessages(
                "Error message of each input element (empty when it was found).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(elements),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var elements1 = new List<object>();
            var elements2 = new List<object>();
            var polygons = new DataTree<object>();

            var connections = JsonOutputHelp.Items(
                response,
                "connections");
            for (var i = 0; i < connections.Count; i++)
            {
                var connection = connections[i];
                var path = new GH_Path(i);
                polygons.EnsurePath(path);

                elements1.Add(
                    ElementsStructuredGetterComponent.ElementIdOf(
                        connection,
                        "elementId1"));
                elements2.Add(
                    ElementsStructuredGetterComponent.ElementIdOf(
                        connection,
                        "elementId2"));

                if (!(connection["polygons"] is JArray connectionPolygons))
                {
                    continue;
                }

                foreach (var polygon in connectionPolygons)
                {
                    polygons.Add(
                        ElementGeometryJsonHelp.Polyline3D(
                            polygon["coordinates"],
                            true),
                        path);
                }
            }

            da.SetDataList(
                0,
                elements1);
            da.SetDataList(
                1,
                elements2);
            da.SetDataTree(
                2,
                polygons);
            da.SetDataList(
                3,
                ExecutionResultsResponse.ErrorMessages(response));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetElementConnections;

        public override Guid ComponentGuid =>
            new Guid("4cf00497-c671-49cf-a1d7-87a7381f4ace");
    }
}
