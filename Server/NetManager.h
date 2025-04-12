#pragma once
#include "enet/enet.h"
#include <sstream>
#include <iostream>
#include <unordered_map>
#define MAX_CONNECTION_ATTEMPTS 5

namespace NetManager
{
    std::string IpToString(enet_uint32 IP)
    {
        unsigned char Byte1 = IP & 0xFF;
        unsigned char Byte2 = (IP >> 8) & 0xFF;
        unsigned char Byte3 = (IP >> 16) & 0xFF;
        unsigned char Byte4 = (IP >> 24) & 0xFF;

        std::ostringstream IpString;
        IpString << static_cast<int>(Byte1) << "."
            << static_cast<int>(Byte2) << "."
            << static_cast<int>(Byte3) << "."
            << static_cast<int>(Byte4);

        return IpString.str();
    }

    static bool InitializeENet()
    {
        if (enet_initialize() != 0) return false;
        atexit(enet_deinitialize);
        return true;
    }

    class Manager
    {
    public:
        ENetHost* Self = nullptr;
        std::unordered_map<uint32_t, ENetPeer*> AllConnections;

        void AddConnection(ENetPeer* Peer)
        {
            if (!Peer) return;
            AllConnections[Peer->address.host + Peer->address.port] = Peer;
        }

        void ForgetConnection(ENetPeer* Peer)
        {
            if (!Peer) return;
            AllConnections[Peer->address.host + Peer->address.port] = nullptr;
        }

        void DisconnectConnection(ENetPeer* Peer)
        {
            if (!Peer) return;
            enet_peer_disconnect(Peer, 0);
            AllConnections[Peer->address.host + Peer->address.port] = nullptr;
        }

        void DisconnectAllConnections()
        {
            for (auto Peer : AllConnections)
            {
                if (!Peer.second) continue;
                enet_peer_disconnect(Peer.second, 0);
                AllConnections[Peer.second->address.host + Peer.second->address.port] = nullptr;
            }
        }

        bool TryCreateLocalServer()
        {
            // self address (IP and port)
            ENetAddress LocalAddress;
            enet_address_set_host(&LocalAddress, "0.0.0.0");
            LocalAddress.port = 8080;

            Self = enet_host_create(&LocalAddress, 32, 2, 0, 0);

            if (Self == nullptr) return false;

            std::cout << "Server running on port: " << Self->address.port << std::endl;
            std::cout << "Server running on address: " << IpToString(Self->address.host) << std::endl;
            return true;
        }
    };
}