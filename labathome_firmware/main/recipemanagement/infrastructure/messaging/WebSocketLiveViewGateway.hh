#pragma once

#include "../core/interfaces/messaging/ILiveViewGateway.hh"
#include "webmanager_interfaces.hh" // Für iWebmanagerCallback

// Implementiert das ILiveViewGateway-Interface mittels WebSockets.
// Nutzt das webmanager-Callback, um Nachrichten zu senden.
class WebSocketLiveViewGateway : public ILiveViewGateway {
public:
    WebSocketLiveViewGateway(webmanager::iWebmanagerCallback* callback);
    // void sendUpdate(const LiveViewDto& dto) override;
private:
    webmanager::iWebmanagerCallback* m_callback;
};
