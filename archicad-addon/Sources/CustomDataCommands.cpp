#include "CustomDataCommands.hpp"

#include <cstring>

// The project data commands keep script state in the project with the module data API: named
// blobs owned by this add-on, saved with the project and shown nowhere in the Archicad UI. Each
// key is one module data name of the add-on; the value is stored as its UTF-8 text with a
// terminating NUL, so even an empty value is a non-empty blob. dataVersion names this layout,
// so a later change of it can still read (or at least recognize) what an older Tapir wrote.
// UTF-8 bytes do not depend on the platform, so platformSign is only recorded, never swapped.

namespace {

constexpr Int32 ProjectDataLayoutVersion = 1;

GS::ObjectState CreateProjectDataItem (const GS::UniString& key, const GS::UniString& value)
{
    GS::ObjectState item;
    item.Add ("key", key);
    item.Add ("value", value);
    return item;
}

GSErrCode StoreProjectData (const GS::UniString& key, const GS::UniString& value)
{
    const auto utf8 = value.ToCStr (CC_UTF8);
    const GSSize size = static_cast<GSSize> (strlen (utf8.Get ())) + 1;

    API_ModulData modulData = {};
    modulData.dataVersion = ProjectDataLayoutVersion;
    modulData.platformSign = GS::Act_Platform_Sign;
    modulData.dataHdl = BMAllocateHandle (size, ALLOCATE_CLEAR, 0);
    if (modulData.dataHdl == nullptr) {
        return APIERR_MEMFULL;
    }
    memcpy (*modulData.dataHdl, utf8.Get (), size);

    // The API copies the data, the handle stays ours to free.
    const GSErrCode err = ACAPI_ModulData_Store (&modulData, key);
    BMKillHandle (&modulData.dataHdl);
    return err;
}

// Reads the value stored under key. A failure fills errorMessage; the error code tells the caller
// whether the key does not exist (whatever code ACAPI_ModulData_Get returns for that) or the stored
// blob has a layout this version cannot read.
GSErrCode ReadProjectData (const GS::UniString& key, GS::UniString& value, GS::UniString& errorMessage)
{
    API_ModulData modulData = {};
    GSErrCode err = ACAPI_ModulData_Get (&modulData, key);
    if (err != NoError) {
        errorMessage = GS::UniString::Printf ("No project data is stored under the key '%T'.", key.ToPrintf ());
        return err;
    }

    if (modulData.dataVersion != ProjectDataLayoutVersion) {
        errorMessage = GS::UniString::Printf ("The project data under the key '%T' was stored with an unknown layout (version %d); update Tapir to read it.",
                                              key.ToPrintf (), static_cast<int> (modulData.dataVersion));
        err = APIERR_BADPARS;
    } else if (modulData.dataHdl != nullptr) {
        GSSize size = BMGetHandleSize (modulData.dataHdl);
        while (size > 0 && (*modulData.dataHdl)[size - 1] == '\0') {
            --size;
        }
        value = GS::UniString (*modulData.dataHdl, static_cast<USize> (size), CC_UTF8);
    } else {
        value = GS::EmptyUniString;
    }

    BMKillHandle (&modulData.dataHdl);
    return err;
}

} // namespace

SetProjectDataCommand::SetProjectDataCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetProjectDataCommand::GetName () const
{
    return "SetProjectData";
}

GS::Optional<GS::UniString> SetProjectDataCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "projectData": {
                "type": "array",
                "description": "The key-value pairs to store. A key that already exists gets the new value; other keys are left as they are.",
                "items": {
                    "type": "object",
                    "properties": {
                        "key": {
                            "type": "string",
                            "description": "The key of the value. Scripts should prefix their keys (for example 'myscript.lastSync') so that different scripts do not overwrite each other's data.",
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
                },
                "minItems": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "projectData"
        ]
    })";
}

GS::Optional<GS::UniString> SetProjectDataCommand::GetRawResponseSchema () const
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

GS::ObjectState SetProjectDataCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> projectData;
    parameters.Get ("projectData", projectData);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    // One undo step for the whole call. The lambda reports success when any item was stored, so a
    // failed item does not roll back the others; each item carries its own result anyway.
    ACAPI_CallUndoableCommand ("SetProjectData", [&] () -> GSErrCode {
        GSErrCode firstErr = NoError;
        bool anyStored = false;
        for (const GS::ObjectState& item : projectData) {
            GS::UniString key;
            GS::UniString value;
            if (!item.Get ("key", key) || key.IsEmpty () || !item.Get ("value", value)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Invalid input: key and value are required."));
                if (firstErr == NoError) {
                    firstErr = APIERR_BADPARS;
                }
                continue;
            }

            const GSErrCode err = StoreProjectData (key, value);
            if (err == NoError) {
                anyStored = true;
                executionResults (CreateSuccessfulExecutionResult ());
            } else {
                executionResults (CreateFailedExecutionResult (err, GS::UniString::Printf ("Failed to store the project data under the key '%T'.", key.ToPrintf ())));
                if (firstErr == NoError) {
                    firstErr = err;
                }
            }
        }
        return anyStored ? NoError : firstErr;
    });

    return response;
}

GetProjectDataCommand::GetProjectDataCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetProjectDataCommand::GetName () const
{
    return "GetProjectData";
}

GS::Optional<GS::UniString> GetProjectDataCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "keys": {
                "type": "array",
                "description": "Optional. The keys to read; the response has one item per key, in this order. When omitted, every key stored in the project is returned with its value.",
                "items": {
                    "type": "string",
                    "minLength": 1
                }
            }
        },
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> GetProjectDataCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "projectData": {
                "type": "array",
                "description": "The stored key-value pairs. With the keys parameter one item per key, in the input order: the pair, or an error when nothing is stored under that key.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
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
            "projectData"
        ]
    })";
}

GS::ObjectState GetProjectDataCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::UniString> keys;
    const bool listAll = !parameters.Contains ("keys");
    if (listAll) {
        const GSErrCode err = ACAPI_ModulData_GetList (&keys);
        if (err != NoError) {
            return CreateErrorResponse (err, "Failed to list the project data stored in the project.");
        }
    } else {
        parameters.Get ("keys", keys);
    }

    GS::ObjectState response;
    const auto& projectData = response.AddList<GS::ObjectState> ("projectData");

    for (const GS::UniString& key : keys) {
        GS::UniString value;
        GS::UniString errorMessage;
        const GSErrCode err = ReadProjectData (key, value, errorMessage);
        if (err == NoError) {
            projectData (CreateProjectDataItem (key, value));
        } else {
            // CreateErrorResponse is already the {"error": {code, message}} shape of an ErrorItem.
            projectData (CreateErrorResponse (err, errorMessage));
        }
    }

    return response;
}

DeleteProjectDataCommand::DeleteProjectDataCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteProjectDataCommand::GetName () const
{
    return "DeleteProjectData";
}

GS::Optional<GS::UniString> DeleteProjectDataCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "keys": {
                "type": "array",
                "description": "The keys to delete from the project.",
                "items": {
                    "type": "string",
                    "minLength": 1
                },
                "minItems": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "keys"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteProjectDataCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults",
                "description": "One result per key, in the input order. Deleting a key that does not exist fails."
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    })";
}

GS::ObjectState DeleteProjectDataCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::UniString> keys;
    parameters.Get ("keys", keys);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("DeleteProjectData", [&] () -> GSErrCode {
        GSErrCode firstErr = NoError;
        bool anyDeleted = false;
        for (const GS::UniString& key : keys) {
            const GSErrCode err = ACAPI_ModulData_Delete (key);
            if (err == NoError) {
                anyDeleted = true;
                executionResults (CreateSuccessfulExecutionResult ());
            } else {
                executionResults (CreateFailedExecutionResult (err, GS::UniString::Printf ("Failed to delete the project data under the key '%T'; it may not exist.", key.ToPrintf ())));
                if (firstErr == NoError) {
                    firstErr = err;
                }
            }
        }
        return anyDeleted ? NoError : firstErr;
    });

    return response;
}
