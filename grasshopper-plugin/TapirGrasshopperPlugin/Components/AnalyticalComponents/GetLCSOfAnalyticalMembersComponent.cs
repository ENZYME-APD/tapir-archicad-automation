using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class GetLCSOfAnalyticalMembersComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLCSOfAnalyticalMembers";

        public GetLCSOfAnalyticalMembersComponent()
            : base(
                "GetLCSOfAnalyticalMembers",
                "Retrieves the local coordinate system of the given analytical member parts, optionally in their " +
                "intersection with another member part. Available from Archicad 26.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "MemberPartRefs",
                "The member parts to get the local coordinate system of. Each is " +
                AnalyticalJsonHelp.MemberPartRefDescription);

            InTexts(
                "IntersectingMemberPartRefs",
                "For each member part, the member part (as JSON, like MemberPartRefs) to get its local coordinate " +
                "system in the intersection with; an empty text for none. Optional; 0 or one per member part.");

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutVectors(
                "XVectors",
                "The X axis of the local coordinate system of each member part (null when it could not be got).");

            OutVectors(
                "YVectors",
                "The Y axis of the local coordinate system of each member part (null when it could not be got).");

            OutErrorMessages(
                "Error message of each member part (empty when its coordinate system was got).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(0, out List<string> texts) ||
                !AnalyticalJsonHelp.TryParseObjects(this, texts, "MemberPartRefs", out List<JObject> partRefs))
            {
                return;
            }

            da.TryGetList(1, out List<string> intersectingTexts);
            intersectingTexts = intersectingTexts ?? new List<string>();
            if (intersectingTexts.Count > 0 &&
                intersectingTexts.Count != partRefs.Count)
            {
                this.AddError(
                    "The size of the input IntersectingMemberPartRefs must be 0 or equal to the size of the input MemberPartRefs.");
                return;
            }

            var memberParts = new JArray();
            for (var i = 0; i < partRefs.Count; i++)
            {
                var memberPart = new JObject { ["memberPartRef"] = partRefs[i] };
                if (intersectingTexts.Count > 0 &&
                    !string.IsNullOrWhiteSpace(intersectingTexts[i]))
                {
                    if (!AnalyticalJsonHelp.TryParseObject(
                            this,
                            intersectingTexts[i],
                            "IntersectingMemberPartRefs",
                            i,
                            out JObject intersecting))
                    {
                        return;
                    }
                    memberPart["intersectingMemberPartRef"] = intersecting;
                }
                memberParts.Add(memberPart);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["memberParts"] = memberParts },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var items = AnalyticalJsonHelp.SplitErrors(
                response,
                "localCoordinateSystems",
                out List<string> errors);

            var xVectors = new List<object>();
            var yVectors = new List<object>();
            foreach (var item in items)
            {
                var system = item?["localCoordinateSystem"];
                xVectors.Add(AnalyticalJsonHelp.VectorOf(system?["xVector"]));
                yVectors.Add(AnalyticalJsonHelp.VectorOf(system?["yVector"]));
            }

            da.SetDataList(0, xVectors);
            da.SetDataList(1, yVectors);
            da.SetDataList(2, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAnalyticalModel;

        public override Guid ComponentGuid =>
            new Guid("eb12d7fb-465b-4916-a4f3-781995c1873e");
    }
}
