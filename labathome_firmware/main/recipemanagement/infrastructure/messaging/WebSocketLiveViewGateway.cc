#include "WebSocketLiveViewGateway.hh"

// Implementierung für WebSocketLiveViewGateway.
WebSocketLiveViewGateway::WebSocketLiveViewGateway(webmanager::iWebmanagerCallback* callback)
    : m_callback(callback) {
}

/*
void WebSocketLiveViewGateway::sendUpdate(const LiveViewDto& dto) {
    // 1. DTO in JSON umwandeln.
    // 2. m_callback->WrapAndSendAsync(...) aufrufen.
}
*/
