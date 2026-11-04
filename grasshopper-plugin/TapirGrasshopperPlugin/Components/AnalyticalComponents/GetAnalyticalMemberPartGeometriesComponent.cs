using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class GetAnalyticalMemberPartGeometriesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAnalyticalMemberPartGeometries";

        public GetAnalyticalMemberPartGeometriesComponent()
            : base(
                "GetAnalyticalMemberPartGeometries",
                "Retrieves the node of the given analytical member vertices and the curve of the given member " +
                "edges. Available from Archicad 26.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "MemberPartRefs",
                "The member parts to get the geometry of: vertices (BegVertex, EndVertex, CustomVertex) give a " +
                "node, edges (Edge, RatioEdge, CustomEdge) a curve. Each is " +
                AnalyticalJsonHelp.MemberPartRefDescription);

            InGeneric(
                "AnalyticalModelGuid",
                AnalyticalJsonHelp.ModelGuidDescription);

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutPoints(
                "NodePoints",
                "The point of the node of each vertex (null for edges and errors).");

            OutTexts(
                "Geometries",
                "The geometry of each member part as JSON: {\"node\": ...} for a vertex, {\"curve\": ...} for an " +
                "edge (null when it could not be got).");

            OutErrorMessages(
                "Error message of each member part (empty when its geometry was got).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(0, out List<string> texts) ||
                !AnalyticalJsonHelp.TryParseObjects(this, texts, "MemberPartRefs", out List<JObject> partRefs))
            {
                return;
            }

            var parameters = new JObject { ["memberPartRefs"] = new JArray(partRefs) };
            if (!AnalyticalJsonHelp.TryAddModelId(this, da, 1, parameters))
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

            var items = AnalyticalJsonHelp.SplitErrors(
                response,
                "geometries",
                out List<string> errors);

            var points = new List<object>();
            var geometries = new List<string>();
            foreach (var item in items)
            {
                points.Add(JsonOutputHelp.Point(item?["node"]?["coord"]));
                geometries.Add(AnalyticalJsonHelp.TextOf(item));
            }

            da.SetDataList(0, points);
            da.SetDataList(1, geometries);
            da.SetDataList(2, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAnalyticalModel;

        public override Guid ComponentGuid =>
            new Guid("4543a9e3-d156-4b39-b82a-357f257c4f02");
    }
}
