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
                if (!client.acceptsNotificationsForEventType (kv.first)) {
                    continue;
                }

                const auto& events = message.AddList<GS::ObjectState> (ElementEventTypeToString (kv.first));
                for (const auto& os : kv.second) {
                    events (os);
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
                "description": "Notify on modification/deletion of an element. Optional parameter, by default true."
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

GS::Optional<GS::UniString> AddElementNotificationClientCommand::GetResponseSchema () const
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

    const GS::UniString connectionUrlStr = GS::UniString::Printf ("http://%T:%d/", host.ToPrintf (), port);
    const IO::URI::URI connectionUrl (connectionUrlStr);
    auto result = clients.emplace(std::piecewise_construct, std::forward_as_tuple(connectionUrlStr), std::forward_as_tuple(connectionUrl, notifyOnNew, notifyOnModification, notifyOnReservationChanges));
    if (!result.second) {
        result.first->second.notifyOnNew = notifyOnNew;
        result.first->second.notifyOnModification = notifyOnModification;
        result.first->second.notifyOnReservationChanges = notifyOnReservationChanges;
    }

    if (notifyOnNew && !hasClientToNotifyOnNew) {
        ACAPI_Element_CatchNewElement (nullptr, ElementEventHandlerProc);
        hasClientToNotifyOnNew = true;
    }

    if (notifyOnModification && !hasClientToNotifyOnModification) {
        GS::Array<API_Guid> elementIds;
        ACAPI_Element_GetElemList (API_ZombieElemID, &elementIds);
        for (const auto& elemId : elementIds) {
            ACAPI_Element_AttachObserver (elemId);
        }
        hasClientToNotifyOnModification = true;
    }

    if (notifyOnReservationChanges && !hasClientToNotifyOnReservationChanges) {
        ACAPI_Notification_CatchElementReservationChange (ElementReservationChangeHandler);
        hasClientToNotifyOnReservationChanges = true;
    }

    ACAPI_Element_InstallElementObserver (ElementEventHandlerProc);

    return CreateSuccessfulExecutionResult ();
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
        if (!client.acceptsNotificationsForEventType (eventType)) {
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
        API_Element	parentElement = {};
        ACAPI_Notification_GetParentElement (&parentElement, nullptr, 0, nullptr);

        const API_ElemTypeID typeID = GetElemTypeId (elemType->elemHead);
        GS::ObjectState message (
            "elementId", CreateGuidObjectState (elemType->elemHead.guid),
            "elementType", GetElementTypeNonLocalizedName (typeID));

        switch (elemType->notifID) {
            case APINotifyElement_New:
            case APINotifyElement_Copy:
            case APINotifyElement_Undo_Deleted:
            case APINotifyElement_Redo_Created:
                if (parentElement.header.guid != APINULLGuid) {
                    message.Add ("copiedElementId", CreateGuidObjectState (parentElement.header.guid));
                }
                SendEventToNotificationClient (ElementEventType::New, message);
                if (hasClientToNotifyOnModification) {
                    ACAPI_Element_AttachObserver (elemType->elemHead.guid);
                }
                break;

            case APINotifyElement_Change:
            case APINotifyElement_Edit:
            case APINotifyElement_Undo_Modified:
            case APINotifyElement_Redo_Modified:
            case APINotifyElement_PropertyValueChange:
            case APINotifyElement_ClassificationChange:
                if (parentElement.header.guid != APINULLGuid) {
                    message.Add ("oldElementId", CreateGuidObjectState (parentElement.header.guid));
                }
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

GS::Optional<GS::UniString> RemoveElementNotificationClientCommand::GetResponseSchema () const
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

    return CreateSuccessfulExecutionResult ();
}
