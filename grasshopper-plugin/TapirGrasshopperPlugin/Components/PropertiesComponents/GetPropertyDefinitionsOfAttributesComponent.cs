using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class GetPropertyDefinitionsOfAttributesComponent
        : ArchicadAccessorComponent
    {
        public override string CommandName =>
            "GetPropertyDefinitionsOfAttributes";

        public GetPropertyDefinitionsOfAttributesComponent()
            : base(
                "GetPropertyDefinitionsOfAttributes",
                "Get the property definitions of the given attributes (for example Building Materials): " +
                "the user defined, the fundamental built-in, or all of them. " +
                "The output has one branch per queried attribute.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "AttributeGuids",
                "Identifiers of the attributes to query.");

            InText(
                "Filter",
                "Which property definitions to return: UserDefined (the default), FundamentalBuiltIn or All.");

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutGenericTree(
                "PropertyGuids",
                "Identifiers of the property definitions of each attribute (one branch per attribute).");

            OutErrorMessages(
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

            var parameters = new JObject { ["attributeIds"] = attributeIds };

            if (da.TryGet(
                    1,
                    out string filter) &&
                !string.IsNullOrEmpty(filter))
            {
                parameters["filter"] = filter;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var propertyGuids = new DataTree<object>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "propertyDefinitionsOfAttributes");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                propertyGuids.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");

                if (item["propertyIds"] is JArray propertyIds)
                {
                    foreach (var propertyId in propertyIds)
                    {
                        propertyGuids.Add(
                            PropertyGuidObject.FromString(
                                JsonOutputHelp.WrappedGuidOf(
                                    propertyId,
                                    "propertyId")),
                            path);
                    }
                }
            }

            da.SetDataTree(0, propertyGuids);
            da.SetDataList(1, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetPropertyDefinitionsOfAttributes;

        public override Guid ComponentGuid =>
            new Guid("06153a5e-39ff-472a-ad70-62a1bde57261");
    }
}
