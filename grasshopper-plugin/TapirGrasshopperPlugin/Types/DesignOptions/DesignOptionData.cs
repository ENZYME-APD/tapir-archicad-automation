using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Types.DesignOptions
{
    public class DesignOptionGuid : GuidObject<DesignOptionGuid>
    {
    }

    public class DesignOptionGuidWrapper
        : GuidWrapper<DesignOptionGuid, DesignOptionGuidWrapper>
    {
        [JsonProperty("designOptionId")]
        public DesignOptionGuid DesignOptionId;

        [JsonIgnore]
        public override DesignOptionGuid Id
        {
            get => DesignOptionId;
            set => DesignOptionId = value;
        }
    }

    public class DesignOptionsObject
        : GuidItemsObject<DesignOptionGuid, DesignOptionGuidWrapper, DesignOptionsObject>
    {
        [JsonProperty("designOptions")]
        public List<DesignOptionGuidWrapper> DesignOptions;

        [JsonIgnore]
        public override List<DesignOptionGuidWrapper> GuidWrappers
        {
            get => DesignOptions;
            set => DesignOptions = value;
        }
    }

    public class DesignOptionCombinationGuid : GuidObject<DesignOptionCombinationGuid>
    {
    }

    public class DesignOptionCombinationGuidWrapper
        : GuidWrapper<DesignOptionCombinationGuid, DesignOptionCombinationGuidWrapper>
    {
        [JsonProperty("designOptionCombinationId")]
        public DesignOptionCombinationGuid DesignOptionCombinationId;

        [JsonIgnore]
        public override DesignOptionCombinationGuid Id
        {
            get => DesignOptionCombinationId;
            set => DesignOptionCombinationId = value;
        }
    }

    public class DesignOptionCombinationsObject
        : GuidItemsObject<DesignOptionCombinationGuid, DesignOptionCombinationGuidWrapper, DesignOptionCombinationsObject>
    {
        [JsonProperty("designOptionCombinations")]
        public List<DesignOptionCombinationGuidWrapper> DesignOptionCombinations;

        [JsonIgnore]
        public override List<DesignOptionCombinationGuidWrapper> GuidWrappers
        {
            get => DesignOptionCombinations;
            set => DesignOptionCombinations = value;
        }
    }

    public class DesignOptionSetGuid : GuidObject<DesignOptionSetGuid>
    {
    }

    public class DesignOptionSetGuidWrapper
        : GuidWrapper<DesignOptionSetGuid, DesignOptionSetGuidWrapper>
    {
        [JsonProperty("designOptionSetId")]
        public DesignOptionSetGuid DesignOptionSetId;

        [JsonIgnore]
        public override DesignOptionSetGuid Id
        {
            get => DesignOptionSetId;
            set => DesignOptionSetId = value;
        }
    }

    public class DesignOptionSetsObject
        : GuidItemsObject<DesignOptionSetGuid, DesignOptionSetGuidWrapper, DesignOptionSetsObject>
    {
        [JsonProperty("designOptionSets")]
        public List<DesignOptionSetGuidWrapper> DesignOptionSets;

        [JsonIgnore]
        public override List<DesignOptionSetGuidWrapper> GuidWrappers
        {
            get => DesignOptionSets;
            set => DesignOptionSets = value;
        }
    }

    public class DesignOptionDetails
    {
        [JsonProperty("designOptionId")]
        public DesignOptionGuid DesignOptionId;

        [JsonProperty("name")]
        public string Name;

        [JsonProperty("id")]
        public string Id;

        [JsonProperty("ownerSetName")]
        public string OwnerSetName;
    }

    public class GetDesignOptionsResponse
    {
        [JsonProperty("designOptions")]
        public List<DesignOptionDetails> DesignOptions;
    }

    public class DesignOptionSetDetails
    {
        [JsonProperty("designOptionSetId")]
        public DesignOptionSetGuid DesignOptionSetId;

        [JsonProperty("name")]
        public string Name;

        [JsonProperty("designOptions")]
        public List<DesignOptionGuidWrapper> DesignOptions;
    }

    public class GetDesignOptionSetsResponse
    {
        [JsonProperty("designOptionSets")]
        public List<DesignOptionSetDetails> DesignOptionSets;
    }

    public class DesignOptionCombinationDetails
    {
        [JsonProperty("designOptionCombinationId")]
        public DesignOptionCombinationGuid DesignOptionCombinationId;

        [JsonProperty("name")]
        public string Name;

        [JsonProperty("activeDesignOptions")]
        public List<DesignOptionGuidWrapper> ActiveDesignOptions;
    }

    public class GetDesignOptionCombinationsResponse
    {
        [JsonProperty("designOptionCombinations")]
        public List<DesignOptionCombinationDetails> DesignOptionCombinations;
    }

    public class ElementsOfDesignOption
    {
        [JsonProperty("designOptionId")]
        public DesignOptionGuid DesignOptionId;

        [JsonProperty("elements")]
        public List<ElementGuidWrapper> Elements;

        // Set instead of the fields above when the design option was not found.
        [JsonProperty("error")]
        public Error Error;
    }

    public class GetElementsOfDesignOptionsResponse
    {
        [JsonProperty("elementsOfDesignOptions")]
        public List<ElementsOfDesignOption> ElementsOfDesignOptions;
    }

    public class DesignOptionForElement
    {
        [JsonProperty("elementId")]
        public ElementGuid ElementId;

        [JsonProperty("type")]
        public string Type;

        [JsonProperty("designOption")]
        public DesignOptionDetails DesignOption;
    }

    public class GetDesignOptionForElementsResponse
    {
        [JsonProperty("designOptionForElements")]
        public List<DesignOptionForElement> DesignOptionForElements;
    }

    // Input parameter payloads

    public class CreateDesignOptionSetsParameters
    {
        [JsonProperty("designOptionSets")]
        public List<string> DesignOptionSets;
    }

    public class CreateDesignOptionParameter
    {
        [JsonProperty("name")]
        public string Name;

        [JsonProperty("id")]
        public string Id;

        [JsonProperty("ownerSetName")]
        public string OwnerSetName;
    }

    public class CreateDesignOptionsParameters
    {
        [JsonProperty("designOptions")]
        public List<CreateDesignOptionParameter> DesignOptions;
    }

    public class CreateDesignOptionCombinationParameter
    {
        [JsonProperty("name")]
        public string Name;

        [JsonProperty("activeDesignOptions")]
        public List<DesignOptionGuidWrapper> ActiveDesignOptions;
    }

    public class CreateDesignOptionCombinationsParameters
    {
        [JsonProperty("designOptionCombinations")]
        public List<CreateDesignOptionCombinationParameter> DesignOptionCombinations;
    }

    public class ActiveDesignOptionsInCombination
    {
        [JsonProperty("designOptionCombinationId")]
        public DesignOptionCombinationGuid DesignOptionCombinationId;

        [JsonProperty("activeDesignOptions")]
        public List<DesignOptionGuidWrapper> ActiveDesignOptions;
    }

    public class SetActiveDesignOptionsInCombinationsParameters
    {
        [JsonProperty("activeDesignOptionsInCombinations")]
        public List<ActiveDesignOptionsInCombination> ActiveDesignOptionsInCombinations;
    }

    public class ElementDesignOptionPair
    {
        [JsonProperty("elementId")]
        public ElementGuid ElementId;

        [JsonProperty("designOptionId")]
        public DesignOptionGuid DesignOptionId;
    }

    public class MoveElementsToDesignOptionsParameters
    {
        [JsonProperty("elementDesignOptionPairs")]
        public List<ElementDesignOptionPair> ElementDesignOptionPairs;
    }

    public class DesignOptionAndSetPair
    {
        [JsonProperty("designOptionId")]
        public DesignOptionGuid DesignOptionId;

        [JsonProperty("setName")]
        public string SetName;
    }

    public class MoveDesignOptionsToAnotherSetParameters
    {
        [JsonProperty("designOptionAndSetPairs")]
        public List<DesignOptionAndSetPair> DesignOptionAndSetPairs;
    }

    public class RenameDesignOptionParameter
    {
        [JsonProperty("designOptionId")]
        public DesignOptionGuid DesignOptionId;

        [JsonProperty(
            "name",
            NullValueHandling = NullValueHandling.Ignore)]
        public string Name;

        [JsonProperty(
            "id",
            NullValueHandling = NullValueHandling.Ignore)]
        public string Id;
    }

    public class RenameDesignOptionsParameters
    {
        [JsonProperty("designOptions")]
        public List<RenameDesignOptionParameter> DesignOptions;
    }

    public class RenameDesignOptionSetParameter
    {
        [JsonProperty("designOptionSetId")]
        public DesignOptionSetGuid DesignOptionSetId;

        [JsonProperty("name")]
        public string Name;
    }

    public class RenameDesignOptionSetsParameters
    {
        [JsonProperty("designOptionSets")]
        public List<RenameDesignOptionSetParameter> DesignOptionSets;
    }

    public class RenameDesignOptionCombinationParameter
    {
        [JsonProperty("designOptionCombinationId")]
        public DesignOptionCombinationGuid DesignOptionCombinationId;

        [JsonProperty("name")]
        public string Name;
    }

    public class RenameDesignOptionCombinationsParameters
    {
        [JsonProperty("designOptionCombinations")]
        public List<RenameDesignOptionCombinationParameter> DesignOptionCombinations;
    }

    public class SetDefaultDesignOptionParameters
    {
        // Null selects the Main Model as the default.
        [JsonProperty(
            "designOptionId",
            NullValueHandling = NullValueHandling.Ignore)]
        public DesignOptionGuid DesignOptionId;
    }

    public class ReorderDesignOptionSetParameter
    {
        [JsonProperty("designOptionSetId")]
        public DesignOptionSetGuid DesignOptionSetId;

        // Null moves the set to the end.
        [JsonProperty(
            "beforeDesignOptionSetId",
            NullValueHandling = NullValueHandling.Ignore)]
        public DesignOptionSetGuid BeforeDesignOptionSetId;
    }

    public class ReorderDesignOptionSetsParameters
    {
        [JsonProperty("designOptionSets")]
        public List<ReorderDesignOptionSetParameter> DesignOptionSets;
    }

    public class ReorderDesignOptionInSetParameter
    {
        [JsonProperty("designOptionId")]
        public DesignOptionGuid DesignOptionId;

        // Null moves the design option to the end of its set.
        [JsonProperty(
            "beforeDesignOptionId",
            NullValueHandling = NullValueHandling.Ignore)]
        public DesignOptionGuid BeforeDesignOptionId;
    }

    public class ReorderDesignOptionsInSetParameters
    {
        [JsonProperty("designOptions")]
        public List<ReorderDesignOptionInSetParameter> DesignOptions;
    }
}
