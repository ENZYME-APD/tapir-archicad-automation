#include "TeamworkCommands.hpp"
#include "MigrationHelper.hpp"

#include <map>

TeamworkSendCommand::TeamworkSendCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String TeamworkSendCommand::GetName () const
{
    return "TeamworkSend";
}

GS::Optional<GS::UniString> TeamworkSendCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState TeamworkSendCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GSErrCode err = ACAPI_Teamwork_SendChanges ();

    return err == NoError
        ? CreateSuccessfulExecutionResult ()
        : CreateFailedExecutionResult (err, "Failed to send changes.");
}

TeamworkReceiveCommand::TeamworkReceiveCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String TeamworkReceiveCommand::GetName () const
{
    return "TeamworkReceive";
}

GS::Optional<GS::UniString> TeamworkReceiveCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState TeamworkReceiveCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GSErrCode err = ACAPI_Teamwork_ReceiveChanges ();

    return err == NoError
        ? CreateSuccessfulExecutionResult ()
        : CreateFailedExecutionResult (err, "Failed to receive changes.");
}

ReserveElementsCommand::ReserveElementsCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String ReserveElementsCommand::GetName () const
{
    return "ReserveElements";
}

GS::Optional<GS::UniString> ReserveElementsCommand::GetInputParametersSchema () const
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

GS::Optional<GS::UniString> ReserveElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResult": {
                "$ref": "#/ExecutionResult"
            },
            "conflicts": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "user": {
                            "type": "object",
                            "properties": {
                                "userId": {
                                    "type": "number"
                                },
                                "userName": {
                                    "type": "string"
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "userId",
                                "userName"
                            ]
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "user"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResult"
        ]
    })";
}

GS::ObjectState ReserveElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    const GS::Array<API_Guid> elemIds = elements.Transform<API_Guid> (GetGuidFromElementsArrayItem);

	GS::HashTable<API_Guid, short> conflicts;
    constexpr bool enableDialogs = false;
    const GSErrCode err = ACAPI_Teamwork_ReserveElements (elemIds, &conflicts, enableDialogs);

    if (err != NoError) {
        return GS::ObjectState ("executionResult", CreateFailedExecutionResult (err, "Failed to reserve elements."));
    }

    GS::ObjectState response ("executionResult", CreateSuccessfulExecutionResult ());

    if (!conflicts.IsEmpty ()) {
        const auto& conflictsOutput = response.AddList<GS::ObjectState> ("conflicts");

        std::map<short, GS::UniString> userIdToNameMap;
        for (const auto& kv : conflicts) {
#ifdef ServerMainVers_2800
            const short userId = kv.value;
#else
            const short userId = *kv.value;
#endif
            ACAPI_Teamwork_GetUsernameFromId (userId, &userIdToNameMap[userId]);
        }

        for (const auto& kv : conflicts) {
#ifdef ServerMainVers_2800
            const API_Guid& elemGuid = kv.key;
            const short userId = kv.value;
#else
            const API_Guid& elemGuid = *kv.key;
            const short userId = *kv.value;
#endif
            conflictsOutput (GS::ObjectState ("elementId", CreateGuidObjectState (elemGuid),
                                            "user", GS::ObjectState ("userId", userId,
                                                                    "userName", userIdToNameMap[userId])));
        }
    }

    return response;
}

ReleaseElementsCommand::ReleaseElementsCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String ReleaseElementsCommand::GetName () const
{
    return "ReleaseElements";
}

GS::Optional<GS::UniString> ReleaseElementsCommand::GetInputParametersSchema () const
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

GS::Optional<GS::UniString> ReleaseElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState ReleaseElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    const GS::Array<API_Guid> elemIds = elements.Transform<API_Guid> (GetGuidFromElementsArrayItem);

    constexpr bool enableDialogs = false;
    const GSErrCode err = ACAPI_Teamwork_ReleaseElements (elemIds, enableDialogs);

    return err == NoError
        ? CreateSuccessfulExecutionResult ()
        : CreateFailedExecutionResult (err, "Failed to release elements.");
}

// Lockable object sets: the non-element things Teamwork can reserve (attributes, favorites,
// project info, project preferences, ...), addressed by the names the DevKit documents for
// ACAPI_Teamwork_FindLockableObjectSet.

static GS::UniString BuildLockableObjectSetInputParametersSchema ()
{
    return R"({
        "type": "object",
        "properties": {
            "objectSetName": {
                "type": "string",
                "description": "The name of the lockable object set, as the Archicad API documents it: BuildingMaterials, Cities, Composites, Favorites, FillTypes, LayerSettingsDialog (layers and layer combinations), LineTypes, MarkupStyles, MEPSystems, ModelViewOptions, OperationProfiles, PenTables, Profiles, ProjectInfo, GeoLocation (project location), PreferencesDialog (project preferences and dimension standards), GraphicOverrides (renovation override styles), Surfaces or ZoneCategories. Other names the Archicad API accepts work too."
            }
        },
        "additionalProperties": false,
        "required": [
            "objectSetName"
        ]
    })";
}

static GS::ObjectState CreateTeamworkUserObjectState (short userId)
{
    GS::UniString userName;
    ACAPI_Teamwork_GetUsernameFromId (userId, &userName);
    return GS::ObjectState ("userId", userId, "userName", userName);
}

static void AddConflictingUsers (GS::ObjectState& response, const GS::PagedArray<short>& conflicts)
{
    if (conflicts.IsEmpty ()) {
        return;
    }

    const auto& conflictsOutput = response.AddList<GS::ObjectState> ("conflicts");
    for (UIndex i = 0; i < conflicts.GetSize (); ++i) {
        conflictsOutput (CreateTeamworkUserObjectState (conflicts[i]));
    }
}

static GS::ObjectState CreateUnknownLockableObjectSetError (const GS::UniString& objectSetName)
{
    return CreateErrorResponse (APIERR_BADPARS, "No lockable object set named '" + objectSetName + "' was found. Check the name and that the project is a Teamwork project.");
}

static const char* LockableStatusToString (API_LockableStatus status)
{
    switch (status) {
        case APILockableStatus_Free:         return "Free";
        case APILockableStatus_Editable:     return "Editable";
        case APILockableStatus_Locked:       return "Locked";
        case APILockableStatus_NotAvailable: return "NotAvailable";
        case APILockableStatus_NotExist:
        default:                             return "NotExist";
    }
}

ReserveLockableObjectSetCommand::ReserveLockableObjectSetCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String ReserveLockableObjectSetCommand::GetName () const
{
    return "ReserveLockableObjectSet";
}

GS::Optional<GS::UniString> ReserveLockableObjectSetCommand::GetInputParametersSchema () const
{
    return BuildLockableObjectSetInputParametersSchema ();
}

GS::Optional<GS::UniString> ReserveLockableObjectSetCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResult": {
                "$ref": "#/ExecutionResult"
            },
            "conflicts": {
                "type": "array",
                "description": "The users whose reservation prevented reserving the object set.",
                "items": {
                    "type": "object",
                    "properties": {
                        "userId": {
                            "type": "number"
                        },
                        "userName": {
                            "type": "string"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "userId",
                        "userName"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResult"
        ]
    })";
}

GS::ObjectState ReserveLockableObjectSetCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString objectSetName;
    parameters.Get ("objectSetName", objectSetName);

    const API_Guid objectSetId = ACAPI_Teamwork_FindLockableObjectSet (objectSetName);
    if (objectSetId == APINULLGuid) {
        return GS::ObjectState ("executionResult", CreateFailedExecutionResult (CreateUnknownLockableObjectSetError (objectSetName)));
    }

    GS::PagedArray<short> conflicts;
    constexpr bool enableDialogs = false;
    const GSErrCode err = ACAPI_Teamwork_ReserveLockable (objectSetId, &conflicts, enableDialogs);

    GS::ObjectState response ("executionResult", err == NoError
        ? CreateSuccessfulExecutionResult ()
        : CreateFailedExecutionResult (err, "Failed to reserve the lockable object set '" + objectSetName + "'."));
    AddConflictingUsers (response, conflicts);

    return response;
}

ReleaseLockableObjectSetCommand::ReleaseLockableObjectSetCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String ReleaseLockableObjectSetCommand::GetName () const
{
    return "ReleaseLockableObjectSet";
}

GS::Optional<GS::UniString> ReleaseLockableObjectSetCommand::GetInputParametersSchema () const
{
    return BuildLockableObjectSetInputParametersSchema ();
}

GS::Optional<GS::UniString> ReleaseLockableObjectSetCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState ReleaseLockableObjectSetCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString objectSetName;
    parameters.Get ("objectSetName", objectSetName);

    const API_Guid objectSetId = ACAPI_Teamwork_FindLockableObjectSet (objectSetName);
    if (objectSetId == APINULLGuid) {
        return CreateFailedExecutionResult (CreateUnknownLockableObjectSetError (objectSetName));
    }

    constexpr bool enableDialogs = false;
    const GSErrCode err = ACAPI_Teamwork_ReleaseLockable (objectSetId, enableDialogs);

    return err == NoError
        ? CreateSuccessfulExecutionResult ()
        : CreateFailedExecutionResult (err, "Failed to release the lockable object set '" + objectSetName + "'.");
}

GetLockableObjectSetStatusCommand::GetLockableObjectSetStatusCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetLockableObjectSetStatusCommand::GetName () const
{
    return "GetLockableObjectSetStatus";
}

GS::Optional<GS::UniString> GetLockableObjectSetStatusCommand::GetInputParametersSchema () const
{
    return BuildLockableObjectSetInputParametersSchema ();
}

GS::Optional<GS::UniString> GetLockableObjectSetStatusCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "status": {
                "type": "string",
                "description": "Free: nobody has reserved it. Editable: the current user has reserved it. Locked: another user has reserved it (see conflicts). NotAvailable: the Teamwork server is offline or not available. NotExist: there is no Teamwork connection, for example the project is not a Teamwork project. An unknown object set name is reported as an error, not as a status.",
                "enum": ["Free", "Editable", "Locked", "NotAvailable", "NotExist"]
            },
            "conflicts": {
                "type": "array",
                "description": "The users who hold a reservation on the object set.",
                "items": {
                    "type": "object",
                    "properties": {
                        "userId": {
                            "type": "number"
                        },
                        "userName": {
                            "type": "string"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "userId",
                        "userName"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "status"
        ]
    })";
}

GS::ObjectState GetLockableObjectSetStatusCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString objectSetName;
    parameters.Get ("objectSetName", objectSetName);

    const API_Guid objectSetId = ACAPI_Teamwork_FindLockableObjectSet (objectSetName);
    if (objectSetId == APINULLGuid) {
        return CreateUnknownLockableObjectSetError (objectSetName);
    }

    GS::PagedArray<short> conflicts;
    const API_LockableStatus status = ACAPI_Teamwork_GetLockableStatus (objectSetId, &conflicts);

    GS::ObjectState response ("status", LockableStatusToString (status));
    AddConflictingUsers (response, conflicts);

    return response;
}

ReserveHotlinkCacheManagementCommand::ReserveHotlinkCacheManagementCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String ReserveHotlinkCacheManagementCommand::GetName () const
{
    return "ReserveHotlinkCacheManagement";
}

GS::Optional<GS::UniString> ReserveHotlinkCacheManagementCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResult": {
                "$ref": "#/ExecutionResult"
            },
            "conflicts": {
                "type": "array",
                "description": "The user whose reservation prevented reserving the Hotlink and XRef management.",
                "items": {
                    "type": "object",
                    "properties": {
                        "userId": {
                            "type": "number"
                        },
                        "userName": {
                            "type": "string"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "userId",
                        "userName"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResult"
        ]
    })";
}

GS::ObjectState ReserveHotlinkCacheManagementCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    short conflict = 0;
    const GSErrCode err = ACAPI_Teamwork_ReserveHotlinkCacheManagement (&conflict);

    GS::ObjectState response ("executionResult", err == NoError
        ? CreateSuccessfulExecutionResult ()
        : CreateFailedExecutionResult (err, "Failed to reserve the Hotlink and XRef management."));

    if (conflict != 0) {
        const auto& conflictsOutput = response.AddList<GS::ObjectState> ("conflicts");
        conflictsOutput (CreateTeamworkUserObjectState (conflict));
    }

    return response;
}

ReleaseHotlinkCacheManagementCommand::ReleaseHotlinkCacheManagementCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String ReleaseHotlinkCacheManagementCommand::GetName () const
{
    return "ReleaseHotlinkCacheManagement";
}

GS::Optional<GS::UniString> ReleaseHotlinkCacheManagementCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState ReleaseHotlinkCacheManagementCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    const GSErrCode err = ACAPI_Teamwork_ReleaseHotlinkCacheManagement ();

    return err == NoError
        ? CreateSuccessfulExecutionResult ()
        : CreateFailedExecutionResult (err, "Failed to release the Hotlink and XRef management.");
}