using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class GetClassificationsOfAttributesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetClassificationsOfAttributes";

        public GetClassificationsOfAttributesComponent()
            : base(
                "GetClassificationsOfAttributes",
                "Get the classifications of the given attributes (building materials, composites, surfaces, etc.). " +
                "Outputs have one branch per queried attribute, with one item per classification system the attribute is classified in.",
                GroupNames.Classifications)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "AttributeGuids",
                "Identifiers of the attributes to query.");
        }

        protected override void AddOutputs()
        {
            OutGenericTree(
                "ClassificationSystemGuids",
                "Classification system of each classification (one branch per attribute).");

            OutGenericTree(
                "ClassificationItemGuids",
                "Classification item of each classification (one branch per attribute).");

            OutTexts(
                "ErrorMessages",
                "Error message for each queried attribute (empty when successful).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> attributeWrappers))
            {
                return;
            }

            var input = new GetClassificationsOfAttributesParameters
            {
                AttributeIds = new List<AttributeGuidWrapper>()
            };

            foreach (var wrapper in attributeWrappers)
            {
                var id = GuidObject<AttributeGuidObject>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError("Invalid attribute identifier in the AttributeGuids input.");
                    return;
                }
                input.AttributeIds.Add(
                    new AttributeGuidWrapper { AttributeId = id });
            }

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(input),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var classificationSystemGuids = new DataTree<object>();
            var classificationItemGuids = new DataTree<object>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "attributeClassifications");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                classificationSystemGuids.EnsurePath(path);
                classificationItemGuids.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");

                if (item["classificationIds"] is JArray classificationIds)
                {
                    foreach (var classificationId in classificationIds)
                    {
                        var systemGuid = JsonOutputHelp.WrappedGuidOf(
                            classificationId,
                            "classificationSystemId");
                        var itemGuid = JsonOutputHelp.WrappedGuidOf(
                            classificationId,
                            "classificationItemId");

                        classificationSystemGuids.Add(
                            systemGuid == null
                                ? null
                                : new ClassificationSystemGuidWrapper
                                {
                                    ClassificationSystemId =
                                        new ClassificationGuid { Guid = systemGuid }
                                },
                            path);
                        classificationItemGuids.Add(
                            itemGuid == null
                                ? null
                                : new ClassificationItemGuidWrapper
                                {
                                    ClassificationItemId =
                                        new ClassificationGuid { Guid = itemGuid }
                                },
                            path);
                    }
                }
            }

            da.SetDataTree(0, classificationSystemGuids);
            da.SetDataTree(1, classificationItemGuids);
            da.SetDataList(2, errors);
        }

        // Shares the icon of GetClassificationsOfElements.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetClassificationsOfElements;

        public override Guid ComponentGuid =>
            new Guid("5612343b-8efb-4095-97e1-17f7ac75eb7b");
    }
}
