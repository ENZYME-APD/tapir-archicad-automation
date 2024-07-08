#include "ElementCommands.hpp"
#include "MigrationHelper.hpp"

#include <array>

GetSelectedElementsCommand::GetSelectedElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetSelectedElementsCommand::GetName () const
{
    return "GetSelectedElements";
}

GS::Optional<GS::UniString> GetSelectedElementsCommand::GetResponseSchema () const
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

GS::ObjectState GetSelectedElementsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    API_SelectionInfo selectionInfo;
    GS::Array<API_Neig> selectedNeigs;
    GSErrCode err = ACAPI_Selection_Get (&selectionInfo, &selectedNeigs, false);
    if (err != NoError && err != APIERR_NOSEL) {
        return CreateErrorResponse (err, "Failed to retrieve selected elements.");
    }

    GS::ObjectState response;
    const auto& elementsList = response.AddList<GS::ObjectState> ("elements");
    if (err == APIERR_NOSEL || selectionInfo.typeID == API_SelEmpty) {
        return response;
    }

    GS::Array<API_Guid> selElemGuids;
    for (API_Neig& selectedNeig : selectedNeigs) {
        API_ElemTypeID elemTypeID = API_ZombieElemID;
#ifdef ServerMainVers_2600
        API_ElemType apiElemType;
        if (ACAPI_Element_NeigIDToElemType (selectedNeig.neigID, apiElemType) != NoError) {
            continue;
        }
        elemTypeID = apiElemType.typeID;
#else
        if (ACAPI_Goodies (APIAny_NeigIDToElemTypeID, &selectedNeig.neigID, &elemTypeID) != NoError) {
            continue;
        }
#endif

        API_Guid elemGuid = selectedNeig.guid;
        if (elemTypeID == API_SectElemID) {
            API_Element element = {};
            element.header.guid = selectedNeig.guid;
            if (ACAPI_Element_Get (&element) != NoError) {
                continue;
            }
            elemGuid = element.sectElem.parentGuid;
        }

        GS::ObjectState elemId;
        elemId.Add ("guid", APIGuidToString (elemGuid));

        GS::ObjectState elemIdArrayItem;
        elemIdArrayItem.Add ("elementId", elemId);

        elementsList (elemIdArrayItem);
    }

    return response;
}

MoveElementsCommand::MoveElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String MoveElementsCommand::GetName () const
{
    return "MoveElements";
}

GS::Optional<GS::UniString> MoveElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementsWithMoveVectors": {
                "type": "array",
                "description": "The elements with move vector pairs.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "moveVector": {
                            "type": "object",
                            "description" : "Move vector of a 3D point.",
                            "properties" : {
                                "x": {
                                    "type": "number",
                                    "description" : "X value of the vector."
                                },
                                "y" : {
                                    "type": "number",
                                    "description" : "Y value of the vector."
                                },
                                "z" : {
                                    "type": "number",
                                    "description" : "Z value of the vector."
                                }
                            },
                            "additionalProperties": false,
                            "required" : [
                                "x",
                                "y",
                                "z"
                            ]
                        },
                        "copy": {
                            "type": "boolean",
                            "description" : "Optional parameter. If true, then a copy of the element will be moved. By default it's false."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "moveVector"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementsWithMoveVectors"
        ]
    })";
}

static GSErrCode MoveElement (const API_Guid& elemGuid, const API_Vector3D& moveVector, bool withCopy)
{
    GS::Array<API_Neig> elementsToEdit = { API_Neig (elemGuid) };

    API_EditPars editPars = {};
    editPars.typeID = APIEdit_Drag;
    editPars.endC = moveVector;
    editPars.withDelete = !withCopy;

    return ACAPI_Element_Edit (&elementsToEdit, editPars);
}

GS::ObjectState	MoveElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elementsWithMoveVectors;
    parameters.Get ("elementsWithMoveVectors", elementsWithMoveVectors);

    API_Guid elemGuid;
    const APIErrCodes err = (APIErrCodes) ACAPI_CallUndoableCommand ("Move Elements", [&] () -> GSErrCode {
        for (const GS::ObjectState& elementWithMoveVector : elementsWithMoveVectors) {
            const GS::ObjectState* elementId = elementWithMoveVector.Get ("elementId");
            const GS::ObjectState* moveVector = elementWithMoveVector.Get ("moveVector");
            if (elementId == nullptr || moveVector == nullptr) {
                continue;
            }

            elemGuid = GetGuidFromObjectState (*elementId);

            bool copy = false;
            elementWithMoveVector.Get ("copy", copy);

            const GSErrCode err = MoveElement (elemGuid,
                                               Get3DCoordinateFromObjectState (*moveVector),
                                               copy);
            if (err != NoError) {
                return err;
            }
        }

        return NoError;
    });

    if (err != NoError) {
        const GS::UniString errorMsg = GS::UniString::Printf ("Failed to move element with guid %T!", APIGuidToString (elemGuid).ToPrintf ());
        return CreateErrorResponse (err, errorMsg);
    }

    return {};
}

#ifdef ServerMainVers_2600

HighlightElementsCommand::HighlightElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String HighlightElementsCommand::GetName () const
{
    return "HighlightElements";
}

GS::Optional<GS::UniString> HighlightElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "highlightedColor": {
                "type": "array",
                "description": "Color of the highlighted elements as an [r, g, b, a] array. Each component must be in the 0-255 range.",
                "items": {
                    "type": "integer"
                },
                "minItems": 4,
                "maxItems": 4
            },
            "nonHighlightedColor": {
                "type": "array",
                "description": "Color of the non highlighted elements as an [r, g, b, a] array. Each component must be in the 0-255 range.",
                "items": {
                    "type": "integer"
                },
                "minItems": 4,
                "maxItems": 4
            }
        },
        "additionalProperties": false,
        "required": [
            "elements",
            "highlightedColor",
            "nonHighlightedColor"
        ]
    })";
}

GS::Optional<GS::UniString> HighlightElementsCommand::GetResponseSchema () const
{
    return {};
}

GS::ObjectState HighlightElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elementIdArray;
    parameters.Get ("elements", elementIdArray);

    GS::Array<GS::Int32> highlightedColorArray;
    parameters.Get ("highlightedColor", highlightedColorArray);
    API_RGBAColor highlightedColor {
        highlightedColorArray[0] / 255.0,
        highlightedColorArray[1] / 255.0,
        highlightedColorArray[2] / 255.0,
        highlightedColorArray[3] / 255.0
    };

    GS::Array<GS::Int32> nonHighlightedColorArray;
    parameters.Get ("nonHighlightedColor", nonHighlightedColorArray);
    API_RGBAColor nonHighlightedColor {
        nonHighlightedColorArray[0] / 255.0,
        nonHighlightedColorArray[1] / 255.0,
        nonHighlightedColorArray[2] / 255.0,
        nonHighlightedColorArray[3] / 255.0
    };

    GS::HashTable<API_Guid, API_RGBAColor> elements;
    for (const GS::ObjectState& elementIdArrayItem : elementIdArray) {
        GS::ObjectState elementId;
        elementIdArrayItem.Get ("elementId", elementId);

        GS::UniString guidStr;
        elementId.Get ("guid", guidStr);

        GS::Guid guid (guidStr);
        API_Guid apiGuid = GSGuid2APIGuid (guid);
        elements.Add (apiGuid, highlightedColor);
    }

    if (!elements.IsEmpty ()) {
        ACAPI_UserInput_SetElementHighlight (elements, GS::NoValue, nonHighlightedColor);
    } else {
        ACAPI_UserInput_ClearElementHighlight ();
    }

    // need to call redraw for changes to take effect
    ACAPI_View_Redraw ();

    return {};
}

#endif
