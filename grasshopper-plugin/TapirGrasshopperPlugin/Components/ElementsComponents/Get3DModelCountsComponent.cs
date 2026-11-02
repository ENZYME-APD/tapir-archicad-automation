using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class Get3DModelCountsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "Get3DModelCounts";

        public Get3DModelCountsComponent()
            : base(
                "Get3DModelCounts",
                "Gets the number of components of each type (bodies, polygons, polygon edges, edges, vertices, " +
                "normal vectors, materials and lights) in the 3D model Archicad currently holds, as " +
                "ACAPI_ModelAccess_GetNum counts them. A type that cannot be counted gets an error item. " +
                "Available from Archicad 27.",
                GroupNames.ElementDetails)
        {
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "ComponentTypes",
                "The type of each counted component: Body, Pgon (polygon), Pedg (polygon edge), Edge, " +
                "Vert (vertex), Vect (normal vector), Umat (material) or Lght (light). Null for an error.");

            OutIntegers(
                "Counts",
                "The number of components of each type (null for an error).");

            OutErrorMessages(
                "Error message of each type (empty when successful).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var types = new List<object>();
            var counts = new List<object>();
            var errors = new List<string>();
            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "counts"))
            {
                types.Add(
                    JsonOutputHelp.Scalar(
                        item,
                        "componentType"));
                counts.Add(
                    JsonOutputHelp.Scalar(
                        item,
                        "count"));
                errors.Add(JsonOutputHelp.ErrorMessage(item));
            }

            da.SetDataList(
                0,
                types);
            da.SetDataList(
                1,
                counts);
            da.SetDataList(
                2,
                errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Get3DModelCounts;

        public override Guid ComponentGuid =>
            new Guid("4d869c7d-b2b0-44e2-ba63-7f7b6275d6ea");
    }
}
