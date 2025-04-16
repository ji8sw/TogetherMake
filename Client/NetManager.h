#pragma once
#include <iostream>

#include "enet/enet.h"
#include "PacketHelper.h"
#include "Object.h"
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
        ENetPeer* Connection = nullptr; // Not used client side, do not access
        std::string Name = "Player";
        uint32_t AddressTotal = 0;
    };

	class Manager
	{
    public:
		ENetHost* Self = nullptr;
		ENetPeer* Server = nullptr;
        Object* MainObject = nullptr;
        std::unordered_map<uint32_t, PlayerData> KnownPlayers;

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
        
        bool TryConnectToServer()
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
        
        // begins disconnecting from server, no data is cleared just yet
        void DisconnectFromServer()
        {
            if (Server == nullptr) return;

            enet_peer_disconnect(Server, 0);
        }

        // after disconnecting from server, clear session related data
        void OnDisconnectFromServer()
        {
            enet_peer_reset(Server);
            Server = nullptr;
            KnownPlayers.clear();
        }

        //
        // Important game related event handling:
        //

        void SendOnSelectVertex(int VertexIndex)
        {
            if (this && this->Server)
            {
                Packet Event(SELECT_VERTEX);
                appendInt(Event.data, VertexIndex);
                sendNow(Event, Server);
            }
        }

        void RecieveOnSelectVertex(int VertexIndex)
        {
            if (this && this->Server && MainObject)
            {
                MainObject->Vertices[VertexIndex].IsGrabbed = true;
#ifdef _DEBUG
                std::cout << "Network: Recieved event: OnSelectVertex\n";
#endif
            }
        }

        void SendOnDeselectVertex(int VertexIndex)
        {
            if (this && this->Server && MainObject)
            {
                Packet Event(DESELECT_VERTEX);
                appendInt(Event.data, VertexIndex);
                sendNow(Event, Server);
            }
        }

        void RecieveOnDeselectVertex(int VertexIndex)
        {
            if (this && this->Server && MainObject)
            {
                MainObject->Vertices[VertexIndex].IsGrabbed = false;

#ifdef _DEBUG
                std::cout << "Network: Recieved event: OnDeselectVertex\n";
#endif
            }
        }

        void SendUpdateVertexPosition(int VertexIndex, glm::vec3 NewPosition)
        {
            if (this && this->Server && MainObject)
            {
                Packet Event(UPDATE_VERTEX_POSITION);
                appendInt(Event.data, VertexIndex);
                appendData<glm::vec3>(Event.data, NewPosition);
                sendNow(Event, Server);
            }
        }

        void RecieveUpdateVertexPosition(int VertexIndex, glm::vec3 NewPosition)
        {
            if (this && this->Server && MainObject)
            {
                MainObject->Vertices[VertexIndex].Position = NewPosition;

#ifdef _DEBUG
                std::cout << "Network: Recieved event: UpdateVertexPosition\n";
#endif
            }
        }

        //

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
                        PlayerData Player(0);
                        Player.Name = extractString(Incoming.data, Offset);
                        Player.AddressTotal = extractInt(Incoming.data, Offset);
                        KnownPlayers[Player.AddressTotal] = Player;
                        std::cout << Player.Name << " has joined the session.\n";
                        break;
                    }
                    case PLAYER_LEFT: // an existing player left, address total is provided to identify them
                    {
                        uint32_t AddressTotal = extractInt(Incoming.data, Offset);
                        PlayerData& Player = KnownPlayers[AddressTotal];
                        if (Player.AddressTotal != 0) // yes, we know this player
                        {
                            std::cout << Player.Name << " has left the session.\n";
                        }
                        break;
                    }
                    case PROVIDE_EXISTING_PLAYER_INFOS: // server is sending us all existing players
                    {
                        int PlayerCount = extractInt(Incoming.data, Offset);
                        for (int PlayerIndex = 0; PlayerIndex < PlayerCount; PlayerIndex++)
                            std::cout << "We joined " << extractString(Incoming.data, Offset) << std::endl;
                        break;
                    }
                    case SELECT_VERTEX:
                    {
                        int VertexIndex = extractInt(Incoming.data, Offset);
                        RecieveOnSelectVertex(VertexIndex);
                        break;
                    }
                    case DESELECT_VERTEX:
                    {
                        int VertexIndex = extractInt(Incoming.data, Offset);
                        RecieveOnDeselectVertex(VertexIndex);
                        break;
                    }
                    case UPDATE_VERTEX_POSITION:
                    {
                        int VertexIndex = extractInt(Incoming.data, Offset);
                        glm::vec3 VertexPosition = extractData<glm::vec3>(Incoming.data, Offset);
                        RecieveUpdateVertexPosition(VertexIndex, VertexPosition);
                        break;
                    }
                    }

                    break;
                }
                case ENET_EVENT_TYPE_DISCONNECT:
                    if (Event.peer == Server) // we have disconnected from the server ( or they disconnected from us :( )
                    {
                        OnDisconnectFromServer();
                        std::cout << "We left the session.\n";
                    }
                    break;
                }

                enet_packet_destroy(Event.packet);
            }
        }
	};
}