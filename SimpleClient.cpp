#include <iostream>
#include "olc_net.h"
enum class CustomMsgTypes : uint32_t // maybe put this in some shared header file between simple client and server?
{
	ServerAccept,
	ServerDeny,
	ServerPing,
	MessageAll,
	ServerMessage
};

class customClient: public net::client<CustomMsgTypes>
{
 
 public:
     void pingServer() 
     {
        net::message<CustomMsgTypes> msg;
        msg.header.id = CustomMsgTypes::ServerPing;
        std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
        msg<<timeNow;
        m_connection->send(msg);
     }
     void messageAll()
     {
        net::message<CustomMsgTypes> msg;
        msg.header.id = CustomMsgTypes::MessageAll;
        m_connection->send(msg);
     }
};
void WaitForUserInput(asio::io_context &context,customClient &client,asio::signal_set &signals)
{
    signals.async_wait(
            [&context,&client,&signals](std::error_code ec,int signal)
            {
                if(!ec)
                {
                    std::cout<<"\nsignal recieved\n";
                    switch(signal)
                    {
                        case SIGINT:
                        {
                            client.pingServer();
                        }
                        break;
                        case SIGTSTP:
                        {
                            client.messageAll();
                        }
                        break;
                        case SIGQUIT:
                        {
                            context.stop();
                        }
                        break;
                        default:
                        {}
                    }
                }
                else
                {
                    std::cerr<<" error while waiting for signal:"<<ec.message()<<std::endl;
                }
                WaitForUserInput(context,client,signals);
            });
}
int main()
{
    net::message<CustomMsgTypes> msg;
    customClient client;
    client.Connect("127.0.0.1",6000);
    asio::io_context context;
    std::thread thread_context([&](){context.run();});
    asio::signal_set signals(context,SIGINT,SIGTSTP,SIGQUIT);
    WaitForUserInput(context,client,signals);
    while(!context.stopped())
    {
        if(client.isConnected())
        {
            if(!client.Incoming().empty())
            {
                auto msg = client.Incoming().pop_front().msg;
                switch(msg.header.id)
                {  
                    case CustomMsgTypes::ServerAccept:
                    {
                        std::cout<<"Server accepted connection\n";
                    }
                    break;
                    case CustomMsgTypes::ServerPing:
                    {
                        std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
                        std::chrono::system_clock::time_point timeThen;
                        msg>>timeThen;
                        std::cout<<"Ping: "<<std::chrono::duration<double>(timeNow - timeThen).count() << "\n";
                    }
                    break;
                    case CustomMsgTypes::ServerMessage:
                    {
                        uint32_t clientID;
                        msg>>clientID;
                        std::cout<<"Hello from Client["<<clientID<<"]\n";
                    }
                    break;
                }
            }
        }
        else
        {
            std::cout<<"Connection closed by server\n";
            context.stop();
        }
    }
    if(thread_context.joinable())
        thread_context.join();
    return 0;
}
