#include "IFCCommands.hpp"
#include "MigrationHelper.hpp"
#include "StringConversion.hpp"

#ifdef ServerMainVers_2800
#include "ACAPI/IFCAssignments.hpp"
#include "ACAPI/IFCAttribute.hpp"
#include "ACAPI/IFCObjectAccessor.hpp"
#include "ACAPI/IFCObjectID.hpp"
#include "ACAPI/IFCProperty.hpp"
#include "ACAPI/IFCPropertyAccessor.hpp"
#endif

GetElementsByIFCIdsCommand::GetElementsByIFCIdsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetElementsByIFCIdsCommand::GetName () const
{
    return "GetElementsByIFCIds";
}

GS::Optional<GS::UniString> GetElementsByIFCIdsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "ifcIds": {
                "type": "array",
                "description": "A list of IFC identifiers to get the corresponding elements for.",
                "items": {
                    "type": "string"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "ifcIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetElementsByIFCIdsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementsByIFCIds": {
                "$ref": "#/ElementsByIFCIds"
            }
        },
        "additionalProperties": false,
        "required": [
            "elementsByIFCIds"
        ]
    })";
}

GS::ObjectState GetElementsByIFCIdsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::UniString> ifcIds;
    parameters.Get ("ifcIds", ifcIds);

    GS::ObjectState response;
    const auto& elementsByIFCIds = response.AddList<GS::ObjectState> ("elementsByIFCIds");

    for (const GS::UniString& ifcId : ifcIds) {
        GS::ObjectState elemsByIFCId ("ifcId", ifcId);
        const auto& elements = elemsByIFCId.AddList<GS::ObjectState> ("elements");

#ifdef ServerMainVers_2800
        auto accessor = IFCAPI::GetObjectAccessor ();
        auto objectIds = accessor.FindElementsByGlobalId (ifcId);
        if (objectIds.IsOk ()) {
            auto unWrappedObjectIds = objectIds.Unwrap ();
            for (auto& objectId : unWrappedObjectIds) {
                auto elementId = accessor.GetAPIElementID (objectId);
                if (elementId.IsOk ()) {
                    elements (CreateElementIdObjectState (elementId.Unwrap ()));
                }
            }
        }
#else
        const API_Guid apiGuid = APIGuidFromString (ifcId.ToCStr ());
        GS::Array<API_Guid> elemGuids;
        auto err = ACAPI_Element_GetElemListByIFCIdentifier (&apiGuid, &apiGuid, elemGuids);
        if (err == NoError) {
            for (auto& elemGuid : elemGuids) {
                elements (CreateElementIdObjectState (elemGuid));
            }
        }
#endif
        elementsByIFCIds (elemsByIFCId);
    }

    return response;
}

GetIFCIdsOfElementsCommand::GetIFCIdsOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetIFCIdsOfElementsCommand::GetName () const
{
    return "GetIFCIdsOfElements";
}

GS::Optional<GS::UniString> GetIFCIdsOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> GetIFCIdsOfElementsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementIFCIds": {
                "$ref": "#/ElementIFCIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elementIFCIds"
        ]
    })";
}

GS::ObjectState GetIFCIdsOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& elementIFCIds = response.AddList<GS::ObjectState> ("elementIFCIds");

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            elementIFCIds (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        const API_Guid elemGuid = GetGuidFromObjectState (*elementId);
        API_Elem_Head elemHeader;
        if (!LoadElementHeaderByGuid (elemGuid, elemHeader)) {
            elementIFCIds (CreateErrorResponse (APIERR_BADPARS, "The given element was not found"));
            continue;
        }

        GS::ObjectState elemIFCIds = CreateElementIdObjectState (elemGuid);

#ifdef ServerMainVers_2800
        auto accessor = IFCAPI::GetObjectAccessor ();
        auto objectID = accessor.CreateElementObjectID (elemHeader);
        if (objectID.IsErr ()) {
            elementIFCIds (CreateErrorResponse (objectID.UnwrapErr ().kind, "Failed to get IFC accessor for the element"));
            continue;
        }

        auto unWrappedObjectID = objectID.Unwrap ();
        auto ifcId = accessor.GetGlobalId (unWrappedObjectID);
        elemIFCIds.Add ("ifcId", ifcId.IsOk () ? ifcId.Unwrap () : GS::EmptyUniString);
        auto externalIFCId = accessor.GetExternalGlobalId (unWrappedObjectID);
        elemIFCIds.Add ("externalIFCId", externalIFCId.IsOk () ? externalIFCId.Unwrap () : GS::EmptyUniString);
#else
        API_Guid ifcId;
        API_Guid externalIFCId;
        auto err = ACAPI_Element_GetIFCIdentifier (elemGuid, ifcId, externalIFCId);
        if (err != NoError) {
            elementIFCIds (CreateErrorResponse (err, "Failed to get IFC identifiers for the element"));
            continue;
        }

        elemIFCIds.Add ("ifcId", APIGuidToString (ifcId));
        elemIFCIds.Add ("externalIFCId", APIGuidToString (externalIFCId));
#endif

        elementIFCIds (elemIFCIds);
    }

    return response;
}

GetIFCTypeOfElementsCommand::GetIFCTypeOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetIFCTypeOfElementsCommand::GetName () const
{
    return "GetIFCTypeOfElements";
}

GS::Optional<GS::UniString> GetIFCTypeOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> GetIFCTypeOfElementsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementIFCTypes": {
                "$ref": "#/ElementIFCTypesOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elementIFCTypes"
        ]
    })";
}

GS::ObjectState GetIFCTypeOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& elementIFCTypes = response.AddList<GS::ObjectState> ("elementIFCTypes");

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            elementIFCTypes (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        const API_Guid elemGuid = GetGuidFromObjectState (*elementId);
        API_Elem_Head elemHeader;
        if (!LoadElementHeaderByGuid (elemGuid, elemHeader)) {
            elementIFCTypes (CreateErrorResponse (APIERR_BADPARS, "The given element was not found"));
            continue;
        }

        GS::ObjectState elemIFCTypes = CreateElementIdObjectState (elemGuid);

#ifdef ServerMainVers_2800
        auto accessor = IFCAPI::GetObjectAccessor ();
        auto objectID = accessor.CreateElementObjectID (elemHeader);
        if (objectID.IsErr ()) {
            elementIFCTypes (CreateErrorResponse (objectID.UnwrapErr ().kind, "Failed to get IFC accessor for the element"));
            continue;
        }

        auto unWrappedObjectID = objectID.Unwrap ();
        auto ifcType = accessor.GetIFCType (unWrappedObjectID);
        elemIFCTypes.Add ("ifcType", ifcType.IsOk () ? ifcType.Unwrap () : GS::EmptyUniString);
        auto typeObjectIFCType = accessor.GetTypeObjectIFCType (unWrappedObjectID);
        elemIFCTypes.Add ("typeObjectIFCType", typeObjectIFCType.IsOk () ? typeObjectIFCType.Unwrap () : GS::EmptyUniString);
#else
        GS::UniString ifcType;
        GS::UniString typeObjectIFCType;
        auto err = ACAPI_Element_GetIFCType (elemGuid, &ifcType, &typeObjectIFCType);
        if (err != NoError) {
            elementIFCTypes (CreateErrorResponse (err, "Failed to get IFC type for the element"));
            continue;
        }

        elemIFCTypes.Add ("ifcType", ifcType);
        elemIFCTypes.Add ("typeObjectIFCType", typeObjectIFCType);
#endif

        elementIFCTypes (elemIFCTypes);
    }

    return response;
}

GetIFCPropertiesOfElementsCommand::GetIFCPropertiesOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetIFCPropertiesOfElementsCommand::GetName () const
{
    return "GetIFCPropertiesOfElements";
}

GS::Optional<GS::UniString> GetIFCPropertiesOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> GetIFCPropertiesOfElementsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementIFCProperties": {
                "$ref": "#/ElementIFCPropertiesOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elementIFCProperties"
        ]
    })";
}

static GS::UniString TrueString ("True");
static GS::UniString FalseString ("False");
static GS::UniString UnknownString ("Unknown");
static GS::UniString SeparatorString ("; ");

#ifdef ServerMainVers_2800
static GS::UniString PropertyValueToString (const IFCAPI::Value& value)
{
	GS::UniString result;

	auto anyValue = value.GetAnyValue ();
	if (anyValue.has_value ()) {
		if (std::holds_alternative<Int64> (*anyValue)) {
			result.Append (GS::ValueToUniString (std::get<Int64> (*anyValue)));
		} else if (std::holds_alternative<double> (*anyValue)) {
			result.Append (GS::ValueToUniString (std::get<double> (*anyValue)));
		} else if (std::holds_alternative<bool> (*anyValue)) {
			result.Append (std::get<bool> (*anyValue) ? TrueString : FalseString);
		} else if (std::holds_alternative<IFCAPI::IfcLogical> (*anyValue)) {
			switch (std::get<IFCAPI::IfcLogical> (*anyValue)) {
			case IFCAPI::IfcLogical::Unknown: result.Append (UnknownString); break;
			case IFCAPI::IfcLogical::False: result.Append (FalseString); break;
			case IFCAPI::IfcLogical::True: result.Append (TrueString); break;
			}
		} else if (std::holds_alternative<GS::UniString> (*anyValue)) {
			result.Append (std::get<GS::UniString> (*anyValue));
		}
	}

	return result;
}

GS::UniString Concatenate (const GS::Array<GS::UniString>& strings, const GS::UniString& separator)
{
	GS::UniString result;

	for (const auto& item : strings) {
		if (!result.IsEmpty ())
			result.Append (separator);

		result.Append (item);
	}

	return result;
}

static GS::UniString ConcatenatePropertyValues (const std::vector<IFCAPI::Value>& values, const GS::UniString& separator)
{
	GS::Array<GS::UniString> valueStrings;

	for (const auto& value: values)
		valueStrings.Push (PropertyValueToString (value));

	return Concatenate (valueStrings, separator);
}

static GS::UniString PropertyBoundedValueToString (const IFCAPI::PropertyBoundedValue& boundedValue)
{
	GS::UniString result;
	GS::UniString lowerBoundValue = PropertyValueToString (boundedValue.GetLowerBoundValue ());
	GS::UniString upperBoundValue = PropertyValueToString (boundedValue.GetUpperBoundValue ());
	GS::UniString setPointValue = PropertyValueToString (boundedValue.GetSetPointValue ());

	if (!lowerBoundValue.IsEmpty () || !upperBoundValue.IsEmpty ()) {
		result.Append (lowerBoundValue);
		result.Append (" ... ");
		result.Append (upperBoundValue);
	}

	if (!setPointValue.IsEmpty ()) {
		if (!result.IsEmpty ())
			result.Append (SeparatorString);
		result.Append (setPointValue);
	}

	return result;
}

static GS::UniString PropertyTableValueToString (const IFCAPI::PropertyTableValue& tableValue)
{
	GS::Array<GS::UniString> names;
	auto definingValues = tableValue.GetDefiningValues ();
	auto definedValues = tableValue.GetDefinedValues ();

	if (definingValues.size () != definedValues.size ())
		return "";

	for (size_t i = 0; i < definingValues.size (); ++i)
		names.Push (PropertyValueToString (definingValues[i]) + ": " + PropertyValueToString (definedValues[i]));

	return Concatenate (names, SeparatorString);
}

static GS::UniString GetPropertyValueString (const IFCAPI::Property& property)
{
	auto propertyVariant = property.GetTyped ();
	
	if (std::holds_alternative<IFCAPI::PropertySingleValue> (propertyVariant)) {
		const auto& propertyCast = std::get<IFCAPI::PropertySingleValue> (propertyVariant);
		return PropertyValueToString (propertyCast.GetNominalValue ());
	} else if (std::holds_alternative<IFCAPI::PropertyListValue> (propertyVariant)) {
		const auto& propertyCast = std::get<IFCAPI::PropertyListValue> (propertyVariant);
		return ConcatenatePropertyValues (propertyCast.GetListValues (), SeparatorString);
	} else if (std::holds_alternative<IFCAPI::PropertyBoundedValue> (propertyVariant)) {
		const auto& propertyCast = std::get<IFCAPI::PropertyBoundedValue> (propertyVariant);
		return PropertyBoundedValueToString (propertyCast);
	} else if (std::holds_alternative<IFCAPI::PropertyEnumeratedValue> (propertyVariant)) {
		const auto& propertyCast = std::get<IFCAPI::PropertyEnumeratedValue> (propertyVariant);
		GS::Array<GS::UniString> names;
		for (const auto& value : propertyCast.GetEnumerationValues ())
			names.Push (PropertyValueToString (value));
		return Concatenate (names, SeparatorString);
	} else {
		const auto& propertyCast = std::get<IFCAPI::PropertyTableValue> (propertyVariant);
		return PropertyTableValueToString (propertyCast);
	}
}
#else
static GS::UniString PropertyValueToString (const API_IFCPropertyValue& value)
{
	GS::UniString result;

    if (value.value.primitiveType == API_IFCPropertyAnyValueIntegerType) {
        result.Append (GS::ValueToUniString (value.value.intValue));
    } else if (value.value.primitiveType == API_IFCPropertyAnyValueRealType) {
        result.Append (GS::ValueToUniString (value.value.doubleValue));
    } else if (value.value.primitiveType == API_IFCPropertyAnyValueBooleanType) {
        result.Append (value.value.boolValue ? TrueString : FalseString);
    } else if (value.value.primitiveType == API_IFCPropertyAnyValueLogicalType) {
        switch (value.value.intValue) {
            case 0: result.Append (UnknownString); break;
            case 1: result.Append (FalseString); break;
            case 2: result.Append (TrueString); break;
        }
    } else if (value.value.primitiveType == API_IFCPropertyAnyValueStringType) {
        result.Append (value.value.stringValue);
    }

	return result;
}

GS::UniString Concatenate (const GS::Array<GS::UniString>& strings, const GS::UniString& separator)
{
	GS::UniString result;

	for (const auto& item : strings) {
		if (!result.IsEmpty ())
			result.Append (separator);

		result.Append (item);
	}

	return result;
}

static GS::UniString ConcatenatePropertyValues (const GS::Array<API_IFCPropertyValue>& values, const GS::UniString& separator)
{
	GS::Array<GS::UniString> valueStrings;

	for (const auto& value: values)
		valueStrings.Push (PropertyValueToString (value));

	return Concatenate (valueStrings, separator);
}

static GS::UniString PropertyBoundedValueToString (const API_IFCPropertyBoundedValue& boundedValue)
{
	GS::UniString result;
	GS::UniString lowerBoundValue = PropertyValueToString (boundedValue.lowerBoundValue);
	GS::UniString upperBoundValue = PropertyValueToString (boundedValue.upperBoundValue);

	if (!lowerBoundValue.IsEmpty () || !upperBoundValue.IsEmpty ()) {
		result.Append (lowerBoundValue);
		result.Append (" ... ");
		result.Append (upperBoundValue);
	}

	return result;
}

static GS::UniString PropertyTableValueToString (const API_IFCPropertyTableValue& tableValue)
{
	GS::Array<GS::UniString> names;
	auto& definingValues = tableValue.definingValues;
	auto& definedValues = tableValue.definedValues;

	if (definingValues.GetSize() != definedValues.GetSize())
		return "";

	for (GS::UIndex i = 0; i < definingValues.GetSize (); ++i)
		names.Push (PropertyValueToString (definingValues[i]) + ": " + PropertyValueToString (definedValues[i]));

	return Concatenate (names, SeparatorString);
}

static GS::UniString GetPropertyValueString (const API_IFCProperty& property)
{
    if (property.head.propertyType == API_IFCPropertySingleValueType) {
        return PropertyValueToString (property.singleValue.nominalValue);
    } else if (property.head.propertyType == API_IFCPropertyListValueType) {
        return ConcatenatePropertyValues (property.listValue.listValues, SeparatorString);
    } else if (property.head.propertyType == API_IFCPropertyBoundedValueType) {
        return PropertyBoundedValueToString (property.boundedValue);
    } else if (property.head.propertyType == API_IFCPropertyEnumeratedValueType) {
        GS::Array<GS::UniString> names;
        for (UIndex i = 0; i < property.enumeratedValue.enumerationValues.GetSize (); ++i)
            names.Push (PropertyValueToString (property.enumeratedValue.enumerationValues[i]));
        return Concatenate (names, SeparatorString);
    } else {
        return PropertyTableValueToString (property.tableValue);
    }
}
#endif

GS::ObjectState GetIFCPropertiesOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& elementIFCProperties = response.AddList<GS::ObjectState> ("elementIFCProperties");

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            elementIFCProperties (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        const API_Guid elemGuid = GetGuidFromObjectState (*elementId);
        API_Elem_Head elemHeader;
        if (!LoadElementHeaderByGuid (elemGuid, elemHeader)) {
            elementIFCProperties (CreateErrorResponse (APIERR_BADPARS, "The given element was not found"));
            continue;
        }

        GS::ObjectState elemIFCProperties = CreateElementIdObjectState (elemGuid);
        const auto& ifcProperties = elemIFCProperties.AddList<GS::ObjectState> ("ifcProperties");

#ifdef ServerMainVers_2800
        auto objectID = IFCAPI::GetObjectAccessor ().CreateElementObjectID (elemHeader);
        if (objectID.IsErr ()) {
            elementIFCProperties (CreateErrorResponse (objectID.UnwrapErr ().kind, "Failed to get IFC accessor for the element"));
            continue;
        }
        IFCAPI::PropertyAccessor propertyAccessor = IFCAPI::PropertyAccessor (*objectID);
        {
            auto properties = propertyAccessor.GetPreviewProperties ();
            if (!properties.IsErr ()) {
                for (const auto& property : properties.Unwrap ()) {
                    ifcProperties (GS::ObjectState (
                        "propertySetName", property.GetPropertySetName (),
                        "name", property.GetName (),
                        "value", GetPropertyValueString (property)
                    ));
                }
            }
        }
        {
            auto properties = propertyAccessor.GetLocalProperties ();
            if (!properties.IsErr ()) {
                for (const auto& property : properties.Unwrap ()) {
                    ifcProperties (GS::ObjectState (
                        "propertySetName", property.GetPropertySetName (),
                        "name", property.GetName (),
                        "value", GetPropertyValueString (property)
                    ));
                }
            }
        }
#else
        GS::Array<API_IFCProperty> properties;
        auto err = ACAPI_Element_GetIFCProperties (elemGuid, false, &properties);
        if (err != NoError) {
            elementIFCProperties (CreateErrorResponse (err, "Failed to get IFC properties for the element"));
            continue;
        }

        for (const API_IFCProperty& property : properties) {
            ifcProperties (GS::ObjectState (
                "propertySetName", property.head.propertySetName,
                "name", property.head.propertyName,
                "value", GetPropertyValueString (property)
            ));
        }
#endif

        elementIFCProperties (elemIFCProperties);
    }

    return response;
}
