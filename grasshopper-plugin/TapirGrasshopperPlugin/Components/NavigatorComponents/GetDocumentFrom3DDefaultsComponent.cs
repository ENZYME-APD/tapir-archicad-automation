using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetDocumentFrom3DDefaultsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetDocumentFrom3DDefaults";

        public GetDocumentFrom3DDefaultsComponent()
            : base(
                "GetDocumentFrom3DDefaults",
                "Gets the defaults a newly created 3D Document starts with: name, reference ID, vectorial hatching " +
                "and sun shadows, transparency, connection to the model, surfaces, boundary contours and the uniform " +
                "cut, uncut, shadow and hidden edge attributes.",
                GroupNames.Navigator)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "Name",
                "The name of a new 3D Document.");

            OutText(
                "RefId",
                "The reference ID of a new 3D Document.");

            OutBoolean(
                "Vect3DHatching",
                "Vectorial 3D hatching.");

            OutBoolean(
                "VectSunShadow",
                "Vectorial sun shadows.");

            OutBoolean(
                "MustBeTransparent",
                "Transparency of the model.");

            OutText(
                "IntPlanConn",
                "The connection of the 3D Document to the model: Online, Offline or Drawing.");

            OutBoolean(
                "MaterialFrom3D",
                "Use the surfaces of the 3D model; when false, CMater is used for every element.");

            OutBoolean(
                "ContureOff",
                "Contours off.");

            OutInteger(
                "CMater",
                "The index of the uniform surface (material), used when MaterialFrom3D is false.");

            OutText(
                "BoundaryDisplay",
                "How the boundary contours are displayed: UncutContours, NoContours or OverrideContours.");

            OutInteger(
                "BoundaryPen",
                "The pen of the boundary contours, used with OverrideContours.");

            OutInteger(
                "BoundaryLineType",
                "The index of the line type of the boundary contours, used with OverrideContours.");

            OutText(
                "AttributeOptions_JSON",
                "The uniform attribute options (cut and uncut surfaces, shading, hidden edges) as a JSON object: " +
                "shouldUseUniformCutAttributes, uniformLineType, uniformFillPen, uniformFillBkgPen, " +
                "shouldUseUncutSurfaceFill, uncutSurfaceBkgPen, shouldUseElemPen, elemPen, shadFillPen, shadFill, " +
                "shadFillBkgPen, shadContPen, hiddenEdgeLineType and hiddenEdgeLinePen.");
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

            da.SetData(0, response["name"]?.ToString());
            da.SetData(1, response["refId"]?.ToString());
            da.SetData(2, response.Value<bool?>("vect3DHatching"));
            da.SetData(3, response.Value<bool?>("vectSunShadow"));
            da.SetData(4, response.Value<bool?>("mustBeTransparent"));
            da.SetData(5, response["intPlanConn"]?.ToString());
            da.SetData(6, response.Value<bool?>("materialFrom3D"));
            da.SetData(7, response.Value<bool?>("contureOff"));
            da.SetData(8, response.Value<int?>("cMater"));
            da.SetData(9, response["boundaryDisplay"]?.ToString());
            da.SetData(10, response.Value<int?>("boundaryPen"));
            da.SetData(11, response.Value<int?>("boundaryLineType"));
            da.SetData(12, NavigatorComponentHelp.JsonText(response["attributeOptions"]));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetDocumentFrom3DDefaults;

        public override Guid ComponentGuid =>
            new Guid("34a50688-34a5-4934-98f6-c261ab93fe4e");
    }
}
