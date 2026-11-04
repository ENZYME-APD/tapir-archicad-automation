using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetCoordinateOriginsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetCoordinateOrigins";

        public GetCoordinateOriginsComponent()
            : base(
                "GetCoordinateOrigins",
                "Gets the offset of the virtual coordinate system origin from the world origin, and the " +
                "user origin of the current database. Use it to tell which origin the coordinates of " +
                "different sources (elements, views, survey point) are relative to.",
                GroupNames.Project)
        {
        }

        protected override void AddOutputs()
        {
            outManager.AddPointParameter(
                "VirtualOriginOffset",
                "VirtualOriginOffset",
                ("The offset of the origin of the virtual coordinate system from the world origin (z is 0). " +
                 "Archicad sets it after a project is opened when the model is far from the world origin. " +
                 "Null when Archicad does not return it for the current window.")
                    .WithTypeName("pointItem"),
                GH_ParamAccess.item);

            outManager.AddPointParameter(
                "UserOrigin",
                "UserOrigin",
                ("The user origin of the current database, relative to the origin of the virtual coordinate " +
                 "system. Null when the current window has none (only floor plan, section, detail and 3D " +
                 "windows have one).")
                    .WithTypeName("pointItem"),
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

            da.SetData(
                0,
                JsonOutputHelp.Point(response["virtualOriginOffset"]));

            da.SetData(
                1,
                JsonOutputHelp.Point(response["userOrigin"]));
        }

        // Shares the icon of GetSurveyPointTransformation until it gets its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetSurveyPointTransformation;

        public override Guid ComponentGuid =>
            new Guid("b7376269-9143-4c6f-8177-d031e6580a15");
    }
}
