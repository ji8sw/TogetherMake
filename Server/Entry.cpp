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
                std::cout << "A new peer connected from " << NetManager::IpToString(Event.peer->address.host) << ":" << Event.peer->address.port << std::endl;
                NManager.AddConnection(Event.peer);
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                
                enet_packet_destroy(Event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                NManager.ForgetConnection(Event.peer);
                break;
            }
        }
    }

    NManager.DisconnectAllConnections();
    enet_host_destroy(NManager.Self);
}