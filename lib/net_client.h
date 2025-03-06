#pragma once
#include "net_common.h"
#include "net_message.h"
#include "net_queue.h"
#include "net_connection.h"
    namespace net
    {
        template<typename T>
        class client
        {
         public:
            client():m_socket(m_context)
            {}
            ~client()
            {
                disconnect();
            }
         public:
            bool connect(const std::string& host,const uint16_t port)
            {
                try
                {
                    //create connection object
                    m_connection=std::make_unique<connection<T>>();
                    asio::ip::tcp::resolver resolver(m_context);
                    m_endpoints = resolver.resolve(host,std::to_string(port));
                    m_connection->connectToServer(m_endpoints);
                    thread_context = std::thread([this](){m_context.run();});

                }
                catch (std::exception& e)
                {
                    std::cerr<<"Client exception: "<<e.what()<<std::endl;
                    return false;
                }
                return true;
            }
            void disconnect()
            {
                if(isConnected())
                    m_connection.dissconnect();
                m_context.stop();
                if(thread_context.joinable())
                    thread_context.join();
                m_connection.release();
            }
            bool isConnected()const
            {
                if(m_connection)
                    return m_connection.isConnected();
                return false;
            }

            Queue<owned_message<T>>& Incoming()
            {
                return m_messages_in;
            }
         protected:
            asio::io_context m_context;
            std::thread thread_context;
            asio::ip::tcp::socket m_socket;
            std::unique_ptr<connection<T>> m_connection;
         private:
            Queue<owned_message<T>> m_messages_in;
        };
    }
