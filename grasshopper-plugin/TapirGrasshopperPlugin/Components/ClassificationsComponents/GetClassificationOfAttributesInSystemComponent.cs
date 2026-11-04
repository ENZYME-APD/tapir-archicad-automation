using Grasshopper.Kernel;
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
    public class GetClassificationOfAttributesInSystemComponent
        : ArchicadAccessorComponent
    {
        public override string CommandName =>
            "GetClassificationOfAttributesInSystem";

        public GetClassificationOfAttributesInSystemComponent()
            : base(
                "GetClassificationOfAttributesInSystem",
                "Get the classification of the given attributes (for example Building Materials) in the given " +
                "classification system. The item is null for an unclassified attribute.",
                GroupNames.Classifications)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "AttributeGuids",
                "Identifiers of the attributes to query.");

            InGeneric(
                "ClassificationSystemGuid",
                "Identifier of the classification system.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ClassificationItemGuids",
                "Classification item of each attribute in the system (null when unclassified or failed).");

            OutErrorMessages(
                "Error message for each queried attribute (empty when successful).");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            AddAsSource<ClassificationSystemValueList>(
                document,
                1);
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

            if (!da.TryGet(
                    1,
                    out GH_ObjectWrapper systemWrapper))
            {
                return;
            }

            var systemId = GuidObject<ClassificationGuid>.CreateFromWrapper(systemWrapper);
            if (systemId == null)
            {
                this.AddError(
                    "Invalid classification system identifier in the ClassificationSystemGuid input.");
                return;
            }

            var attributeIds = new JArray();
            foreach (var wrapper in attributeWrappers)
            {
                var id = GuidObject<AttributeGuidObject>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError("Invalid attribute identifier in the AttributeGuids input.");
                    return;
                }
                attributeIds.Add(
                    new JObject
                    {
                        ["attributeId"] = new JObject { ["guid"] = id.Guid }
                    });
            }

            var parameters = new JObject
            {
                ["attributeIds"] = attributeIds,
                ["classificationSystemId"] = new JObject { ["guid"] = systemId.Guid }
            };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var itemGuids = new List<ClassificationItemGuidWrapper>();
            var errors = new List<string>();

            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "attributeClassifications"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    itemGuids.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                var itemGuid = JsonOutputHelp.WrappedGuidOf(
                    item["classificationId"],
                    "classificationItemId");
                itemGuids.Add(
                    itemGuid == null
                        ? null
                        : new ClassificationItemGuidWrapper
                        {
                            ClassificationItemId =
                                new ClassificationGuid { Guid = itemGuid }
                        });
                errors.Add("");
            }

            da.SetDataList(0, itemGuids);
            da.SetDataList(1, errors);
        }

        // Shares the icon of GetClassificationsOfElements.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetClassificationsOfElements;

        public override Guid ComponentGuid =>
            new Guid("5421c8d6-b70c-4f66-b777-09b25d25557d");
    }
}
