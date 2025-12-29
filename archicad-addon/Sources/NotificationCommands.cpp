#include "NotificationCommands.hpp"

#include	"HTTP/Client/ClientConnection.hpp"
#include	"HTTP/Client/Request.hpp"
#include	"HTTP/Client/Response.hpp"
#include	"HTTP/Client/Authentication/BasicAuthentication.hpp"
#include	"JSON/JDOMParser.hpp"
#include	"IBinaryChannelUtilities.hpp"
#include	"IOBinProtocolXs.hpp"
#include	"IChannelX.hpp"
#include    "ObjectStateJSONConversion.hpp"

#include "MigrationHelper.hpp"

#include <map>

std::vector<std::pair<GS::UniString, Int32>> SetElementNotificationClientCommand::clientConnections;

SetElementNotificationClientCommand::SetElementNotificationClientCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String SetElementNotificationClientCommand::GetName () const
{
    return "SetElementNotificationClient";
}

GS::Optional<GS::UniString> SetElementNotificationClientCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "port": {
                "type": "integer",
                "description": "The port number of the notification client."
            },
            "host": {
                "type": "string",
                "description": "The host address of the notification client. If not provided, localhost is used."
            }
        },
        "additionalProperties": false,
        "required": [
            "port"
        ]
    })";
}

GS::ObjectState SetElementNotificationClientCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
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

    clientConnections.emplace_back (host, port);

    if (clientConnections.size () == 1) {
        ACAPI_Element_CatchNewElement (nullptr, ElementEventHandlerProc);
        ACAPI_Element_AttachObserver (APINULLGuid);
        ACAPI_Element_InstallElementObserver (ElementEventHandlerProc);
    }

    return CreateSuccessfulExecutionResult ();
}

void
SetElementNotificationClientCommand::SendMessageToNotificationClient (const GS::ObjectState& os)
{
    for (const auto& hostAndPort : clientConnections) {
        IO::URI::URI connectionUrl (GS::UniString::Printf ("http://%T:%d/", hostAndPort.first.ToPrintf (), hostAndPort.second));
        HTTP::Client::ClientConnection clientConnection (connectionUrl);
        clientConnection.Connect ();

        HTTP::Client::Request postRequest (HTTP::MessageHeader::Method::Post, "");

        GS::UniString postBody;
        JSON::CreateFromObjectState (os, postBody);

        postRequest.GetRequestHeaderFieldCollection ().Add (HTTP::MessageHeader::HeaderFieldName::ContentType, "application/json");
        postRequest.GetRequestHeaderFieldCollection ().Add (HTTP::MessageHeader::HeaderFieldName::ContentLength, GS::UniString::Printf ("%zu", postBody.GetLength ()));

        clientConnection.Send (postRequest, postBody.ToCStr ().Get (), postBody.GetLength ());
        clientConnection.FinishReceive ();
        clientConnection.Close (false);
    }
}

GSErrCode
SetElementNotificationClientCommand::ElementEventHandlerProc (const API_NotifyElementType *elemType)
{
    if (elemType->notifID == APINotifyElement_BeginEvents || elemType->notifID == APINotifyElement_EndEvents) {
        return NoError;
    } else {
        API_Element	parentElement = {};
        ACAPI_Notification_GetParentElement (&parentElement, nullptr, 0, nullptr);

        switch (elemType->notifID) {
            case APINotifyElement_New:
                        if (parentElement.header.guid != APINULLGuid)
                            SendMessageToNotificationClient (GS::ObjectState(
                                "event", "ElementCopied",
                                "newElementId", CreateGuidObjectState (elemType->elemHead.guid),
                                "copiedElementId", CreateGuidObjectState (parentElement.header.guid)
                            ));
                        else
                            SendMessageToNotificationClient (GS::ObjectState(
                                "event", "ElementCreated",
                                "newElementId", CreateGuidObjectState (elemType->elemHead.guid)
                            ));

                        ACAPI_Element_AttachObserver (elemType->elemHead.guid);
                        break;

            case APINotifyElement_Copy:
                        if (parentElement.header.guid != APINULLGuid)
                            SendMessageToNotificationClient (GS::ObjectState(
                                "event", "ElementCopied",
                                "newElementId", CreateGuidObjectState (elemType->elemHead.guid),
                                "copiedElementId", CreateGuidObjectState (parentElement.header.guid)
                            ));

                        ACAPI_Element_AttachObserver (elemType->elemHead.guid);
                        break;

            case APINotifyElement_Change:
            case APINotifyElement_Edit:
                        if (parentElement.header.guid != APINULLGuid)
                            SendMessageToNotificationClient (GS::ObjectState(
                                "event", "ElementChanged",
                                "newElementId", CreateGuidObjectState (elemType->elemHead.guid),
                                "oldElementId", CreateGuidObjectState (parentElement.header.guid)
                            ));
                        else
                            SendMessageToNotificationClient (GS::ObjectState(
                                "event", "ElementChanged",
                                "elementId", CreateGuidObjectState (elemType->elemHead.guid)
                            ));
                        break;

            case APINotifyElement_Delete:
                            SendMessageToNotificationClient (GS::ObjectState(
                                "event", "ElementDeleted",
                                "elementId", CreateGuidObjectState (elemType->elemHead.guid)
                            ));
                        break;

            case APINotifyElement_Undo_Created:
                            SendMessageToNotificationClient (GS::ObjectState(
                                "event", "ElementUndoCreated",
                                "elementId", CreateGuidObjectState (elemType->elemHead.guid)
                            ));
                        break;

            case APINotifyElement_Undo_Modified:
                            SendMessageToNotificationClient (GS::ObjectState(
                                "event", "ElementUndoModified",
                                "elementId", CreateGuidObjectState (elemType->elemHead.guid)
                            ));
                        break;

            case APINotifyElement_Undo_Deleted:
                            SendMessageToNotificationClient (GS::ObjectState(
                                "event", "ElementUndoDeleted",
                                "elementId", CreateGuidObjectState (elemType->elemHead.guid)
                            ));
                        break;

            case APINotifyElement_Redo_Created:
                            SendMessageToNotificationClient (GS::ObjectState(
                                "event", "ElementRedoCreated",
                                "elementId", CreateGuidObjectState (elemType->elemHead.guid)
                            ));
                        break;

            case APINotifyElement_Redo_Modified:
                            SendMessageToNotificationClient (GS::ObjectState(
                                "event", "ElementRedoModified",
                                "elementId", CreateGuidObjectState (elemType->elemHead.guid)
                            ));
                        break;

            case APINotifyElement_Redo_Deleted:
                            SendMessageToNotificationClient (GS::ObjectState(
                                "event", "ElementRedoDeleted",
                                "elementId", CreateGuidObjectState (elemType->elemHead.guid)
                            ));
                        break;

            case APINotifyElement_PropertyValueChange:
                            SendMessageToNotificationClient (GS::ObjectState(
                                "event", "ElementPropertyValueChange",
                                "elementId", CreateGuidObjectState (elemType->elemHead.guid)
                            ));
                        break;

            case APINotifyElement_ClassificationChange:
                            SendMessageToNotificationClient (GS::ObjectState(
                                "event", "ElementClassificationChange",
                                "elementId", CreateGuidObjectState (elemType->elemHead.guid)
                            ));
                        break;
            default:
                        break;
        }
    }

    return NoError;
}