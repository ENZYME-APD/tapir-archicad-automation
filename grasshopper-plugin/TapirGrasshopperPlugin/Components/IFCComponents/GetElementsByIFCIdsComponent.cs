using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Commands;

namespace TapirGrasshopperPlugin.Components.IFCComponents
{
    public class GetElementsByIFCIdsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetElementsByIFCIds";

        public GetElementsByIFCIdsComponent()
            : base(
                "GetElementsByIFCIds",
                "Retrieve the elements by the given IFC identifiers.",
                GroupNames.IFC)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "IFCIds",
                "IFC identifiers to get the corresponding elements for.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "IFCIds",
                "IFC identifier of each result.");

            OutGenericTree(
                "ElementGuids",
                "Identifiers of the elements corresponding to each IFC identifier (one branch per IFC identifier).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> inputIfcIds))
            {
                return;
            }

            var input = new GetElementsByIFCIdsParameters { IfcIds = inputIfcIds };

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(input),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var ifcIds = new List<object>();
            var elementGuids = new DataTree<object>();

            var items = JsonOutputHelp.Items(
                response,
                "elementsByIFCIds");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                elementGuids.EnsurePath(path);

                ifcIds.Add(JsonOutputHelp.Scalar(item, "ifcId"));

                if (item["elements"] is JArray elements)
                {
                    foreach (var element in elements)
                    {
                        elementGuids.Add(
                            ElementsStructuredGetterComponent.ElementIdOf(element),
                            path);
                    }
                }
            }

            da.SetDataList(0, ifcIds);
            da.SetDataTree(1, elementGuids);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetElementsByIFCIds;

        public override Guid ComponentGuid =>
            new Guid("96698d90-7896-4bd4-8e2e-de53363d2e12");
    }
}
