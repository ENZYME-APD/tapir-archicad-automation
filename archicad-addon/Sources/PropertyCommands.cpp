#include "PropertyCommands.hpp"
#include "MigrationHelper.hpp"
#include "HashTable.hpp"
#include <tuple>

using PropertyTypeTuple = std::tuple<API_PropertyCollectionType, API_VariantType, API_PropertyMeasureType>;

static GS::HashTable<GS::UniString, PropertyTypeTuple> PropertyTypeDictionary = {
    { "number",  PropertyTypeTuple (API_PropertySingleCollectionType, API_PropertyRealValueType,    API_PropertyDefaultMeasureType) },
    { "integer", PropertyTypeTuple (API_PropertySingleCollectionType, API_PropertyIntegerValueType, API_PropertyDefaultMeasureType) },
    { "string",  PropertyTypeTuple (API_PropertySingleCollectionType, API_PropertyStringValueType,  API_PropertyDefaultMeasureType) },
    { "boolean", PropertyTypeTuple (API_PropertySingleCollectionType, API_PropertyBooleanValueType, API_PropertyDefaultMeasureType) },
    { "guid",    PropertyTypeTuple (API_PropertySingleCollectionType, API_PropertyGuidValueType,    API_PropertyDefaultMeasureType) },

    { "length", PropertyTypeTuple (API_PropertySingleCollectionType, API_PropertyRealValueType,  API_PropertyLengthMeasureType) },
    { "area",   PropertyTypeTuple (API_PropertySingleCollectionType, API_PropertyRealValueType,  API_PropertyAreaMeasureType) },
    { "volume", PropertyTypeTuple (API_PropertySingleCollectionType, API_PropertyRealValueType,  API_PropertyVolumeMeasureType) },
    { "angle",  PropertyTypeTuple (API_PropertySingleCollectionType, API_PropertyRealValueType,  API_PropertyAngleMeasureType) },

    { "singleEnum", PropertyTypeTuple (API_PropertySingleChoiceEnumerationCollectionType,   API_PropertyStringValueType, API_PropertyDefaultMeasureType) },
    { "multiEnum",  PropertyTypeTuple (API_PropertyMultipleChoiceEnumerationCollectionType, API_PropertyStringValueType, API_PropertyDefaultMeasureType) },

    { "numberList",  PropertyTypeTuple (API_PropertyListCollectionType, API_PropertyRealValueType,    API_PropertyDefaultMeasureType) },
    { "integerList", PropertyTypeTuple (API_PropertyListCollectionType, API_PropertyIntegerValueType, API_PropertyDefaultMeasureType) },
    { "stringList",  PropertyTypeTuple (API_PropertyListCollectionType, API_PropertyStringValueType,  API_PropertyDefaultMeasureType) },
    { "booleanList", PropertyTypeTuple (API_PropertyListCollectionType, API_PropertyBooleanValueType,  API_PropertyDefaultMeasureType) },

    { "lengthList", PropertyTypeTuple (API_PropertyListCollectionType, API_PropertyRealValueType, API_PropertyLengthMeasureType) },
    { "areaList",   PropertyTypeTuple (API_PropertyListCollectionType, API_PropertyRealValueType, API_PropertyAreaMeasureType) },
    { "volumeList", PropertyTypeTuple (API_PropertyListCollectionType, API_PropertyRealValueType, API_PropertyVolumeMeasureType) },
    { "angleList",  PropertyTypeTuple (API_PropertyListCollectionType, API_PropertyRealValueType, API_PropertyAngleMeasureType) },
};

static GS::UniString GetPropertyTypeString (API_PropertyDefinitionType type)
{
    static GS::HashTable<API_PropertyDefinitionType, GS::UniString> TypeToString ({
        { API_PropertyStaticBuiltInDefinitionType, "StaticBuiltIn" },
        { API_PropertyDynamicBuiltInDefinitionType, "DynamicBuiltIn" },
        { API_PropertyCustomDefinitionType, "Custom" }
    });
    if (!TypeToString.ContainsKey (type)) {
        return GS::EmptyUniString;
    }
    return TypeToString[type];
}

static GS::UniString GetPropertyTypeString (API_PropertyCollectionType type)
{
    static GS::HashTable<API_PropertyCollectionType, GS::UniString> TypeToString ({
        { API_PropertyUndefinedCollectionType, "Undefined" },
        { API_PropertySingleCollectionType, "Single" },
        { API_PropertyListCollectionType, "List" },
        { API_PropertySingleChoiceEnumerationCollectionType, "SingleChoiceEnumeration" },
        { API_PropertyMultipleChoiceEnumerationCollectionType, "MultipleChoiceEnumeration" }
    });
    if (!TypeToString.ContainsKey (type)) {
        return GS::EmptyUniString;
    }
    return TypeToString[type];
}

static GS::UniString GetPropertyTypeString (API_VariantType type)
{
    static GS::HashTable<API_VariantType, GS::UniString> TypeToString ({
        { API_PropertyUndefinedValueType, "Undefined" },
        { API_PropertyIntegerValueType, "Integer" },
        { API_PropertyRealValueType, "Real" },
        { API_PropertyStringValueType, "String" },
        { API_PropertyBooleanValueType, "Boolean" },
        { API_PropertyGuidValueType, "Guid" }
    });
    if (!TypeToString.ContainsKey (type)) {
        return GS::EmptyUniString;
    }
    return TypeToString[type];
}

static GS::UniString GetPropertyTypeString (API_PropertyMeasureType type)
{
    static GS::HashTable<API_PropertyMeasureType, GS::UniString> TypeToString ({
        { API_PropertyUndefinedMeasureType, "Undefined" },
        { API_PropertyDefaultMeasureType, "Default" },
        { API_PropertyLengthMeasureType, "Length" },
        { API_PropertyAreaMeasureType, "Area" },
        { API_PropertyVolumeMeasureType, "Volume" },
        { API_PropertyAngleMeasureType, "Angle" }
    });
    if (!TypeToString.ContainsKey (type)) {
        return GS::EmptyUniString;
    }
    return TypeToString[type];
}

static API_Guid GetRandomGuid ()
{
    GS::Guid guid;
    guid.Generate ();
    return GSGuid2APIGuid (guid);
}

// The enumValue shape shared by CreatePropertyDefinitions, UpdatePropertyDefinitions and
// GetAllProperties - see PossibleEnumValues in CommonSchemaDefinitions.json.
static GS::ObjectState CreateEnumValueObjectState (const API_SingleEnumerationVariant& variant)
{
    GS::ObjectState enumValue;
    enumValue.Add ("displayValue", variant.displayVariant.uniStringValue);
    if (variant.nonLocalizedValue.HasValue ()) {
        enumValue.Add ("nonLocalizedValue", *variant.nonLocalizedValue);
    }

    // Not enumValueId: that is a oneOf over displayValue / nonLocalizedValue, so a guid
    // does not validate as one and a value read here could not be sent back.
    enumValue.Add ("guid", APIGuidToString (variant.keyVariant.guidValue));

    return GS::ObjectState ("enumValue", enumValue);
}

static API_Guid FindEnumValueGuid (const GS::Array<API_SingleEnumerationVariant>& possibleEnumValues,
                                   const GS::UniString& enumValueIdTypeStr,
                                   const GS::UniString& valueStr)
{
    const bool isNonLocalizedValue = enumValueIdTypeStr == "nonLocalizedValue";

    for (const API_SingleEnumerationVariant& v : possibleEnumValues) {
        if (isNonLocalizedValue) {
            if (v.nonLocalizedValue.HasValue () && *v.nonLocalizedValue == valueStr) {
                return v.keyVariant.guidValue;
            }
        } else if (v.displayVariant.uniStringValue == valueStr) {
            return v.keyVariant.guidValue;
        }
    }

    return APINULLGuid;
}

class PropertyConversionUtils : public API_PropertyConversionUtilsInterface
{
private:
    const GS::UniString degreeSymbol = L ("\u00B0");
    const GS::UniString minuteSymbol = "'";
    const GS::UniString secondSymbol = "\"";
    const GS::UniString gradientSymbol = "G";
    const GS::UniString radianSymbol = "R";
    const GS::UniString northSymbol = "N";
    const GS::UniString southSymbol = "S";
    const GS::UniString eastSymbol = "E";
    const GS::UniString westSymbol = "w";

public:
    PropertyConversionUtils () = default;
    virtual ~PropertyConversionUtils () = default;

    virtual const GS::UniString& GetDegreeSymbol1 () const { return degreeSymbol; }
    virtual const GS::UniString& GetDegreeSymbol2 () const { return degreeSymbol; }
    virtual const GS::UniString& GetMinuteSymbol () const { return minuteSymbol; }
    virtual const GS::UniString& GetSecondSymbol () const { return secondSymbol; }

    virtual const GS::UniString& GetGradientSymbol () const { return gradientSymbol; }
    virtual const GS::UniString& GetRadianSymbol () const { return radianSymbol; }

    virtual const GS::UniString& GetNorthSymbol () const { return northSymbol; }
    virtual const GS::UniString& GetSouthSymbol () const { return southSymbol; }
    virtual const GS::UniString& GetEastSymbol () const { return eastSymbol; }
    virtual const GS::UniString& GetWestSymbol () const { return westSymbol; }

    virtual GS::uchar_t GetDecimalDelimiterChar () const { return '.'; }
    virtual GS::Optional<GS::UniChar> GetThousandSeparatorChar () const { return ' '; }

    virtual API_LengthTypeID GetLengthType () const { return API_LengthTypeID::Meter; }
    virtual API_AreaTypeID GetAreaType () const { return API_AreaTypeID::SquareMeter; }
    virtual API_VolumeTypeID GetVolumeType () const { return API_VolumeTypeID::CubicMeter; }
    virtual API_AngleTypeID GetAngleType () const { return API_AngleTypeID::DecimalDegree; }
};

constexpr uint32_t PackTypes (API_PropertyCollectionType colType, API_VariantType valType)
{
    return (static_cast<uint32_t> (colType) << 16) | static_cast<uint32_t> (valType);
}

static GSErrCode GetPropertyValueString (const API_Property& propertyValue, GS::UniString& resultString)
{
    if (propertyValue.value.variantStatus == API_VariantStatusUserUndefined) {
        resultString = "<Undefined>";
        return NoError;
    }

    switch (PackTypes (propertyValue.definition.collectionType, propertyValue.definition.valueType)) {
        case PackTypes (API_PropertySingleCollectionType, API_PropertyStringValueType):
            resultString = propertyValue.value.singleVariant.variant.uniStringValue;
            return NoError;

        case PackTypes (API_PropertySingleCollectionType, API_PropertyBooleanValueType):
            resultString = propertyValue.value.singleVariant.variant.boolValue ? "True" : "False";
            return NoError;

        case PackTypes (API_PropertySingleCollectionType, API_PropertyIntegerValueType):
            resultString = GS::UniString::Printf ("%d", (int) propertyValue.value.singleVariant.variant.intValue);
            return NoError;

        case PackTypes (API_PropertySingleChoiceEnumerationCollectionType, API_PropertyStringValueType):
            {
                const API_Guid& selectedGuid = propertyValue.value.singleVariant.variant.guidValue;
                for (const API_SingleEnumerationVariant& enumVar : propertyValue.definition.possibleEnumValues) {
                    if (enumVar.keyVariant.guidValue == selectedGuid) {
                        resultString = enumVar.displayVariant.uniStringValue;
                        return NoError;
                    }
                }
                return ACAPI_Property_GetPropertyValueString (propertyValue, &resultString);
            }

        default:
            return ACAPI_Property_GetPropertyValueString (propertyValue, &resultString);
    }
}

GetAllPropertiesCommand::GetAllPropertiesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAllPropertiesCommand::GetName () const
{
    return "GetAllProperties";
}

GS::Optional<GS::UniString> GetAllPropertiesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "properties": {
                "type": "array",
                "description": "A list of property identifiers.",
                "items": {
                    "$ref": "#/PropertyDetails"
                }
            },
            "propertyGroups": {
                "type": "array",
                "description": "Every property group, including empty ones.",
                "items": {
                    "type": "object",
                    "properties": {
                        "propertyGroupId": { "$ref": "#/PropertyGroupId" },
                        "name": { "type": "string" },
                        "description": { "type": "string" },
                        "isCustom": { "type": "boolean" }
                    },
                    "additionalProperties": false,
                    "required": [ "propertyGroupId", "name", "isCustom" ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "properties"
        ]
    })";
}

GS::ObjectState GetAllPropertiesCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::ObjectState response;
    auto propertyAdder = response.AddList<GS::ObjectState> ("properties");

    GS::Array<API_PropertyGroup> groups;
    ACAPI_Property_GetPropertyGroups (groups);
    const auto& groupList = response.AddList<GS::ObjectState> ("propertyGroups");
    for (const API_PropertyGroup& group : groups) {
        GS::ObjectState groupDetails;
        groupDetails.Add ("propertyGroupId", CreateGuidObjectState (group.guid));
        groupDetails.Add ("name", group.name);
        groupDetails.Add ("description", group.description);
        groupDetails.Add ("isCustom", group.groupType == API_PropertyCustomGroupType);
        groupList (groupDetails);

        GS::Array<API_PropertyDefinition> definitions;
        ACAPI_Property_GetPropertyDefinitions (group.guid, definitions);
        for (const API_PropertyDefinition& definition : definitions) {
            GS::ObjectState details;

            GS::ObjectState propertyId;
            propertyId.Add ("guid", APIGuidToString (definition.guid));
            details.Add ("propertyId", propertyId);

            details.Add ("propertyType", GetPropertyTypeString (definition.definitionType));
            details.Add ("propertyGroupName", group.name);
            details.Add ("propertyName", definition.name);
            details.Add ("propertyCollectionType", GetPropertyTypeString (definition.collectionType));
            details.Add ("propertyValueType", GetPropertyTypeString (definition.valueType));
            details.Add ("propertyMeasureType", GetPropertyTypeString (definition.measureType));
            details.Add ("propertyIsEditable", definition.canValueBeEditable);
            details.Add ("isExpressionBased", definition.defaultValue.hasExpression);
            details.Add ("propertyGroupId", CreateGuidObjectState (group.guid));
            details.Add ("propertyDescription", definition.description);
            if (definition.definitionType == API_PropertyCustomDefinitionType) {
                if (!definition.defaultValue.hasExpression &&
                    definition.defaultValue.basicValue.variantStatus == API_VariantStatusNormal) {
                    API_Property defaultProperty;
                    defaultProperty.definition = definition;
                    defaultProperty.isDefault = true;
                    defaultProperty.status = API_Property_HasValue;
                    defaultProperty.value = definition.defaultValue.basicValue;
                    GS::UniString defaultString;
                    if (GetPropertyValueString (defaultProperty, defaultString) == NoError) {
                        details.Add ("defaultValueDisplay", defaultString);
                    }
                }
                const auto& availabilityList = details.AddList<GS::ObjectState> ("availability");
                for (const API_Guid& itemGuid : definition.availability) {
                    availabilityList (CreateIdObjectState ("classificationItemId", itemGuid));
                }
                // The option guids an enum default holds. The display text above joins the
                // options of a multi-choice default, and two options can share a text, so a
                // caller checking "is this option the default" needs the guids.
                if (!definition.defaultValue.hasExpression &&
                    definition.defaultValue.basicValue.variantStatus == API_VariantStatusNormal) {
                    if (definition.collectionType == API_PropertySingleChoiceEnumerationCollectionType) {
                        const auto& ids = details.AddList<GS::ObjectState> ("defaultEnumValueIds");
                        ids (CreateGuidObjectState (definition.defaultValue.basicValue.singleVariant.variant.guidValue));
                    } else if (definition.collectionType == API_PropertyMultipleChoiceEnumerationCollectionType) {
                        const auto& ids = details.AddList<GS::ObjectState> ("defaultEnumValueIds");
                        for (const API_Variant& v : definition.defaultValue.basicValue.listVariant.variants) {
                            ids (CreateGuidObjectState (v.guidValue));
                        }
                    }
                }
            }

            if (!definition.possibleEnumValues.IsEmpty ()) {
                const auto& enumValueList = details.AddList<GS::ObjectState> ("possibleEnumValues");
                for (const API_SingleEnumerationVariant& variant : definition.possibleEnumValues) {
                    enumValueList (CreateEnumValueObjectState (variant));
                }
            }
            if (definition.defaultValue.hasExpression) {
                const auto& expressionList = details.AddList<GS::UniString> ("expressions");
                for (const GS::UniString& expr : definition.defaultValue.propertyExpressions) {
                    expressionList (expr);
                }
            }

            propertyAdder (details);
        }
    }

    return response;
}

GetPropertyValuesOfElementsCommand::GetPropertyValuesOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetPropertyValuesOfElementsCommand::GetName () const
{
    return "GetPropertyValuesOfElements";
}

GS::Optional<GS::UniString> GetPropertyValuesOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "properties": {
                "$ref": "#/PropertyIds"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements",
            "properties"
        ]
    })";
}

GS::Optional<GS::UniString> GetPropertyValuesOfElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "propertyValuesForElements": {
                "$ref": "#/PropertyValuesOrErrorArray",
                "description": "List of property value lists. The order of the outer list is that of the given elements. The order of the inner lists are that of the given properties."
            }
        },
        "additionalProperties": false,
        "required": [
            "propertyValuesForElements"
        ]
    })";
}

GS::ObjectState GetPropertyValuesOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::Array<GS::ObjectState> properties;
    parameters.Get ("properties", properties);

    GS::ObjectState response;
    const auto& propertyValuesForElements = response.AddList<GS::ObjectState> ("propertyValuesForElements");

    GS::Array<API_Guid> propertyGuids;
    for (const GS::ObjectState& property : properties) {
        const GS::ObjectState* propertyId = property.Get ("propertyId");
        if (propertyId != nullptr) {
            const API_Guid propertyGuid = GetGuidFromObjectState (*propertyId);
            if (propertyGuid != APINULLGuid) {
                propertyGuids.Push (propertyGuid);
            }
        }
    }

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            propertyValuesForElements (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        const API_Guid elemGuid = GetGuidFromObjectState (*elementId);

        GS::Array<API_Property> fetchedProperties;
        GSErrCode err = ACAPI_Element_GetPropertyValuesByGuid (elemGuid, propertyGuids, fetchedProperties);
        if (err != NoError) {
            propertyValuesForElements (CreateErrorResponse (err, "Failed to get property values for element"));
            continue;
        }

        GS::HashTable<API_Guid, const API_Property*> propertyMap;
        for (const API_Property& prop : fetchedProperties) {
            if (!propertyMap.ContainsKey (prop.definition.guid)) {
                propertyMap.Add (prop.definition.guid, &prop);
            }
        }

        GS::ObjectState propertyValuesForElement;
        const auto& propertyValues = propertyValuesForElement.AddList<GS::ObjectState> ("propertyValues");

        for (const GS::ObjectState& property : properties) {
            const GS::ObjectState* propertyId = property.Get ("propertyId");
            if (propertyId == nullptr) {
                propertyValues (CreateErrorResponse (APIERR_BADPARS, "The 'propertyId' field is missing"));
                continue;
            }

            const API_Guid propertyGuid = GetGuidFromObjectState (*propertyId);

            if (!propertyMap.ContainsKey (propertyGuid)) {
                propertyValues (CreateErrorResponse (APIERR_BADPROPERTY, "Property not found or invalid"));
                continue;
            }

            const API_Property& propertyValue = *propertyMap[propertyGuid];
            if (propertyValue.status == API_Property_NotAvailable || propertyValue.status == API_Property_NotEvaluated) {
                propertyValues (CreateErrorResponse (APIERR_BADPROPERTY, "Not available or not evaluated property"));
                continue;
            }

            GS::UniString propertyValueString;
            err = GetPropertyValueString (propertyValue, propertyValueString);

            if (err != NoError) {
                propertyValues (CreateErrorResponse (err, "Failed to get property value as string"));
                continue;
            }

            propertyValues (GS::ObjectState ("propertyValue", GS::ObjectState ("value", propertyValueString)));
        }

        propertyValuesForElements (propertyValuesForElement);
    }

    return response;
}

SetPropertyValuesOfElementsCommand::SetPropertyValuesOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetPropertyValuesOfElementsCommand::GetName () const
{
    return "SetPropertyValuesOfElements";
}

GS::Optional<GS::UniString> SetPropertyValuesOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementPropertyValues": {
                "$ref": "#/ElementPropertyValues"
            }
        },
        "additionalProperties": false,
        "required": [
            "elementPropertyValues"
        ]
    })";
}

GS::Optional<GS::UniString> SetPropertyValuesOfElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    })";
}

GS::ObjectState SetPropertyValuesOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elementPropertyValues;
    parameters.Get ("elementPropertyValues", elementPropertyValues);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    GS::Array<GS::ObjectState> results (elementPropertyValues.GetSize ());
    GS::HashTable<GS::Pair<API_Guid, API_Guid>, GSIndex> resultIndices;
    GS::HashTable<API_Guid, GS::Array<API_Guid>> propertiesForElements;
    GS::HashTable<GS::Pair<API_Guid, API_Guid>, GS::UniString> propertyValuesForElements;

    PropertyConversionUtils conversionUtils;

    for (const GS::ObjectState& elementPropertyValue : elementPropertyValues) {
        const GS::ObjectState* elementId = elementPropertyValue.Get ("elementId");
        if (elementId == nullptr) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        const GS::ObjectState* propertyId = elementPropertyValue.Get ("propertyId");
        if (propertyId == nullptr) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "propertyId is missing"));
            continue;
        }

        const GS::ObjectState* propertyValue = elementPropertyValue.Get ("propertyValue");
        if (propertyValue == nullptr) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "propertyValue is missing"));
            continue;
        }

        GS::UniString propertyValueDisplayString;
        if (!propertyValue->Get ("value", propertyValueDisplayString)) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "value is missing from propertyValue"));
            continue;
        }

        const API_Guid elemGuid = GetGuidFromObjectState (*elementId);
        const API_Guid propertyGuid = GetGuidFromObjectState (*propertyId);

        GS::Array<API_Guid>* properties;
        propertiesForElements.Add (elemGuid, {}, &properties);
        properties->Push (propertyGuid);

        const auto guidPair = GS::NewPair (elemGuid, propertyGuid);
        propertyValuesForElements.Add (guidPair, propertyValueDisplayString);
        resultIndices.Add (guidPair, results.GetSize ());
        results.PushNew ();
    }

    ACAPI_CallUndoableCommand ("SetPropertyValuesOfElementsCommand", [&]() -> GSErrCode {
        for (const auto& kv : propertiesForElements) {
#ifdef ServerMainVers_2800
            const API_Guid& elemGuid = kv.key;
            const GS::Array<API_Guid>& properties = kv.value;
#else
            const API_Guid& elemGuid = *kv.key;
            const GS::Array<API_Guid>& properties = *kv.value;
#endif

            GS::Array<API_Property> propertyValues;
            const GSErrCode getErr = ACAPI_Element_GetPropertyValuesByGuid (elemGuid, properties, propertyValues);

            for (API_Property& propertyValue : propertyValues) {
                const auto guidPair = GS::NewPair (elemGuid, propertyValue.definition.guid);
                auto& result = results[resultIndices[guidPair]];

                if (getErr != NoError) {
                    result = CreateFailedExecutionResult (getErr, "Failed to get property values for element");
                    continue;
                }

                GSErrCode err = ACAPI_Property_SetPropertyValueFromString (propertyValuesForElements[guidPair], conversionUtils, &propertyValue);

                if (err != NoError) {
                    result = CreateFailedExecutionResult (err, "Failed to set property value for element");
                    continue;
                }

                err = ACAPI_Element_SetProperty (elemGuid, propertyValue);

                if (err != NoError) {
                    result = CreateFailedExecutionResult (err, "Failed to set property value for element");
                    continue;
                }

                result = CreateSuccessfulExecutionResult ();
            }
        }

        return NoError;
    });

    for (const GS::ObjectState& result : results) {
        executionResults (result);
    }

    return response;
}

GetPropertyValuesOfAttributesCommand::GetPropertyValuesOfAttributesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetPropertyValuesOfAttributesCommand::GetName () const
{
    return "GetPropertyValuesOfAttributes";
}

GS::Optional<GS::UniString> GetPropertyValuesOfAttributesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            },
            "properties": {
                "$ref": "#/PropertyIds"
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds",
            "properties"
        ]
    })";
}

GS::Optional<GS::UniString> GetPropertyValuesOfAttributesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "propertyValuesForAttributes": {
                "$ref": "#/PropertyValuesOrErrorArray",
                "description": "List of property value lists. The order of the outer list is that of the given attributes. The order of the inner lists are that of the given properties."
            }
        },
        "additionalProperties": false,
        "required": [
            "propertyValuesForAttributes"
        ]
    })";
}

GS::ObjectState GetPropertyValuesOfAttributesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> attributeIds;
    parameters.Get ("attributeIds", attributeIds);

    GS::Array<GS::ObjectState> properties;
    parameters.Get ("properties", properties);

    GS::ObjectState response;
    const auto& propertyValuesForAttributes = response.AddList<GS::ObjectState> ("propertyValuesForAttributes");

    for (const GS::ObjectState& attribute : attributeIds) {
        const GS::ObjectState* attributeId = attribute.Get ("attributeId");
        if (attributeId == nullptr) {
            propertyValuesForAttributes (CreateErrorResponse (APIERR_BADPARS, "attributeId is missing"));
            continue;
        }

        const API_Guid attGuid = GetGuidFromObjectState (*attributeId);

        GS::ObjectState propertyValuesForAttribute;
        const auto& propertyValues = propertyValuesForAttribute.AddList<GS::ObjectState> ("propertyValues");

        for (const GS::ObjectState& property : properties) {
            const GS::ObjectState* propertyId = property.Get ("propertyId");
            if (propertyId == nullptr) {
                propertyValues (CreateErrorResponse (APIERR_BADPARS, "propertyId is missing"));
                continue;
            }

            const API_Guid propertyGuid = GetGuidFromObjectState (*propertyId);

            API_Property propertyValue;
            API_Attr_Head attrHead = GetAttributeHeadFromGuid (attGuid);
            GSErrCode err = ACAPI_Attribute_GetPropertyValue (attrHead, propertyGuid, propertyValue);

            if (err != NoError) {
                propertyValues (CreateErrorResponse (err, "Failed to get property value"));
                continue;
            }

            if (propertyValue.status == API_Property_NotAvailable || propertyValue.status == API_Property_NotEvaluated) {
                propertyValues (CreateErrorResponse (APIERR_BADPROPERTY, "Not available or not evaluated property"));
                continue;
            }

            GS::UniString propertyValueString;
            err = ACAPI_Property_GetPropertyValueString (propertyValue, &propertyValueString);

            if (err != NoError) {
                propertyValues (CreateErrorResponse (err, "Failed to get property value as string"));
                continue;
            }

            propertyValues (GS::ObjectState ("propertyValue", GS::ObjectState ("value", propertyValueString)));
        }

        propertyValuesForAttributes (propertyValuesForAttribute);
    }

    return response;
}

SetPropertyValuesOfAttributesCommand::SetPropertyValuesOfAttributesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetPropertyValuesOfAttributesCommand::GetName () const
{
    return "SetPropertyValuesOfAttributes";
}

GS::Optional<GS::UniString> SetPropertyValuesOfAttributesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributePropertyValues": {
                "$ref": "#/AttributePropertyValues"
            }
        },
        "additionalProperties": false,
        "required": [
            "attributePropertyValues"
        ]
    })";
}

GS::Optional<GS::UniString> SetPropertyValuesOfAttributesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    })";
}

GS::ObjectState SetPropertyValuesOfAttributesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> attributePropertyValues;
    parameters.Get ("attributePropertyValues", attributePropertyValues);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    GS::Array<GS::ObjectState> results (attributePropertyValues.GetSize ());
    GS::HashTable<GS::Pair<API_Guid, API_Guid>, GSIndex> resultIndices;
    GS::HashTable<API_Guid, GS::Array<API_Guid>> propertiesForAttributes;
    GS::HashTable<GS::Pair<API_Guid, API_Guid>, GS::UniString> propertyValuesForAttributes;

    PropertyConversionUtils conversionUtils;

    for (const GS::ObjectState& attributePropertyValue : attributePropertyValues) {
        const GS::ObjectState* attributeId = attributePropertyValue.Get ("attributeId");
        if (attributeId == nullptr) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "attributeId is missing"));
            continue;
        }

        const GS::ObjectState* propertyId = attributePropertyValue.Get ("propertyId");
        if (propertyId == nullptr) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "propertyId is missing"));
            continue;
        }

        const GS::ObjectState* propertyValue = attributePropertyValue.Get ("propertyValue");
        if (propertyValue == nullptr) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "propertyValue is missing"));
            continue;
        }

        GS::UniString propertyValueDisplayString;
        if (!propertyValue->Get ("value", propertyValueDisplayString)) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "value is missing from propertyValue"));
            continue;
        }

        const API_Guid attGuid = GetGuidFromObjectState (*attributeId);
        const API_Guid propertyGuid = GetGuidFromObjectState (*propertyId);

        GS::Array<API_Guid>* properties;
        propertiesForAttributes.Add (attGuid, {}, &properties);
        properties->Push (propertyGuid);

        const auto guidPair = GS::NewPair (attGuid, propertyGuid);
        propertyValuesForAttributes.Add (guidPair, propertyValueDisplayString);
        resultIndices.Add (guidPair, results.GetSize ());
        results.PushNew ();
    }

    ACAPI_CallUndoableCommand ("SetPropertyValuesOfAttributesCommand", [&]() -> GSErrCode {
        for (const auto& kv : propertiesForAttributes) {
#ifdef ServerMainVers_2800
            const API_Guid& attGuid = kv.key;
            const GS::Array<API_Guid>& properties = kv.value;
#else
            const API_Guid& attGuid = *kv.key;
            const GS::Array<API_Guid>& properties = *kv.value;
#endif

            GS::Array<API_Property> propertyValues;

            API_Attr_Head attrHead = GetAttributeHeadFromGuid (attGuid);
            const GSErrCode getErr = ACAPI_Attribute_GetPropertyValuesByGuid (attrHead, properties, propertyValues);

            for (API_Property& propertyValue : propertyValues) {
                const auto guidPair = GS::NewPair (attGuid, propertyValue.definition.guid);
                auto& result = results[resultIndices[guidPair]];

                if (getErr != NoError) {
                    result = CreateFailedExecutionResult (getErr, "Failed to get property values for attribute");
                    continue;
                }

                GSErrCode err = ACAPI_Property_SetPropertyValueFromString (propertyValuesForAttributes[guidPair], conversionUtils, &propertyValue);

                if (err != NoError) {
                    result = CreateFailedExecutionResult (err, "Failed to set property value for attribute");
                    continue;
                }

                err = ACAPI_Attribute_SetProperty (attrHead, propertyValue);

                if (err != NoError) {
                    result = CreateFailedExecutionResult (err, "Failed to set property value for attribute");
                    continue;
                }

                result = CreateSuccessfulExecutionResult ();
            }
        }

        return NoError;
    });

    for (const GS::ObjectState& result : results) {
        executionResults (result);
    }

    return response;
}

CreatePropertyGroupsCommand::CreatePropertyGroupsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreatePropertyGroupsCommand::GetName () const
{
    return "CreatePropertyGroups";
}

GS::Optional<GS::UniString> CreatePropertyGroupsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "propertyGroups": {
                "type": "array",
                "description": "The parameters of the new property groups.",
                "items": {
                    "$ref": "#/PropertyGroupArrayItem"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "propertyGroups"
        ]
    })";
}

GS::Optional<GS::UniString> CreatePropertyGroupsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "propertyGroupIds": {
                "type": "array",
                "description": "The identifiers of the created property groups.",
                "items": {
                    "$ref": "#/PropertyGroupIdArrayItem"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "propertyGroupIds"
        ]
    })";
}

GS::ObjectState CreatePropertyGroupsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> propertyGroups;
    parameters.Get ("propertyGroups", propertyGroups);

    GS::ObjectState response;
    const auto& propertyGroupIds = response.AddList<GS::ObjectState> ("propertyGroupIds");

    ACAPI_CallUndoableCommand ("CreatePropertyGroups", [&]() -> GSErrCode {
        for (const GS::ObjectState& g : propertyGroups) {
            const GS::ObjectState* propertyGroup = g.Get ("propertyGroup");
            if (propertyGroup == nullptr) {
                propertyGroupIds (CreateErrorResponse (APIERR_BADPARS, "propertyGroup is missing"));
                continue;
            }

            API_PropertyGroup apiPropertyGroup;
            if (!propertyGroup->Get ("name", apiPropertyGroup.name) || apiPropertyGroup.name.IsEmpty ()) {
                propertyGroupIds (CreateErrorResponse (APIERR_BADPARS, "name is missing or empty"));
                continue;
            }

            propertyGroup->Get ("description", apiPropertyGroup.description);
            GSErrCode err = ACAPI_Property_CreatePropertyGroup (apiPropertyGroup);
            if (err != NoError) {
                propertyGroupIds (CreateErrorResponse (err, "failed to create the property group"));
                continue;
            }

            propertyGroupIds (CreateIdObjectState ("propertyGroupId", apiPropertyGroup.guid));
        }

        return NoError;
    });

    return response;
}

DeletePropertyGroupsCommand::DeletePropertyGroupsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeletePropertyGroupsCommand::GetName () const
{
    return "DeletePropertyGroups";
}

GS::Optional<GS::UniString> DeletePropertyGroupsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "propertyGroupIds": {
                "type": "array",
                "description": "The identifiers of property groups to delete.",
                "items": {
                    "$ref": "#/PropertyGroupIdArrayItem"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "propertyGroupIds"
        ]
    })";
}

GS::Optional<GS::UniString> DeletePropertyGroupsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    })";
}

GS::ObjectState DeletePropertyGroupsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> propertyGroupIds;
    parameters.Get ("propertyGroupIds", propertyGroupIds);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("DeletePropertyGroups", [&]() -> GSErrCode {
        for (const GS::ObjectState& p : propertyGroupIds) {
            const GS::ObjectState* propertyGroupId = p.Get ("propertyGroupId");
            if (propertyGroupId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "propertyGroupId is missing"));
                continue;
            }

            GSErrCode err = ACAPI_Property_DeletePropertyGroup (GetGuidFromObjectState (*propertyGroupId));
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "failed to delete property group"));
                continue;
            }

            executionResults (CreateSuccessfulExecutionResult ());
        }

        return NoError;
    });

    return response;
}

// Fills definition.defaultValue from a PropertyDefaultValue object: {expressions: [...]}
// or {basicDefaultValue: {...}}. Enum defaults are resolved against
// definition.possibleEnumValues, so apply enum edits before calling this.
static bool ParseDefaultValue (const GS::ObjectState& defaultValue, API_PropertyDefinition& definition, GS::UniString& error)
{
    GS::Array<GS::UniString> expressions;
    defaultValue.Get ("expressions", expressions);
    if (!expressions.IsEmpty ()) {
        definition.defaultValue.hasExpression = true;
        definition.defaultValue.propertyExpressions = expressions;
        return true;
    }

    const GS::ObjectState* basicDefaultValue = defaultValue.Get ("basicDefaultValue");
    if (basicDefaultValue == nullptr) {
        error = "both defaultValue/basicDefaultValue and defaultValue/expressions are missing or empty";
        return false;
    }

    definition.defaultValue.hasExpression = false;
    definition.defaultValue.propertyExpressions.Clear ();
    definition.defaultValue.basicValue = API_PropertyValue ();
    definition.defaultValue.basicValue.singleVariant.variant.type = definition.valueType;

    GS::UniString statusStr;
    if (!basicDefaultValue->Get ("status", statusStr) || statusStr.IsEmpty ()) {
        error = "defaultValue/basicDefaultValue/status is missing or empty";
        return false;
    }

    if (statusStr == "normal") {
        definition.defaultValue.basicValue.variantStatus = API_VariantStatusNormal;
        switch (definition.collectionType) {
            case API_PropertySingleCollectionType:
                switch (definition.valueType) {
                    case API_PropertyRealValueType:
                        basicDefaultValue->Get ("value", definition.defaultValue.basicValue.singleVariant.variant.doubleValue);
                        break;
                    case API_PropertyIntegerValueType:
                        basicDefaultValue->Get ("value", definition.defaultValue.basicValue.singleVariant.variant.intValue);
                        break;
                    case API_PropertyStringValueType:
                        basicDefaultValue->Get ("value", definition.defaultValue.basicValue.singleVariant.variant.uniStringValue);
                        break;
                    case API_PropertyBooleanValueType:
                        basicDefaultValue->Get ("value", definition.defaultValue.basicValue.singleVariant.variant.boolValue);
                        break;
                    case API_PropertyGuidValueType:
                        {
                            GS::String guidStr;
                            basicDefaultValue->Get ("value", guidStr);
                            definition.defaultValue.basicValue.singleVariant.variant.guidValue = APIGuidFromString (guidStr.ToCStr ());
                        }
                        break;
                }
                break;
            case API_PropertySingleChoiceEnumerationCollectionType:
                {
                    const GS::ObjectState* enumValueId = basicDefaultValue->Get ("value");
                    if (enumValueId == nullptr) {
                        error = "defaultValue/basicDefaultValue/value is missing";
                        return false;
                    }

                    GS::UniString enumValueIdTypeStr;
                    enumValueId->Get ("type", enumValueIdTypeStr);
                    if (enumValueIdTypeStr.IsEmpty ()) {
                        error = "defaultValue/basicDefaultValue/value/type is missing or empty";
                        return false;
                    }

                    definition.defaultValue.basicValue.singleVariant.variant.type = API_PropertyGuidValueType;
                    GS::UniString valueStr;
                    enumValueId->Get (enumValueIdTypeStr.ToCStr ().Get (), valueStr);
                    definition.defaultValue.basicValue.singleVariant.variant.guidValue = FindEnumValueGuid (definition.possibleEnumValues, enumValueIdTypeStr, valueStr);
                }
                break;
            case API_PropertyMultipleChoiceEnumerationCollectionType:
                {
                    GS::Array<GS::ObjectState> enumValueIds;
                    basicDefaultValue->Get ("value", enumValueIds);
                    if (enumValueIds.IsEmpty ()) {
                        error = "defaultValue/basicDefaultValue/value is missing or empty";
                        return false;
                    }
                    API_Variant v;
                    v.type = API_PropertyGuidValueType;
                    for (UIndex i = 0; i < enumValueIds.GetSize (); ++i) {
                        const GS::ObjectState* enumValueId = enumValueIds[i].Get ("enumValueId");
                        if (enumValueId == nullptr) {
                            error = GS::UniString::Printf ("defaultValue/basicDefaultValue/value[%d]/enumValueId is missing or empty", i);
                            return false;
                        }

                        GS::UniString enumValueIdTypeStr;
                        enumValueId->Get ("type", enumValueIdTypeStr);
                        if (enumValueIdTypeStr.IsEmpty ()) {
                            error = GS::UniString::Printf ("defaultValue/basicDefaultValue/value[%d]/enumValueId/type is missing or empty", i);
                            return false;
                        }

                        GS::UniString valueStr;
                        enumValueId->Get (enumValueIdTypeStr.ToCStr ().Get (), valueStr);
                        v.guidValue = FindEnumValueGuid (definition.possibleEnumValues, enumValueIdTypeStr, valueStr);
                        if (v.guidValue == APINULLGuid) {
                            error = GS::UniString::Printf ("defaultValue/basicDefaultValue/value[%d]/enumValueId/%T is missing or invalid", i, enumValueIdTypeStr.ToPrintf ());
                            return false;
                        }
                        definition.defaultValue.basicValue.listVariant.variants.Push (v);
                    }
                }
                break;
            case API_PropertyListCollectionType:
                switch (definition.valueType) {
                    case API_PropertyRealValueType:
                        {
                            GS::Array<double> doubleValues;
                            basicDefaultValue->Get ("value", doubleValues);
                            for (double d : doubleValues) {
                                API_Variant v;
                                v.type = definition.valueType;
                                v.doubleValue = d;
                                definition.defaultValue.basicValue.listVariant.variants.Push (v);
                            }
                        }
                        break;
                    case API_PropertyIntegerValueType:
                        {
                            GS::Array<int> intValues;
                            basicDefaultValue->Get ("value", intValues);
                            for (int i : intValues) {
                                API_Variant v;
                                v.type = definition.valueType;
                                v.intValue = i;
                                definition.defaultValue.basicValue.listVariant.variants.Push (v);
                            }
                        }
                        break;
                    case API_PropertyStringValueType:
                        {
                            GS::Array<GS::UniString> uniStringValues;
                            basicDefaultValue->Get ("value", uniStringValues);
                            for (GS::UniString s : uniStringValues) {
                                API_Variant v;
                                v.type = definition.valueType;
                                v.uniStringValue = s;
                                definition.defaultValue.basicValue.listVariant.variants.Push (v);
                            }
                        }
                        break;
                    case API_PropertyBooleanValueType:
                        {
                            GS::Array<bool> boolValues;
                            basicDefaultValue->Get ("value", boolValues);
                            for (bool b : boolValues) {
                                API_Variant v;
                                v.type = definition.valueType;
                                v.boolValue = b;
                                definition.defaultValue.basicValue.listVariant.variants.Push (v);
                            }
                        }
                        break;
                }
                break;
        }
    } else if (statusStr == "userUndefined") {
        definition.defaultValue.basicValue.variantStatus = API_VariantStatusUserUndefined;
    } else {
        definition.defaultValue.basicValue.variantStatus = API_VariantStatusNull;
    }
    return true;
}

CreatePropertyDefinitionsCommand::CreatePropertyDefinitionsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreatePropertyDefinitionsCommand::GetName () const
{
    return "CreatePropertyDefinitions";
}

GS::Optional<GS::UniString> CreatePropertyDefinitionsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "propertyDefinitions": {
                "type": "array",
                "description": "The parameters of the new properties.",
                "items": {
                    "$ref" : "#/PropertyDefinitionArrayItem"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "propertyDefinitions"
        ]
    })";
}

GS::Optional<GS::UniString> CreatePropertyDefinitionsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "propertyIds": {
                "$ref" : "#/PropertyIdOrErrorArray"
            }
        },
        "additionalProperties": false,
        "required": [
            "propertyIds"
        ]
    })";
}

GS::ObjectState CreatePropertyDefinitionsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> propertyDefinitions;
    parameters.Get ("propertyDefinitions", propertyDefinitions);

    GS::ObjectState response;
    const auto& propertyIds = response.AddList<GS::ObjectState> ("propertyIds");

    ACAPI_CallUndoableCommand ("CreatePropertyDefinitions", [&]() -> GSErrCode {
        for (const GS::ObjectState& p : propertyDefinitions) {
            const GS::ObjectState* propertyDefinition = p.Get ("propertyDefinition");
            if (propertyDefinition == nullptr) {
                propertyIds (CreateErrorResponse (APIERR_BADPARS, "property is missing"));
                continue;
            }

            API_PropertyDefinition apiPropertyDefinition;
            apiPropertyDefinition.definitionType = API_PropertyCustomDefinitionType;

            const GS::ObjectState* group = propertyDefinition->Get ("group");
            if (group == nullptr) {
                propertyIds (CreateErrorResponse (APIERR_BADPARS, "group is missing"));
                continue;
            }
            const GS::ObjectState* groupGuid = group->Get ("propertyGroupId");
            if (groupGuid != nullptr) {
                apiPropertyDefinition.groupGuid = GetGuidFromObjectState (*groupGuid);
            } else {
                GS::UniString groupName;
                if (group->Get ("name", groupName) && !groupName.IsEmpty ()) {
                    GS::Array<API_PropertyGroup> groups;
                    ACAPI_Property_GetPropertyGroups (groups);
                    for (const auto& group : groups) {
                        if (group.name == groupName) {
                            apiPropertyDefinition.groupGuid = group.guid;
                            break;
                        }
                    }
                }
            }
            if (apiPropertyDefinition.groupGuid == APINULLGuid) {
                propertyIds (CreateErrorResponse (APIERR_BADPARS, "both group/name and group/propertyGroupId are missing or invalid"));
                continue;
            }

            if (!propertyDefinition->Get ("name", apiPropertyDefinition.name) || apiPropertyDefinition.name.IsEmpty ()) {
                propertyIds (CreateErrorResponse (APIERR_BADPARS, "name is missing or empty"));
                continue;
            }

            propertyDefinition->Get ("description", apiPropertyDefinition.description);

            GS::UniString typeStr;
            if (!propertyDefinition->Get ("type", typeStr) || typeStr.IsEmpty ()) {
                propertyIds (CreateErrorResponse (APIERR_BADPARS, "type is missing or empty"));
                continue;
            }
            const PropertyTypeTuple* typeTuple = PropertyTypeDictionary.GetPtr (typeStr);
            if (typeTuple == nullptr) {
                propertyIds (CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("invalid type '%T'", typeStr.ToPrintf ())));
                continue;
            }
            apiPropertyDefinition.collectionType = std::get<0> (*typeTuple);
            apiPropertyDefinition.valueType = std::get<1> (*typeTuple);
            apiPropertyDefinition.measureType = std::get<2> (*typeTuple);

            GS::Array<GS::ObjectState> availability;
            if (propertyDefinition->Get ("availability", availability)) {
                for (const GS::ObjectState& c : availability) {
                    const GS::ObjectState* classificationItemId = c.Get ("classificationItemId");
                    if (classificationItemId == nullptr) {
                        continue;
                    }
                    apiPropertyDefinition.availability.Push (GetGuidFromObjectState (*classificationItemId));
                }
            }

            GS::Array<GS::ObjectState> possibleEnumValues;
            if (propertyDefinition->Get ("possibleEnumValues", possibleEnumValues)) {
                API_SingleEnumerationVariant v;
                v.displayVariant.type = apiPropertyDefinition.valueType;
                v.keyVariant.type = API_PropertyGuidValueType;
                for (const GS::ObjectState& e : possibleEnumValues) {
                    const GS::ObjectState* enumValue = e.Get ("enumValue");
                    if (enumValue == nullptr) {
                        continue;
                    }
                    v.keyVariant.guidValue = GetRandomGuid ();

                    if (!enumValue->Get ("displayValue", v.displayVariant.uniStringValue)) {
                        continue;
                    }

                    GS::UniString nonLocalizedValueStr;
                    if (enumValue->Get ("nonLocalizedValue", nonLocalizedValueStr)) {
                        v.nonLocalizedValue = nonLocalizedValueStr;
                    }

                    apiPropertyDefinition.possibleEnumValues.Push (v);
                }
            }

            apiPropertyDefinition.defaultValue.hasExpression = false;
            apiPropertyDefinition.defaultValue.basicValue.singleVariant.variant.type = apiPropertyDefinition.valueType;
            apiPropertyDefinition.defaultValue.basicValue.variantStatus = API_VariantStatusNull;
            const GS::ObjectState* defaultValue = propertyDefinition->Get ("defaultValue");
            if (defaultValue != nullptr) {
                GS::UniString defaultError;
                if (!ParseDefaultValue (*defaultValue, apiPropertyDefinition, defaultError)) {
                    propertyIds (CreateErrorResponse (APIERR_BADPARS, defaultError));
                    continue;
                }
            }

            propertyDefinition->Get ("isEditable", apiPropertyDefinition.canValueBeEditable);

            GSErrCode err = ACAPI_Property_CreatePropertyDefinition (apiPropertyDefinition);
            if (err != NoError) {
                propertyIds (CreateErrorResponse (err, "failed to create the property"));
                continue;
            }

            propertyIds (CreateIdObjectState ("propertyId", apiPropertyDefinition.guid));
        }

        return NoError;
    });

    return response;
}

DeletePropertyDefinitionsCommand::DeletePropertyDefinitionsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeletePropertyDefinitionsCommand::GetName () const
{
    return "DeletePropertyDefinitions";
}

GS::Optional<GS::UniString> DeletePropertyDefinitionsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "propertyIds": {
                "type": "array",
                "description": "The identifiers of properties to delete.",
                "items": {
                    "$ref": "#/PropertyIdArrayItem"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "propertyIds"
        ]
    })";
}

GS::Optional<GS::UniString> DeletePropertyDefinitionsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    })";
}

GS::ObjectState DeletePropertyDefinitionsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> propertyIds;
    parameters.Get ("propertyIds", propertyIds);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("DeletePropertyDefinitions", [&]() -> GSErrCode {
        for (const GS::ObjectState& p : propertyIds) {
            const GS::ObjectState* propertyId = p.Get ("propertyId");
            if (propertyId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "propertyId is missing"));
                continue;
            }

            GSErrCode err = ACAPI_Property_DeletePropertyDefinition (GetGuidFromObjectState (*propertyId));
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "failed to delete property"));
                continue;
            }

            executionResults (CreateSuccessfulExecutionResult ());
        }

        return NoError;
    });

    return response;
}

static bool ApplyEnumEdits (const GS::ObjectState& item, API_PropertyDefinition& definition, GS::UniString& error)
{
    GS::Array<GS::ObjectState> renames, removes, adds;
    GS::Array<GS::UniString> order;
    const bool hasRenames = item.Get ("renameEnumValues", renames);
    const bool hasRemoves = item.Get ("removeEnumValues", removes);
    const bool hasAdds = item.Get ("possibleEnumValues", adds);
    const bool hasOrder = item.Get ("enumOrder", order);
    if (!hasRenames && !hasRemoves && !hasAdds && !hasOrder) {
        return true;
    }

    if (definition.collectionType != API_PropertySingleChoiceEnumerationCollectionType &&
        definition.collectionType != API_PropertyMultipleChoiceEnumerationCollectionType) {
        error = "property is not an enumeration";
        return false;
    }

    auto indexOfGuid = [&] (const API_Guid& guid) -> Int32 {
        for (UIndex i = 0; i < definition.possibleEnumValues.GetSize (); ++i) {
            if (definition.possibleEnumValues[i].keyVariant.guidValue == guid) {
                return (Int32) i;
            }
        }
        return -1;
    };

    // Renames keep the option's guid, so element values holding it follow the new text.
    for (const GS::ObjectState& r : renames) {
        const GS::ObjectState* enumValueId = r.Get ("enumValueId");
        GS::UniString displayValue;
        if (enumValueId == nullptr || !r.Get ("displayValue", displayValue)) {
            error = "a renameEnumValues entry needs enumValueId and displayValue";
            return false;
        }
        const Int32 i = indexOfGuid (GetGuidFromObjectState (*enumValueId));
        if (i < 0) {
            error = "renameEnumValues: enumValueId is not an option of this property";
            return false;
        }
        definition.possibleEnumValues[i].displayVariant.uniStringValue = displayValue;
        GS::UniString nonLocalizedValue;
        if (r.Get ("nonLocalizedValue", nonLocalizedValue)) {
            definition.possibleEnumValues[i].nonLocalizedValue = nonLocalizedValue;
        }
    }

    for (const GS::ObjectState& r : removes) {
        const GS::ObjectState* enumValueId = r.Get ("enumValueId");
        if (enumValueId == nullptr) {
            error = "a removeEnumValues entry needs enumValueId";
            return false;
        }
        const Int32 i = indexOfGuid (GetGuidFromObjectState (*enumValueId));
        if (i < 0) {
            error = "removeEnumValues: enumValueId is not an option of this property";
            return false;
        }
        definition.possibleEnumValues.Delete ((UIndex) i);
    }

    // Appending is the 1.5.4 behaviour, kept as it was: a value already on the property,
    // matched by nonLocalizedValue or displayValue, is not added twice.
    for (const GS::ObjectState& e : adds) {
        const GS::ObjectState* enumValue = e.Get ("enumValue");
        if (enumValue == nullptr) {
            error = "an enumValue is missing or has no displayValue";
            return false;
        }
        API_SingleEnumerationVariant variant;
        variant.displayVariant.type = definition.valueType;
        variant.keyVariant.type = API_PropertyGuidValueType;
        if (!enumValue->Get ("displayValue", variant.displayVariant.uniStringValue)) {
            error = "an enumValue is missing or has no displayValue";
            return false;
        }
        GS::UniString nonLocalizedValueStr;
        if (enumValue->Get ("nonLocalizedValue", nonLocalizedValueStr)) {
            variant.nonLocalizedValue = nonLocalizedValueStr;
        }
        // Matched on both keys, not just the one the incoming value happens to carry: a
        // value already on the property as {displayValue: "Door"} which is sent again with
        // a nonLocalizedValue added has to match the existing entry.
        const bool alreadyThere =
            (variant.nonLocalizedValue.HasValue () &&
             FindEnumValueGuid (definition.possibleEnumValues, "nonLocalizedValue", *variant.nonLocalizedValue) != APINULLGuid) ||
            FindEnumValueGuid (definition.possibleEnumValues, "displayValue", variant.displayVariant.uniStringValue) != APINULLGuid;
        if (alreadyThere) {
            continue;
        }
        variant.keyVariant.guidValue = GetRandomGuid ();
        definition.possibleEnumValues.Push (variant);
    }

    // enumOrder is by display text after the edits above, and must name every option once.
    if (hasOrder) {
        if (order.GetSize () != definition.possibleEnumValues.GetSize ()) {
            error = "enumOrder must list every option exactly once";
            return false;
        }
        GS::Array<API_SingleEnumerationVariant> reordered;
        for (const GS::UniString& text : order) {
            Int32 found = -1;
            for (UIndex i = 0; i < definition.possibleEnumValues.GetSize (); ++i) {
                if (definition.possibleEnumValues[i].displayVariant.uniStringValue == text) {
                    if (found >= 0) {
                        error = "enumOrder: two options share the text '" + text + "'";
                        return false;
                    }
                    found = (Int32) i;
                }
            }
            if (found < 0) {
                error = "enumOrder: '" + text + "' is not an option of this property";
                return false;
            }
            for (const API_SingleEnumerationVariant& already : reordered) {
                if (already.keyVariant.guidValue == definition.possibleEnumValues[found].keyVariant.guidValue) {
                    error = "enumOrder must list every option exactly once";
                    return false;
                }
            }
            reordered.Push (definition.possibleEnumValues[found]);
        }
        definition.possibleEnumValues = reordered;
    }

    return true;
}

static bool ApplyAvailabilityEdits (const GS::ObjectState& item, API_PropertyDefinition& definition, GS::UniString& error)
{
    const GS::ObjectState* availability = item.Get ("availability");
    if (availability == nullptr) {
        return true;
    }

    GS::Array<GS::ObjectState> set, add, remove;
    const bool hasSet = availability->Get ("set", set);
    const bool hasAdd = availability->Get ("add", add);
    const bool hasRemove = availability->Get ("remove", remove);
    if (hasSet && (hasAdd || hasRemove)) {
        error = "availability takes either set, or add and/or remove";
        return false;
    }

    auto guidsOf = [] (const GS::Array<GS::ObjectState>& list) {
        GS::Array<API_Guid> guids;
        for (const GS::ObjectState& os : list) {
            const GS::ObjectState* id = os.Get ("classificationItemId");
            if (id != nullptr) {
                guids.Push (GetGuidFromObjectState (*id));
            }
        }
        return guids;
    };

    if (hasSet) {
        definition.availability = guidsOf (set);
        return true;
    }

    for (const API_Guid& guid : guidsOf (add)) {
        if (!definition.availability.Contains (guid)) {
            definition.availability.Push (guid);
        }
    }
    const GS::Array<API_Guid> toRemove = guidsOf (remove);
    GS::Array<API_Guid> kept;
    for (const API_Guid& guid : definition.availability) {
        if (!toRemove.Contains (guid)) {
            kept.Push (guid);
        }
    }
    definition.availability = kept;
    return true;
}

UpdatePropertyDefinitionsCommand::UpdatePropertyDefinitionsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String UpdatePropertyDefinitionsCommand::GetName () const
{
    return "UpdatePropertyDefinitions";
}

GS::Optional<GS::UniString> UpdatePropertyDefinitionsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "propertyDefinitions": {
                "type": "array",
                "description": "The property definitions to update. Only the fields given change; the definition keeps its guid, so element values survive.",
                "items": {
                    "type": "object",
                    "properties": {
                        "propertyId": { "$ref": "#/PropertyId" },
                        "name": { "type": "string", "description": "New name of the property." },
                        "description": { "type": "string", "description": "New description of the property." },
                        "groupId": { "$ref": "#/PropertyGroupId", "description": "Move the property into this custom property group." },
                        "defaultValue": { "$ref": "#/PropertyDefaultValue", "description": "New default value: a basic value or expressions. Switching between the two is allowed." },
                        "expressions": {
                            "type": "array",
                            "description": "The new expression strings for the property. Only for expression-based properties.",
                            "items": { "type": "string" },
                            "minItems": 1
                        },
                        "availability": {
                            "type": "object",
                            "description": "Classification items the property is available for: set replaces the list, add and remove edit it.",
                            "properties": {
                                "set": { "type": "array", "items": { "$ref": "#/ClassificationItemIdArrayItem" } },
                                "add": { "type": "array", "items": { "$ref": "#/ClassificationItemIdArrayItem" } },
                                "remove": { "type": "array", "items": { "$ref": "#/ClassificationItemIdArrayItem" } }
                            },
                            "additionalProperties": false
                        },
                        "possibleEnumValues": {
                            "$ref": "#/EnumValuesToAdd",
                            "description": "The enum values to add to an enumeration property. Values already on the property keep their identifier, so element values assigned to them survive; values not listed here are kept as well."
                        },
                        "renameEnumValues": {
                            "type": "array",
                            "description": "Change the text of existing enum options. The option keeps its identifier, so element values follow the new text.",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "enumValueId": { "type": "object", "properties": { "guid": { "$ref": "#/Guid" } }, "additionalProperties": false, "required": [ "guid" ] },
                                    "displayValue": { "type": "string" },
                                    "nonLocalizedValue": { "type": "string" }
                                },
                                "additionalProperties": false,
                                "required": [ "enumValueId", "displayValue" ]
                            }
                        },
                        "removeEnumValues": {
                            "type": "array",
                            "description": "Enum options to remove. Elements holding a removed option lose that value.",
                            "items": {
                                "type": "object",
                                "properties": { "enumValueId": { "type": "object", "properties": { "guid": { "$ref": "#/Guid" } }, "additionalProperties": false, "required": [ "guid" ] } },
                                "additionalProperties": false,
                                "required": [ "enumValueId" ]
                            }
                        },
                        "enumOrder": {
                            "type": "array",
                            "description": "Every option's display text, once, in the new order (applied after rename, remove and add).",
                            "items": { "type": "string" }
                        }
                    },
                    "additionalProperties": false,
                    "required": [ "propertyId" ]
                }
            }
        },
        "additionalProperties": false,
        "required": [ "propertyDefinitions" ]
    })";
}

GS::Optional<GS::UniString> UpdatePropertyDefinitionsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    })";
}

GS::ObjectState UpdatePropertyDefinitionsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> propertyDefinitions;
    parameters.Get ("propertyDefinitions", propertyDefinitions);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("UpdatePropertyDefinitions", [&]() -> GSErrCode {
        for (const GS::ObjectState& item : propertyDefinitions) {
            const GS::ObjectState* propertyId = item.Get ("propertyId");
            if (propertyId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "propertyId is missing"));
                continue;
            }

            API_PropertyDefinition definition;
            definition.guid = GetGuidFromObjectState (*propertyId);
            if (ACAPI_Property_GetPropertyDefinition (definition) != NoError) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "property not found"));
                continue;
            }
            if (definition.definitionType != API_PropertyCustomDefinitionType) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "built-in properties cannot be changed"));
                continue;
            }

            item.Get ("name", definition.name);
            item.Get ("description", definition.description);
            const GS::ObjectState* groupId = item.Get ("groupId");
            if (groupId != nullptr) {
                definition.groupGuid = GetGuidFromObjectState (*groupId);
            }

            GS::UniString error;
            if (!ApplyEnumEdits (item, definition, error) || !ApplyAvailabilityEdits (item, definition, error)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, error));
                continue;
            }

            GS::Array<GS::UniString> expressions;
            const bool hasExpressions = item.Get ("expressions", expressions);
            const GS::ObjectState* defaultValue = item.Get ("defaultValue");
            if (hasExpressions && defaultValue != nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "send expressions or defaultValue, not both"));
                continue;
            }
            if (hasExpressions) {
                if (!definition.defaultValue.hasExpression) {
                    executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "property is not expression-based"));
                    continue;
                }
                definition.defaultValue.propertyExpressions = expressions;
            }
            if (defaultValue != nullptr && !ParseDefaultValue (*defaultValue, definition, error)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, error));
                continue;
            }

            // A removed option can still be the default. Archicad would answer APIERR_BADVALUE;
            // say which rule was broken instead.
            if (!definition.defaultValue.hasExpression &&
                definition.defaultValue.basicValue.variantStatus == API_VariantStatusNormal &&
                (definition.collectionType == API_PropertySingleChoiceEnumerationCollectionType ||
                 definition.collectionType == API_PropertyMultipleChoiceEnumerationCollectionType)) {
                auto isOption = [&] (const API_Guid& guid) {
                    for (const API_SingleEnumerationVariant& v : definition.possibleEnumValues) {
                        if (v.keyVariant.guidValue == guid) {
                            return true;
                        }
                    }
                    return false;
                };
                bool defaultStillThere = true;
                if (definition.collectionType == API_PropertySingleChoiceEnumerationCollectionType) {
                    defaultStillThere = isOption (definition.defaultValue.basicValue.singleVariant.variant.guidValue);
                } else {
                    for (const API_Variant& v : definition.defaultValue.basicValue.listVariant.variants) {
                        defaultStillThere = defaultStillThere && isOption (v.guidValue);
                    }
                }
                if (!defaultStillThere) {
                    executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "the default value is an option being removed; send a new defaultValue in the same item"));
                    continue;
                }
            }

            const GSErrCode err = ACAPI_Property_ChangePropertyDefinition (definition);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, DescribeDefinitionChangeError (err)));
                continue;
            }
            executionResults (CreateSuccessfulExecutionResult ());
        }
        return NoError;
    });

    return response;
}

UpdatePropertyGroupsCommand::UpdatePropertyGroupsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String UpdatePropertyGroupsCommand::GetName () const
{
    return "UpdatePropertyGroups";
}

GS::Optional<GS::UniString> UpdatePropertyGroupsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "propertyGroups": {
                "type": "array",
                "description": "The custom property groups to update. Only the fields given change.",
                "items": {
                    "type": "object",
                    "properties": {
                        "propertyGroupId": { "$ref": "#/PropertyGroupId" },
                        "name": { "type": "string" },
                        "description": { "type": "string" }
                    },
                    "additionalProperties": false,
                    "required": [ "propertyGroupId" ]
                }
            }
        },
        "additionalProperties": false,
        "required": [ "propertyGroups" ]
    })";
}

GS::Optional<GS::UniString> UpdatePropertyGroupsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": { "executionResults": { "$ref": "#/ExecutionResults" } },
        "additionalProperties": false,
        "required": [ "executionResults" ]
    })";
}

GS::ObjectState UpdatePropertyGroupsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> propertyGroups;
    parameters.Get ("propertyGroups", propertyGroups);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("UpdatePropertyGroups", [&]() -> GSErrCode {
        for (const GS::ObjectState& item : propertyGroups) {
            const GS::ObjectState* groupId = item.Get ("propertyGroupId");
            if (groupId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "propertyGroupId is missing"));
                continue;
            }
            API_PropertyGroup group;
            group.guid = GetGuidFromObjectState (*groupId);
            if (ACAPI_Property_GetPropertyGroup (group) != NoError) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, "property group not found"));
                continue;
            }
            if (group.groupType != API_PropertyCustomGroupType) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "built-in property groups cannot be changed"));
                continue;
            }
            item.Get ("name", group.name);
            item.Get ("description", group.description);
            const GSErrCode err = ACAPI_Property_ChangePropertyGroup (group);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, DescribeDefinitionChangeError (err)));
                continue;
            }
            executionResults (CreateSuccessfulExecutionResult ());
        }
        return NoError;
    });

    return response;
}

// The import calls return only an error code. What they did is read off the
// definitions before and after: a guid-preserving replace is in neither list.
static GS::HashSet<API_Guid> AllCustomPropertyGuids ()
{
    GS::HashSet<API_Guid> guids;
    GS::Array<API_PropertyGroup> groups;
    ACAPI_Property_GetPropertyGroups (groups);
    for (const API_PropertyGroup& group : groups) {
        GS::Array<API_PropertyDefinition> definitions;
        ACAPI_Property_GetPropertyDefinitions (group.guid, definitions);
        for (const API_PropertyDefinition& d : definitions) {
            if (d.definitionType == API_PropertyCustomDefinitionType) {
                guids.Add (d.guid);
            }
        }
    }
    return guids;
}

ImportPropertiesXmlCommand::ImportPropertiesXmlCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String ImportPropertiesXmlCommand::GetName () const
{
    return "ImportPropertiesXml";
}

GS::Optional<GS::UniString> ImportPropertiesXmlCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "xml": { "type": "string", "description": "A Property Manager export (XML) to import." },
            "conflictPolicy": { "type": "string", "enum": [ "append", "replace", "skip" ], "description": "What to do with a property whose name already exists in its group: append imports it under a new unused name, replace replaces the existing definition, skip keeps the existing one." }
        },
        "additionalProperties": false,
        "required": [ "xml", "conflictPolicy" ]
    })";
}

GS::Optional<GS::UniString> ImportPropertiesXmlCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResult": { "$ref": "#/ExecutionResult" },
            "created": { "type": "array", "items": { "type": "object", "properties": { "guid": { "$ref": "#/Guid" } }, "additionalProperties": false, "required": [ "guid" ] } },
            "removed": { "type": "array", "items": { "type": "object", "properties": { "guid": { "$ref": "#/Guid" } }, "additionalProperties": false, "required": [ "guid" ] } }
        },
        "additionalProperties": false,
        "required": [ "executionResult", "created", "removed" ]
    })";
}

GS::ObjectState ImportPropertiesXmlCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString xml, policyStr;
    parameters.Get ("xml", xml);
    parameters.Get ("conflictPolicy", policyStr);
    const API_PropertyDefinitionNameConflictResolutionPolicy policy =
        policyStr == "replace" ? API_ReplaceConflictingProperties :
        policyStr == "skip"    ? API_SkipConflictingProperties :
                                 API_AppendConflictingProperties;

    GS::ObjectState response;
    const GS::HashSet<API_Guid> before = AllCustomPropertyGuids ();
    GSErrCode err = NoError;
    ACAPI_CallUndoableCommand ("ImportPropertiesXml", [&]() -> GSErrCode {
        err = ACAPI_Property_Import (xml, policy);
        return err;
    });
    const GS::HashSet<API_Guid> after = AllCustomPropertyGuids ();

    response.Add ("executionResult", err == NoError ? CreateSuccessfulExecutionResult ()
                                                    : CreateFailedExecutionResult (err, err == APIERR_BADPARS ? GS::UniString ("invalid property XML") : DescribeDefinitionChangeError (err)));
    AddGuidDifference (after, before, response.AddList<GS::ObjectState> ("created"));
    AddGuidDifference (before, after, response.AddList<GS::ObjectState> ("removed"));
    return response;
}
