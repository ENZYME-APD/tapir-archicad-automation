#include "NotificationCommands.hpp"

#include "HTTP/Client/ClientConnection.hpp"
#include "HTTP/Client/Request.hpp"
#include "HTTP/Client/Response.hpp"
#include "HTTP/Client/Authentication/BasicAuthentication.hpp"
#include "JSON/JDOMParser.hpp"
#include "IBinaryChannelUtilities.hpp"
#include "IOBinProtocolXs.hpp"
#include "IChannelX.hpp"
#include "ObjectStateJSONConversion.hpp"

#include "MigrationHelper.hpp"

static
GS::String ElementEventTypeToString (AddElementNotificationClientCommand::ElementEventType eventType)
{
    switch (eventType) {
        case AddElementNotificationClientCommand::ElementEventType::New:
            return "newElements";
        case AddElementNotificationClientCommand::ElementEventType::Changed:
            return "changedElements";
        case AddElementNotificationClientCommand::ElementEventType::Deleted:
            return "deletedElements";
        case AddElementNotificationClientCommand::ElementEventType::Reserved:
            return "reservedElements";
        case AddElementNotificationClientCommand::ElementEventType::Released:
            return "releasedElements";
        default:
            return "unknown";
    }
}

class MessageSenderTask : public GS::Runnable {
    AddElementNotificationClientCommand::EventQueue eventQueue;
public:
    MessageSenderTask (AddElementNotificationClientCommand::EventQueue& q)
    {
        std::swap (eventQueue, q);
    }
    virtual void Run ()
    {
        for (auto& kv : AddElementNotificationClientCommand::clients) {
            auto& client = kv.second;

            bool isEmptyMessage = true;
            GS::ObjectState message;
            for (const auto& kv : eventQueue) {
                std::vector<const GS::ObjectState*> acceptedEvents;
                for (const auto& os : kv.second) {
                    if (client.acceptsNotificationsForEventType (kv.first, os)) {
                        acceptedEvents.push_back (&os);
                    }
                }
                if (acceptedEvents.empty ()) {
                    continue;
                }

                const auto& events = message.AddList<GS::ObjectState> (ElementEventTypeToString (kv.first));
                for (const GS::ObjectState* os : acceptedEvents) {
                    events (*os);
                    isEmptyMessage = false;
                }
            }

            if (isEmptyMessage) {
                continue;
            }

            AddElementNotificationClientCommand::SendMessageToNotificationClient (client, message);
        }
    }
};

std::map<GS::UniString, AddElementNotificationClientCommand::Client> AddElementNotificationClientCommand::clients;
bool AddElementNotificationClientCommand::hasClientToNotifyOnNew = false;
bool AddElementNotificationClientCommand::hasClientToNotifyOnModification = false;
std::set<API_ElemTypeID> AddElementNotificationClientCommand::observedElementTypes;
bool AddElementNotificationClientCommand::hasClientToNotifyOnReservationChanges = false;
std::unique_ptr<AddElementNotificationClientCommand::EventQueue> AddElementNotificationClientCommand::queuedEvents;
GS::Thread AddElementNotificationClientCommand::messageSenderThread;

void AddElementNotificationClientCommand::Client::Send(HTTP::Client::Request& request, const GS::UniString* body)
{
    try {
        HTTP::Client::ClientConnection clientConnection (url);
        clientConnection.SetKeepAlive (false);
        clientConnection.SetTimeout (100); // 0.1 seconds
        clientConnection.Connect ();
        clientConnection.Send (request, body->ToCStr ().Get (), body->GetLength ());
        clientConnection.Close (true);
    } catch (const GS::Exception& e) {
        e.Print (dbChannel);
    } catch (...) {
    }
}

AddElementNotificationClientCommand::AddElementNotificationClientCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String AddElementNotificationClientCommand::GetName () const
{
    return "SetElementNotificationClient";
}

GS::Optional<GS::UniString> AddElementNotificationClientCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "host": {
                "type": "string",
                "description": "The host address of the notification client. If not provided, localhost is used."
            },
            "port": {
                "type": "integer",
                "description": "The port number of the notification client."
            },
            "notifyOnNewElement": {
                "type": "boolean",
                "description": "Notify on creation of a new element. Optional parameter, by default true."
            },
            "notifyOnModificationOfAnElement": {
                "type": "boolean",
                "description": "Notify on modification/deletion of an element. Optional parameter, by default true. Archicad attaches an observer to each element to observe (see elementTypesToObserve) when the first client asks for this, and detaches them when the last one stops, and it is blocked meanwhile: the time grows faster than the number of elements, from seconds for a few hundred elements to many minutes for a few thousand. Count the elements first (GetAllElements with IncludeSubElemObjects, or GetElementsByType) and limit the types on large projects."
            },
            "elementTypesToObserve": {
                "type": "array",
                "description": "Only modifications/deletions of elements of these types are observed and notified to this client. Optional parameter, by default every element (including the parts of curtain walls, stairs and railings, which are most of the elements of large projects). Registering the same client again with other types changes them; only the elements of types no client observed before get an observer then. Does not limit the other notifications.",
                "items": {
                    "$ref": "#/ElementType"
                }
            },
            "notifyOnReservationChanges": {
                "type": "boolean",
                "description": "Notify on reservation changes of an element. Optional parameter, by default true."
            }
        },
        "additionalProperties": false,
        "required": [
            "port"
        ]
    })";
}

GS::Optional<GS::UniString> AddElementNotificationClientCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState AddElementNotificationClientCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    Int32 port = -1;
    if (parameters.Get ("port", port)) {
        if (port <= 0 || port > 65535) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid port number.");
        }
    } else {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Port number is required.");
    }

    GS::UniString host = "localhost";
    parameters.Get ("host", host);

    bool notifyOnNew = true;
    parameters.Get ("notifyOnNewElement", notifyOnNew);

    bool notifyOnModification = true;
    parameters.Get ("notifyOnModificationOfAnElement", notifyOnModification);

    bool notifyOnReservationChanges = true;
    parameters.Get ("notifyOnReservationChanges", notifyOnReservationChanges);

    std::set<API_ElemTypeID> elementTypesToObserve;
    GS::Array<GS::UniString> elementTypeNames;
    if (parameters.Get ("elementTypesToObserve", elementTypeNames)) {
        if (elementTypeNames.IsEmpty ()) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "elementTypesToObserve is empty; leave it out to observe every element.");
        }
        for (const GS::UniString& elementTypeName : elementTypeNames) {
            const API_ElemTypeID elementType = GetElementTypeFromNonLocalizedName (elementTypeName);
            if (elementType == API_ZombieElemID) {
                return CreateFailedExecutionResult (APIERR_BADPARS, GS::UniString ("Unknown element type: ") + elementTypeName);
            }
            elementTypesToObserve.insert (elementType);
        }
    }

    const GS::UniString connectionUrlStr = GS::UniString::Printf ("http://%T:%d/", host.ToPrintf (), port);
    const IO::URI::URI connectionUrl (connectionUrlStr);
    auto result = clients.emplace(std::piecewise_construct, std::forward_as_tuple(connectionUrlStr), std::forward_as_tuple(connectionUrl, notifyOnNew, notifyOnModification, notifyOnReservationChanges, elementTypesToObserve));
    if (!result.second) {
        result.first->second.notifyOnNew = notifyOnNew;
        result.first->second.notifyOnModification = notifyOnModification;
        result.first->second.notifyOnReservationChanges = notifyOnReservationChanges;
        result.first->second.elementTypesToObserve = elementTypesToObserve;
    }

    if (notifyOnNew && !hasClientToNotifyOnNew) {
        ACAPI_Element_CatchNewElement (nullptr, ElementEventHandlerProc);
        hasClientToNotifyOnNew = true;
    }

    // Attaches the observers this client needs and, when it was registered again
    // with fewer types or without modification notifications, detaches the ones no
    // client needs any more, as RemoveElementNotificationClient does.
    UpdateElementObservers ();

    if (notifyOnReservationChanges && !hasClientToNotifyOnReservationChanges) {
        ACAPI_Notification_CatchElementReservationChange (ElementReservationChangeHandler);
        hasClientToNotifyOnReservationChanges = true;
    }

    ACAPI_Element_InstallElementObserver (ElementEventHandlerProc);

    return CreateSuccessfulExecutionResult ();
}

// The elements of the given types; every element when the set is empty.
static GS::Array<API_Guid>
GetElementsOfTypes (const std::set<API_ElemTypeID>& elementTypes)
{
    GS::Array<API_Guid> elementIds;
    if (elementTypes.empty ()) {
        ACAPI_Element_GetElemList (API_ZombieElemID, &elementIds);
        return elementIds;
    }

    for (const API_ElemTypeID elementType : elementTypes) {
        GS::Array<API_Guid> elementIdsOfType;
        if (ACAPI_Element_GetElemList (elementType, &elementIdsOfType) != NoError) {
            continue;
        }
        elementIds.Append (elementIdsOfType);
    }
    return elementIds;
}

bool
AddElementNotificationClientCommand::IsObservedElementType (API_ElemTypeID typeID)
{
    return hasClientToNotifyOnModification && (observedElementTypes.empty () || observedElementTypes.count (typeID) > 0);
}

void
AddElementNotificationClientCommand::UpdateElementObservers ()
{
    // The types the clients need now: empty means every element.
    bool needed = false;
    bool neededForEveryType = false;
    std::set<API_ElemTypeID> neededTypes;
    for (const auto& kv : clients) {
        const Client& client = kv.second;
        if (!client.notifyOnModification) {
            continue;
        }
        needed = true;
        if (client.elementTypesToObserve.empty ()) {
            neededForEveryType = true;
        } else {
            neededTypes.insert (client.elementTypesToObserve.begin (), client.elementTypesToObserve.end ());
        }
    }
    if (neededForEveryType) {
        neededTypes.clear ();
    }

    if (needed == hasClientToNotifyOnModification && (!needed || neededTypes == observedElementTypes)) {
        return;
    }

    // ACAPI_Element_AttachObserver and ACAPI_Element_DetachObserver are slow on
    // many elements, and Archicad is blocked meanwhile (see issue #1840), so only
    // the elements whose observer changes are touched.
    GS::Array<API_Guid> attachedIds;
    if (hasClientToNotifyOnModification) {
        attachedIds = GetElementsOfTypes (observedElementTypes);
    }
    GS::Array<API_Guid> neededIds;
    if (needed) {
        neededIds = GetElementsOfTypes (neededTypes);
    }

    GS::HashSet<API_Guid> neededIdSet;
    for (const API_Guid& elemId : neededIds) {
        neededIdSet.Add (elemId);
    }
    GS::HashSet<API_Guid> attachedIdSet;
    for (const API_Guid& elemId : attachedIds) {
        attachedIdSet.Add (elemId);
        if (!neededIdSet.Contains (elemId)) {
            // Without this the observer stays attached for the rest of the session
            // after the last client was removed (see issue #762).
            ACAPI_Element_DetachObserver (elemId);
        }
    }
    for (const API_Guid& elemId : neededIds) {
        if (!attachedIdSet.Contains (elemId)) {
            ACAPI_Element_AttachObserver (elemId);
        }
    }

    hasClientToNotifyOnModification = needed;
    observedElementTypes = needed ? neededTypes : std::set<API_ElemTypeID> ();
}

void
AddElementNotificationClientCommand::SendEventToNotificationClient (ElementEventType eventType, const GS::ObjectState& os)
{
    if (queuedEvents) {
        (*queuedEvents)[eventType].push_back (os);
        return;
    }

    GS::ObjectState message;
    const auto& events = message.AddList<GS::ObjectState> (ElementEventTypeToString (eventType));
    events (os);

    for (auto& kv : clients) {
        auto& client = kv.second;
        if (!client.acceptsNotificationsForEventType (eventType, os)) {
            continue;
        }

        SendMessageToNotificationClient (client, message);
    }
}

void
AddElementNotificationClientCommand::SendQueuedEventsToNotificationClient ()
{
    if (!queuedEvents) {
        return;
    }

    messageSenderThread = GS::Thread (new MessageSenderTask (*queuedEvents), "ElementNotifications");
    messageSenderThread.Start ();
    queuedEvents.reset ();
}

void
AddElementNotificationClientCommand::SendMessageToNotificationClient (Client& client, const GS::ObjectState& os)
{
    HTTP::Client::Request postRequest (HTTP::MessageHeader::Method::Post, "/element_notification");

    GS::UniString postBody;
    JSON::CreateFromObjectState (os, postBody);
    if (postBody.GetLast () != '\n') {
        postBody.Append ("\n");
    }

    postRequest.GetRequestHeaderFieldCollection ().Add (HTTP::MessageHeader::HeaderFieldName::ContentType, "application/json");

    client.Send (postRequest, &postBody);
}

GSErrCode
AddElementNotificationClientCommand::ElementEventHandlerProc (const API_NotifyElementType *elemType)
{
    if (elemType->notifID == APINotifyElement_BeginEvents) {
        SendQueuedEventsToNotificationClient ();
        queuedEvents.reset (new EventQueue ());
    } else if (elemType->notifID == APINotifyElement_EndEvents) {
        SendQueuedEventsToNotificationClient ();
    } else {
        // Only the header of the notified element is used here. Building the parent
        // element with ACAPI_Notification_GetParentElement runs inside the command's
        // post-processing, and for a library part it also runs its 2D script; a script
        // that requests a property (REQUEST "Properties_Of_Parent" or
        // "Property_Value_Of_Parent") hits an ODB assertion at that point and takes
        // Archicad down (see issue #762).
        const API_ElemTypeID typeID = GetElemTypeId (elemType->elemHead);
        GS::ObjectState message (
            "elementId", CreateGuidObjectState (elemType->elemHead.guid),
            "elementType", GetElementTypeNonLocalizedName (typeID));

        switch (elemType->notifID) {
            case APINotifyElement_New:
            case APINotifyElement_Copy:
            case APINotifyElement_Undo_Deleted:
            case APINotifyElement_Redo_Created:
                SendEventToNotificationClient (ElementEventType::New, message);
                if (IsObservedElementType (typeID)) {
                    ACAPI_Element_AttachObserver (elemType->elemHead.guid);
                }
                break;

            case APINotifyElement_Change:
            case APINotifyElement_Edit:
            case APINotifyElement_Undo_Modified:
            case APINotifyElement_Redo_Modified:
            case APINotifyElement_PropertyValueChange:
            case APINotifyElement_ClassificationChange:
                SendEventToNotificationClient (ElementEventType::Changed, message);
                break;

            case APINotifyElement_Delete:
            case APINotifyElement_Undo_Created:
            case APINotifyElement_Redo_Deleted:
                SendEventToNotificationClient (ElementEventType::Deleted, message);
                break;

            default:
                SendEventToNotificationClient (ElementEventType::Unknown, message);
                break;
        }
    }

    return NoError;
}

static API_ElemTypeID
GetElemTypeID (const API_Guid& elemGuid)
{
    API_Elem_Head elemHead = {};
    elemHead.guid = elemGuid;
    ACAPI_Element_GetHeader (&elemHead);
    return GetElemTypeId (elemHead);
}

GSErrCode
AddElementNotificationClientCommand::ElementReservationChangeHandler (
    const GS::HashTable<API_Guid, short>& reserved,
    const GS::HashSet<API_Guid>&          released,
    const GS::HashSet<API_Guid>&          /*deleted*/)
{
    std::map<short, GS::UniString> userIdToNameMap;
    for (const auto& kv : reserved) {
#ifdef ServerMainVers_2800
        const auto& userId = kv.value;
#else
        const auto& userId = *kv.value;
#endif
        ACAPI_Teamwork_GetUsernameFromId (userId, &userIdToNameMap[userId]);
    }
    for (const auto& kv : reserved) {
#ifdef ServerMainVers_2800
        const auto& guid = kv.key;
        const auto& userId = kv.value;
#else
        const auto& guid = *kv.key;
        const auto& userId = *kv.value;
#endif
        GS::ObjectState message (
            "elementId", CreateGuidObjectState (guid),
            "elementType", GetElementTypeNonLocalizedName (GetElemTypeID (guid)),
            "reservedBy", userIdToNameMap[userId]);

        SendEventToNotificationClient (ElementEventType::Reserved, message);
    }
    for (const auto& guid : released) {
        GS::ObjectState message (
            "elementId", CreateGuidObjectState (guid),
            "elementType", GetElementTypeNonLocalizedName (GetElemTypeID (guid)));

        SendEventToNotificationClient (ElementEventType::Released, message);
    }

    return NoError;
}

RemoveElementNotificationClientCommand::RemoveElementNotificationClientCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String RemoveElementNotificationClientCommand::GetName () const
{
    return "RemoveElementNotificationClient";
}

GS::Optional<GS::UniString> RemoveElementNotificationClientCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "host": {
                "type": "string",
                "description": "The host address of the notification client. If not provided, localhost is used."
            },
            "port": {
                "type": "integer",
                "description": "The port number of the notification client."
            }
        },
        "additionalProperties": false,
        "required": [
            "port"
        ]
    })";
}

GS::Optional<GS::UniString> RemoveElementNotificationClientCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState RemoveElementNotificationClientCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    Int32 port = -1;
    if (parameters.Get ("port", port)) {
        if (port <= 0 || port > 65535) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid port number.");
        }
    } else {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Port number is required.");
    }

    GS::UniString host = "localhost";
    parameters.Get ("host", host);

    const GS::UniString connectionUrlStr = GS::UniString::Printf ("http://%T:%d/", host.ToPrintf (), port);

    if (AddElementNotificationClientCommand::clients.erase (connectionUrlStr) == 0) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "No such notification client registered.");
    }

    AddElementNotificationClientCommand::UpdateElementObservers ();

    return CreateSuccessfulExecutionResult ();
}
