#include <iostream>
#include "olc_net.h"
enum class CustomMsgTypes
{
    ServerAccept,
    ServerDeny,
    ServerPing,
    MessageAll,
    ServerMessage
};
class customServer : public net::server <CustomMsgTypes>
{
public:
    customServer(uint16_t port):net::server <CustomMsgTypes>(port)
    {}

protected:
    virtual bool onClientConnect(std::shared_ptr<net::connection<CustomMsgTypes>> client)
    {
        return true;
    }
    virtual void onClientDisconnect(std::shared_ptr<net::connection<CustomMsgTypes>> client)
    {

    }
    virtual void onMessage(std::shared_ptr<net::connection<CustomMsgTypes>> client,net::message<CustomMsgTypes> &msg)
    {

    }
};

int main()
{
    customServer server(6000);
    server.start();
    while(1)
    {
        server.update();
    }
    return 0;
}
