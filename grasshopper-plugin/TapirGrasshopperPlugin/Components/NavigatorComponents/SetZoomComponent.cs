using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class SetZoomComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetZoom";

        public SetZoomComponent()
            : base(
                "SetZoom",
                "Zooms the current 2D window to the given box in model coordinates.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            inManager.AddRectangleParameter(
                "ZoomBox",
                "ZoomBox",
                ("The area to zoom the current 2D window to, in model coordinates (meters); its bounding box " +
                 "on the XY plane is used. The window keeps its aspect ratio, so the shown area may be larger " +
                 "than the box.").WithTypeName("rectangleItem"),
                GH_ParamAccess.item);
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult.Message),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out Rectangle3d zoomRectangle))
            {
                return;
            }

            var box = zoomRectangle.BoundingBox;

            var zoomBox = new JObject
            {
                ["xMin"] = box.Min.X,
                ["yMin"] = box.Min.Y,
                ["xMax"] = box.Max.X,
                ["yMax"] = box.Max.Y
            };

            if (!TryGetConvertedCadValues(
                    CommandName,
                    new JObject { ["zoomBox"] = zoomBox },
                    ToAddOn,
                    ExecutionResult.Deserialize,
                    out ExecutionResult response))
            {
                return;
            }

            if (!response.Success)
            {
                this.AddError(response.Message());
            }

            da.SetData(
                0,
                response.Message());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetZoom;

        public override Guid ComponentGuid =>
            new Guid("3ea7dd0d-f692-40cf-89f1-29087272789c");
    }
}
