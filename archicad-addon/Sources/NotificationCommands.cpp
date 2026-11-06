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

#include "ApplicationCommands.hpp"
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
bool AddElementNotificationClientCommand::hasClientToNotifyOnSelectionChange = false;
bool AddElementNotificationClientCommand::hasClientToNotifyOnAttributeReplacement = false;
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
                "description": "Notify on modification/deletion of an element. The observed elements are the ones ACAPI_Element_GetElemList returns, for API_ZombieElemID or, with elementTypesToObserve, for each of those types, when the first client asking for these notifications of them is registered, and every element of the observed types created after that. Archicad does not report the changes made by Tapir's own commands, so Tapir reports them itself for SetDetailsOfElements, SetPropertyValuesOfElements, MoveElements, RotateElements, MirrorElements (not for the copies they make, which are new elements), DeleteElements, DeleteSelectedElements, the Modify<Tool> commands (ModifyWalls, ModifyBeams, ModifySlabs, ModifyColumns, ModifyWindows, ModifyDoors, ModifyOpenings, ModifyMorphs, ModifyRoofs, ModifyShells, ModifyMeshes, ModifyObjects, ModifyLamps, ModifyTexts, ModifyLabels, ModifyCurtainWalls, ModifyCurtainWallFrames, ModifyZones, ModifySections, ModifyDetailMarkers, ModifyRailings, ModifyStairs), ChangeTextContent, SetGDLParametersOfElements, RunParameterScriptOfElements (only when the script changed the element), SetMarkerParametersOfElements, SetMarkerLibraryPartOfElements, ChangeLibraryPartOfElements, SetClassificationsOfElements, SetCategoryValuesOfElements, ModifyPropertyValueOfElements, SetElementInfoStrings, SetURLReferencesOfElements, SetIFCAttributesOfElements, SetIFCPropertiesOfElements, RemoveIFCPropertiesOfElements, SetIFCClassificationReferencesOfElements, RemoveIFCClassificationReferencesOfElements, LockElements, UnlockElements, SetOpeningDetails, SetOpeningFloorPlanParameters and SetOpeningSymbolParameters. Of the elements these commands change, only the given ones are reported: an element deleted together with its owner (a door or window of a deleted wall) is not. Optional parameter, by default true. Archicad attaches an observer to each element to observe (see elementTypesToObserve) when the first client asks for this, and detaches them when the last one stops, and it is blocked meanwhile: the time grows faster than the number of elements, from seconds for a few hundred elements to many minutes for a few thousand. Count the elements first (GetAllElements with IncludeSubElemObjects, or GetElementsByType) and limit the types on large projects."
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
            },
            "notifyOnSelectionChange": {
                "type": "boolean",
                "description": "Notify when the selection changes. The message is {\"selectedElements\": [...]}: the whole current selection, each item an elementId as in the response of GetSelectedElements, an empty list when nothing is selected. Optional parameter, by default false."
            },
            "notifyOnWindowChange": {
                "type": "boolean",
                "description": "Notify when the current (active) window changes or the user switches to another story. The message is {\"windowChanged\": {\"event\": \"Window\" or \"Floor\", \"windowType\": ..., \"databaseId\": ..., \"storyIndex\": ...}}: windowType and databaseId describe the new current window as in the response of GetCurrentWindowType (databaseId is missing when that response has none), storyIndex is the active story, missing when it cannot be read. Optional parameter, by default false."
            },
            "notifyOnProjectEvents": {
                "type": "boolean",
                "description": "Notify on project events. The message is {\"projectEvent\": ...}, one of \"New\", \"NewAndReset\", \"Open\", \"Close\", \"Quit\", \"SendChanges\" (a Teamwork send is about to start) and \"ReceiveChanges\" (Teamwork changes were received). The client stays registered when the project is closed or another one is opened. Optional parameter, by default false."
            },
            "notifyOnAttributeReplacement": {
                "type": "boolean",
                "description": "Notify when attributes are deleted and replaced by other attributes of the same type (for example Delete and Replace in the Attribute Manager). The message is {\"replacedAttributes\": [{\"attributeType\": ..., \"oldIndex\": ..., \"newIndex\": ..., \"newAttributeId\": ...}, ...]}: attributeType is one of the type names of GetAttributesByType (\"Layer\", \"Line\", \"Fill\", \"Composite\", \"Surface\", \"LayerCombination\", \"ZoneCategory\", \"Profile\", \"PenTable\", \"MEPSystem\", \"OperationProfile\", \"BuildingMaterial\", or \"Unknown\"), oldIndex is the index of the replaced attribute and newIndex the index of the attribute that replaces it, newAttributeId is the attributeId of that attribute, missing when it cannot be found. Not available in Archicad 25: setting it to true there fails. Optional parameter, by default false."
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

    bool notifyOnSelectionChange = false;
    parameters.Get ("notifyOnSelectionChange", notifyOnSelectionChange);

    // Both are served by the add-on's project event handler, which is installed for the whole
    // session (ProjectEventHandler.cpp), so nothing is installed or removed for them here.
    bool notifyOnWindowChange = false;
    parameters.Get ("notifyOnWindowChange", notifyOnWindowChange);

    bool notifyOnProjectEvents = false;
    parameters.Get ("notifyOnProjectEvents", notifyOnProjectEvents);

    bool notifyOnAttributeReplacement = false;
    parameters.Get ("notifyOnAttributeReplacement", notifyOnAttributeReplacement);
#ifndef ServerMainVers_2600
    // Refused before the client is registered or changed, so a failed call changes nothing.
    if (notifyOnAttributeReplacement) {
        return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "notifyOnAttributeReplacement is not available in Archicad 25.");
    }
#endif

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
    auto result = clients.emplace(std::piecewise_construct, std::forward_as_tuple(connectionUrlStr), std::forward_as_tuple(connectionUrl, notifyOnNew, notifyOnModification, notifyOnReservationChanges, notifyOnSelectionChange, notifyOnWindowChange, notifyOnProjectEvents, notifyOnAttributeReplacement, elementTypesToObserve));
    if (!result.second) {
        result.first->second.notifyOnNew = notifyOnNew;
        result.first->second.notifyOnModification = notifyOnModification;
        result.first->second.notifyOnReservationChanges = notifyOnReservationChanges;
        result.first->second.notifyOnSelectionChange = notifyOnSelectionChange;
        result.first->second.notifyOnWindowChange = notifyOnWindowChange;
        result.first->second.notifyOnProjectEvents = notifyOnProjectEvents;
        result.first->second.notifyOnAttributeReplacement = notifyOnAttributeReplacement;
        result.first->second.elementTypesToObserve = elementTypesToObserve;
        // A client registered again without these notifications may have been the
        // last one that wanted them; uninstall the handlers in that case, as
        // RemoveElementNotificationClient does. The element observers are updated
        // below.
        UninstallSelectionChangeHandlerIfNotNeeded ();
        UninstallAttributeReplacementHandlerIfNotNeeded ();
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

    if (notifyOnSelectionChange && !hasClientToNotifyOnSelectionChange) {
        ACAPI_Notification_CatchSelectionChange (SelectionChangeHandler);
        hasClientToNotifyOnSelectionChange = true;
    }

#ifdef ServerMainVers_2600
    if (notifyOnAttributeReplacement && !hasClientToNotifyOnAttributeReplacement) {
        ACAPI_Notification_CatchAttributeReplacement (AttributeReplacementHandler);
        hasClientToNotifyOnAttributeReplacement = true;
    }
#endif

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
AddElementNotificationClientCommand::UninstallSelectionChangeHandlerIfNotNeeded ()
{
    if (!hasClientToNotifyOnSelectionChange) {
        return;
    }

    for (const auto& kv : clients) {
        if (kv.second.notifyOnSelectionChange) {
            return;
        }
    }

    ACAPI_Notification_CatchSelectionChange (nullptr);
    hasClientToNotifyOnSelectionChange = false;
}

GSErrCode
AddElementNotificationClientCommand::SelectionChangeHandler (const API_Neig* /*selElemNeig*/)
{
    // The handler only gets the last selected element, so the whole selection is read here,
    // as GetSelectedElements does (and the DevKit's Browser_Control example in its handler).
    API_SelectionInfo selectionInfo = {};
    GS::Array<API_Neig> selectedNeigs;
    const GSErrCode err = ACAPI_Selection_Get (&selectionInfo, &selectedNeigs, false);
    // The marquee polygon handle is owned by the caller (same as the Selection_Manager DevKit example).
    BMKillHandle (reinterpret_cast<GSHandle*> (&selectionInfo.marquee.coords));
    if (err != NoError && err != APIERR_NOSEL) {
        return NoError;
    }

    GS::ObjectState message;
    const auto& elementsList = message.AddList<GS::ObjectState> ("selectedElements");
    if (err != APIERR_NOSEL && selectionInfo.typeID != API_SelEmpty) {
        for (const API_Neig& selectedNeig : selectedNeigs) {
            elementsList (CreateElementIdObjectState (GetParentElemOfSectElem (selectedNeig.guid)));
        }
    }

    // Sent at once, not queued with the element events: a selection change is not an element
    // event, and only the current selection matters to the client.
    for (auto& kv : clients) {
        auto& client = kv.second;
        if (!client.notifyOnSelectionChange) {
            continue;
        }

        SendMessageToNotificationClient (client, message);
    }

    return NoError;
}

void
AddElementNotificationClientCommand::UninstallAttributeReplacementHandlerIfNotNeeded ()
{
    if (!hasClientToNotifyOnAttributeReplacement) {
        return;
    }

    for (const auto& kv : clients) {
        if (kv.second.notifyOnAttributeReplacement) {
            return;
        }
    }

#ifdef ServerMainVers_2600
    ACAPI_Notification_CatchAttributeReplacement (nullptr);
#endif
    hasClientToNotifyOnAttributeReplacement = false;
}

#ifdef ServerMainVers_2600

// The attribute type names of the official GetAttributesByType command (AttributeType).
static GS::UniString AttributeTypeToString (API_AttrTypeID typeID)
{
    switch (typeID) {
        case API_LayerID:               return "Layer";
        case API_LinetypeID:            return "Line";
        case API_FilltypeID:            return "Fill";
        case API_CompWallID:            return "Composite";
        case API_MaterialID:            return "Surface";
        case API_LayerCombID:           return "LayerCombination";
        case API_ZoneCatID:             return "ZoneCategory";
        case API_ProfileID:             return "Profile";
        case API_PenTableID:            return "PenTable";
        case API_MEPSystemID:           return "MEPSystem";
        case API_OperationProfileID:    return "OperationProfile";
        case API_BuildingMaterialID:    return "BuildingMaterial";
        default:                        return "Unknown";
    }
}

GSErrCode
AddElementNotificationClientCommand::AttributeReplacementHandler (const API_AttributeReplaceIndexTable& table)
{
    GS::ObjectState message;
    const auto& replacedAttributes = message.AddList<GS::ObjectState> ("replacedAttributes");
    bool isEmptyMessage = true;
    for (const auto& typeEntry : table) {
#ifdef ServerMainVers_2800
        const API_AttrTypeID typeID = typeEntry.key;
        const auto& indexTable = typeEntry.value;
#else
        const API_AttrTypeID typeID = *typeEntry.key;
        const auto& indexTable = *typeEntry.value;
#endif
        const GS::UniString typeName = AttributeTypeToString (typeID);
        for (const auto& indexEntry : indexTable) {
#ifdef ServerMainVers_2800
            const API_AttributeIndex& oldIndex = indexEntry.key;
            const API_AttributeIndex& newIndex = indexEntry.value;
#else
            const API_AttributeIndex& oldIndex = *indexEntry.key;
            const API_AttributeIndex& newIndex = *indexEntry.value;
#endif
            GS::ObjectState replacement (
                "attributeType", typeName,
                "oldIndex", GetAttributeIndex (oldIndex),
                "newIndex", GetAttributeIndex (newIndex));
            // The replaced attribute is deleted, so only the new one has an id to report.
            const API_Guid newAttributeGuid = GetAttributeGuidFromIndex (typeID, newIndex);
            if (newAttributeGuid != APINULLGuid) {
                replacement.Add ("newAttributeId", CreateAttributeIdObjectState (newAttributeGuid));
            }
            replacedAttributes (replacement);
            isEmptyMessage = false;
        }
    }

    if (isEmptyMessage) {
        return NoError;
    }

    // Sent at once, like the selection: these are not element events.
    for (auto& kv : clients) {
        auto& client = kv.second;
        if (!client.notifyOnAttributeReplacement) {
            continue;
        }

        SendMessageToNotificationClient (client, message);
    }

    return NoError;
}

#endif

static GS::Optional<GS::UniString> ProjectEventToString (API_NotifyEventID notifID)
{
    switch (notifID) {
        case APINotify_New:             return GS::UniString ("New");
        case APINotify_NewAndReset:     return GS::UniString ("NewAndReset");
        case APINotify_Open:            return GS::UniString ("Open");
        case APINotify_Close:           return GS::UniString ("Close");
        case APINotify_Quit:            return GS::UniString ("Quit");
        case APINotify_SendChanges:     return GS::UniString ("SendChanges");
        case APINotify_ReceiveChanges:  return GS::UniString ("ReceiveChanges");
        default:                        return GS::NoValue;
    }
}

static GS::ObjectState CreateWindowChangedMessage (API_NotifyEventID notifID)
{
    GS::ObjectState windowChanged ("event", GS::UniString (notifID == APINotify_ChangeFloor ? "Floor" : "Window"));

    // The same window type and databaseId as GetCurrentWindowType returns: custom add-on windows
    // and windows without a database of their own get no databaseId.
    API_WindowInfo windowInfo = {};
    if (ACAPI_Window_GetCurrentWindow (&windowInfo) == NoError) {
        windowChanged.Add ("windowType", ConvertWindowTypeToString (windowInfo.typeID));
        if (windowInfo.typeID != APIWind_MyDrawID && windowInfo.typeID != APIWind_MyTextID) {
            const API_Guid databaseId = DatabaseIdResolver::Instance ().GetIdOfDatabase (windowInfo);
            if (databaseId != APINULLGuid) {
                windowChanged.Add ("databaseId", CreateGuidObjectState (databaseId));
            }
        }
    } else {
        windowChanged.Add ("windowType", ConvertWindowTypeToString (API_ZombieWindowID));
    }

    API_StoryInfo storyInfo = {};
    const GSErrCode err = ACAPI_ProjectSetting_GetStorySettings (&storyInfo);
    BMKillHandle (reinterpret_cast<GSHandle*> (&storyInfo.data));
    if (err == NoError) {
        windowChanged.Add ("storyIndex", storyInfo.actStory);
    }

    return GS::ObjectState ("windowChanged", windowChanged);
}

void
AddElementNotificationClientCommand::ProjectEventHandler (API_NotifyEventID notifID, Int32 /*param*/)
{
    const bool isWindowEvent = (notifID == APINotify_ChangeWindow || notifID == APINotify_ChangeFloor);
    const GS::Optional<GS::UniString> projectEvent = ProjectEventToString (notifID);
    if (!isWindowEvent && !projectEvent.HasValue ()) {
        return;
    }

    const auto wantsEvent = [isWindowEvent] (const Client& client) {
        return isWindowEvent ? client.notifyOnWindowChange : client.notifyOnProjectEvents;
    };

    // Most of the time no client wants these events: read nothing from Archicad then.
    bool anyClientWantsEvent = false;
    for (const auto& kv : clients) {
        if (wantsEvent (kv.second)) {
            anyClientWantsEvent = true;
            break;
        }
    }
    if (!anyClientWantsEvent) {
        return;
    }

    const GS::ObjectState message = isWindowEvent
        ? CreateWindowChangedMessage (notifID)
        : GS::ObjectState ("projectEvent", projectEvent.Get ());

    // Sent at once, like the selection: these are not element events.
    for (auto& kv : clients) {
        auto& client = kv.second;
        if (!wantsEvent (client)) {
            continue;
        }

        SendMessageToNotificationClient (client, message);
    }
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
        const GS::ObjectState message = CreateElementEvent (elemType->elemHead);

        switch (elemType->notifID) {
            case APINotifyElement_New:
            case APINotifyElement_Copy:
            case APINotifyElement_Undo_Deleted:
            case APINotifyElement_Redo_Created:
                SendEventToNotificationClient (ElementEventType::New, message);
                if (IsObservedElementType (GetElemTypeId (elemType->elemHead))) {
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

bool
AddElementNotificationClientCommand::HasClientToNotifyOnModification ()
{
    for (const auto& kv : clients) {
        if (kv.second.notifyOnModification) {
            return true;
        }
    }
    return false;
}

GS::ObjectState
AddElementNotificationClientCommand::CreateElementEvent (const API_Elem_Head& elemHead)
{
    return GS::ObjectState (
        "elementId", CreateGuidObjectState (elemHead.guid),
        "elementType", GetElementTypeNonLocalizedName (GetElemTypeId (elemHead)));
}

void
AddElementNotificationClientCommand::ReportElementEvents (const std::vector<GS::ObjectState>& changedEvents, const std::vector<GS::ObjectState>& deletedEvents)
{
    if (changedEvents.empty () && deletedEvents.empty ()) {
        return;
    }

    // Inside a batch opened by APINotifyElement_BeginEvents the events join it; otherwise
    // they form a batch of their own, sent in one message per client by the sender thread,
    // so the command does not wait for the clients one event at a time.
    const bool ownBatch = (queuedEvents == nullptr);
    if (ownBatch) {
        queuedEvents.reset (new EventQueue ());
    }

    if (!changedEvents.empty ()) {
        auto& queue = (*queuedEvents)[ElementEventType::Changed];
        queue.insert (queue.end (), changedEvents.begin (), changedEvents.end ());
    }
    if (!deletedEvents.empty ()) {
        auto& queue = (*queuedEvents)[ElementEventType::Deleted];
        queue.insert (queue.end (), deletedEvents.begin (), deletedEvents.end ());
    }

    if (ownBatch) {
        SendQueuedEventsToNotificationClient ();
    }
}

ElementChangeReporter::ElementChangeReporter () :
    enabled (AddElementNotificationClientCommand::HasClientToNotifyOnModification ())
{
}

ElementChangeReporter::ElementChangeReporter (const GS::Array<API_Guid>& elementsToWatch) :
    ElementChangeReporter ()
{
    for (const API_Guid& element : elementsToWatch) {
        Add (element);
    }
}

void
ElementChangeReporter::Add (const API_Guid& element)
{
    if (!enabled || headersBefore.ContainsKey (element)) {
        return;
    }

    API_Elem_Head header = {};
    if (!LoadElementHeaderByGuid (element, header)) {
        return;
    }

    elements.Push (element);
    headersBefore.Add (element, header);
}

void
ElementChangeReporter::AddElementIdsOf (const GS::Array<GS::ObjectState>& items)
{
    if (!enabled) {
        return;
    }

    for (const GS::ObjectState& item : items) {
        const GS::ObjectState* elementId = item.Get ("elementId");
        if (elementId != nullptr) {
            Add (GetGuidFromObjectState (*elementId));
        }
    }
}

void
ElementChangeReporter::MarkChanged (const API_Guid& element)
{
    if (!enabled) {
        return;
    }

    markedChanged.Add (element);
}

void
ElementChangeReporter::Report () const
{
    if (!enabled) {
        return;
    }

    std::vector<GS::ObjectState> changedEvents;
    std::vector<GS::ObjectState> deletedEvents;
    for (const API_Guid& element : elements) {
        const API_Elem_Head& headerBefore = headersBefore[element];
        API_Elem_Head headerNow = {};
        if (!LoadElementHeaderByGuid (element, headerNow)) {
            // The element is gone, so its type is taken from the header read before.
            deletedEvents.push_back (AddElementNotificationClientCommand::CreateElementEvent (headerBefore));
        } else if (headerNow.modiStamp != headerBefore.modiStamp || markedChanged.Contains (element)) {
            changedEvents.push_back (AddElementNotificationClientCommand::CreateElementEvent (headerNow));
        }
    }

    AddElementNotificationClientCommand::ReportElementEvents (changedEvents, deletedEvents);
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
    AddElementNotificationClientCommand::UninstallSelectionChangeHandlerIfNotNeeded ();
    AddElementNotificationClientCommand::UninstallAttributeReplacementHandlerIfNotNeeded ();

    return CreateSuccessfulExecutionResult ();
}
