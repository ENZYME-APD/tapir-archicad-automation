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

GetAllPropertiesCommand::GetAllPropertiesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAllPropertiesCommand::GetName () const
{
    return "GetAllProperties";
}

GS::Optional<GS::UniString> GetAllPropertiesCommand::GetResponseSchema () const
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
    for (const API_PropertyGroup& group : groups) {
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

GS::Optional<GS::UniString> GetPropertyValuesOfElementsCommand::GetResponseSchema () const
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

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            propertyValuesForElements (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        const API_Guid elemGuid = GetGuidFromObjectState (*elementId);

        GS::ObjectState propertyValuesForElement;
        const auto& propertyValues = propertyValuesForElement.AddList<GS::ObjectState> ("propertyValues");

        for (const GS::ObjectState& property : properties) {
            const GS::ObjectState* propertyId = property.Get ("propertyId");
            if (propertyId == nullptr) {
                propertyValues (CreateErrorResponse (APIERR_BADPARS, "propertyId is missing"));
                continue;
            }

            const API_Guid propertyGuid = GetGuidFromObjectState (*propertyId);

            API_Property propertyValue;
            GSErrCode err = ACAPI_Element_GetPropertyValue (elemGuid, propertyGuid, propertyValue);

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

GS::Optional<GS::UniString> SetPropertyValuesOfElementsCommand::GetResponseSchema () const
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
            GSErrCode err = ACAPI_Element_GetPropertyValuesByGuid (elemGuid, properties, propertyValues);

            for (API_Property& propertyValue : propertyValues) {
                const auto guidPair = GS::NewPair (elemGuid, propertyValue.definition.guid);
                auto& result = results[resultIndices[guidPair]];

                if (err != NoError) {
                    result = CreateFailedExecutionResult (err, "Failed to get property values for element");
                    continue;
                }

                err = ACAPI_Property_SetPropertyValueFromString (propertyValuesForElements[guidPair], conversionUtils, &propertyValue);

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

GS::Optional<GS::UniString> GetPropertyValuesOfAttributesCommand::GetResponseSchema () const
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
            API_Attr_Head attrHead = {};
            attrHead.guid = attGuid;
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

GS::Optional<GS::UniString> SetPropertyValuesOfAttributesCommand::GetResponseSchema () const
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

            API_Attr_Head attrHead = {};
            attrHead.guid = attGUid;
            GSErrCode err = ACAPI_Attribute_GetPropertyValuesByGuid (attrHead, properties, propertyValues);

            for (API_Property& propertyValue : propertyValues) {
                const auto guidPair = GS::NewPair (attGuid, propertyValue.definition.guid);
                auto& result = results[resultIndices[guidPair]];

                if (err != NoError) {
                    result = CreateFailedExecutionResult (err, "Failed to get property values for attribute");
                    continue;
                }

                err = ACAPI_Property_SetPropertyValueFromString (propertyValuesForAttributes[guidPair], conversionUtils, &propertyValue);

                if (err != NoError) {
                    result = CreateFailedExecutionResult (err, "Failed to set property value for attribute");
                    continue;
                }

                API_Attr_Head attrHead = {};
                attrHead.guid = attGuid;
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
                    "type": "object",
                    "properties": {
                        "propertyGroup": {
                            "type": "object",
                            "properties": {
                                "name": {
                                    "type": "string"
                                },
                                "description": {
                                    "type": "string"
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "name"
                            ]
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "propertyGroup"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "propertyGroups"
        ]
    })";
}

GS::Optional<GS::UniString> CreatePropertyGroupsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "propertyGroupIds": {
                "type": "array",
                "description": "The identifiers of the created property groups.",
                "items": {
                    "type": "object",
                    "properties": {
                        "propertyGroupId": {
                            "$ref": "#/PropertyGroupId"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "propertyGroupId"
                    ]
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
                    "type": "object",
                    "properties": {
                        "propertyGroupId": {
                            "$ref": "#/PropertyGroupId"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "propertyGroupId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "propertyGroupIds"
        ]
    })";
}

GS::Optional<GS::UniString> DeletePropertyGroupsCommand::GetResponseSchema () const
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
                    "type": "object",
                    "properties": {
                        "propertyDefinition": {
                            "type": "object",
                            "properties": {
                                "name": {
                                    "type": "string"
                                },
                                "description": {
                                    "type": "string"
                                },
                                "type": {
                                    "$ref": "#/PropertyType"
                                },
                                "isEditable": {
                                    "type": "boolean"
                                },
                                "defaultValue": {
                                    "$ref": "#/PropertyDefaultValue"
                                },
                                "possibleEnumValues": {
                                    "type": "array",
                                    "description": "The possible enum values of the property when the property type is enumeration.",
                                    "items": {
                                        "type": "object",
                                        "properties": {
                                            "enumValue": {
                                                "type": "object",
                                                "description": "The description of an enumeration value.",
                                                "properties": {
                                                    "enumValueId": {
                                                        "$ref": "#/EnumValueId"
                                                    },
                                                    "displayValue": {
                                                        "type": "string",
                                                        "description": "Displayed value of the enumeration."
                                                    },
                                                    "nonLocalizedValue": {
                                                        "type": "string",
                                                        "description": "Nonlocalized value of the enumeration if there is one."
                                                    }
                                                },
                                                "required": [
                                                    "displayValue"
                                                ]
                                            }
                                        },
                                        "additionalProperties": false,
                                        "required": [
                                            "enumValue"
                                        ]
                                    }
                                },
                                "availability": {
                                    "type": "array",
                                    "description": "The identifiers of classification items the new property is available for.",    
                                    "items": {
                                        "$ref": "#/ClassificationItemIdArrayItem"
                                    }
                                },
                                "group": {
                                    "type": "object",
                                    "description": "The property group defined by name or id. If both fields exists the id will be used.",
                                    "properties": {
                                        "propertyGroupId": {
                                            "$ref": "#/PropertyGroupId"
                                        },
                                        "name": {
                                            "type": "string"
                                        }
                                    },
                                    "additionalProperties": false,
                                    "required": []
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "name",
                                "description",
                                "type",
                                "isEditable",
                                "availability",
                                "group"
                            ]
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "propertyDefinition"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "propertyDefinitions"
        ]
    })";
}

GS::Optional<GS::UniString> CreatePropertyDefinitionsCommand::GetResponseSchema () const
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
            const GS::ObjectState* defaultValue = propertyDefinition->Get ("defaultValue");
            if (defaultValue != nullptr) {
                defaultValue->Get ("expressions", apiPropertyDefinition.defaultValue.propertyExpressions);
                apiPropertyDefinition.defaultValue.hasExpression = !apiPropertyDefinition.defaultValue.propertyExpressions.IsEmpty ();

                if (!apiPropertyDefinition.defaultValue.hasExpression) {
                    const GS::ObjectState* basicDefaultValue = defaultValue->Get ("basicDefaultValue");
                    if (basicDefaultValue == nullptr) {
                        propertyIds (CreateErrorResponse (APIERR_BADPARS, "both defaultValue/basicDefaultValue and defaultValue/expressions are missing or empty"));
                        continue;
                    }

                    GS::UniString statusStr;
                    if (!basicDefaultValue->Get ("status", statusStr) || statusStr.IsEmpty ()) {
                        propertyIds (CreateErrorResponse (APIERR_BADPARS, "defaultValue/basicDefaultValue/status is missing or empty"));
                        continue;
                    }

                    if (statusStr == "normal") {
                        apiPropertyDefinition.defaultValue.basicValue.variantStatus = API_VariantStatusNormal;

                        if (!basicDefaultValue->Get ("type", typeStr) || typeStr.IsEmpty ()) {
                            propertyIds (CreateErrorResponse (APIERR_BADPARS, "defaultValue/basicDefaultValue/type is missing or empty"));
                            continue;
                        }
                        const PropertyTypeTuple* typeTuple = PropertyTypeDictionary.GetPtr (typeStr);
                        if (typeTuple == nullptr) {
                            propertyIds (CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("invalid type '%T'", typeStr.ToPrintf ())));
                            continue;
                        }
                        switch (std::get<0> (*typeTuple)) {
                            case API_PropertySingleCollectionType:
                                switch (apiPropertyDefinition.valueType) {
                                    case API_PropertyRealValueType:
                                        basicDefaultValue->Get ("value", apiPropertyDefinition.defaultValue.basicValue.singleVariant.variant.doubleValue);
                                        break;
                                    case API_PropertyIntegerValueType:
                                        basicDefaultValue->Get ("value", apiPropertyDefinition.defaultValue.basicValue.singleVariant.variant.intValue);
                                        break;
                                    case API_PropertyStringValueType:
                                        basicDefaultValue->Get ("value", apiPropertyDefinition.defaultValue.basicValue.singleVariant.variant.uniStringValue);
                                        break;
                                    case API_PropertyBooleanValueType:
                                        basicDefaultValue->Get ("value", apiPropertyDefinition.defaultValue.basicValue.singleVariant.variant.boolValue);
                                        break;
                                    case API_PropertyGuidValueType:
                                        {
                                            GS::String guidStr;
                                            basicDefaultValue->Get ("value", guidStr);
                                            apiPropertyDefinition.defaultValue.basicValue.singleVariant.variant.guidValue = APIGuidFromString (guidStr.ToCStr ());
                                        }
                                        break;
                                }
                                break;
                            case API_PropertySingleChoiceEnumerationCollectionType:
                                {
                                    const GS::ObjectState* enumValueId = basicDefaultValue->Get ("value");
                                    if (enumValueId == nullptr) {
                                        propertyIds (CreateErrorResponse (APIERR_BADPARS, "defaultValue/basicDefaultValue/value is missing"));
                                        continue;
                                    }

                                    GS::UniString enumValueIdTypeStr;
                                    enumValueId->Get ("type", enumValueIdTypeStr);
                                    if (enumValueIdTypeStr.IsEmpty ()) {
                                        propertyIds (CreateErrorResponse (APIERR_BADPARS, "defaultValue/basicDefaultValue/value/type is missing or empty"));
                                        continue;
                                    }

                                    apiPropertyDefinition.defaultValue.basicValue.singleVariant.variant.type = API_PropertyGuidValueType;
                                    GS::UniString valueStr;
                                    enumValueId->Get (enumValueIdTypeStr.ToCStr ().Get (), valueStr);
                                    apiPropertyDefinition.defaultValue.basicValue.singleVariant.variant.guidValue = FindEnumValueGuid (apiPropertyDefinition.possibleEnumValues, enumValueIdTypeStr, valueStr);
                                }
                                break;
                            case API_PropertyMultipleChoiceEnumerationCollectionType:
                                {
                                    GS::Array<GS::ObjectState> enumValueIds;
                                    basicDefaultValue->Get ("value", enumValueIds);
                                    if (enumValueIds.IsEmpty ()) {
                                        propertyIds (CreateErrorResponse (APIERR_BADPARS, "defaultValue/basicDefaultValue/value is missing or empty"));
                                        continue;
                                    }

                                    bool failed = false;
                                    API_Variant v;
                                    v.type = API_PropertyGuidValueType;
                                    for (UIndex i = 0; i < enumValueIds.GetSize (); ++i) {
                                        const GS::ObjectState* enumValueId = enumValueIds[i].Get ("enumValueId");
                                        if (enumValueId == nullptr) {
                                            failed = true;
                                            propertyIds (CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("defaultValue/basicDefaultValue/value[%d]/enumValueId is missing or empty", i)));
                                            break;
                                        }

                                        GS::UniString enumValueIdTypeStr;
                                        enumValueId->Get ("type", enumValueIdTypeStr);
                                        if (enumValueIdTypeStr.IsEmpty ()) {
                                            failed = true;
                                            propertyIds (CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("defaultValue/basicDefaultValue/value[%d]/enumValueId/type is missing or empty", i)));
                                            break;
                                        }

                                        GS::UniString valueStr;
                                        enumValueId->Get (enumValueIdTypeStr.ToCStr ().Get (), valueStr);
                                        v.guidValue = FindEnumValueGuid (apiPropertyDefinition.possibleEnumValues, enumValueIdTypeStr, valueStr);
                                        if (v.guidValue == APINULLGuid) {
                                            failed = true;
                                            propertyIds (CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("defaultValue/basicDefaultValue/value[%d]/enumValueId/%T is missing or invalid", i, enumValueIdTypeStr.ToPrintf ())));
                                            break;
                                        }
                                        apiPropertyDefinition.defaultValue.basicValue.listVariant.variants.Push (v);
                                    }
                                    if (failed) {
                                        continue;
                                    }
                                }
                                break;
                            case API_PropertyListCollectionType:
                                switch (apiPropertyDefinition.valueType) {
                                    case API_PropertyRealValueType:
                                        {
                                            GS::Array<double> doubleValues;
                                            basicDefaultValue->Get ("value", doubleValues);
                                            for (double d : doubleValues) {
                                                API_Variant v;
                                                v.type = apiPropertyDefinition.valueType;
                                                v.doubleValue = d;
                                                apiPropertyDefinition.defaultValue.basicValue.listVariant.variants.Push (v);
                                            }
                                        }
                                        break;
                                    case API_PropertyIntegerValueType:
                                        {
                                            GS::Array<int> intValues;
                                            basicDefaultValue->Get ("value", intValues);
                                            for (int i : intValues) {
                                                API_Variant v;
                                                v.type = apiPropertyDefinition.valueType;
                                                v.intValue = i;
                                                apiPropertyDefinition.defaultValue.basicValue.listVariant.variants.Push (v);
                                            }
                                        }
                                        break;
                                    case API_PropertyStringValueType:
                                        {
                                            GS::Array<GS::UniString> uniStringValues;
                                            basicDefaultValue->Get ("value", uniStringValues);
                                            for (GS::UniString s : uniStringValues) {
                                                API_Variant v;
                                                v.type = apiPropertyDefinition.valueType;
                                                v.uniStringValue = s;
                                                apiPropertyDefinition.defaultValue.basicValue.listVariant.variants.Push (v);
                                            }
                                        }
                                        break;
                                    case API_PropertyBooleanValueType:
                                        {
                                            GS::Array<bool> boolValues;
                                            basicDefaultValue->Get ("value", boolValues);
                                            for (bool b : boolValues) {
                                                API_Variant v;
                                                v.type = apiPropertyDefinition.valueType;
                                                v.boolValue = b;
                                                apiPropertyDefinition.defaultValue.basicValue.listVariant.variants.Push (v);
                                            }
                                        }
                                        break;
                                }
                                break;
                        }
                    } else if (statusStr == "userUndefined") {
                        apiPropertyDefinition.defaultValue.basicValue.variantStatus = API_VariantStatusUserUndefined;
                    } else {
                        apiPropertyDefinition.defaultValue.basicValue.variantStatus = API_VariantStatusNull;
                    }
                }
            } else {
                apiPropertyDefinition.defaultValue.basicValue.variantStatus = API_VariantStatusNull;
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
                    "type": "object",
                    "properties": {
                        "propertyId": {
                            "$ref": "#/PropertyId"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "propertyId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "propertyIds"
        ]
    })";
}

GS::Optional<GS::UniString> DeletePropertyDefinitionsCommand::GetResponseSchema () const
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