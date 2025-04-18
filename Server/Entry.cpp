#include "NetManager.h"
#include <iostream>
#include <vector>
#define MAKETOGETHER_VERSION 1

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
                            int PlayerVersion = extractInt(Incoming.data, Offset);

                            if (PlayerVersion != MAKETOGETHER_VERSION)
                            { // the players game is out of date so we will tell them to disconnect
                                Packet Response(REFUSE_JOIN);
                                sendNow(Response, Player->Connection);
                                break;
                            }

                            // in the below we have to manually append the player data as using appendData on it does not go well...
                            Packet Response(PROVIDE_JOINER_INFO);
                            appendString(Response.data, Player->Name);
                            appendInt(Response.data, Player->AddressTotal);
                            auto AllOtherPlayers = NManager.GetAllPlayerConnectionsExcept(Event.peer);
                            sendBroadcastNow(AllOtherPlayers, Response);

                            // send all other players to the joiner
                            Packet JoinerResponse(PROVIDE_EXISTING_PLAYER_INFOS);
                            appendInt(JoinerResponse.data, AllOtherPlayers.size());
                            for (NetManager::PlayerData OtherPlayer : AllOtherPlayers)
                            {
                                appendString(JoinerResponse.data, OtherPlayer.Name);
                                appendInt(JoinerResponse.data, OtherPlayer.AddressTotal);
                            }
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
                    case REQUEST_VERTICES: // a player is asking to sync vertices
                    { // we will ask the host to provide us with vertices
                        NetManager::PlayerData* Player = &NManager.AllConnections[NManager.GetAddressTotal(Event.peer)];
                        if (NManager.Host == 0) break;
                        auto& Host = NManager.AllConnections[NManager.Host];
                        if (Player && Player->Connection != Host.Connection)
                        {
                            Packet HostRequest(REQUEST_VERTICES);
                            sendNow(HostRequest, Host.Connection);
                            NManager.VertexRequestQueue.push_back(Player);
                        }
                        break;
                    }
                    case PROVIDE_VERTICES: // the host has provided us with vertices
                    { // we will send them to everyone who requested an update
                        // the list will be a list of raw floats in the format: [x, y, z, u, v]
                        // the recipient will format that back into NetVertex
                        NetManager::PlayerData* Player = &NManager.AllConnections[NManager.GetAddressTotal(Event.peer)];
                        auto& Host = NManager.AllConnections[NManager.Host];
                        if (Player && Player->Connection == Host.Connection)
                        {
                            for (const auto Requester : NManager.VertexRequestQueue)
                            {
                                sendNow(Incoming, Requester->Connection);
                            }
                        }
                        break;
                    }
                }

                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT: // a peer disconnected
                // if its a connected player then send a notif to all players that the player has left
                if (NManager.AllConnections[NManager.GetAddressTotal(Event.peer)].Connection != nullptr)
                {
                    Packet LeaveAlert(PLAYER_LEFT);
                    appendInt(LeaveAlert.data, NManager.GetAddressTotal(Event.peer));

                    auto AllOtherPlayers = NManager.GetAllPlayerConnectionsExcept(Event.peer);
                    sendBroadcastNow(AllOtherPlayers, LeaveAlert);
                }
                NManager.ForgetConnection(Event.peer);
                break;
            }

            enet_packet_destroy(Event.packet);
        }
    }

    NManager.DisconnectAllConnections();
    enet_host_destroy(NManager.Self);
}