#include "TeamworkCommands.hpp"
#include "MigrationHelper.hpp"

#include <limits>
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
    CommandBase (CommonSchema::Used)
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
                            "$ref": "#/User"
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

static GS::UniString GetTeamworkUserName (short userId)
{
    GS::UniString userName;
    ACAPI_Teamwork_GetUsernameFromId (userId, &userName);
    return userName;
}

// The element reservation is in the element header: userId is the member who owns the element
// and lockId is the member who has it reserved (0 when nobody has). ACAPI_Teamwork_GetLockableStatus
// is for lockable object sets only, not for elements.

GetReservationStatusOfElementsCommand::GetReservationStatusOfElementsCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetReservationStatusOfElementsCommand::GetName () const
{
    return "GetReservationStatusOfElements";
}

GS::Optional<GS::UniString> GetReservationStatusOfElementsCommand::GetInputParametersSchema () const
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

GS::Optional<GS::UniString> GetReservationStatusOfElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "reservationStatuses": {
                "type": "array",
                "description": "One item per input element, in the same order.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "status": {
                                    "type": "string",
                                    "description": "Free: nobody has reserved the element. Editable: the current user has reserved it. Locked: another user has reserved it (see reservedBy).",
                                    "enum": ["Free", "Editable", "Locked"]
                                },
                                "reservedBy": {
                                    "type": "object",
                                    "description": "The user who has reserved the element. Left out when the element is free.",
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
                                },
                                "owner": {
                                    "type": "object",
                                    "description": "The user who owns the element in Teamwork, usually the one who created it. Left out when the element has no owner.",
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
                                "status"
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
            "reservationStatuses"
        ]
    })";
}

GS::ObjectState GetReservationStatusOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    API_ProjectInfo projectInfo = {};
    const GSErrCode projectErr = ACAPI_ProjectOperation_Project (&projectInfo);
    if (projectErr != NoError) {
        return CreateErrorResponse (projectErr, "Failed to retrieve project information. Check the opened project!");
    }
    if (!projectInfo.teamwork) {
        return CreateErrorResponse (APIERR_NOTEAMWORKPROJECT, "The opened project is not a Teamwork project, so its elements have no reservation status.");
    }
    const short currentUserId = projectInfo.userId;

    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& reservationStatuses = response.AddList<GS::ObjectState> ("reservationStatuses");

    std::map<short, GS::UniString> userIdToNameMap;
    const auto createUserObjectState = [&] (short userId) -> GS::ObjectState {
        if (userIdToNameMap.find (userId) == userIdToNameMap.end ()) {
            userIdToNameMap[userId] = GetTeamworkUserName (userId);
        }
        return GS::ObjectState ("userId", userId, "userName", userIdToNameMap[userId]);
    };

    for (const GS::ObjectState& element : elements) {
        API_Elem_Head elemHead = {};
        elemHead.guid = GetGuidFromElementsArrayItem (element);
        const GSErrCode err = ACAPI_Element_GetHeader (&elemHead);
        if (err != NoError) {
            reservationStatuses (CreateErrorResponse (err, "Failed to find the element."));
            continue;
        }

        const short lockerId = elemHead.lockId;
        const short ownerId = elemHead.userId;

        GS::ObjectState reservationStatus ("status",
            lockerId == 0 ? "Free" : (lockerId == currentUserId ? "Editable" : "Locked"));
        if (lockerId != 0) {
            reservationStatus.Add ("reservedBy", createUserObjectState (lockerId));
        }
        if (ownerId != 0) {
            reservationStatus.Add ("owner", createUserObjectState (ownerId));
        }
        reservationStatuses (reservationStatus);
    }

    return response;
}

GrantElementsCommand::GrantElementsCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GrantElementsCommand::GetName () const
{
    return "GrantElements";
}

GS::Optional<GS::UniString> GrantElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "userId": {
                "type": "number",
                "description": "The Teamwork user id of the user to hand the elements over to, as GetTeamworkProjectInfo, GetReservationStatusOfElements and the conflicts of ReserveElements report users."
            }
        },
        "additionalProperties": false,
        "required": [
            "elements",
            "userId"
        ]
    })";
}

GS::Optional<GS::UniString> GrantElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState GrantElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
// ACAPI_Teamwork_GrantElements is new in the Archicad 27 API; the AC25 and AC26 DevKits have no
// ACAPI_TeamworkControl_GrantElements to map it to.
#ifdef ServerMainVers_2700
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    Int32 userId = 0;
    parameters.Get ("userId", userId);
    if (userId <= 0 || userId > std::numeric_limits<short>::max ()) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "The userId must be a positive Teamwork user id.");
    }

    const GS::Array<API_Guid> elemIds = elements.Transform<API_Guid> (GetGuidFromElementsArrayItem);

    const GSErrCode err = ACAPI_Teamwork_GrantElements (elemIds, static_cast<short> (userId));

    return err == NoError
        ? CreateSuccessfulExecutionResult ()
        : CreateFailedExecutionResult (err, GS::UniString::Printf ("Failed to grant the elements to user %d.", (int) userId));
#else
    UNUSED_PARAMETER (parameters);
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "GrantElements requires Archicad 27 or newer.");
#endif
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
    return GS::ObjectState ("userId", userId, "userName", GetTeamworkUserName (userId));
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
    CommandBase (CommonSchema::Used)
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
                    "$ref": "#/User"
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
    CommandBase (CommonSchema::Used)
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
                    "$ref": "#/User"
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
    CommandBase (CommonSchema::Used)
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
                    "$ref": "#/User"
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
