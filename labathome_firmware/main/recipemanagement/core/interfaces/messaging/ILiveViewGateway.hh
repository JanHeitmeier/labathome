#pragma once

// Interface für das Senden von Live-Daten während der Rezeptausführung.
// Wird von der Infrastructure-Schicht implementiert (z.B. als WebSocketGateway).
class ILiveViewGateway {
public:
    virtual ~ILiveViewGateway() = default;
    // virtual void sendUpdate(const LiveViewDto& dto) = 0;
};
