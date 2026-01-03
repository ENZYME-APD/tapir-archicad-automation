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
    ~AddElementNotificationClientCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;

    static void SendEventToNotificationClient (ElementEventType eventType, const GS::ObjectState& os);
    static void SendQueuedEventsToNotificationClient ();
    static void SendMessageToNotificationClient (Client& client, const GS::ObjectState& os);
    static GSErrCode ElementEventHandlerProc (const API_NotifyElementType *elemType);
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

class RemoveElementNotificationClientCommand : public CommandBase
{
public:
    RemoveElementNotificationClientCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};
