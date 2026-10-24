#include "CustomDataCommands.hpp"

#include <cstring>
#include <string>
#include <utility>
#include <vector>

// The element user data commands keep script state on an element with the element user data API:
// one blob per element per add-on, stored in the element itself, so it is saved with the project,
// goes away with the element and is shown nowhere in the Archicad UI. Because the whole add-on has
// a single blob on each element, it holds every key of every script, as pairs that are merged on
// set: a script touches only the keys it names. Layout version 1 is the pairs as UTF-8 text, each
// key and each value followed by a NUL; a blob with a layout version this build does not know is
// reported, never overwritten. UTF-8 bytes do not depend on the platform, so platformSign is only
// recorded, never swapped.

namespace {

constexpr short ElementUserDataLayoutVersion = 1;

using UserDataPairs = std::vector<std::pair<GS::UniString, GS::UniString>>;

GS::ObjectState CreateUserDataItem (const GS::UniString& key, const GS::UniString& value)
{
    GS::ObjectState item;
    item.Add ("key", key);
    item.Add ("value", value);
    return item;
}

void SetUserDataPair (UserDataPairs& pairs, const GS::UniString& key, const GS::UniString& value)
{
    for (auto& pair : pairs) {
        if (pair.first == key) {
            pair.second = value;
            return;
        }
    }
    pairs.emplace_back (key, value);
}

void DeleteUserDataPair (UserDataPairs& pairs, const GS::UniString& key)
{
    for (auto it = pairs.begin (); it != pairs.end (); ++it) {
        if (it->first == key) {
            pairs.erase (it);
            return;
        }
    }
}

// Layout version 1. An element without any pair keeps a one byte blob (a single NUL), which reads
// back as no pairs, so the element does not need its user data removed.
GSHandle SerializeElementUserData (const UserDataPairs& pairs)
{
    std::string bytes;
    for (const auto& pair : pairs) {
        bytes.append (pair.first.ToCStr (CC_UTF8).Get ());
        bytes.push_back ('\0');
        bytes.append (pair.second.ToCStr (CC_UTF8).Get ());
        bytes.push_back ('\0');
    }
    if (bytes.empty ()) {
        bytes.push_back ('\0');
    }

    const GSSize size = static_cast<GSSize> (bytes.size ());
    GSHandle handle = BMAllocateHandle (size, ALLOCATE_CLEAR, 0);
    if (handle != nullptr) {
        memcpy (*handle, bytes.data (), size);
    }
    return handle;
}

UserDataPairs ParseElementUserData (GSHandle dataHdl)
{
    UserDataPairs pairs;
    if (dataHdl == nullptr) {
        return pairs;
    }

    const char* data = *dataHdl;
    const GSSize size = BMGetHandleSize (dataHdl);
    GSSize pos = 0;

    // Reads the next NUL terminated string; false when the data has ended.
    const auto readString = [&] (GS::UniString& str) -> bool {
        if (pos >= size) {
            return false;
        }
        GSSize end = pos;
        while (end < size && data[end] != '\0') {
            ++end;
        }
        str = GS::UniString (data + pos, static_cast<USize> (end - pos), CC_UTF8);
        pos = end + 1;
        return true;
    };

    GS::UniString key;
    GS::UniString value;
    while (readString (key)) {
        if (!readString (value)) {
            value = GS::EmptyUniString;
        }
        if (!key.IsEmpty ()) {
            SetUserDataPair (pairs, key, value);
        }
    }
    return pairs;
}

// Reads the pairs stored on the element: none when the element has no user data of this add-on.
// A failure fills errorMessage.
GSErrCode ReadElementUserData (API_Elem_Head& elemHead, UserDataPairs& pairs, GS::UniString& errorMessage)
{
    pairs.clear ();

    API_UserData userData = {};
    GSErrCode err = ACAPI_Element_GetUserData (&elemHead, &userData);
    if (err == APIERR_NOUSERDATA) {
        return NoError;
    }
    if (err != NoError) {
        errorMessage = "Failed to read the user data of the element.";
        return err;
    }
    if (userData.dataHdl == nullptr) {
        return NoError;
    }

    if (userData.dataVersion != ElementUserDataLayoutVersion) {
        errorMessage = GS::UniString::Printf ("The user data of the element was stored with an unknown layout (version %d); update Tapir to read it.",
                                              static_cast<int> (userData.dataVersion));
        err = APIERR_BADPARS;
    } else {
        pairs = ParseElementUserData (userData.dataHdl);
    }

    BMKillHandle (&userData.dataHdl);
    return err;
}

GSErrCode StoreElementUserData (API_Elem_Head& elemHead, const UserDataPairs& pairs)
{
    API_UserData userData = {};
    userData.dataVersion = ElementUserDataLayoutVersion;
    userData.platformSign = GS::Act_Platform_Sign;
    // Undoable, so Edit > Undo reverts the stored pairs. Not FillWith or Pickup: the data is script
    // state (ids, timestamps), not a setting of the element, so it must not travel with parameter
    // pick-up or injection into the tool defaults.
    userData.flags = APIUserDataFlag_UndoAble;
    userData.dataHdl = SerializeElementUserData (pairs);
    if (userData.dataHdl == nullptr) {
        return APIERR_MEMFULL;
    }

    // The API copies the data, the handle stays ours to free.
    const GSErrCode err = ACAPI_Element_SetUserData (&elemHead, &userData);
    BMKillHandle (&userData.dataHdl);
    return err;
}

} // namespace

SetElementUserDataCommand::SetElementUserDataCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetElementUserDataCommand::GetName () const
{
    return "SetElementUserData";
}

GS::Optional<GS::UniString> SetElementUserDataCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementUserData": {
                "type": "array",
                "description": "The elements with the key-value pairs to store on them. Each item needs userData, keysToDelete or both.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "userData": {
                            "type": "array",
                            "description": "The key-value pairs to store on the element. A key that already exists on the element gets the new value; other keys are left as they are.",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "key": {
                                        "type": "string",
                                        "description": "The key of the value. Scripts should prefix their keys (for example 'myscript.databaseId') so that different scripts do not overwrite each other's data.",
                                        "minLength": 1
                                    },
                                    "value": {
                                        "type": "string",
                                        "description": "The value to store. Any text, for example an id, a timestamp or a JSON document the script encodes itself."
                                    }
                                },
                                "additionalProperties": false,
                                "required": [
                                    "key",
                                    "value"
                                ]
                            }
                        },
                        "keysToDelete": {
                            "type": "array",
                            "description": "The keys to remove from the element. A key the element does not hold is ignored.",
                            "items": {
                                "type": "string",
                                "minLength": 1
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId"
                    ]
                },
                "minItems": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "elementUserData"
        ]
    })";
}

GS::Optional<GS::UniString> SetElementUserDataCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults",
                "description": "One result per input item, in the input order."
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    })";
}

GS::ObjectState SetElementUserDataCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elementUserData;
    parameters.Get ("elementUserData", elementUserData);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    // One undo step for the whole call. The lambda reports success when any element was changed,
    // so a failed element does not roll back the others; each element carries its own result.
    bool executed = false;
    const GSErrCode undoErr = ACAPI_CallUndoableCommand ("SetElementUserData", [&] () -> GSErrCode {
        executed = true;
        GSErrCode firstErr = NoError;
        bool anyStored = false;

        const auto fail = [&] (GSErrCode err, const GS::UniString& message) {
            executionResults (CreateFailedExecutionResult (err, message));
            if (firstErr == NoError) {
                firstErr = err;
            }
        };

        for (const GS::ObjectState& item : elementUserData) {
            const GS::ObjectState* elementId = item.Get ("elementId");
            if (elementId == nullptr) {
                fail (APIERR_BADPARS, "Invalid input: elementId is missing.");
                continue;
            }

            GS::Array<GS::ObjectState> pairsToSet;
            GS::Array<GS::UniString> keysToDelete;
            const bool hasPairsToSet = item.Get ("userData", pairsToSet);
            const bool hasKeysToDelete = item.Get ("keysToDelete", keysToDelete);
            if (!hasPairsToSet && !hasKeysToDelete) {
                fail (APIERR_BADPARS, "Invalid input: userData or keysToDelete is required.");
                continue;
            }

            bool pairsAreValid = true;
            for (const GS::ObjectState& pair : pairsToSet) {
                GS::UniString key;
                GS::UniString value;
                if (!pair.Get ("key", key) || key.IsEmpty () || !pair.Get ("value", value)) {
                    pairsAreValid = false;
                    break;
                }
            }
            if (!pairsAreValid) {
                fail (APIERR_BADPARS, "Invalid input: every userData item needs a non-empty key and a value.");
                continue;
            }

            API_Elem_Head elemHead = {};
            if (!LoadElementHeaderByGuid (GetGuidFromObjectState (*elementId), elemHead)) {
                fail (APIERR_BADID, "Element not found.");
                continue;
            }

            UserDataPairs pairs;
            GS::UniString errorMessage;
            GSErrCode err = ReadElementUserData (elemHead, pairs, errorMessage);
            if (err != NoError) {
                fail (err, errorMessage);
                continue;
            }

            for (const GS::ObjectState& pair : pairsToSet) {
                GS::UniString key;
                GS::UniString value;
                pair.Get ("key", key);
                pair.Get ("value", value);
                SetUserDataPair (pairs, key, value);
            }
            for (const GS::UniString& key : keysToDelete) {
                DeleteUserDataPair (pairs, key);
            }

            err = StoreElementUserData (elemHead, pairs);
            if (err == NoError) {
                anyStored = true;
                executionResults (CreateSuccessfulExecutionResult ());
            } else if (!ACAPI_Element_Filter (elemHead.guid, APIFilt_IsEditable)) {
                fail (err, "Failed to store the user data: the element is not editable (locked, on a locked layer, or not reserved by you in Teamwork).");
            } else {
                fail (err, "Failed to store the user data of the element.");
            }
        }

        return anyStored ? NoError : firstErr;
    });

    if (!executed) {
        return CreateErrorResponse (undoErr != NoError ? undoErr : APIERR_GENERAL, "Failed to start the undoable operation that stores the user data.");
    }

    return response;
}

GetElementUserDataCommand::GetElementUserDataCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetElementUserDataCommand::GetName () const
{
    return "GetElementUserData";
}

GS::Optional<GS::UniString> GetElementUserDataCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "keys": {
                "type": "array",
                "description": "Optional. Only the pairs with these keys are returned; a key an element does not hold is left out of its pairs. When omitted, every pair stored on each element is returned.",
                "items": {
                    "type": "string",
                    "minLength": 1
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> GetElementUserDataCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementUserData": {
                "type": "array",
                "description": "One item per element, in the input order: the pairs stored on the element (none when the element holds no user data of Tapir), or an error when the element was not found or its data could not be read.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "userData": {
                                    "type": "array",
                                    "description": "The key-value pairs stored on the element.",
                                    "items": {
                                        "type": "object",
                                        "properties": {
                                            "key": {
                                                "type": "string",
                                                "description": "The key of the value."
                                            },
                                            "value": {
                                                "type": "string",
                                                "description": "The stored value."
                                            }
                                        },
                                        "additionalProperties": false,
                                        "required": [
                                            "key",
                                            "value"
                                        ]
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "userData"
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
            "elementUserData"
        ]
    })";
}

GS::ObjectState GetElementUserDataCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::Array<GS::UniString> keys;
    const bool filterKeys = parameters.Contains ("keys");
    if (filterKeys) {
        parameters.Get ("keys", keys);
    }

    GS::ObjectState response;
    const auto& elementUserData = response.AddList<GS::ObjectState> ("elementUserData");

    for (const GS::ObjectState& element : elements) {
        API_Elem_Head elemHead = {};
        if (!LoadElementHeaderByGuid (GetGuidFromElementsArrayItem (element), elemHead)) {
            // CreateErrorResponse is already the {"error": {code, message}} shape of an ErrorItem.
            elementUserData (CreateErrorResponse (APIERR_BADID, "Element not found."));
            continue;
        }

        UserDataPairs pairs;
        GS::UniString errorMessage;
        const GSErrCode err = ReadElementUserData (elemHead, pairs, errorMessage);
        if (err != NoError) {
            elementUserData (CreateErrorResponse (err, errorMessage));
            continue;
        }

        GS::ObjectState item;
        const auto& userData = item.AddList<GS::ObjectState> ("userData");
        for (const auto& pair : pairs) {
            if (!filterKeys || keys.Contains (pair.first)) {
                userData (CreateUserDataItem (pair.first, pair.second));
            }
        }
        elementUserData (item);
    }

    return response;
}
