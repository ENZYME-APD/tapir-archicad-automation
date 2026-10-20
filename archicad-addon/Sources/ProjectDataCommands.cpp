#include "ProjectDataCommands.hpp"
#include "MigrationHelper.hpp"

#include "HashSet.hpp"

#include <cstring>

// Project data is stored in Tapir's add-on objects: named blobs saved in the project file.
// Archicad keeps them per add-on, so these commands see only the objects Tapir created. The
// content is stored as UTF-8 text without a terminating zero. An add-on object is one of three
// kinds: a normal one, a unique one (in Teamwork only one object can have its name) and a
// client-only one (kept on this client, not shared in Teamwork). The kind is chosen when the
// data is created; setting existing data keeps its kind.

static bool FindProjectDataObject (const GS::UniString& name, API_Guid& guid, bool& clientOnly)
{
    guid = APINULLGuid;
    clientOnly = false;
    if (ACAPI_AddOnObject_GetObjectGuidFromName (name, &guid) == NoError && guid != APINULLGuid) {
        return true;
    }

    guid = APINULLGuid;
    if (ACAPI_AddOnObject_GetUniqueObjectGuidFromName (name, &guid) == NoError && guid != APINULLGuid) {
        return true;
    }

    guid = APINULLGuid;
    if (ACAPI_AddOnObject_GetClientOnlyObjectGuidFromName (name, &guid) == NoError && guid != APINULLGuid) {
        clientOnly = true;
        return true;
    }

    guid = APINULLGuid;
    return false;
}

static bool IsClientOnlyProjectDataObject (const API_Guid& guid, const GS::UniString& name)
{
    API_Guid clientOnlyGuid = APINULLGuid;
    return ACAPI_AddOnObject_GetClientOnlyObjectGuidFromName (name, &clientOnlyGuid) == NoError && clientOnlyGuid == guid;
}

static GSHandle CreateProjectDataContentHandle (const GS::UniString& content)
{
    const auto utf8Content = content.ToCStr (CC_UTF8);
    const char* bytes = utf8Content.Get ();
    const GSSize size = static_cast<GSSize> (strlen (bytes));

    GSHandle handle = BMAllocateHandle (size, ALLOCATE_CLEAR, 0);
    if (handle != nullptr && size > 0) {
        std::memcpy (*handle, bytes, size);
    }
    return handle;
}

static GSErrCode GetProjectDataContent (const API_Guid& guid, GS::UniString& name, GS::UniString& content)
{
    GSHandle handle = nullptr;
    const GSErrCode err = ACAPI_AddOnObject_GetObjectContent (guid, &name, &handle);
    if (handle != nullptr) {
        const GSSize size = BMGetHandleSize (handle);
        if (err == NoError && size > 0) {
            content = GS::UniString (*handle, static_cast<GS::USize> (size), CC_UTF8);
        }
        BMKillHandle (&handle);
    }
    return err;
}

GetProjectDataKeysCommand::GetProjectDataKeysCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetProjectDataKeysCommand::GetName () const
{
    return "GetProjectDataKeys";
}

GS::Optional<GS::UniString> GetProjectDataKeysCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "keys": {
                "type": "array",
                "description": "The project data stored by Tapir in the project.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The name of the project data."
                        },
                        "guid": {
                            "type": "string",
                            "description": "The identifier of the add-on object that stores the project data."
                        },
                        "clientOnly": {
                            "type": "boolean",
                            "description": "True when the project data is kept on this client only and is not shared in Teamwork."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "guid",
                        "clientOnly"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "keys"
        ]
    })";
}

GS::ObjectState GetProjectDataKeysCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<API_Guid> objectGuids;
    const GSErrCode err = ACAPI_AddOnObject_GetObjectList (&objectGuids);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the list of project data.");
    }

    GS::ObjectState response;
    const auto& keys = response.AddList<GS::ObjectState> ("keys");

    for (const API_Guid& objectGuid : objectGuids) {
        GS::UniString name;
        GS::UniString content;
        if (GetProjectDataContent (objectGuid, name, content) != NoError) {
            continue;
        }

        keys (GS::ObjectState (
            "name", name,
            "guid", APIGuidToString (objectGuid),
            "clientOnly", IsClientOnlyProjectDataObject (objectGuid, name)));
    }

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
            "names": {
                "type": "array",
                "description": "The names of the project data to get.",
                "items": {
                    "type": "string",
                    "minLength": 1
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "names"
        ]
    })";
}

GS::Optional<GS::UniString> GetProjectDataCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "values": {
                "type": "array",
                "description": "The project data in the order of the given names, or an error for a name that has no project data.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "name": {
                                    "type": "string",
                                    "description": "The name of the project data."
                                },
                                "content": {
                                    "type": "string",
                                    "description": "The content of the project data."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "name",
                                "content"
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
            "values"
        ]
    })";
}

GS::ObjectState GetProjectDataCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::UniString> names;
    if (!parameters.Get ("names", names)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'names' parameter.");
    }

    GS::ObjectState response;
    const auto& values = response.AddList<GS::ObjectState> ("values");

    for (const GS::UniString& name : names) {
        API_Guid objectGuid;
        bool clientOnly;
        if (!FindProjectDataObject (name, objectGuid, clientOnly)) {
            values (CreateErrorResponse (APIERR_BADPARS, "There is no project data with the name '" + name + "'."));
            continue;
        }

        GS::UniString objectName;
        GS::UniString content;
        const GSErrCode err = GetProjectDataContent (objectGuid, objectName, content);
        if (err != NoError) {
            values (CreateErrorResponse (err, "Failed to get the project data with the name '" + name + "'."));
            continue;
        }

        values (GS::ObjectState ("name", name, "content", content));
    }

    return response;
}

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
            "values": {
                "type": "array",
                "description": "The project data to create or to overwrite. Prefix the names with the name of your tool (for example 'my-tool/settings') so different scripts do not overwrite each other's data.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The name of the project data.",
                            "minLength": 1
                        },
                        "content": {
                            "type": "string",
                            "description": "The content of the project data, for example a JSON string."
                        },
                        "clientOnly": {
                            "type": "boolean",
                            "description": "Creates the project data on this client only, it is not shared in Teamwork. Used only when the project data is created. The default is false."
                        },
                        "unique": {
                            "type": "boolean",
                            "description": "Creates the project data as a unique add-on object: in Teamwork only one can exist with its name, even when several users create it at the same time. Used only when the project data is created, and cannot be combined with clientOnly. The default is false."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "content"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "values"
        ]
    })";
}

GS::Optional<GS::UniString> SetProjectDataCommand::GetRawResponseSchema () const
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

GS::ObjectState SetProjectDataCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> values;
    if (!parameters.Get ("values", values)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'values' parameter.");
    }

    GS::Array<GS::ObjectState> results;
    for (UIndex i = 0; i < values.GetSize (); ++i) {
        results.Push (CreateSuccessfulExecutionResult ());
    }

    const GSErrCode err = ACAPI_CallUndoableCommand ("Set Project Data", [&] () -> GSErrCode {
        GS::HashSet<GS::UniString> seenNames;
        GS::Array<UIndex> newUniqueIndices;
        GS::Array<GS::UniString> newUniqueNames;

        for (UIndex i = 0; i < values.GetSize (); ++i) {
            const GS::ObjectState& value = values[i];

            GS::UniString name;
            GS::UniString content;
            if (!value.Get ("name", name) || name.IsEmpty () || !value.Get ("content", content)) {
                results[i] = CreateFailedExecutionResult (APIERR_BADPARS, "Invalid or missing 'name' or 'content'.");
                continue;
            }
            if (seenNames.Contains (name)) {
                results[i] = CreateFailedExecutionResult (APIERR_BADPARS, "The name '" + name + "' is given more than once.");
                continue;
            }
            seenNames.Add (name);

            bool clientOnly = false;
            bool unique = false;
            value.Get ("clientOnly", clientOnly);
            value.Get ("unique", unique);

            API_Guid objectGuid;
            bool isClientOnlyObject;
            const bool exists = FindProjectDataObject (name, objectGuid, isClientOnlyObject);

            if (!exists && clientOnly && unique) {
                results[i] = CreateFailedExecutionResult (APIERR_BADPARS, "Project data cannot be both clientOnly and unique.");
                continue;
            }

            if (!exists && unique) {
                newUniqueIndices.Push (i);
                newUniqueNames.Push (name);
                continue;
            }

            GSHandle contentHandle = CreateProjectDataContentHandle (content);
            if (contentHandle == nullptr) {
                results[i] = CreateFailedExecutionResult (APIERR_MEMFULL, "Failed to allocate the content of the project data '" + name + "'.");
                continue;
            }

            GSErrCode setErr = NoError;
            if (exists) {
                setErr = ACAPI_AddOnObject_ModifyObject (objectGuid, nullptr, &contentHandle);
            } else if (clientOnly) {
                setErr = ACAPI_AddOnObject_CreateClientOnlyObject (name, contentHandle, &objectGuid);
            } else {
                setErr = ACAPI_AddOnObject_CreateObject (name, contentHandle, &objectGuid);
            }
            BMKillHandle (&contentHandle);

            if (setErr != NoError) {
                results[i] = CreateFailedExecutionResult (setErr, "Failed to set the project data '" + name + "'.");
            }
        }

        if (newUniqueNames.IsEmpty ()) {
            return NoError;
        }

        // A unique add-on object is created without content; the content is set after.
        GS::Array<API_Guid> newUniqueGuids;
        const GSErrCode createErr = ACAPI_AddOnObject_CreateUniqueObjectMore (newUniqueNames, newUniqueGuids);
        for (UIndex j = 0; j < newUniqueIndices.GetSize (); ++j) {
            const UIndex i = newUniqueIndices[j];
            const GS::UniString& name = newUniqueNames[j];

            if (createErr != NoError || j >= newUniqueGuids.GetSize () || newUniqueGuids[j] == APINULLGuid) {
                results[i] = CreateFailedExecutionResult (createErr != NoError ? createErr : APIERR_GENERAL, "Failed to create the unique project data '" + name + "'.");
                continue;
            }

            GS::UniString content;
            values[i].Get ("content", content);
            GSHandle contentHandle = CreateProjectDataContentHandle (content);
            if (contentHandle == nullptr) {
                results[i] = CreateFailedExecutionResult (APIERR_MEMFULL, "Failed to allocate the content of the project data '" + name + "'.");
                continue;
            }

            const GSErrCode modifyErr = ACAPI_AddOnObject_ModifyObject (newUniqueGuids[j], nullptr, &contentHandle);
            BMKillHandle (&contentHandle);

            if (modifyErr != NoError) {
                results[i] = CreateFailedExecutionResult (modifyErr, "Failed to set the content of the unique project data '" + name + "'.");
            }
        }

        return NoError;
    });

    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to set the project data.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");
    for (const GS::ObjectState& result : results) {
        executionResults (result);
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
            "names": {
                "type": "array",
                "description": "The names of the project data to delete.",
                "items": {
                    "type": "string",
                    "minLength": 1
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "names"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteProjectDataCommand::GetRawResponseSchema () const
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

GS::ObjectState DeleteProjectDataCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::UniString> names;
    if (!parameters.Get ("names", names)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'names' parameter.");
    }

    GS::Array<GS::ObjectState> results;

    const GSErrCode err = ACAPI_CallUndoableCommand ("Delete Project Data", [&] () -> GSErrCode {
        for (const GS::UniString& name : names) {
            API_Guid objectGuid;
            bool clientOnly;
            if (!FindProjectDataObject (name, objectGuid, clientOnly)) {
                results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "There is no project data with the name '" + name + "'."));
                continue;
            }

            const GSErrCode deleteErr = ACAPI_AddOnObject_DeleteObject (objectGuid);
            results.Push (deleteErr == NoError
                ? CreateSuccessfulExecutionResult ()
                : CreateFailedExecutionResult (deleteErr, "Failed to delete the project data '" + name + "'."));
        }
        return NoError;
    });

    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to delete the project data.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");
    for (const GS::ObjectState& result : results) {
        executionResults (result);
    }

    return response;
}
