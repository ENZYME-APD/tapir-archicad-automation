using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class GetAnalyticalMemberTypesOfElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAnalyticalMemberTypesOfElements";

        public GetAnalyticalMemberTypesOfElementsComponent()
            : base(
                "GetAnalyticalMemberTypesOfElements",
                "Retrieves the analytical member type (Beam, Column, Roof, Slab, Wall or Unknown) " +
                "of the given elements.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to get the analytical member type of.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "MemberTypes",
                "Analytical member type of each element: Unknown, Beam, Column, Roof, Slab or Wall " +
                "(null when it could not be got).");

            OutErrorMessages(
                "Error message of each element (empty when its member type was got).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(elements),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var memberTypes = new List<object>();
            var errors = new List<string>();
            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "memberTypesOfElements"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    memberTypes.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                memberTypes.Add(
                    JsonOutputHelp.Scalar(
                        item,
                        "memberType"));
                errors.Add("");
            }

            da.SetDataList(
                0,
                memberTypes);
            da.SetDataList(
                1,
                errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAnalyticalMemberTypesOfElements;

        public override Guid ComponentGuid =>
            new Guid("42615ac3-5606-4dfc-ae41-7b3bb7f79cf5");
    }
}
