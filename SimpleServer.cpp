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
        net::message<CustomMsgTypes> msg;
        msg.header.id = CustomMsgTypes::ServerAccept;
        client->send(msg);
        return true;
    }
    virtual void onClientDisconnect(std::shared_ptr<net::connection<CustomMsgTypes>> client)
    {
        std::cout<<"Removing client["<<client->getID()<<"]\n";
    }
    virtual void onMessage(std::shared_ptr<net::connection<CustomMsgTypes>> client,net::message<CustomMsgTypes> &msg)
    {
        switch(msg.header.id)
        {
            case CustomMsgTypes::ServerAccept:
            {
                
            }
            break;
            case CustomMsgTypes::ServerPing:
            {
                std::cout<<"["<<client->getID() << "]: Server Pinged"<<"\n";
                client->send(msg);
            }
            break;
            case CustomMsgTypes::MessageAll:
            {
                std::cout<<"["<<client->getID()<<"]: Message All\n";
                net::message<CustomMsgTypes> msg;
                msg.header.id = CustomMsgTypes::ServerMessage;
                msg<<client->getID();
                messageAllClient(msg,client);
            }
            break;
        }
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
