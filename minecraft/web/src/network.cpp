#include "network.h"

#include <emscripten/emscripten.h>
#include <emscripten/websocket.h>

#include <string>
#include <vector>
#include <cstdint>

#include "../../handheld/src/network/Packet.h"
#include "../../handheld/src/raknet/BitStream.h"

const int assumeLocal = 6767;

static int socketId = 0;
static std::vector<std::vector<uint8_t>> queueIn;
static std::vector<std::vector<uint8_t>> queueOut;
static bool pendingConnectEvent = false;
static bool pendingDisconnectEvent = false;

static void closeSocket() {
    if (!socketId) return;
    emscripten_websocket_close(socketId, 1000, "disconnect");
    emscripten_websocket_delete(socketId);
    socketId = 0;
}

static bool onSocketOpen(int, const EmscriptenWebSocketOpenEvent*, void*) {
    pendingConnectEvent = true;
    emscripten_log(EM_LOG_CONSOLE, "[ws] open");
    return true;
}

static bool onSocketError(int, const EmscriptenWebSocketErrorEvent*, void*) {
    pendingDisconnectEvent = true;
    emscripten_log(EM_LOG_CONSOLE, "[ws] error");
    return true;
}

static bool onSocketClose(int, const EmscriptenWebSocketCloseEvent* event, void*) {
    pendingDisconnectEvent = true;
    emscripten_log(EM_LOG_CONSOLE, "[ws] close code=%d", event->code);
    return true;
}

static bool onSocketMessage(int, const EmscriptenWebSocketMessageEvent* event, void*) {
    if (!event || !event->data || event->numBytes <= 0) return true;

    const uint8_t* data = reinterpret_cast<const uint8_t*>(event->data);
    queueIn.emplace_back(data, data + event->numBytes);
    return true;
}

static bool openSocket(const std::string& url) {
    if (!emscripten_websocket_is_supported()) {
        emscripten_log(EM_LOG_CONSOLE, "[ws] websocket not supported");
        return false;
    }

    closeSocket();

    EmscriptenWebSocketCreateAttributes attrs;
    attrs.url = url.c_str();
    attrs.protocols = 0;
    attrs.createOnMainThread = EM_TRUE;

    socketId = emscripten_websocket_new(&attrs);
    if (socketId <= 0) {
        socketId = 0;
        emscripten_log(EM_LOG_CONSOLE, "[ws] failed to create websocket");
        return false;
    }

    emscripten_websocket_set_onopen_callback(socketId, 0, onSocketOpen);
    emscripten_websocket_set_onerror_callback(socketId, 0, onSocketError);
    emscripten_websocket_set_onclose_callback(socketId, 0, onSocketClose);
    emscripten_websocket_set_onmessage_callback(socketId, 0, onSocketMessage);
    pendingConnectEvent = false;
    pendingDisconnectEvent = false;
    return true;
}

static void queuePacketData(const unsigned char* data, unsigned int size) {
    if (!data || size == 0) return;

    queueOut.push_back(std::vector<uint8_t>(data, data + size));
}

static bool isSocketReadyForSend() {
    if (!socketId) return false;

    unsigned short readyState = 0;
    EMSCRIPTEN_RESULT result = emscripten_websocket_get_ready_state(socketId, &readyState);
    return result == EMSCRIPTEN_RESULT_SUCCESS && readyState == 1;
}

static void flushOutboundPackets() {
    if (queueOut.empty() || !isSocketReadyForSend()) return;

    std::vector<std::vector<uint8_t>> pending;
    pending.reserve(queueOut.size());

    for (size_t i = 0; i < queueOut.size(); ++i) {
        const std::vector<uint8_t>& payload = queueOut[i];
        if (payload.empty()) continue;

        EMSCRIPTEN_RESULT result = emscripten_websocket_send_binary(socketId, const_cast<uint8_t*>(payload.data()), payload.size());
        if (result != EMSCRIPTEN_RESULT_SUCCESS) pending.push_back(payload);
    }

    queueOut.swap(pending);
}

static void dispatchInboundPackets(NetEventCallback* callback) {
    if (!callback || queueIn.empty()) {
        queueIn.clear();
        return;
    }

    for (size_t i = 0; i < queueIn.size(); ++i) {
        std::vector<uint8_t>& packetBytes = queueIn[i];
        if (packetBytes.empty()) continue;

        size_t offset = 0;
        RakNet::RakNetGUID sourceGuid = RakNet::UNASSIGNED_RAKNET_GUID;

        if (packetBytes.size() > 9 && packetBytes[8] >= ID_USER_PACKET_ENUM) {
            uint64_t sourceGuidValue = 0;
            for (int b = 0; b < 8; ++b)
                sourceGuidValue |= ((uint64_t)packetBytes[b]) << (8 * b);
            sourceGuid = RakNet::RakNetGUID(sourceGuidValue);
            offset = 8;
        }

        int packetId = packetBytes[offset];
        if (packetBytes.size() <= offset + 1) continue;

        RakNet::BitStream activeBitStream(packetBytes.data() + offset + 1, packetBytes.size() - offset - 1, false);
        if (Packet* packet = MinecraftPackets::createPacket(packetId)) {
            packet->read(&activeBitStream);
            packet->handle(sourceGuid, callback);
            delete packet;
        }
    }

    queueIn.clear();
}

WSNetInstance::WSNetInstance() {}

bool WSNetInstance::host(const std::string& name, int, int) {
    localName = name;
    return connect("127.0.0.1", assumeLocal);
}

bool WSNetInstance::connect(const char* host, int port) {
    std::string url = "ws://127.0.0.1:" + std::to_string(port);
    emscripten_log(EM_LOG_CONSOLE, "[ws] connect %s", url.c_str());
    return openSocket(url);
}

const ServerList& WSNetInstance::getServerList() {
    if (servers.empty()) {
        RakNet::SystemAddress addr = RakNet::UNASSIGNED_SYSTEM_ADDRESS;
        addr.SetPort(assumeLocal);
        servers.push_back(PingedCompatibleServer { .name = "devel", .address = addr, .pingTime = 0, .isSpecial = false });
    }
    return servers;
}

void WSNetInstance::disconnect() {
    closeSocket();
    pendingConnectEvent = false;
    pendingDisconnectEvent = false;
    queueIn.clear();
    queueOut.clear();
}

void WSNetInstance::announceServer(const std::string& name) { localName = name; }

void WSNetInstance::runEvents(NetEventCallback* cb) {
    if (cb && pendingConnectEvent) {
        pendingConnectEvent = false;
        cb->onConnect(RakNet::UNASSIGNED_RAKNET_GUID);
    }

    if (cb && pendingDisconnectEvent) {
        pendingDisconnectEvent = false;
        cb->onDisconnect(RakNet::UNASSIGNED_RAKNET_GUID);
    }

    flushOutboundPackets();
    dispatchInboundPackets(cb);
}

void WSNetInstance::send(Packet& pack) {
    RakNet::BitStream bitStream;
    pack.write(&bitStream);
    queuePacketData(bitStream.GetData(), bitStream.GetNumberOfBytesUsed());
}

void WSNetInstance::send(const RakNet::RakNetGUID&, Packet& pack) { send(pack); }