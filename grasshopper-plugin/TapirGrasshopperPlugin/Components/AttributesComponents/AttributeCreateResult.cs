using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.Attributes;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    // The attribute creator commands all answer with an "attributeIds" array holding
    // one item per requested attribute - either the identifier of the created
    // attribute or an error. Those identifiers are what the attribute inputs of the
    // element components (buildingMaterialId, compositeId, profileId, fillId, ...)
    // expect, so every creator component has to hand them out.
    internal static class AttributeCreateResult
    {
        public static void AddOutputs(
            Component component)
        {
            component.OutGenerics(
                "AttributeGuids",
                "Identifiers of the created attributes (empty for the failed ones).");

            component.OutTexts(
                "ErrorMessages",
                "Error message of each attribute (empty when it was created successfully).");
        }

        public static void SetOutputs(
            IGH_DataAccess da,
            JObject response,
            int guidsOutputIndex = 0,
            int errorsOutputIndex = 1)
        {
            var attributeGuids = new List<AttributeGuidObject>();
            var errorMessages = new List<string>();

            if (response?["attributeIds"] is JArray attributeIds)
            {
                foreach (var item in attributeIds)
                {
                    var error = item?["error"];
                    if (error != null)
                    {
                        attributeGuids.Add(null);
                        errorMessages.Add(
                            error["message"]?.ToString() ??
                            "Failed to create the attribute.");
                        continue;
                    }

                    var guid = item?["attributeId"]?["guid"]?.ToString();
                    attributeGuids.Add(
                        guid == null
                            ? null
                            : new AttributeGuidObject { Guid = guid });
                    errorMessages.Add("");
                }
            }

            da.SetDataList(
                guidsOutputIndex,
                attributeGuids);

            da.SetDataList(
                errorsOutputIndex,
                errorMessages);
        }
    }
}
