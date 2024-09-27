#include "PropertyCommands.hpp"
#include "MigrationHelper.hpp"

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
    PropertyConversionUtils ();

    virtual ~PropertyConversionUtils ();

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

PropertyConversionUtils::PropertyConversionUtils () = default;
PropertyConversionUtils::~PropertyConversionUtils () = default;

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
                            "type": "object",
                            "properties": {
                                "guid": {
                                    "$ref": "#/Guid"
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "guid"
                            ]
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

    GS::Array<GS::ObjectState> properties;
    parameters.Get ("properties", properties);

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
                propertyGroupIds (CreateErrorResponse (APIERR_BADPARS, "name is missing or empty"));
                continue;
            }

            propertyGroupIds (CreateIdObjectState ("propertyGroupId", apiPropertyGroup.guid));
        }

        return NoError;
    });

    return response;
}