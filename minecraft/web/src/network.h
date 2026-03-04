#pragma once
#include "../../handheld/src/network/RakNetInstance.h"

class WSNetInstance : public IRakNetInstance {
public:
    WSNetInstance();

    bool host(const std::string& localName, int port, int maxConnections = 4) override;
    bool connect(const char* host, int port) override;
    const ServerList& getServerList() override;
    void disconnect() override;
    
    void runEvents(NetEventCallback* callback) override;
    void send(Packet& packet) override;
    void send(const RakNet::RakNetGUID& guid, Packet& packet) override;
    
    void announceServer(const std::string& localName) override;
    
    // void pingForHosts(int port) override;
    // void stopPingForHosts() override;
    // void clearServerList() override;

private:
    ServerList servers;
    std::string localName;
};