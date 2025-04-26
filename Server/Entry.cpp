#include "NetManager.h"
#include <iostream>
#include <vector>
#include <fstream>
#include "Value.h"
#define MAKETOGETHER_VERSION 1

int main(int argc, char* argv[])
{
	// Initialize server.
	if (!NetManager::InitializeENet()) throw "Failed to start.";
	NetManager::Manager NManager = NetManager::Manager();
	if (!NManager.TryCreateLocalServer()) throw "Failed to start.";
	bool Running = true;

    // Load configuration from parameter if provided.
    Value::Set Configuration;
    if (argc >= 2)
    {
        const char* ServerConfigPath = argv[1];
        std::fstream ServerConfigFile(ServerConfigPath);
        if (!Configuration.AppendFrom(ServerConfigPath))
            std::cout << "Failed to open provided server configuration, it will be ignored.\n";
    }

    // Load password from config
    auto PasswordKey = Configuration.Get("Password");
    if (PasswordKey) NManager.Password = PasswordKey->value;
    if (!NManager.Password.empty()) std::cout << "Server Password: " << NManager.Password << std::endl;

    while (Running)
    {
        ENetEvent Event;
        while (enet_host_service(NManager.Self, &Event, 100) > 0)
        {
            switch (Event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
            { // we will handle their join later via PROVIDE_JOINER_INFO
                NManager.AddConnection(Event.peer);
                break;
            }
            case ENET_EVENT_TYPE_RECEIVE:
            {
                Packet Incoming = Packet::deserialize((char*)Event.packet->data, Event.packet->dataLength);
                size_t Offset = 0;
                auto Player = &NManager.AllConnections[NManager.GetAddressTotal(Event.peer)];
                if (!Player) break; // we only communicate with existing players, k thx bye

                switch (Incoming.type)
                {
                    case PROVIDE_JOINER_INFO: // a new player is broadcasting their name
                    { // we will send their name to all OTHER players (hence GetAllPlayerConnectionsExcept)
                        Player->Name = extractString(Incoming.data, Offset);
                        int PlayerVersion = extractInt(Incoming.data, Offset);
                        std::string ProvidedPassword = extractString(Incoming.data, Offset);

                        if (PlayerVersion != MAKETOGETHER_VERSION)
                        { // the players game is out of date so we will tell them to disconnect
                            Packet Response(REFUSE_JOIN);
                            sendNow(Response, Player->Connection);
                            NManager.ForgetConnection(Player->Connection);
                            break;
                        }

                        if (!NManager.Password.empty() && ProvidedPassword != NManager.Password)
                        { // incorrect password, we will tell them to disconnect
                            Packet Response(REFUSE_JOIN);
                            sendNow(Response, Player->Connection);
                            NManager.ForgetConnection(Player->Connection);
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
                        std::cout << Player->Name << " has joined the session." << std::endl;
                        break;
                    }
                    case SELECT_VERTEX:
                    case DESELECT_VERTEX:
                    case UPDATE_VERTEX_POSITION:
                    {
                        auto AllOtherPlayers = NManager.GetAllPlayerConnectionsExcept(Event.peer);
                        sendBroadcastNow(AllOtherPlayers, Incoming);
                        break;
                    }
                    case REQUEST_VERTICES: // a player is asking to sync vertices
                    { // we will ask the host to provide us with vertices
                        if (NManager.Host == 0) break;
                        auto& Host = NManager.AllConnections[NManager.Host];
                        if (Player->Connection != Host.Connection)
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
                        auto& Host = NManager.AllConnections[NManager.Host];
                        if (Player->Connection == Host.Connection)
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
                    if (NManager.GetAddressTotal(Event.peer) == NManager.Host)
                    { // Host migration: set host to the next available player, if they exist
                        if (NManager.AllConnections.size() > 1)
                        {
                            NManager.Host = NManager.AllConnections[1].AddressTotal;
                            std::cout << "The host has migrated to " << NManager.AllConnections[1].Name << "\n";
                        }
                    }
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