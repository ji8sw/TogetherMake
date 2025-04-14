#pragma once
#include <sstream>
#include <iostream>
#include <unordered_map>

#include "enet/enet.h"
#include "PacketHelper.h"
using namespace Samurai;
#define MAX_CONNECTION_ATTEMPTS 5

namespace NetManager
{
    static bool InitializeENet()
    {
        if (enet_initialize() != 0) return false;
        atexit(enet_deinitialize);
        return true;
    }

    struct PlayerData
    {
        ENetPeer* Connection = nullptr;
        std::string Name = "Player";

        uint32_t GetAddressTotal() { return Connection->address.host + Connection->address.port; }
        PlayerData(ENetPeer* InConnection = nullptr) : Connection(InConnection) {}
    };

    class Manager
    {
    public:
        ENetHost* Self = nullptr;
        std::unordered_map<uint32_t, PlayerData> AllConnections;

        void AddConnection(ENetPeer* Peer)
        {
            if (!Peer) return;
            AllConnections[Peer->address.host + Peer->address.port] = PlayerData(Peer);
        }

        void ForgetConnection(ENetPeer* Peer)
        {
            if (!Peer) return;
            AllConnections[Peer->address.host + Peer->address.port] = PlayerData();
        }

        void DisconnectConnection(ENetPeer* Peer)
        {
            if (!Peer) return;
            enet_peer_disconnect(Peer, 0);
            AllConnections[Peer->address.host + Peer->address.port] = PlayerData();
        }

        void DisconnectAllConnections()
        {
            for (auto Player : AllConnections)
            {
                if (!Player.second.Connection) continue;
                enet_peer_disconnect(Player.second.Connection, 0);
                AllConnections[Player.second.GetAddressTotal()] = PlayerData();
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

        uint32_t GetAddressTotal(ENetPeer* Of)
        {
            return Of->address.host + Of->address.port;
        }

        std::vector<ENetPeer*> GetAllPlayerConnectionsExcept(ENetPeer* Except)
        {
            std::vector<ENetPeer*> Result;
            for (auto Player : AllConnections)
            {
                if (Player.first == GetAddressTotal(Except)) continue;
                Result.push_back(Player.second.Connection);
            }

            return Result;
        }
    };
}