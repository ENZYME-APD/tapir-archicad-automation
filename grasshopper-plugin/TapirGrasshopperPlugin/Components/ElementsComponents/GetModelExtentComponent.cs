using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetModelExtentComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetModelExtent";

        public GetModelExtentComponent()
            : base(
                "GetModelExtent",
                "Gets the 3D extent of the current database: the region Archicad's Fit in Window would show, " +
                "its vertical range included.",
                GroupNames.ElementDetails)
        {
        }

        protected override void AddOutputs()
        {
            outManager.AddBoxParameter(
                "BoundingBox",
                "BoundingBox",
                "The extent of the current database as a box (xMin, yMin, zMin to xMax, yMax, zMax)."
                    .WithTypeName("boxItem"),
                GH_ParamAccess.item);
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

            var min = new Point3d(
                response.Value<double?>("xMin") ?? 0.0,
                response.Value<double?>("yMin") ?? 0.0,
                response.Value<double?>("zMin") ?? 0.0);
            var max = new Point3d(
                response.Value<double?>("xMax") ?? 0.0,
                response.Value<double?>("yMax") ?? 0.0,
                response.Value<double?>("zMax") ?? 0.0);

            da.SetData(
                0,
                new Box(new BoundingBox(min, max)));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetModelExtent;

        public override Guid ComponentGuid =>
            new Guid("799b4324-ed78-4a0a-b61e-ed13636b6a94");
    }
}
