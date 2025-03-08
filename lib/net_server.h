#pragma once
#include "net_common.h"
#include "net_message.h"
#include "net_queue.h"
#include "net_connection.h"

namespace net
{
    template<typename T>
        class server
        {
            public:
                server(uint16_t port)
                    :m_acceptor{m_context,asio::ip::tcp::endpoint(asio::ip::tcp::v4(),port)}
                {

                }
                virtual ~server()
                {
                    stop();
                }
                bool start()
                {
                    try
                    {
                        waitForClientConnection();
                        //we are giving serever som job before because the context can complete its job and finish the program
                        m_thread = std::thread([this](){m_context.run();});
                    }
                    catch(std::exception &e)
                    {
                        std::cerr<<"[SERVER] Exception: "<<e.what()<<std::endl;
                        return false;
                    }
                    std::cout<<"[SERVER] started: \n";
                    return true;
                }
                void stop()
                {
                    m_context.stop();
                    if(m_thread.joinable())
                        m_thread.join();
                    std::cout<<"[SERVER] stopped: \n"; //TODO: make a log file instead
                }
                //ASYNC 
                void waitForClientConnection()
                {
                    m_acceptor.async_accept(
                            [this](std::error_code ec,asio::ip::tcp::socket socket)
                            {
                            if(!ec)
                            {
                               std::cout<<"[SERVER] New Connection: "<<socket.remote_endpoint()<<std::endl;
                            
                            //we want the connection to have a owner context and move the socket as its now part of connection, here the queue(message)                                     is passed by reference since it will be shared with every connection (or client)
                            std::shared_ptr<connection<T>> newConn = std::make_shared<connection<T>>(connection<T>::owner::server,
                            m_context,std::move(socket),m_message);
                            //give user the chance to reject the connection
                            if(onClientConnect(newConn))
                            {
                                m_connectionList.push_back(std::move(newConn));
                                m_connectionList.back()->connectToClient(idCount++);
                                std::cout<<"["<<m_connectionList.back()->getID()<<"] connection approved\n";
                            }
                            else
                            {
                                std::cout<<"[------] connection refused \n";
                            }
                            }
                            else
                            {
                                std::cout<<"[SERVER] New client error: "<<ec.message()<<std::endl;
                            }
                            //prime the asio context to wait again for next connection
                            waitForClientConnection();
                            });
                }
                //send message to specific client
                void messageClient(std::shared_ptr<connection<T>> client,const message<T> &msg)
                {
                    if(client && client->isConnected())
                    {
                        client->send(msg);
                    }
                    else
                    {
                        onClientDisconnect(client);
                        client.reset();
                        m_connectionList.erase(
                                std::remove(m_connectionList.begin(),m_connectionList.end(),client),m_connectionList.end());
                    }
                }

                //broadcast to all client
                void messageAllClient(const message<T> &msg,std::shared_ptr<connection<T>> pIgnoreClient = nullptr)
                {
                    bool bInvalidClientFound = false;
                    std::cout<<"Test:"<<msg<<std::endl;
                    for(auto &client:m_connectionList)
                    {
                        if(client && client->isConnected())
                        {
                            std::cout<<"valid client:"<<client->getID()<<std::endl;
                            if(client!=pIgnoreClient)
                                client->send(msg);
                        }
                        else
                        {
                            //assume this client has disconnected 
                            onClientDisconnect(client);
                            client.reset();
                            bInvalidClientFound = true;
                        }
                    }

                    if(bInvalidClientFound)
                    {
                        m_connectionList.erase(
                                std::remove(m_connectionList.begin(),m_connectionList.end(),nullptr),m_connectionList.end());                    
                    }
                }
                //called by the user to explicitly process some of the messages in the queue
                //size_t as -1 sets it to max
                void update(size_t maxMessages=-1)
                {
                    size_t messageCount = 0;
                    while(messageCount < maxMessages && !m_message.empty())
                    {
                        auto msg = m_message.pop_front();
                        onMessage(msg.remote,msg.msg);
                        messageCount++;
                    }
                }
            protected:
                virtual bool onClientConnect(std::shared_ptr<connection<T>> client)
                {
                    std::cout<<"server\n";
                    return false;
                }	
                virtual void onClientDisconnect(std::shared_ptr<connection<T>> client)
                {

                }
                virtual void onMessage(std::shared_ptr<connection<T>> client,message<T> &msg)
                {

                }
            protected:
                //server will have its own thread safe queue of messages
                Queue<owned_message<T>> m_message;
                //list of all connections with server
                std::deque<std::shared_ptr<connection<T>>> m_connectionList;			
                //order of declaration is important 
                asio::io_context m_context;
                std::thread m_thread;
                //context require these for accepting socket from clients
                asio::ip::tcp::acceptor m_acceptor;
                //clients will be identified based on a unique id, the value is irrelevant as long as they are unique
                uint32_t idCount = 100000;
        };
}
