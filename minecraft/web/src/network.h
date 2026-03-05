#pragma once
#include "../../handheld/src/network/RakNetInstance.h"
typedef ::Packet mcPacket;

class WSRakInstance : public IRakNetInstance {
public:
    WSRakInstance();

    bool host(const std::string& localName, int port, int maxConnections = 4) override;
    bool connect(const char* host, int port) override;
    const ServerList& getServerList() override;
    
    void runEvents(NetEventCallback* callback) override;
    void send(mcPacket& packet) override;
    void send(const RakNet::RakNetGUID& guid, mcPacket& packet) override;
    
    // void disconnect() override;
    // void announceServer(const std::string& localName) override;
    // void pingForHosts(int port) override;
    // void stopPingForHosts() override;
    // void clearServerList() override;

    std::vector<std::vector<uint8_t>> queueIn, queueOut;
    ServerList servers;
    int sock = -1, didOnConnect = 0;

};