#include "PropertyObjectCommands.hpp"
#include "MigrationHelper.hpp"
#include "GSUnID.hpp"

// Property objects are the GDL library parts of the old listing system, linked to construction
// elements by criteria or individually; the descriptors are the listing data in them.

// Adds the name and guid of the library part under the given keys, when the library part can be read.
static void AddNameAndGuidOfLibraryPart (GS::ObjectState& os, Int32 libIndex, const char* nameKey, const char* guidKey)
{
    if (libIndex <= 0) {
        return;
    }

    API_LibPart libPart = {};
    libPart.index = libIndex;
    if (ACAPI_LibraryPart_Get (&libPart) != NoError) {
        return;
    }

    os.Add (nameKey, GS::UniString (libPart.docu_UName));
    os.Add (guidKey, GS::UnID (libPart.ownUnID).GetMainGuid ().ToUniString ());

    // ACAPI_LibraryPart_Get allocates libPart.location.
    delete libPart.location;
    libPart.location = nullptr;
}

// Adds the index of the library part, and its name and guid when the library part can be read.
// The index is the same as the index returned by GetAvailableLibraryParts.
static void AddLibraryPartOfPropertyObject (GS::ObjectState& os, Int32 libIndex)
{
    os.Add ("libraryPartIndex", libIndex);
    AddNameAndGuidOfLibraryPart (os, libIndex, "name", "guid");
}

static GSErrCode GetElementHeader (const GS::ObjectState& element, API_Elem_Head& elemHead, GS::ObjectState& error)
{
    const GS::ObjectState* elementId = element.Get ("elementId");
    if (elementId == nullptr) {
        error = CreateErrorResponse (APIERR_BADPARS, "elementId is missing");
        return APIERR_BADPARS;
    }

    elemHead = {};
    elemHead.guid = GetGuidFromObjectState (*elementId);
    const GSErrCode err = ACAPI_Element_GetHeader (&elemHead);
    if (err != NoError) {
        error = CreateErrorResponse (err, "Failed to find element in Archicad");
    }
    return err;
}

// Reads the header of the element given by elementId, or makes the header of the element defaults
// of the type given by elementType (guid is APINULLGuid).
static GSErrCode GetElementOrDefaultsHeader (const GS::ObjectState& element, API_Elem_Head& elemHead, GS::ObjectState& error)
{
    if (element.Get ("elementId") != nullptr) {
        return GetElementHeader (element, elemHead, error);
    }

    GS::UniString elementTypeStr;
    if (!element.Get ("elementType", elementTypeStr)) {
        error = CreateErrorResponse (APIERR_BADPARS, "elementId or elementType is missing");
        return APIERR_BADPARS;
    }

    const API_ElemTypeID elemType = GetElementTypeFromNonLocalizedName (elementTypeStr);
    if (elemType == API_ZombieElemID) {
        error = CreateErrorResponse (APIERR_BADPARS,
            GS::UniString::Printf ("Invalid elementType '%T'.", elementTypeStr.ToPrintf ()));
        return APIERR_BADPARS;
    }

    elemHead = {};
#ifdef ServerMainVers_2600
    elemHead.type = elemType;
#else
    elemHead.typeID = elemType;
#endif
    elemHead.guid = APINULLGuid;
    return NoError;
}

// The pointer overload of ACAPI_Element_GetPropertyObjects is deprecated from Archicad 30, and
// deprecation warnings fail the Windows build, so Archicad 30 uses the std::vector overload.
static GSErrCode GetPropertyObjectRefs (const API_Elem_Head& elemHead, GS::Array<API_PropertyObjectRefType>& propRefs)
{
#ifdef ServerMainVers_3000
    std::vector<API_PropertyObjectRefType> propRefVector;
    const GSErrCode err = ACAPI_Element_GetPropertyObjects (elemHead, propRefVector);
    if (err != NoError) {
        return err;
    }
    for (const API_PropertyObjectRefType& propRef : propRefVector) {
        propRefs.Push (propRef);
    }
    return NoError;
#else
    API_PropertyObjectRefType** propRefHandle = nullptr;
    Int32 nProp = 0;
    const GSErrCode err = ACAPI_Element_GetPropertyObjects (&elemHead, &propRefHandle, &nProp);
    const GS::OnExit guard ([&propRefHandle] () {
        if (propRefHandle != nullptr) {
            BMKillHandle (reinterpret_cast<GSHandle*> (&propRefHandle));
        }
    });
    if (err != NoError) {
        return err;
    }
    if (propRefHandle != nullptr) {
        for (Int32 i = 0; i < nProp; ++i) {
            propRefs.Push ((*propRefHandle)[i]);
        }
    }
    return NoError;
#endif
}

static GS::UniString DescriptorStatusToString (Int32 status)
{
    switch (status) {
        case APIDBRef_Normal:  return "Normal";
        case APIDBRef_Deleted: return "Deleted";
        case APIDBRef_Local:   return "Local";
        default:               return "Unknown";
    }
}

GetPropertyObjectsOfElementsCommand::GetPropertyObjectsOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetPropertyObjectsOfElementsCommand::GetName () const
{
    return "GetPropertyObjectsOfElements";
}

GS::Optional<GS::UniString> GetPropertyObjectsOfElementsCommand::GetInputParametersSchema () const
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

GS::Optional<GS::UniString> GetPropertyObjectsOfElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "propertyObjectsOfElements": {
                "type": "array",
                "description": "The property objects of each element, aligned with the input array of elements, or an error.",
                "items": {
                    "oneOf": [
                        {
                            "$ref": "#/ElementPropertyObjects"
                        },
                        {
                            "$ref": "#/ErrorItem"
                        }
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "propertyObjectsOfElements"
        ]
    })";
}

GS::ObjectState GetPropertyObjectsOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& propertyObjectsOfElements = response.AddList<GS::ObjectState> ("propertyObjectsOfElements");

    for (const GS::ObjectState& element : elements) {
        API_Elem_Head elemHead = {};
        GS::ObjectState error;
        if (GetElementHeader (element, elemHead, error) != NoError) {
            propertyObjectsOfElements (error);
            continue;
        }

        GS::Array<API_PropertyObjectRefType> propRefs;
        const GSErrCode err = GetPropertyObjectRefs (elemHead, propRefs);
        if (err != NoError) {
            propertyObjectsOfElements (CreateErrorResponse (err, "Failed to get the property objects of the element"));
            continue;
        }

        GS::ObjectState propertyObjectsOfElement;
        const auto& propertyObjects = propertyObjectsOfElement.AddList<GS::ObjectState> ("propertyObjects");
        for (const API_PropertyObjectRefType& propRef : propRefs) {
            GS::ObjectState propertyObject;
            AddLibraryPartOfPropertyObject (propertyObject, propRef.libIndex);
            propertyObject.Add ("isIndividuallyAssigned", propRef.assign);
            propertyObjects (propertyObject);
        }
        propertyObjectsOfElements (propertyObjectsOfElement);
    }

    return response;
}

GetDescriptorsOfElementsCommand::GetDescriptorsOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetDescriptorsOfElementsCommand::GetName () const
{
    return "GetDescriptorsOfElements";
}

GS::Optional<GS::UniString> GetDescriptorsOfElementsCommand::GetInputParametersSchema () const
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

GS::Optional<GS::UniString> GetDescriptorsOfElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "descriptorsOfElements": {
                "type": "array",
                "description": "The descriptors of each element, aligned with the input array of elements, or an error.",
                "items": {
                    "oneOf": [
                        {
                            "$ref": "#/ElementDescriptors"
                        },
                        {
                            "$ref": "#/ErrorItem"
                        }
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "descriptorsOfElements"
        ]
    })";
}

GS::ObjectState GetDescriptorsOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& descriptorsOfElements = response.AddList<GS::ObjectState> ("descriptorsOfElements");

    for (const GS::ObjectState& element : elements) {
        API_Elem_Head elemHead = {};
        GS::ObjectState error;
        if (GetElementHeader (element, elemHead, error) != NoError) {
            descriptorsOfElements (error);
            continue;
        }

        API_DescriptorRefType** descRefHandle = nullptr;
        Int32 nDesc = 0;
        const GSErrCode err = ACAPI_Element_GetDescriptors (&elemHead, &descRefHandle, &nDesc);
        const GS::OnExit guard ([&descRefHandle] () {
            if (descRefHandle != nullptr) {
                BMKillHandle (reinterpret_cast<GSHandle*> (&descRefHandle));
            }
        });
        if (err != NoError) {
            descriptorsOfElements (CreateErrorResponse (err, "Failed to get the descriptors of the element"));
            continue;
        }

        GS::ObjectState descriptorsOfElement;
        const auto& descriptors = descriptorsOfElement.AddList<GS::ObjectState> ("descriptors");
        if (descRefHandle != nullptr) {
            for (Int32 i = 0; i < nDesc; ++i) {
                const API_DescriptorRefType& descRef = (*descRefHandle)[i];
                GS::ObjectState descriptor;
                AddLibraryPartOfPropertyObject (descriptor, descRef.libIndex);
                descriptor.Add ("setIndex", descRef.setIndex);
                descriptor.Add ("index", descRef.index);
                descriptor.Add ("status", DescriptorStatusToString (descRef.status));
                descriptors (descriptor);
            }
        }
        descriptorsOfElements (descriptorsOfElement);
    }

    return response;
}

GetLinkedPropertyObjectsOfElementsCommand::GetLinkedPropertyObjectsOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetLinkedPropertyObjectsOfElementsCommand::GetName () const
{
    return "GetLinkedPropertyObjectsOfElements";
}

// The element alternative of the oneOf is written out instead of a bare "$ref": "#/ElementIdArrayItem",
// which was the only bare $ref alternative of an inline oneOf in an input schema and is suspected of
// crashing Archicad 28's schema validation (#1911). It accepts the same objects.
GS::Optional<GS::UniString> GetLinkedPropertyObjectsOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "type": "array",
                "description": "A list of elements, or of element types to read the links of the element defaults of that type.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "elementId": {
                                    "$ref": "#/ElementId"
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "elementId"
                            ]
                        },
                        {
                            "type": "object",
                            "properties": {
                                "elementType": {
                                    "$ref": "#/ElementType",
                                    "description": "Read the links of the element defaults of this type instead of an element."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "elementType"
                            ]
                        }
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> GetLinkedPropertyObjectsOfElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "linkedPropertyObjectsOfElements": {
                "type": "array",
                "description": "The property object links of each element or element default, aligned with the input array of elements, or an error.",
                "items": {
                    "oneOf": [
                        {
                            "$ref": "#/ElementLinkedPropertyObjects"
                        },
                        {
                            "$ref": "#/ErrorItem"
                        }
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "linkedPropertyObjectsOfElements"
        ]
    })";
}

GS::ObjectState GetLinkedPropertyObjectsOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& linkedPropertyObjectsOfElements = response.AddList<GS::ObjectState> ("linkedPropertyObjectsOfElements");

    for (const GS::ObjectState& element : elements) {
        API_Elem_Head elemHead = {};
        GS::ObjectState error;
        if (GetElementOrDefaultsHeader (element, elemHead, error) != NoError) {
            linkedPropertyObjectsOfElements (error);
            continue;
        }

        bool criteria = false;
        Int32 individuallyLibInd = 0;
        GS::Array<Int32> critLibInds;
        const GSErrCode err = TAPIR_ElementLink_GetLinkedPropertyObjects (elemHead, criteria, individuallyLibInd, critLibInds);
        if (err != NoError) {
            linkedPropertyObjectsOfElements (CreateErrorResponse (err, "Failed to get the linked property objects of the element"));
            continue;
        }

        GS::ObjectState linkedPropertyObjects;
        linkedPropertyObjects.Add ("linkByCriteria", criteria);
        linkedPropertyObjects.Add ("individualLibraryPartIndex", individuallyLibInd);
        AddNameAndGuidOfLibraryPart (linkedPropertyObjects, individuallyLibInd, "individualLibraryPartName", "individualLibraryPartGuid");
        const auto& criteriaPropertyObjects = linkedPropertyObjects.AddList<GS::ObjectState> ("criteriaPropertyObjects");
        for (Int32 critLibInd : critLibInds) {
            GS::ObjectState propertyObject;
            AddLibraryPartOfPropertyObject (propertyObject, critLibInd);
            criteriaPropertyObjects (propertyObject);
        }
        linkedPropertyObjectsOfElements (linkedPropertyObjects);
    }

    return response;
}

SetLinkedPropertyObjectsOfElementsCommand::SetLinkedPropertyObjectsOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetLinkedPropertyObjectsOfElementsCommand::GetName () const
{
    return "SetLinkedPropertyObjectsOfElements";
}

GS::Optional<GS::UniString> SetLinkedPropertyObjectsOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementsWithLinkedPropertyObjects": {
                "type": "array",
                "description": "The property object links to set, each for an element or for the element defaults of an element type. A link that is not given stays unchanged.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "elementId": {
                                    "$ref": "#/ElementId"
                                },
                                "linkByCriteria": {
                                    "type": "boolean",
                                    "description": "Link the property objects matching their criteria to the element."
                                },
                                "individualLibraryPartIndex": {
                                    "type": "integer",
                                    "minimum": 0,
                                    "description": "The index of the library part of the property object to assign to the element individually, the same as the index returned by GetAvailableLibraryParts. 0 removes the individually assigned property object."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "elementId"
                            ]
                        },
                        {
                            "type": "object",
                            "properties": {
                                "elementType": {
                                    "$ref": "#/ElementType",
                                    "description": "Set the links of the element defaults of this type instead of an element."
                                },
                                "linkByCriteria": {
                                    "type": "boolean",
                                    "description": "Link the property objects matching their criteria to the element defaults."
                                },
                                "individualLibraryPartIndex": {
                                    "type": "integer",
                                    "minimum": 0,
                                    "description": "The index of the library part of the property object to assign to the element defaults individually, the same as the index returned by GetAvailableLibraryParts. 0 removes the individually assigned property object."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "elementType"
                            ]
                        }
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementsWithLinkedPropertyObjects"
        ]
    })";
}

GS::Optional<GS::UniString> SetLinkedPropertyObjectsOfElementsCommand::GetRawResponseSchema () const
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

GS::ObjectState SetLinkedPropertyObjectsOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elementsWithLinkedPropertyObjects;
    parameters.Get ("elementsWithLinkedPropertyObjects", elementsWithLinkedPropertyObjects);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("SetLinkedPropertyObjectsOfElementsCommand", [&]() {
        for (const GS::ObjectState& elementWithLinkedPropertyObjects : elementsWithLinkedPropertyObjects) {
            API_Elem_Head elemHead = {};
            GS::ObjectState error;
            if (GetElementOrDefaultsHeader (elementWithLinkedPropertyObjects, elemHead, error) != NoError) {
                executionResults (CreateFailedExecutionResult (error));
                continue;
            }

            // A link that is not given keeps its current value.
            bool criteria = false;
            Int32 individuallyLibInd = 0;
            GS::Array<Int32> critLibInds;
            GSErrCode err = TAPIR_ElementLink_GetLinkedPropertyObjects (elemHead, criteria, individuallyLibInd, critLibInds);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to get the linked property objects of the element"));
                continue;
            }

            elementWithLinkedPropertyObjects.Get ("linkByCriteria", criteria);
            elementWithLinkedPropertyObjects.Get ("individualLibraryPartIndex", individuallyLibInd);
            if (individuallyLibInd < 0) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "individualLibraryPartIndex must not be negative"));
                continue;
            }

            err = TAPIR_ElementLink_SetLinkedPropertyObjects (elemHead, criteria, individuallyLibInd);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to set the linked property objects of the element"));
                continue;
            }

            executionResults (CreateSuccessfulExecutionResult ());
        }

        return NoError;
    });

    return response;
}
