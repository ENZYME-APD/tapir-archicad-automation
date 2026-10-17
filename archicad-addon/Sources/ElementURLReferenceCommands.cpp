#include "ElementURLReferenceCommands.hpp"
#include "MigrationHelper.hpp"

#include <cstring>

// The URL and the description of an element's web link are fixed-size char arrays
// (API_ElemURLRef::urlLink and ::description), exchanged with Archicad as UTF-8.
// Texts that do not fit are rejected instead of being truncated.

static GS::UniString GetUniStringFromCharArray (const char* text, USize maxLength)
{
    const USize length = static_cast<USize> (strnlen (text, maxLength));
    return GS::UniString (text, length, CC_UTF8);
}

template<std::size_t N>
static bool SetCharArrayFromUniString (const GS::UniString& text, char (&target)[N], GS::UniString& errorMessage, const char* fieldName)
{
    const auto utf8Text = text.ToCStr (CC_UTF8);
    const std::size_t utf8Length = strlen (utf8Text.Get ());
    if (utf8Length >= N) {
        errorMessage = GS::UniString::Printf ("%s is longer than %d bytes in UTF-8 (%d bytes).", fieldName, static_cast<int> (N - 1), static_cast<int> (utf8Length));
        return false;
    }

    CHTruncate (utf8Text.Get (), target, N);
    return true;
}

GetURLReferencesOfElementsCommand::GetURLReferencesOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetURLReferencesOfElementsCommand::GetName () const
{
    return "GetURLReferencesOfElements";
}

GS::Optional<GS::UniString> GetURLReferencesOfElementsCommand::GetInputParametersSchema () const
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

GS::Optional<GS::UniString> GetURLReferencesOfElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "urlReferences": {
                "type": "array",
                "description": "The web link of each given element, or an error. An element without a link gets the error APIERR_NOUSERDATA.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "title": "URLReference",
                            "description": "The web link of an element.",
                            "properties": {
                                "url": {
                                    "type": "string",
                                    "description": "The URL of the link, e.g. https://www.graphisoft.com."
                                },
                                "description": {
                                    "type": "string",
                                    "description": "The description of the link, e.g. GRAPHISOFT Home Page."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "url",
                                "description"
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
            "urlReferences"
        ]
    })";
}

GS::ObjectState GetURLReferencesOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    if (!parameters.Get ("elements", elements)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'elements' parameter.");
    }

    GS::ObjectState response;
    const auto& urlReferences = response.AddList<GS::ObjectState> ("urlReferences");

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            urlReferences (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        API_Elem_Head elemHead = {};
        elemHead.guid = GetGuidFromObjectState (*elementId);

        API_ElemURLRef urlRef = {};
        const GSErrCode err = ACAPI_Element_GetElementURLRef (&elemHead, &urlRef);
        if (err == APIERR_NOUSERDATA) {
            urlReferences (CreateErrorResponse (err, "The element does not hold a URL reference."));
            continue;
        }
        if (err == APIERR_BADID) {
            urlReferences (CreateErrorResponse (err, "The element cannot hold a URL reference."));
            continue;
        }
        if (err != NoError) {
            urlReferences (CreateErrorResponse (err, "Failed to get the URL reference of the element."));
            continue;
        }

        GS::ObjectState urlReference;
        urlReference.Add ("url", GetUniStringFromCharArray (urlRef.urlLink, sizeof (urlRef.urlLink)));
        urlReference.Add ("description", GetUniStringFromCharArray (urlRef.description, sizeof (urlRef.description)));
        urlReferences (urlReference);
    }

    return response;
}

SetURLReferencesOfElementsCommand::SetURLReferencesOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetURLReferencesOfElementsCommand::GetName () const
{
    return "SetURLReferencesOfElements";
}

GS::Optional<GS::UniString> SetURLReferencesOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementsWithURLReferences": {
                "type": "array",
                "description": "The elements and the web link to set on each.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "url": {
                            "type": "string",
                            "description": "The URL of the link, e.g. https://www.graphisoft.com. At most 511 bytes in UTF-8."
                        },
                        "description": {
                            "type": "string",
                            "description": "The description of the link, e.g. GRAPHISOFT Home Page. At most 255 bytes in UTF-8. Empty when omitted."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "url"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementsWithURLReferences"
        ]
    })";
}

GS::Optional<GS::UniString> SetURLReferencesOfElementsCommand::GetRawResponseSchema () const
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

GS::ObjectState SetURLReferencesOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elementsWithURLReferences;
    if (!parameters.Get ("elementsWithURLReferences", elementsWithURLReferences)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'elementsWithURLReferences' parameter.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("SetURLReferencesOfElements", [&]() -> GSErrCode {
        for (const GS::ObjectState& elementWithURLReference : elementsWithURLReferences) {
            const GS::ObjectState* elementId = elementWithURLReference.Get ("elementId");
            if (elementId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
                continue;
            }

            GS::UniString url;
            if (!elementWithURLReference.Get ("url", url)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "url is missing"));
                continue;
            }
            GS::UniString description;
            elementWithURLReference.Get ("description", description);

            API_ElemURLRef urlRef = {};
            GS::UniString errorMessage;
            if (!SetCharArrayFromUniString (url, urlRef.urlLink, errorMessage, "url") ||
                !SetCharArrayFromUniString (description, urlRef.description, errorMessage, "description")) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, errorMessage));
                continue;
            }

            API_Elem_Head elemHead = {};
            elemHead.guid = GetGuidFromObjectState (*elementId);

            const GSErrCode err = ACAPI_Element_SetElementURLRef (&elemHead, &urlRef);
            if (err == APIERR_MOREUSER) {
                executionResults (CreateFailedExecutionResult (err, "The element already holds a URL reference."));
                continue;
            }
            if (err == APIERR_BADID) {
                executionResults (CreateFailedExecutionResult (err, "The element cannot hold a URL reference."));
                continue;
            }
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to set the URL reference of the element."));
                continue;
            }

            executionResults (CreateSuccessfulExecutionResult ());
        }

        return NoError;
    });

    return response;
}
