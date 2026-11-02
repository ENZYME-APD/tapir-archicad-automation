using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetHotlinkProxyElementTableComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetHotlinkProxyElementTable";

        public GetHotlinkProxyElementTableComponent()
            : base(
                "GetHotlinkProxyElementTable",
                "Maps the proxy elements of placed hotlink instances to the elements of the source file they were " +
                "made from, so an element seen in a hotlinked module can be traced back to its source element.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "HotlinkInstanceGuids",
                "The placed hotlink instances: elements of type Hotlink, for example the HotlinkInstanceGuids of " +
                "GetHotlinkOwnersOfElements. Not the nodes GetHotlinks returns.");
        }

        protected override void AddOutputs()
        {
            OutGenericTree(
                "ProxyElementGuids",
                "The proxy elements of each instance in the current project (one branch per instance, in no particular order).");

            OutGenericTree(
                "SourceElementGuids",
                "The element in the hotlink's source file each proxy element was made from (one branch per instance).");

            OutErrorMessages(
                "Error message of each instance (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject instances))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["hotlinkInstances"] = JArray.FromObject(instances.Elements) },
                    ToAddOn,
                    out var response))
            {
                return;
            }

            var proxyElementIds = new DataTree<ElementGuidWrapper>();
            var sourceElementIds = new DataTree<ElementGuidWrapper>();
            var errors = new List<string>();

            var tables = JsonOutputHelp.Items(
                response,
                "proxyElementTables");
            for (var i = 0; i < tables.Count; i++)
            {
                var table = tables[i];
                var path = new GH_Path(i);
                proxyElementIds.EnsurePath(path);
                sourceElementIds.EnsurePath(path);

                if (JsonOutputHelp.IsError(table))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(table));
                    continue;
                }

                errors.Add("");

                if (!(table["proxyElements"] is JArray proxyElements))
                {
                    continue;
                }

                foreach (var proxyElement in proxyElements)
                {
                    proxyElementIds.Add(
                        new ElementGuidWrapper
                        {
                            ElementId = new ElementGuid
                            {
                                Guid = JsonOutputHelp.WrappedGuidOf(
                                    proxyElement,
                                    "proxyElementId")
                            }
                        },
                        path);
                    sourceElementIds.Add(
                        new ElementGuidWrapper
                        {
                            ElementId = new ElementGuid
                            {
                                Guid = JsonOutputHelp.WrappedGuidOf(
                                    proxyElement,
                                    "sourceElementId")
                            }
                        },
                        path);
                }
            }

            da.SetDataTree(0, proxyElementIds);
            da.SetDataTree(1, sourceElementIds);
            da.SetDataList(2, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetHotlinkProxyElementTable;

        public override Guid ComponentGuid =>
            new Guid("d51d89ff-d977-4fa2-b35d-70cb765ebd98");
    }
}
