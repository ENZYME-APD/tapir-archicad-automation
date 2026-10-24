#include "PropertyObjectCommands.hpp"
#include "MigrationHelper.hpp"
#include "GSUnID.hpp"

// Property objects are the GDL library parts of the old listing system, linked to construction
// elements by criteria or individually; the descriptors are the listing data in them.

// Adds the index of the library part, and its name and guid when the library part can be read.
// The index is the same as the index returned by GetAvailableLibraryParts.
static void AddLibraryPartOfPropertyObject (GS::ObjectState& os, Int32 libIndex)
{
    os.Add ("libraryPartIndex", libIndex);
    if (libIndex <= 0) {
        return;
    }

    API_LibPart libPart = {};
    libPart.index = libIndex;
    if (ACAPI_LibraryPart_Get (&libPart) != NoError) {
        return;
    }

    os.Add ("name", GS::UniString (libPart.docu_UName));
    os.Add ("guid", GS::UnID (libPart.ownUnID).GetMainGuid ().ToUniString ());

    // ACAPI_LibraryPart_Get allocates libPart.location.
    delete libPart.location;
    libPart.location = nullptr;
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
                            "type": "object",
                            "properties": {
                                "propertyObjects": {
                                    "type": "array",
                                    "description": "The property objects (library parts of the old listing system) of the element, linked to it by criteria or individually. Empty when the element has none.",
                                    "items": {
                                        "type": "object",
                                        "properties": {
                                            "libraryPartIndex": {
                                                "type": "integer",
                                                "description": "The index of the property object's library part, the same as the index returned by GetAvailableLibraryParts."
                                            },
                                            "name": {
                                                "type": "string",
                                                "description": "The name of the library part. Absent when the library part cannot be read."
                                            },
                                            "guid": {
                                                "type": "string",
                                                "description": "The guid of the library part, the same as the guid returned by GetAvailableLibraryParts. Absent when the library part cannot be read."
                                            },
                                            "isIndividuallyAssigned": {
                                                "type": "boolean",
                                                "description": "True when the property object is assigned to the element individually, false when it is linked by criteria."
                                            }
                                        },
                                        "additionalProperties": false,
                                        "required": [
                                            "libraryPartIndex",
                                            "isIndividuallyAssigned"
                                        ]
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "propertyObjects"
                            ]
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
                            "type": "object",
                            "properties": {
                                "descriptors": {
                                    "type": "array",
                                    "description": "The descriptors in the property objects of the element. Empty when the element has none.",
                                    "items": {
                                        "type": "object",
                                        "properties": {
                                            "libraryPartIndex": {
                                                "type": "integer",
                                                "description": "The index of the library part that declares the descriptor, the same as the index returned by GetAvailableLibraryParts."
                                            },
                                            "name": {
                                                "type": "string",
                                                "description": "The name of the library part. Absent when the library part cannot be read."
                                            },
                                            "guid": {
                                                "type": "string",
                                                "description": "The guid of the library part, the same as the guid returned by GetAvailableLibraryParts. Absent when the library part cannot be read."
                                            },
                                            "setIndex": {
                                                "type": "integer",
                                                "description": "The index of the listing database set of the descriptor. Meaningful only when status is Normal."
                                            },
                                            "index": {
                                                "type": "integer",
                                                "description": "The index of the descriptor in the listing database set when status is Normal, in the library part when status is Local."
                                            },
                                            "status": {
                                                "type": "string",
                                                "description": "Normal: the descriptor exists in the listing database. Deleted: it is referenced, but deleted from the database. Local: it is defined in the library part. Unknown for a value this version of Tapir does not name.",
                                                "enum": [
                                                    "Normal",
                                                    "Deleted",
                                                    "Local",
                                                    "Unknown"
                                                ]
                                            }
                                        },
                                        "additionalProperties": false,
                                        "required": [
                                            "libraryPartIndex",
                                            "setIndex",
                                            "index",
                                            "status"
                                        ]
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "descriptors"
                            ]
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
