#pragma once
#include <iostream>

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

        void RecievePackets()
        {
            ENetEvent Event;
            while (enet_host_service(Self, &Event, 1) > 0)
            {
                switch (Event.type)
                {
                case ENET_EVENT_TYPE_RECEIVE:
                {
                    Packet Incoming = Packet::deserialize((char*)Event.packet->data, Event.packet->dataLength);
                    size_t Offset = 0;

                    switch (Incoming.type)
                    {
                        case PROVIDE_JOINER_INFO: // a new player is broadcasting their name
                        {
                            std::string PlayerName = extractString(Incoming.data, Offset);
                            std::cout << PlayerName << " has joined the session.\n";
                            break;
                        }
                        case PROVIDE_EXISTING_PLAYER_INFOS: // server is sending us all existing players
                        {
                            int PlayerCount = extractInt(Incoming.data, Offset);
                            for (int PlayerIndex = 0; PlayerIndex < PlayerCount; PlayerIndex++)
                                std::cout << "We joined " << extractString(Incoming.data, Offset) << std::endl;
                            break;
                        }
                    }

                    break;
                }
                }

                enet_packet_destroy(Event.packet);
            }
        }
	};
}