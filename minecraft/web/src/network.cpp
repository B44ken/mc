#include "network.h"
#include <emscripten/emscripten.h>
#include <emscripten/websocket.h>
#include <string>
#include <vector>
#include <cstdint>
#include <numeric>
#include "../../handheld/src/network/Packet.h"
#include "../../handheld/src/raknet/BitStream.h"
using namespace RakNet;
typedef ::Packet mcPacket;

// WSRakInstance is a thin wrapper - by default minecraft just sends all its packets
WSRakInstance::WSRakInstance() {}
static std::vector<std::vector<uint8_t>> queueIn, queueOut;

bool WSRakInstance::host(const std::string& name, int port, int) {
    return connect("127.0.0.1", port);
}

bool pushQueue(int, const EmscriptenWebSocketMessageEvent* e, void* q) {
    static_cast<std::vector<std::vector<uint8_t>>*>(q)->emplace_back(e->data, e->data + e->numBytes);
    return true;
}

bool logErr(int, const EmscriptenWebSocketErrorEvent* e, void*) {
    emscripten_console_error("[ws] err"); // todo print err
    return true;
}

bool WSRakInstance::connect(const char* host, int) {
    if(sock > 0) return true;
    EmscriptenWebSocketCreateAttributes attrs { .url = "ws://127.0.0.1:19132", .protocols = 0, .createOnMainThread = true };
    sock = emscripten_websocket_new(&attrs);
    emscripten_websocket_set_onmessage_callback(sock, &queueIn, pushQueue);
    emscripten_websocket_set_onerror_callback(sock, 0, logErr);
    return true;
}

const ServerList& WSRakInstance::getServerList() {
    if(servers.empty()) {
        SystemAddress addr = UNASSIGNED_SYSTEM_ADDRESS;
        servers.push_back(PingedCompatibleServer { .name = "localhost", .address = addr, .pingTime = 0, .isSpecial = false });
    }
    return servers;
}

void WSRakInstance::runEvents(NetEventCallback* cb) {
    uint16_t ready = -1;
    emscripten_websocket_get_ready_state(sock, &ready);
    if(ready == 1) {
        if(didOnConnect++ == 0) cb->onConnect(UNASSIGNED_RAKNET_GUID);
        for(auto &p : queueOut)
            emscripten_websocket_send_binary(sock, const_cast<uint8_t*>(p.data()), p.size());
        queueOut.clear();
    }
    for(auto& msg : queueIn) { // always 8byte guid + 1byte id + data
        uint64_t guid = 0;
        for(int i = 0; i < 8; i++) guid = msg[i] + (guid << 8);
        BitStream bs(msg.data() + 9, msg.size() - 9, false);
        if(auto packet = MinecraftPackets::createPacket(msg[8])) {
            packet->read(&bs);
            packet->handle(RakNetGUID(guid), cb);
            delete packet;
        }
    }
    queueIn.clear();
}

void WSRakInstance::send(mcPacket& pack) {
    BitStream s;
    s.Write(0ull);
    pack.write(&s);
    queueOut.push_back(std::vector<uint8_t>(s.GetData(), s.GetData() + s.GetNumberOfBytesUsed()));
}

void WSRakInstance::send(const RakNetGUID& guid, mcPacket& pack) {  // todo
    BitStream s;
    s.Write(guid.g);
    pack.write(&s);
    queueOut.push_back(std::vector<uint8_t>(s.GetData(), s.GetData() + s.GetNumberOfBytesUsed()));
}