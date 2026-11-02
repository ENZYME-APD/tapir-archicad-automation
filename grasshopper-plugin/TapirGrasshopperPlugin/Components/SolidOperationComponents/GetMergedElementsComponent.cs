using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.SolidOperationComponents
{
    public class GetMergedElementsComponent : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetMergedElements";

        public GetMergedElementsComponent()
            : base(
                "GetMergedElements",
                "Gets the elements each queried element is merged with, one branch per queried element; " +
                "an unknown element gets an error message.",
                GroupNames.ElementSolidOperations)
        {
        }

        protected override string ResponseArrayKey => "elementMerges";

        protected override void AddOutputs()
        {
            OutGenericTree(
                "MergedElementGuids",
                "Elements each queried element is merged with (one branch per queried element, empty when it " +
                "is not merged with any).");

            OutErrorMessages(
                "Error message of each queried element (empty when it was read).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var mergedElementGuids = new DataTree<ElementGuidWrapper>();
            var errorMessages = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var path = new GH_Path(i);
                mergedElementGuids.EnsurePath(path);

                if (JsonOutputHelp.IsError(items[i]))
                {
                    errorMessages.Add(JsonOutputHelp.ErrorMessage(items[i]));
                    continue;
                }

                if (items[i]["mergedElements"] is JArray mergedElements)
                {
                    foreach (var element in mergedElements)
                    {
                        mergedElementGuids.Add(
                            ElementIdOf(element),
                            path);
                    }
                }

                errorMessages.Add("");
            }

            da.SetDataTree(0, mergedElementGuids);
            da.SetDataList(1, errorMessages);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetMergedElements;

        public override Guid ComponentGuid =>
            new Guid("2e3ddbe3-3c95-4358-8916-2cf7dcd85e0a");
    }
}
