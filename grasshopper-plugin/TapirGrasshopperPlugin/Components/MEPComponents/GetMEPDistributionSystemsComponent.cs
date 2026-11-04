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

        protected override void AddInputs()
        {
            InBoolean(
                "IncludeTree",
                "Whether to return the tree of each system: its elements from the root element on, each with the " +
                "element it is connected to on the way from the root. False when not given.",
                false);

            SetOptionality(0);
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

            OutGenericTree(
                "TreeElementGuids",
                "With IncludeTree, the elements of the tree of each distribution system from the root element on " +
                "(one branch per system).");

            OutGenericTree(
                "TreePreviousElementGuids",
                "With IncludeTree, the element each tree element is connected to on the way from the root " +
                "(null for the root; one branch per system).");

            OutIntegerTree(
                "TreeDepths",
                "With IncludeTree, the depth of each tree element from the root element (one branch per system).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            bool includeTree = false;
            da.GetData(0, ref includeTree);
            var parameters = new JObject();
            if (includeTree)
            {
                parameters["includeTree"] = true;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var domains = new List<object>();
            var mepSystemGuids = new List<object>();
            var elementGuids = new DataTree<object>();
            var treeElementGuids = new DataTree<object>();
            var treePreviousElementGuids = new DataTree<object>();
            var treeDepths = new DataTree<object>();

            var items = response["distributionSystems"] as JArray ?? new JArray();
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                elementGuids.EnsurePath(path);
                treeElementGuids.EnsurePath(path);
                treePreviousElementGuids.EnsurePath(path);
                treeDepths.EnsurePath(path);

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

                if (item["tree"] is JArray tree)
                {
                    foreach (var node in tree)
                    {
                        treeElementGuids.Add(ElementGuidWrapperOf(node["elementId"]), path);
                        treePreviousElementGuids.Add(ElementGuidWrapperOf(node["previousElementId"]), path);
                        treeDepths.Add(node["depth"]?.ToObject<int?>(), path);
                    }
                }
            }

            da.SetDataList(0, domains);
            da.SetDataList(1, mepSystemGuids);
            da.SetDataTree(2, elementGuids);
            da.SetDataTree(3, treeElementGuids);
            da.SetDataTree(4, treePreviousElementGuids);
            da.SetDataTree(5, treeDepths);
        }

        // The element identifier in an id object like {"guid": "..."}; null when it is absent.
        private static ElementGuidWrapper ElementGuidWrapperOf(
            JToken idObject)
        {
            var guid = idObject?["guid"]?.ToString();
            return guid == null
                ? null
                : new ElementGuidWrapper { ElementId = new ElementGuid { Guid = guid } };
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetMEPDistributionSystems;

        public override Guid ComponentGuid =>
            new Guid("cb1e58a0-1ce4-4313-b84e-06ad8cbb8982");
    }
}
