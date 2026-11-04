#pragma once

#include "CommandBase.hpp"
#include "Thread.hpp"
#include "URI.hpp"
#include "HTTP/MessageHeader/StatusCode.hpp"

#include <set>
#include <map>
#include <atomic>
#include <vector>

namespace HTTP {
namespace Client {
class Response;
class Request;
}
}

class AddElementNotificationClientCommand : public CommandBase
{
public:
    enum class ElementEventType {
        New,
        Changed,
        Deleted,
        Reserved,
        Released,
        Unknown
    };

    struct Client {
        Client(
            const IO::URI::URI& url,
            bool onNew, bool onMod, bool onResChanges)
            : url(url), notifyOnNew(onNew), notifyOnModification(onMod), notifyOnReservationChanges(onResChanges) {}

        IO::URI::URI url;
        bool         notifyOnNew;
        bool         notifyOnModification;
        bool         notifyOnReservationChanges;

        bool acceptsNotificationsForEventType (ElementEventType eventType) const
        {
            if (eventType == ElementEventType::New) {
                return notifyOnNew;
            } else if (eventType == ElementEventType::Changed || eventType == ElementEventType::Deleted) {
                return notifyOnModification;
            } else if (eventType == ElementEventType::Reserved || eventType == ElementEventType::Released) {
                return notifyOnReservationChanges;
            }
            return false;
        }

        void Send(HTTP::Client::Request& request, const GS::UniString* body);
    };

public:
    AddElementNotificationClientCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;

    static void DetachElementObserversIfNotNeeded ();
    static void SendEventToNotificationClient (ElementEventType eventType, const GS::ObjectState& os);
    static void SendQueuedEventsToNotificationClient ();
    static void SendMessageToNotificationClient (Client& client, const GS::ObjectState& os);
    static GSErrCode ElementEventHandlerProc (const API_NotifyElementType *elemType);
    static bool HasClientToNotifyOnModification ();
    static GS::ObjectState CreateElementEvent (const API_Elem_Head& elemHead);
    static void ReportElementEvents (const std::vector<GS::ObjectState>& changedEvents, const std::vector<GS::ObjectState>& deletedEvents);
    static GSErrCode ElementReservationChangeHandler (const GS::HashTable<API_Guid, short>& reserved,
                                                      const GS::HashSet<API_Guid>&          released,
                                                      const GS::HashSet<API_Guid>&          deleted);

private:
    friend class MessageSenderTask;
    friend class RemoveElementNotificationClientCommand;
    static std::map<GS::UniString, Client> clients;
    static bool hasClientToNotifyOnNew;
    static bool hasClientToNotifyOnModification;
    static bool hasClientToNotifyOnReservationChanges;

    using EventQueue = std::map<ElementEventType, std::vector<GS::ObjectState>>;
    static std::unique_ptr<EventQueue> queuedEvents;

    static GS::Thread messageSenderThread;
};

// Reports the elements a Tapir command changed or deleted to the notification clients as
// changedElements and deletedElements (#1825). Archicad does not call the element observers
// of an add-on for the changes that add-on makes itself, so without this the clients never
// hear of Tapir's own changes. Create it before the command changes anything, with every
// element the command may change or delete, and call Report after its undoable command.
// An element is reported as deleted when it no longer exists, and as changed when its
// modification stamp changed or the command marked it with MarkChanged (a property value
// change, for example, may leave the stamp as it was). Does nothing without a client that
// wants modification notifications.
class ElementChangeReporter
{
public:
    ElementChangeReporter ();
    explicit ElementChangeReporter (const GS::Array<API_Guid>& elementsToWatch);

    void Add (const API_Guid& element);
    // Adds the "elementId" of each item (the items of a Modify<Tool> command); items without one are skipped.
    void AddElementIdsOf (const GS::Array<GS::ObjectState>& items);
    void MarkChanged (const API_Guid& element);
    void Report () const;

private:
    bool                                    enabled;
    GS::Array<API_Guid>                     elements;
    GS::HashTable<API_Guid, API_Elem_Head>  headersBefore;
    GS::HashSet<API_Guid>                   markedChanged;
};

class RemoveElementNotificationClientCommand : public CommandBase
{
public:
    RemoveElementNotificationClientCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};
