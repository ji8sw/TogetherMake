#include "NetManager.h"
#include <iostream>
#include <vector>

int main()
{
	// Initialize server.
	if (!NetManager::InitializeENet()) throw "Failed to start.";
	NetManager::Manager NManager = NetManager::Manager();
	if (!NManager.TryCreateLocalServer()) throw "Failed to start.";
	bool Running = true;

    while (Running)
    {
        ENetEvent Event;
        while (enet_host_service(NManager.Self, &Event, 100) > 0)
        {
            switch (Event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                std::cout << "A new peer connected from " << IpToString(Event.peer->address.host) << ":" << Event.peer->address.port << std::endl;
                NManager.AddConnection(Event.peer);
                break;
            case ENET_EVENT_TYPE_RECEIVE:
            {
                Packet Incoming = Packet::deserialize((char*)Event.packet->data, Event.packet->dataLength);
                size_t Offset = 0;

                switch (Incoming.type)
                {
                    case PROVIDE_JOINER_INFO: // a new player is broadcasting their name
                    { // we will send their name to all OTHER players (hence GetAllPlayerConnectionsExcept)
                        // We might use the player name later
                        NetManager::PlayerData* Player = &NManager.AllConnections[NManager.GetAddressTotal(Event.peer)];
                        if (Player)
                        {
                            Player->Name = extractString(Incoming.data, Offset);
                            Packet Response(PROVIDE_JOINER_INFO);
                            appendString(Response.data, Player->Name);
                            auto AllOtherPlayers = NManager.GetAllPlayerConnectionsExcept(Event.peer);
                            sendBroadcastNow(AllOtherPlayers, Response);
                            // send all other players to the joiner
                            Packet JoinerResponse(PROVIDE_EXISTING_PLAYER_INFOS);
                            appendInt(JoinerResponse.data, AllOtherPlayers.size());
                            for (NetManager::PlayerData OtherPlayer : AllOtherPlayers)
                                appendString(JoinerResponse.data, OtherPlayer.Name);
                            sendNow(JoinerResponse, Player->Connection);
                        }
                        break;
                    }
                    case SELECT_VERTEX:
                    case DESELECT_VERTEX:
                    case UPDATE_VERTEX_POSITION:
                    {
                        NetManager::PlayerData* Player = &NManager.AllConnections[NManager.GetAddressTotal(Event.peer)];
                        if (Player)
                        {
                            auto AllOtherPlayers = NManager.GetAllPlayerConnectionsExcept(Event.peer);
                            sendBroadcastNow(AllOtherPlayers, Incoming);
                        }
                        break;
                    }
                }

                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT:
                NManager.ForgetConnection(Event.peer);
                break;
            }

            enet_packet_destroy(Event.packet);
        }
    }

    NManager.DisconnectAllConnections();
    enet_host_destroy(NManager.Self);
}