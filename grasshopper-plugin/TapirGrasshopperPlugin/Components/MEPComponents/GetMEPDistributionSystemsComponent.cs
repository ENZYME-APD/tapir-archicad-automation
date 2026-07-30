using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class GetMEPDistributionSystemsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetMEPDistributionSystems";

        public GetMEPDistributionSystemsComponent()
            : base(
                "GetMEPDistributionSystems",
                "Get the MEP distribution systems of the project with their domain, MEP system attribute and member elements. " +
                "Available from Archicad 28.",
                GroupNames.MEP)
        {
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Domains",
                "MEP domain of each distribution system.");

            OutGenerics(
                "MEPSystemGuids",
                "MEP system attribute of each distribution system.");

            OutGenericTree(
                "ElementGuids",
                "Identifiers of the member elements of each distribution system (one branch per system).");
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

            var domains = new List<object>();
            var mepSystemGuids = new List<object>();
            var elementGuids = new DataTree<object>();

            var items = response["distributionSystems"] as JArray ?? new JArray();
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                elementGuids.EnsurePath(path);

                domains.Add(item["domain"]?.ToString());
                var mepSystemGuid = item["mepSystemId"]?["guid"]?.ToString();
                mepSystemGuids.Add(
                    mepSystemGuid == null
                        ? null
                        : new AttributeGuidWrapper
                        {
                            AttributeId = new AttributeGuidObject { Guid = mepSystemGuid }
                        });

                if (item["elements"] is JArray elements)
                {
                    foreach (var element in elements)
                    {
                        var guid = element["elementId"]?["guid"]?.ToString();
                        elementGuids.Add(
                            guid == null
                                ? null
                                : new ElementGuidWrapper
                                {
                                    ElementId = new ElementGuid { Guid = guid }
                                },
                            path);
                    }
                }
            }

            da.SetDataList(0, domains);
            da.SetDataList(1, mepSystemGuids);
            da.SetDataTree(2, elementGuids);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetMEPDistributionSystems;

        public override Guid ComponentGuid =>
            new Guid("cb1e58a0-1ce4-4313-b84e-06ad8cbb8982");
    }
}
