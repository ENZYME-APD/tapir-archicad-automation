using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class CompareLibraryPartUnIdsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "CompareLibraryPartUnIds";

        public CompareLibraryPartUnIdsComponent()
            : base(
                "CompareLibraryPartUnIds",
                "Compares two library part unique IDs with Archicad's own comparison and tells whether their main " +
                "and revision GUIDs are the same.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "UnId1",
                "A library part unique ID, '{main GUID}-{revision GUID}', as GetLibraryPartIdentities returns it.");

            InText(
                "UnId2",
                "The library part unique ID to compare UnId1 with.");
        }

        protected override void AddOutputs()
        {
            OutInteger(
                "ApiErrorCode",
                "The code Archicad's comparison returned: 0 when it reports the two unique IDs as matching, " +
                "otherwise the API error code that tells how they differ.");

            OutBoolean(
                "SameMainGuid",
                "True when the two unique IDs have the same main GUID, that is they are revisions of the same " +
                "library part.");

            OutBoolean(
                "SameRevisionGuid",
                "True when the two unique IDs have the same revision GUID.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(0, out string unId1) ||
                !da.TryGet(1, out string unId2))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject
                    {
                        ["unId1"] = unId1,
                        ["unId2"] = unId2
                    },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(0, (int?)response["apiErrorCode"]);
            da.SetData(1, (bool?)response["sameMainGuid"]);
            da.SetData(2, (bool?)response["sameRevisionGuid"]);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAvailableLibraryParts;

        public override Guid ComponentGuid =>
            new Guid("1351a221-c817-4fd3-9a7c-21cdcff0ffbf");
    }
}
