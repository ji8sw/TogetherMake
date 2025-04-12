#pragma once
#include "enet/enet.h"
#define MAX_CONNECTION_ATTEMPTS 5
#include <iostream>

namespace NetManager
{
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
		ENetPeer* Server = nullptr;

		bool TryCreateLocalServer()
		{
            // self address (IP and port)
            ENetAddress LocalAddress;
            LocalAddress.host = ENET_HOST_ANY;
            LocalAddress.port = ENET_PORT_ANY;

            // Create a local server which will send and recieve packets
            int Tries = MAX_CONNECTION_ATTEMPTS - 1;
            while (Tries--) // try to create an ENet server 5 times
            {
                Self = enet_host_create(&LocalAddress, 5, 2, 0, 0);
                if (Self == nullptr)
                    LocalAddress.port++; // try again on another port
                else return true;
            }
            return false;
		}
        
        bool TryConnectToMatchmakingServer()
        {
            ENetAddress ServerAddress;
            enet_address_set_host(&ServerAddress, "127.0.0.1");
            ServerAddress.port = 8080;

            Server = enet_host_connect(Self, &ServerAddress, 2, 0);
            if (Server == nullptr) return false;

            ENetEvent event;
            if (enet_host_service(Self, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
            {
                enet_host_flush(Self);
                return true;
            }
            else
            {
                enet_peer_reset(Server);
                Server = nullptr;
                return false;
            }
        }
	};
}