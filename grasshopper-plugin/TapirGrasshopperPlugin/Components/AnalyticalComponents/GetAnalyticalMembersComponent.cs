using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class GetAnalyticalMembersComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAnalyticalMembers";

        public GetAnalyticalMembersComponent()
            : base(
                "GetAnalyticalMembers",
                "Lists the elements of the curve and surface members of the given Structural Analytical Model, " +
                "or of the model of the current view when no model is given (the model is required before Archicad 27).",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "AnalyticalModelGuid",
                "Identifier of the Structural Analytical Model. Optional; the model of the current view is used " +
                "when it is omitted. Required before Archicad 27.");

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "CurveMemberElementGuids",
                "The elements of the curve members (beams, columns) of the model.");

            OutGenerics(
                "SurfaceMemberElementGuids",
                "The elements of the surface members (walls, slabs, roofs) of the model.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();
            if (!AnalyticalJsonHelp.TryAddModelId(this, da, 0, parameters))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetDataList(
                0,
                AnalyticalJsonHelp.ElementsOf(response["curveMembers"]));
            da.SetDataList(
                1,
                AnalyticalJsonHelp.ElementsOf(response["surfaceMembers"]));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAnalyticalMemberTypesOfElements;

        public override Guid ComponentGuid =>
            new Guid("21650f04-1c25-4d2a-a270-7131d0bb3cd3");
    }
}
