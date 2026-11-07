using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class GetElementsByClassificationComponent
        : ArchicadAccessorComponent
    {
        public override string CommandName => "GetElementsByClassification";

        public GetElementsByClassificationComponent()
            : base(
                "GetElementsByClassification",
                "Get the elements classified with the given classification items, one branch per item, " +
                "so elements can be filtered by classification in a single call.",
                GroupNames.Classifications)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ClassificationItemGuids",
                "Identifiers of the classification items to find the elements of.");
        }

        protected override void AddOutputs()
        {
            OutGenericTree(
                "ElementGuids",
                "Elements classified with each classification item (one branch per item).");

            OutErrorMessages(
                "Error message for each classification item (empty when successful).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> itemWrappers))
            {
                return;
            }

            var itemIds = new JArray();
            foreach (var wrapper in itemWrappers)
            {
                var id = GuidObject<ClassificationGuid>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError(
                        "Invalid classification item identifier in the ClassificationItemGuids input.");
                    return;
                }
                itemIds.Add(
                    new JObject
                    {
                        ["classificationItemId"] = new JObject { ["guid"] = id.Guid }
                    });
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["classificationItemIds"] = itemIds },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var elementGuids = new DataTree<object>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "elementsOfClassificationItems");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                elementGuids.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");

                if (item["elements"] is JArray elements)
                {
                    foreach (var element in elements)
                    {
                        var guid = JsonOutputHelp.WrappedGuidOf(
                            element,
                            "elementId");
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

            da.SetDataTree(0, elementGuids);
            da.SetDataList(1, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetElementsByClassification;

        public override Guid ComponentGuid =>
            new Guid("a6ec3882-8958-48ae-8477-ec3b04ee817c");
    }
}
