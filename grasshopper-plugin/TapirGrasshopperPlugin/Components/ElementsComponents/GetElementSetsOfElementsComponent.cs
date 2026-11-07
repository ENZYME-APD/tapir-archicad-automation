using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetElementSetsOfElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetElementSetsOfElements";

        public GetElementSetsOfElementsComponent()
            : base(
                "GetElementSetsOfElements",
                "Get the element sets that contain each given element. Only element sets created by the Tapir " +
                "Add-On are examined; sets created by other add-ons are not found.",
                GroupNames.ElementGrouping)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to query.");
        }

        protected override void AddOutputs()
        {
            OutGenericTree(
                "ElementSetGuids",
                "Element sets that contain each element (one branch per element; empty when it is in none).");

            OutErrorMessages(
                "Error message for each queried element (empty when successful).");
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

            var elementSetGuids = new DataTree<object>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "elementSetsOfElements");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                elementSetGuids.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");

                if (item["elementSets"] is JArray elementSets)
                {
                    foreach (var elementSet in elementSets)
                    {
                        var guid = JsonOutputHelp.WrappedGuidOf(
                            elementSet,
                            "elementSetId");
                        elementSetGuids.Add(
                            guid == null
                                ? null
                                : new ElementSetGuidWrapper
                                {
                                    ElementSetId = new ElementSetGuid { Guid = guid }
                                },
                            path);
                    }
                }
            }

            da.SetDataTree(0, elementSetGuids);
            da.SetDataList(1, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetElementSetsOfElements;

        public override Guid ComponentGuid =>
            new Guid("3d4afee1-3cc2-4541-a63e-81c9ccec92c9");
    }
}
