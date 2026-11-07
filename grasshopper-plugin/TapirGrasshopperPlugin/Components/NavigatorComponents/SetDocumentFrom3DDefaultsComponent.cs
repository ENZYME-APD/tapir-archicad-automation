using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class SetDocumentFrom3DDefaultsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetDocumentFrom3DDefaults";

        private static readonly string[] AttributeOptionKeys =
        {
            "shouldUseUniformCutAttributes",
            "uniformLineType",
            "uniformFillPen",
            "uniformFillBkgPen",
            "shouldUseUncutSurfaceFill",
            "uncutSurfaceBkgPen",
            "shouldUseElemPen",
            "elemPen",
            "shadFillPen",
            "shadFill",
            "shadFillBkgPen",
            "shadContPen",
            "hiddenEdgeLineType",
            "hiddenEdgeLinePen"
        };

        public SetDocumentFrom3DDefaultsComponent()
            : base(
                "SetDocumentFrom3DDefaults",
                "Sets the defaults a newly created 3D Document starts with. Only the given fields are changed; " +
                "existing 3D Documents are not affected. The fields are the ones returned by " +
                "GetDocumentFrom3DDefaults.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            NavigatorComponentHelp.AddOptionalText(
                this,
                "Name",
                "The name of a new 3D Document. Optional.");

            NavigatorComponentHelp.AddOptionalText(
                this,
                "RefId",
                "The reference ID of a new 3D Document. Optional.");

            NavigatorComponentHelp.AddOptionalBoolean(
                this,
                "Vect3DHatching",
                "Vectorial 3D hatching. Optional.");

            NavigatorComponentHelp.AddOptionalBoolean(
                this,
                "VectSunShadow",
                "Vectorial sun shadows. Optional.");

            NavigatorComponentHelp.AddOptionalBoolean(
                this,
                "MustBeTransparent",
                "Transparency of the model. Optional.");

            NavigatorComponentHelp.AddOptionalText(
                this,
                "IntPlanConn",
                "The connection of the 3D Document to the model: Online, Offline or Drawing. Optional.");

            NavigatorComponentHelp.AddOptionalBoolean(
                this,
                "MaterialFrom3D",
                "Use the surfaces of the 3D model; when false, CMater is used for every element. Optional.");

            NavigatorComponentHelp.AddOptionalBoolean(
                this,
                "ContureOff",
                "Contours off. Optional.");

            NavigatorComponentHelp.AddOptionalInteger(
                this,
                "CMater",
                "The index of the uniform surface (material), used when MaterialFrom3D is false. Optional.");

            NavigatorComponentHelp.AddOptionalText(
                this,
                "BoundaryDisplay",
                "How the boundary contours are displayed: UncutContours, NoContours or OverrideContours. Optional.");

            NavigatorComponentHelp.AddOptionalInteger(
                this,
                "BoundaryPen",
                "The pen of the boundary contours, used with OverrideContours. Optional.");

            NavigatorComponentHelp.AddOptionalInteger(
                this,
                "BoundaryLineType",
                "The index of the line type of the boundary contours, used with OverrideContours. Optional.");

            NavigatorComponentHelp.AddOptionalText(
                this,
                "AttributeOptions_JSON",
                "The uniform attribute options (cut and uncut surfaces, shading, hidden edges) as a JSON object: " +
                "shouldUseUniformCutAttributes, uniformLineType, uniformFillPen, uniformFillBkgPen, " +
                "shouldUseUncutSurfaceFill, uncutSurfaceBkgPen, shouldUseElemPen, elemPen, shadFillPen, shadFill, " +
                "shadFillBkgPen, shadContPen, hiddenEdgeLineType and hiddenEdgeLinePen. Only the given keys are " +
                "changed. Optional.");
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
            var parameters = new JObject();

            NavigatorComponentHelp.CopyOptional<string>(da, 0, parameters, "name");
            NavigatorComponentHelp.CopyOptional<string>(da, 1, parameters, "refId");
            NavigatorComponentHelp.CopyOptional<bool>(da, 2, parameters, "vect3DHatching");
            NavigatorComponentHelp.CopyOptional<bool>(da, 3, parameters, "vectSunShadow");
            NavigatorComponentHelp.CopyOptional<bool>(da, 4, parameters, "mustBeTransparent");
            NavigatorComponentHelp.CopyOptional<string>(da, 5, parameters, "intPlanConn");
            NavigatorComponentHelp.CopyOptional<bool>(da, 6, parameters, "materialFrom3D");
            NavigatorComponentHelp.CopyOptional<bool>(da, 7, parameters, "contureOff");
            NavigatorComponentHelp.CopyOptional<int>(da, 8, parameters, "cMater");
            NavigatorComponentHelp.CopyOptional<string>(da, 9, parameters, "boundaryDisplay");
            NavigatorComponentHelp.CopyOptional<int>(da, 10, parameters, "boundaryPen");
            NavigatorComponentHelp.CopyOptional<int>(da, 11, parameters, "boundaryLineType");

            var attributeOptionsText = "";
            if (da.GetData(
                    12,
                    ref attributeOptionsText) &&
                !string.IsNullOrWhiteSpace(attributeOptionsText))
            {
                if (!NavigatorComponentHelp.TryParseObject(
                        this,
                        "AttributeOptions_JSON",
                        attributeOptionsText,
                        AttributeOptionKeys,
                        out JObject attributeOptions))
                {
                    return;
                }

                parameters["attributeOptions"] = attributeOptions;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    parameters,
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
            Properties.Resources.SetDocumentFrom3DDefaults;

        public override Guid ComponentGuid =>
            new Guid("fef170c8-4a4d-4f08-9175-8b34aa56ba7c");
    }
}
