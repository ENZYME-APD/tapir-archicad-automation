#include "ElementCommands.hpp"

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
        if (ACAPI_Goodies_NeigIDToElemType (selectedNeig.neigID, apiElemType) != NoError) {
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
        ACAPI_Interface_SetElementHighlight (elements, GS::NoValue, nonHighlightedColor);
    } else {
        ACAPI_Interface_ClearElementHighlight ();
    }

    // need to call redraw for changes to take effect
    ACAPI_Automate (APIDo_RedrawID);

    return {};
}

#endif
