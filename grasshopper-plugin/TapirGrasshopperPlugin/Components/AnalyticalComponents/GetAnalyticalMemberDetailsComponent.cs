using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class GetAnalyticalMemberDetailsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAnalyticalMemberDetails";

        public GetAnalyticalMemberDetailsComponent()
            : base(
                "GetAnalyticalMemberDetails",
                "Retrieves the curve, surface or segmented surface member of the given elements in a " +
                "Structural Analytical Model, with their connections. Cross sections, curves, nodes and " +
                "surfaces are returned with every field the Archicad version has.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to get the analytical member of.");

            InGeneric(
                "AnalyticalModelGuid",
                "Identifier of the Structural Analytical Model. Optional; the model of the current view is used " +
                "when it is omitted. Required before Archicad 27.");

            inManager.AddNumberParameter(
                "SegmentationCentralAngle",
                "SegmentationCentralAngle",
                "When given, surface members are read as segmented surface members (for curved walls), each " +
                "segment spanning at most this central angle, in radians. Optional.",
                GH_ParamAccess.item);

            SetOptionality(new[] { 1, 2 });
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "MemberTypes",
                "Kind of the member of each element: CurveMember, SurfaceMember or SegmentedSurfaceMembers " +
                "(null when it could not be got).");

            OutTexts(
                "Members",
                "The member of each element as JSON, holding the key named by MemberTypes " +
                "(curveMember, surfaceMember or segmentedSurfaceMembers); it can be fed to " +
                "CreateElementsFromAnalyticalMembers as it is.");

            OutTexts(
                "Connections",
                "The connections of the member of each element as JSON.");

            OutTexts(
                "IdMappings",
                "The mapping of the parts of each surface member to the element as JSON " +
                "(null for curve members and in Archicad 25).");

            OutErrorMessages(
                "Error message of each element (empty when its member was got).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();
            if (!AnalyticalJsonHelp.TryAddElements(da, 0, parameters) ||
                !AnalyticalJsonHelp.TryAddModelId(this, da, 1, parameters))
            {
                return;
            }

            double centralAngle = 0.0;
            if (da.GetData(2, ref centralAngle))
            {
                parameters["segmentationCentralAngle"] = centralAngle;
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
                "analyticalMembers",
                out List<string> errors);

            var memberTypes = new List<string>();
            var members = new List<string>();
            foreach (var item in items)
            {
                string memberType = null;
                JObject member = null;
                if (item != null)
                {
                    foreach (var pair in AnalyticalJsonHelp.MemberKinds)
                    {
                        if (item[pair.Key] != null)
                        {
                            memberType = pair.Name;
                            member = new JObject { [pair.Key] = item[pair.Key] };
                            break;
                        }
                    }
                }
                memberTypes.Add(memberType);
                members.Add(AnalyticalJsonHelp.TextOf(member));
            }

            da.SetDataList(0, memberTypes);
            da.SetDataList(1, members);
            da.SetDataList(2, AnalyticalJsonHelp.TextsOf(items, "connections"));
            da.SetDataList(3, AnalyticalJsonHelp.TextsOf(items, "idMapping"));
            da.SetDataList(4, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAnalyticalMemberTypesOfElements;

        public override Guid ComponentGuid =>
            new Guid("6ccbafda-7e6d-412f-9ee9-f172de2760cb");
    }
}
