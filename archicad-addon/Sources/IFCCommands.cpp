#include "IFCCommands.hpp"
#include "MigrationHelper.hpp"
#include "StringConversion.hpp"

#ifdef ServerMainVers_2800
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

GS::Optional<GS::UniString> GetElementsByIFCIdsCommand::GetRawResponseSchema () const
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

GS::Optional<GS::UniString> GetIFCIdsOfElementsCommand::GetRawResponseSchema () const
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

GS::Optional<GS::UniString> GetIFCTypeOfElementsCommand::GetRawResponseSchema () const
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

GS::Optional<GS::UniString> GetIFCPropertiesOfElementsCommand::GetRawResponseSchema () const
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

static GS::UniString GetPropertyTypeString (const IFCAPI::Property& property)
{
	auto propertyVariant = property.GetTyped ();

	if (std::holds_alternative<IFCAPI::PropertySingleValue> (propertyVariant)) {
		return "SingleValue";
	} else if (std::holds_alternative<IFCAPI::PropertyListValue> (propertyVariant)) {
		return "ListValue";
	} else if (std::holds_alternative<IFCAPI::PropertyBoundedValue> (propertyVariant)) {
		return "BoundedValue";
	} else if (std::holds_alternative<IFCAPI::PropertyEnumeratedValue> (propertyVariant)) {
		return "EnumeratedValue";
	} else {
		return "TableValue";
	}
}

static GS::UniString GetFirstValueType (const std::vector<IFCAPI::Value>& values)
{
	if (values.empty ())
		return GS::EmptyUniString;

	return GS::UniString (values.front ().GetType ());
}

static GS::UniString GetPropertyBoundedValueType (const IFCAPI::PropertyBoundedValue& boundedValue)
{
	const auto& lowerBoundValue = boundedValue.GetLowerBoundValue ();
	const auto& upperBoundValue = boundedValue.GetUpperBoundValue ();
	const auto& setPointValue = boundedValue.GetSetPointValue ();

	if (lowerBoundValue.GetAnyValue ().has_value ())
		return GS::UniString (lowerBoundValue.GetType ());
	if (upperBoundValue.GetAnyValue ().has_value ())
		return GS::UniString (upperBoundValue.GetType ());
	if (setPointValue.GetAnyValue ().has_value ())
		return GS::UniString (setPointValue.GetType ());
	return GS::UniString (lowerBoundValue.GetType ());
}

// The IFC type of the values of the property, for a table value the type of its defined values
static GS::UniString GetPropertyValueType (const IFCAPI::Property& property)
{
	auto propertyVariant = property.GetTyped ();

	if (std::holds_alternative<IFCAPI::PropertySingleValue> (propertyVariant)) {
		const auto& propertyCast = std::get<IFCAPI::PropertySingleValue> (propertyVariant);
		return GS::UniString (propertyCast.GetNominalValue ().GetType ());
	} else if (std::holds_alternative<IFCAPI::PropertyListValue> (propertyVariant)) {
		const auto& propertyCast = std::get<IFCAPI::PropertyListValue> (propertyVariant);
		return GetFirstValueType (propertyCast.GetListValues ());
	} else if (std::holds_alternative<IFCAPI::PropertyBoundedValue> (propertyVariant)) {
		const auto& propertyCast = std::get<IFCAPI::PropertyBoundedValue> (propertyVariant);
		return GetPropertyBoundedValueType (propertyCast);
	} else if (std::holds_alternative<IFCAPI::PropertyEnumeratedValue> (propertyVariant)) {
		const auto& propertyCast = std::get<IFCAPI::PropertyEnumeratedValue> (propertyVariant);
		return GetFirstValueType (propertyCast.GetEnumerationValues ());
	} else {
		const auto& propertyCast = std::get<IFCAPI::PropertyTableValue> (propertyVariant);
		return GetFirstValueType (propertyCast.GetDefinedValues ());
	}
}

static GS::ObjectState CreateIFCPropertyObjectState (const IFCAPI::Property& property, const GS::UniString& source)
{
	GS::ObjectState ifcProperty (
		"propertySetName", property.GetPropertySetName (),
		"name", property.GetName (),
		"value", GetPropertyValueString (property),
		"propertyType", GetPropertyTypeString (property),
		"source", source
	);
	const GS::UniString valueType = GetPropertyValueType (property);
	if (!valueType.IsEmpty ())
		ifcProperty.Add ("valueType", valueType);
	return ifcProperty;
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

static GS::UniString GetPropertyTypeString (const API_IFCProperty& property)
{
    if (property.head.propertyType == API_IFCPropertySingleValueType) {
        return "SingleValue";
    } else if (property.head.propertyType == API_IFCPropertyListValueType) {
        return "ListValue";
    } else if (property.head.propertyType == API_IFCPropertyBoundedValueType) {
        return "BoundedValue";
    } else if (property.head.propertyType == API_IFCPropertyEnumeratedValueType) {
        return "EnumeratedValue";
    } else {
        return "TableValue";
    }
}

static GS::UniString GetFirstValueType (const GS::Array<API_IFCPropertyValue>& values)
{
    if (values.IsEmpty ())
        return GS::EmptyUniString;

    return values[0].valueType;
}

// The IFC type of the values of the property, for a table value the type of its defined values
static GS::UniString GetPropertyValueType (const API_IFCProperty& property)
{
    if (property.head.propertyType == API_IFCPropertySingleValueType) {
        return property.singleValue.nominalValue.valueType;
    } else if (property.head.propertyType == API_IFCPropertyListValueType) {
        return GetFirstValueType (property.listValue.listValues);
    } else if (property.head.propertyType == API_IFCPropertyBoundedValueType) {
        if (!property.boundedValue.lowerBoundValue.valueType.IsEmpty ())
            return property.boundedValue.lowerBoundValue.valueType;
        return property.boundedValue.upperBoundValue.valueType;
    } else if (property.head.propertyType == API_IFCPropertyEnumeratedValueType) {
        return GetFirstValueType (property.enumeratedValue.enumerationValues);
    } else {
        return GetFirstValueType (property.tableValue.definedValues);
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
                    ifcProperties (CreateIFCPropertyObjectState (property, "preview"));
                }
            }
        }
        {
            auto properties = propertyAccessor.GetLocalProperties ();
            if (!properties.IsErr ()) {
                for (const auto& property : properties.Unwrap ()) {
                    ifcProperties (CreateIFCPropertyObjectState (property, "local"));
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
            GS::ObjectState ifcProperty (
                "propertySetName", property.head.propertySetName,
                "name", property.head.propertyName,
                "value", GetPropertyValueString (property),
                "propertyType", GetPropertyTypeString (property)
            );
            const GS::UniString valueType = GetPropertyValueType (property);
            if (!valueType.IsEmpty ())
                ifcProperty.Add ("valueType", valueType);
            ifcProperties (ifcProperty);
        }
#endif

        elementIFCProperties (elemIFCProperties);
    }

    return response;
}

GetIFCAttributesOfElementsCommand::GetIFCAttributesOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetIFCAttributesOfElementsCommand::GetName () const
{
    return "GetIFCAttributesOfElements";
}

GS::Optional<GS::UniString> GetIFCAttributesOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "storedOnly": {
                "type": "boolean",
                "description": "Return only the attributes stored on the elements, not the ones Archicad would generate on IFC export. Used only in Archicad 25, 26 and 27. Default: false."
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> GetIFCAttributesOfElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementIFCAttributes": {
                "$ref": "#/ElementIFCAttributesOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elementIFCAttributes"
        ]
    })";
}

GS::ObjectState GetIFCAttributesOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

#ifndef ServerMainVers_2800
    bool storedOnly = false;
    parameters.Get ("storedOnly", storedOnly);
#endif

    GS::ObjectState response;
    const auto& elementIFCAttributes = response.AddList<GS::ObjectState> ("elementIFCAttributes");

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            elementIFCAttributes (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        const API_Guid elemGuid = GetGuidFromObjectState (*elementId);
        API_Elem_Head elemHeader;
        if (!LoadElementHeaderByGuid (elemGuid, elemHeader)) {
            elementIFCAttributes (CreateErrorResponse (APIERR_BADPARS, "The given element was not found"));
            continue;
        }

#ifdef ServerMainVers_2800
        auto objectID = IFCAPI::GetObjectAccessor ().CreateElementObjectID (elemHeader);
        if (objectID.IsErr ()) {
            elementIFCAttributes (CreateErrorResponse (objectID.UnwrapErr ().kind, "Failed to get IFC accessor for the element"));
            continue;
        }
        IFCAPI::PropertyAccessor propertyAccessor = IFCAPI::PropertyAccessor (*objectID);
        auto attributes = propertyAccessor.GetAttributes ();
        if (attributes.IsErr ()) {
            elementIFCAttributes (CreateErrorResponse (attributes.UnwrapErr ().kind, "Failed to get the IFC attributes of the element"));
            continue;
        }

        GS::ObjectState elemIFCAttributes = CreateElementIdObjectState (elemGuid);
        const auto& ifcAttributes = elemIFCAttributes.AddList<GS::ObjectState> ("ifcAttributes");
        for (const auto& attribute : attributes.Unwrap ()) {
            GS::ObjectState ifcAttribute (
                "name", attribute.GetName (),
                "valueType", GS::UniString (attribute.GetValueType ())
            );
            // Optional IFC attributes can have no value, then value is left out
            const auto value = attribute.GetValue ();
            if (value.has_value ()) {
                ifcAttribute.Add ("value", *value);
            }
            ifcAttributes (ifcAttribute);
        }

        elementIFCAttributes (elemIFCAttributes);
#else
        GS::Array<API_IFCAttribute> attributes;
        const GSErrCode err = ACAPI_Element_GetIFCAttributes (elemGuid, storedOnly, &attributes);
        if (err != NoError) {
            elementIFCAttributes (CreateErrorResponse (err, "Failed to get the IFC attributes of the element"));
            continue;
        }

        GS::ObjectState elemIFCAttributes = CreateElementIdObjectState (elemGuid);
        const auto& ifcAttributes = elemIFCAttributes.AddList<GS::ObjectState> ("ifcAttributes");
        for (const API_IFCAttribute& attribute : attributes) {
            GS::ObjectState ifcAttribute (
                "name", attribute.attributeName,
                "valueType", attribute.attributeType
            );
            // Optional IFC attributes can have no value (an empty string here), then value is left out
            if (!attribute.attributeValue.IsEmpty ()) {
                ifcAttribute.Add ("value", attribute.attributeValue);
            }
            ifcAttributes (ifcAttribute);
        }

        elementIFCAttributes (elemIFCAttributes);
#endif
    }

    return response;
}

SetIFCAttributesOfElementsCommand::SetIFCAttributesOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetIFCAttributesOfElementsCommand::GetName () const
{
    return "SetIFCAttributesOfElements";
}

GS::Optional<GS::UniString> SetIFCAttributesOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementIFCAttributeValues": {
                "type": "array",
                "description": "The IFC attribute values to set.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "name": {
                            "type": "string",
                            "description": "The name of the IFC attribute, for example Name, Description, ObjectType or Tag. It must be one of the attributes GetIFCAttributesOfElements returns for the element."
                        },
                        "value": {
                            "type": "string",
                            "description": "The new value of the IFC attribute."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "name",
                        "value"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementIFCAttributeValues"
        ]
    })";
}

GS::Optional<GS::UniString> SetIFCAttributesOfElementsCommand::GetRawResponseSchema () const
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

GS::ObjectState SetIFCAttributesOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    UNUSED_PARAMETER (parameters);
    // The IFCAPI introduced in Archicad 28 replaced ACAPI_Element_SetIFCAttribute without a write counterpart here yet
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "Writing IFC attributes is not supported in Archicad 28 and newer yet.");
#else
    GS::Array<GS::ObjectState> elementIFCAttributeValues;
    parameters.Get ("elementIFCAttributeValues", elementIFCAttributeValues);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    struct SettableIFCAttribute {
        API_Guid         elemGuid;
        API_IFCAttribute attribute;
        UIndex           resultIndex;
    };

    GS::Array<GS::ObjectState> results (elementIFCAttributeValues.GetSize ());
    GS::Array<SettableIFCAttribute> attributesToSet;

    for (const GS::ObjectState& elementIFCAttributeValue : elementIFCAttributeValues) {
        const GS::ObjectState* elementId = elementIFCAttributeValue.Get ("elementId");
        if (elementId == nullptr) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        GS::UniString name;
        if (!elementIFCAttributeValue.Get ("name", name) || name.IsEmpty ()) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "name is missing"));
            continue;
        }

        GS::UniString value;
        if (!elementIFCAttributeValue.Get ("value", value)) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "value must be a string"));
            continue;
        }

        const API_Guid elemGuid = GetGuidFromObjectState (*elementId);
        API_Elem_Head elemHeader;
        if (!LoadElementHeaderByGuid (elemGuid, elemHeader)) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "The given element was not found"));
            continue;
        }

        // Start from the attribute of the element, so its IFC value type is kept
        GS::Array<API_IFCAttribute> attributes;
        const GSErrCode err = ACAPI_Element_GetIFCAttributes (elemGuid, false, &attributes);
        if (err != NoError) {
            results.Push (CreateFailedExecutionResult (err, "Failed to get the IFC attributes of the element"));
            continue;
        }

        const API_IFCAttribute* foundAttribute = nullptr;
        for (const API_IFCAttribute& attribute : attributes) {
            if (attribute.attributeName == name) {
                foundAttribute = &attribute;
                break;
            }
        }
        if (foundAttribute == nullptr) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS, GS::UniString::Printf ("The element has no IFC attribute named %T", name.ToPrintf ())));
            continue;
        }

        API_IFCAttribute attribute = *foundAttribute;
        attribute.attributeValue = value;

        attributesToSet.Push ({ elemGuid, attribute, results.GetSize () });
        results.PushNew ();
    }

    ACAPI_CallUndoableCommand ("SetIFCAttributesOfElementsCommand", [&]() -> GSErrCode {
        for (const SettableIFCAttribute& attributeToSet : attributesToSet) {
            const GSErrCode err = ACAPI_Element_SetIFCAttribute (attributeToSet.elemGuid, attributeToSet.attribute);
            results[attributeToSet.resultIndex] = err == NoError
                ? CreateSuccessfulExecutionResult ()
                : CreateFailedExecutionResult (err, "Failed to set the IFC attribute for the element");
        }
        return NoError;
    });

    for (const GS::ObjectState& result : results) {
        executionResults (result);
    }

    return response;
#endif
}

SetIFCPropertiesOfElementsCommand::SetIFCPropertiesOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetIFCPropertiesOfElementsCommand::GetName () const
{
    return "SetIFCPropertiesOfElements";
}

GS::Optional<GS::UniString> SetIFCPropertiesOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementIFCPropertyValues": {
                "type": "array",
                "description": "The IFC property values to set. Only single value (IfcPropertySingleValue) properties are supported.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "propertySetName": {
                            "type": "string",
                            "description": "The name of the property set, for example Pset_DoorCommon. Custom property set names are also accepted."
                        },
                        "propertyName": {
                            "type": "string",
                            "description": "The name of the property, for example FireRating."
                        },
                        "type": {
                            "type": "string",
                            "description": "The IFC type of the value, for example IfcLabel, IfcText, IfcIdentifier, IfcBoolean, IfcLogical, IfcInteger, IfcReal or a measure type like IfcThermalTransmittanceMeasure. A type Archicad does not know is rejected."
                        },
                        "value": {
                            "description": "The new value of the property. Provide a boolean for IfcBoolean and IfcLogical (IfcLogical also accepts the string Unknown), a number for the numeric types and a string for every other type."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "propertySetName",
                        "propertyName",
                        "type",
                        "value"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementIFCPropertyValues"
        ]
    })";
}

GS::Optional<GS::UniString> SetIFCPropertiesOfElementsCommand::GetRawResponseSchema () const
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

#ifndef ServerMainVers_2800

static bool ParseIFCPropertyValue (const GS::ObjectState& elementIFCPropertyValue, const GS::UniString& valueType, API_IFCPropertyAnyValue& anyValue, GS::UniString& errorMessage)
{
    // Archicad tells which primitive type an IFC value type is stored as, and fails for an unknown value type
    API_IFCPropertyValuePrimitiveType primitiveType;
    if (ACAPI_Element_GetIFCPropertyValuePrimitiveType (valueType, &primitiveType) != NoError) {
        errorMessage = GS::UniString::Printf ("%T is not a known IFC value type", valueType.ToPrintf ());
        return false;
    }

    if (primitiveType == API_IFCPropertyAnyValueBooleanType || primitiveType == API_IFCPropertyAnyValueLogicalType) {
        bool boolValue = false;
        if (elementIFCPropertyValue.Get ("value", boolValue)) {
            if (primitiveType == API_IFCPropertyAnyValueLogicalType) {
                // Logicals are stored in intValue as 0 = Unknown, 1 = False, 2 = True (see PropertyValueToString above)
                anyValue.primitiveType = API_IFCPropertyAnyValueLogicalType;
                anyValue.intValue = boolValue ? 2 : 1;
            } else {
                anyValue.primitiveType = API_IFCPropertyAnyValueBooleanType;
                anyValue.boolValue = boolValue;
            }
            return true;
        }
        GS::UniString stringValue;
        if (primitiveType == API_IFCPropertyAnyValueLogicalType && elementIFCPropertyValue.Get ("value", stringValue) && stringValue == UnknownString) {
            anyValue.primitiveType = API_IFCPropertyAnyValueLogicalType;
            anyValue.intValue = 0;
            return true;
        }
        errorMessage = GS::UniString::Printf ("value must be a boolean for %T", valueType.ToPrintf ());
        return false;
    }

    if (primitiveType == API_IFCPropertyAnyValueIntegerType) {
        Int64 intValue = 0;
        if (!elementIFCPropertyValue.Get ("value", intValue)) {
            errorMessage = GS::UniString::Printf ("value must be an integer for %T", valueType.ToPrintf ());
            return false;
        }
        anyValue.primitiveType = API_IFCPropertyAnyValueIntegerType;
        anyValue.intValue = intValue;
        return true;
    }

    if (primitiveType == API_IFCPropertyAnyValueRealType) {
        double doubleValue = 0.0;
        if (!elementIFCPropertyValue.Get ("value", doubleValue)) {
            errorMessage = GS::UniString::Printf ("value must be a number for %T", valueType.ToPrintf ());
            return false;
        }
        anyValue.primitiveType = API_IFCPropertyAnyValueRealType;
        anyValue.doubleValue = doubleValue;
        return true;
    }

    GS::UniString stringValue;
    if (!elementIFCPropertyValue.Get ("value", stringValue)) {
        errorMessage = GS::UniString::Printf ("value must be a string for %T", valueType.ToPrintf ());
        return false;
    }
    anyValue.primitiveType = API_IFCPropertyAnyValueStringType;
    anyValue.stringValue = stringValue;
    return true;
}

#endif

GS::ObjectState SetIFCPropertiesOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    UNUSED_PARAMETER (parameters);
    // The IFCAPI introduced in Archicad 28 replaced ACAPI_Element_SetIFCProperty without a write counterpart here yet
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "Writing IFC properties is not supported in Archicad 28 and newer yet.");
#else
    GS::Array<GS::ObjectState> elementIFCPropertyValues;
    parameters.Get ("elementIFCPropertyValues", elementIFCPropertyValues);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    struct SettableIFCProperty {
        API_Guid        elemGuid;
        API_IFCProperty property;
        UIndex          resultIndex;
    };

    GS::Array<GS::ObjectState> results (elementIFCPropertyValues.GetSize ());
    GS::Array<SettableIFCProperty> propertiesToSet;

    for (const GS::ObjectState& elementIFCPropertyValue : elementIFCPropertyValues) {
        const GS::ObjectState* elementId = elementIFCPropertyValue.Get ("elementId");
        if (elementId == nullptr) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        GS::UniString propertySetName;
        if (!elementIFCPropertyValue.Get ("propertySetName", propertySetName) || propertySetName.IsEmpty ()) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "propertySetName is missing"));
            continue;
        }

        GS::UniString propertyName;
        if (!elementIFCPropertyValue.Get ("propertyName", propertyName) || propertyName.IsEmpty ()) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "propertyName is missing"));
            continue;
        }

        GS::UniString valueType;
        if (!elementIFCPropertyValue.Get ("type", valueType) || valueType.IsEmpty ()) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "type is missing"));
            continue;
        }

        const API_Guid elemGuid = GetGuidFromObjectState (*elementId);
        API_Elem_Head elemHeader;
        if (!LoadElementHeaderByGuid (elemGuid, elemHeader)) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "The given element was not found"));
            continue;
        }

        API_IFCProperty property = {};
        property.head.propertySetName = propertySetName;
        property.head.propertyName = propertyName;
        property.head.propertyType = API_IFCPropertySingleValueType;
        property.singleValue.nominalValue.valueType = valueType;

        GS::UniString errorMessage;
        if (!ParseIFCPropertyValue (elementIFCPropertyValue, valueType, property.singleValue.nominalValue.value, errorMessage)) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS, errorMessage));
            continue;
        }

        propertiesToSet.Push ({ elemGuid, property, results.GetSize () });
        results.PushNew ();
    }

    ACAPI_CallUndoableCommand ("SetIFCPropertiesOfElementsCommand", [&]() -> GSErrCode {
        for (const SettableIFCProperty& propertyToSet : propertiesToSet) {
            const GSErrCode err = ACAPI_Element_SetIFCProperty (propertyToSet.elemGuid, propertyToSet.property);
            results[propertyToSet.resultIndex] = err == NoError
                ? CreateSuccessfulExecutionResult ()
                : CreateFailedExecutionResult (err, "Failed to set the IFC property for the element");
        }
        return NoError;
    });

    for (const GS::ObjectState& result : results) {
        executionResults (result);
    }

    return response;
#endif
}

RemoveIFCPropertiesOfElementsCommand::RemoveIFCPropertiesOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String RemoveIFCPropertiesOfElementsCommand::GetName () const
{
    return "RemoveIFCPropertiesOfElements";
}

GS::Optional<GS::UniString> RemoveIFCPropertiesOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementIFCProperties": {
                "type": "array",
                "description": "The local IFC properties to remove.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "propertySetName": {
                            "type": "string",
                            "description": "The name of the property set, for example Pset_DoorCommon."
                        },
                        "propertyName": {
                            "type": "string",
                            "description": "The name of the property, for example FireRating."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "propertySetName",
                        "propertyName"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementIFCProperties"
        ]
    })";
}

GS::Optional<GS::UniString> RemoveIFCPropertiesOfElementsCommand::GetRawResponseSchema () const
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

GS::ObjectState RemoveIFCPropertiesOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    UNUSED_PARAMETER (parameters);
    // The IFCAPI introduced in Archicad 28 replaced ACAPI_Element_RemoveIFCProperty without a write counterpart here yet
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "Removing IFC properties is not supported in Archicad 28 and newer yet.");
#else
    GS::Array<GS::ObjectState> elementIFCProperties;
    parameters.Get ("elementIFCProperties", elementIFCProperties);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    struct RemovableIFCProperty {
        API_Guid        elemGuid;
        API_IFCProperty property;
        UIndex          resultIndex;
    };

    GS::Array<GS::ObjectState> results (elementIFCProperties.GetSize ());
    GS::Array<RemovableIFCProperty> propertiesToRemove;

    for (const GS::ObjectState& elementIFCProperty : elementIFCProperties) {
        const GS::ObjectState* elementId = elementIFCProperty.Get ("elementId");
        if (elementId == nullptr) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        GS::UniString propertySetName;
        if (!elementIFCProperty.Get ("propertySetName", propertySetName) || propertySetName.IsEmpty ()) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "propertySetName is missing"));
            continue;
        }

        GS::UniString propertyName;
        if (!elementIFCProperty.Get ("propertyName", propertyName) || propertyName.IsEmpty ()) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "propertyName is missing"));
            continue;
        }

        const API_Guid elemGuid = GetGuidFromObjectState (*elementId);
        API_Elem_Head elemHeader;
        if (!LoadElementHeaderByGuid (elemGuid, elemHeader)) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "The given element was not found"));
            continue;
        }

        // Only the properties stored on the element can be removed, the ones the translator generates cannot
        GS::Array<API_IFCProperty> storedProperties;
        const GSErrCode err = ACAPI_Element_GetIFCProperties (elemGuid, true, &storedProperties);
        if (err != NoError) {
            results.Push (CreateFailedExecutionResult (err, "Failed to get IFC properties for the element"));
            continue;
        }

        const API_IFCProperty* propertyToRemove = nullptr;
        for (const API_IFCProperty& storedProperty : storedProperties) {
            if (storedProperty.head.propertySetName == propertySetName && storedProperty.head.propertyName == propertyName) {
                propertyToRemove = &storedProperty;
                break;
            }
        }
        if (propertyToRemove == nullptr) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "The element has no local IFC property with the given name in the given property set"));
            continue;
        }

        propertiesToRemove.Push ({ elemGuid, *propertyToRemove, results.GetSize () });
        results.PushNew ();
    }

    ACAPI_CallUndoableCommand ("RemoveIFCPropertiesOfElementsCommand", [&]() -> GSErrCode {
        for (const RemovableIFCProperty& propertyToRemove : propertiesToRemove) {
            const GSErrCode err = ACAPI_Element_RemoveIFCProperty (propertyToRemove.elemGuid, propertyToRemove.property);
            results[propertyToRemove.resultIndex] = err == NoError
                ? CreateSuccessfulExecutionResult ()
                : CreateFailedExecutionResult (err, "Failed to remove the IFC property from the element");
        }
        return NoError;
    });

    for (const GS::ObjectState& result : results) {
        executionResults (result);
    }

    return response;
#endif
}

GetIFCExportTranslatorsCommand::GetIFCExportTranslatorsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetIFCExportTranslatorsCommand::GetName () const
{
    return "GetIFCExportTranslators";
}

GS::Optional<GS::UniString> GetIFCExportTranslatorsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "translators": {
                "type": "array",
                "description": "The IFC export translators of the project, the preview translator first.",
                "items": {
                    "$ref": "#/IFCExportTranslator"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "translators"
        ]
    })";
}

GS::ObjectState GetIFCExportTranslatorsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<API_IFCTranslatorIdentifier> translators;
    const GSErrCode err = ACAPI_IFC_GetIFCExportTranslatorsList (translators);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to list the IFC export translators of the project");
    }

    GS::ObjectState response;
    const auto& translatorList = response.AddList<GS::ObjectState> ("translators");
    for (UIndex i = 0; i < translators.GetSize (); ++i) {
        GS::ObjectState item;
        item.Add ("name", translators[i].name);
        item.Add ("preview", i == 0);
        translatorList (item);
    }

    return response;
}
