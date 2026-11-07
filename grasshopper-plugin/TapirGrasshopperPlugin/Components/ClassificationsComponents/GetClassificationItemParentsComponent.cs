using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class GetClassificationItemParentsComponent
        : ArchicadAccessorComponent
    {
        public override string CommandName => "GetClassificationItemParents";

        public GetClassificationItemParentsComponent()
            : base(
                "GetClassificationItemParents",
                "Get the parent item and the classification system of the given classification items; " +
                "a top level item has no parent. The outputs are aligned with the input items.",
                GroupNames.Classifications)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ClassificationItemGuids",
                "Identifiers of the classification items.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ClassificationSystemGuids",
                "The classification system each item belongs to (null on error).");

            OutGenerics(
                "ParentItemGuids",
                "The parent item of each item (null for a top level item of its system).");

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

            var systemGuids = new List<object>();
            var parentGuids = new List<object>();
            var errors = new List<string>();

            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "classificationItemParents"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    systemGuids.Add(null);
                    parentGuids.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                var systemGuid = JsonOutputHelp.WrappedGuidOf(
                    item,
                    "classificationSystemId");
                var parentGuid = JsonOutputHelp.WrappedGuidOf(
                    item,
                    "parentClassificationItemId");

                systemGuids.Add(
                    systemGuid == null
                        ? null
                        : new ClassificationSystemGuidWrapper
                        {
                            ClassificationSystemId =
                                new ClassificationGuid { Guid = systemGuid }
                        });
                parentGuids.Add(
                    parentGuid == null
                        ? null
                        : new ClassificationItemGuidWrapper
                        {
                            ClassificationItemId =
                                new ClassificationGuid { Guid = parentGuid }
                        });
                errors.Add("");
            }

            da.SetDataList(0, systemGuids);
            da.SetDataList(1, parentGuids);
            da.SetDataList(2, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetClassificationItemParents;

        public override Guid ComponentGuid =>
            new Guid("80c02c2a-96fc-4b6d-bf2d-a9e7b6c7cc67");
    }
}
