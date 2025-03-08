#pragma once
#include "net_common.h"
#include "net_queue.h"
#include "net_message.h"
namespace net
{
    //forward declare for the server ptr in connectToClient
    template<typename T>
    class server;

    template<typename T>
        class connection : public std::enable_shared_from_this<connection<T>>//helps us use this instance with shared_ptr and avoiding having 
    {                                                                       //different refernce count as you would if made multiple shared_ptr 
        public:                                                              // via a raw ptr such as *this
            enum class owner
            {
                server,
                client
            };

        public:
            connection(owner parent,asio::io_context &context,asio::ip::tcp::socket socket,Queue<owned_message<T>>& qIn)
                : m_context{context},m_socket{std::move(socket)},m_messages_in{qIn}
            {
                m_owner = parent;
                if(m_owner == owner::server)
                {
                    m_handshake_out = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());
                    m_handshake_check = scramble(m_handshake_out);
                }
                else
                {
                    m_handshake_out = 0;
                    m_handshake_check = 0;
                }
            }
            virtual ~connection()
            {}

        public:
            void connectToServer(asio::ip::tcp::resolver::results_type &endpoints)//instead of using asio::ip::tcp::resolver::result_types use 
            {
                if(m_owner == owner::client)// tcp::endpoint -> cannot since the endpoint recieved from resolver.resolve is above type
                {
                    asio::async_connect(m_socket,endpoints,
                            [this](std::error_code ec,asio::ip::tcp::endpoint endpoint)
                            {
                                if(!ec)
                                {
                                    // was : readHeader();
                                    // first thing server will do is send packet to client to validate so first thing client will do is read it
                                    readValidation();
                                }
                            });
                }
            }
            void disconnect()
            {
                if(isConnected())
                    asio::post(m_context,[this](){m_socket.close();});
            }
            bool isConnected()const
            {
                return m_socket.is_open();
            }
            void connectToClient(net::server<T>* server,uint32_t id = 0)
            {
                if(m_owner == owner::server)
                {
                    if(isConnected())
                    {
                        this->id = id;
                        //this should happen whenever it can happen so we prime the context when a client is connected
                        // was : readHeader();
                        // server will immediately send validation handshake 
                        writeValidation();
                        // will read it after sending
                        readValidation(server);
                    }
                }
            }
        public:
            void send(const message<T>& msg)
            {
                asio::post(m_context,[this,msg]()
                        {
                            bool isAlreadyWriting = !m_messages_out.empty();
						// If the queue has a message in it, then we must 
						// assume that it is in the process of asynchronously being written.
						// Either way add the message to the queue to be output. If no messages
						// were available to be written, then start the process of writing the
						// message at the front of the queue.                            
                            m_messages_out.push_back(msg);
                            if(!isAlreadyWriting)
                                writeHeader();
                        });
            }
            //All ASYNC functions
            void readValidation(net::server<T>* server = nullptr) // ground work for derived classes
            {
					asio::async_read(m_socket,asio::buffer(&m_handshake_in,sizeof(uint64_t)),
						[this,server](std::error_code ec,std::size_t length)
							{
								if(!ec)
								{
									if(m_owner==owner::server)
									{
							            //check the validation data recieved from the client
                                        std::cout<<"client: "<<m_handshake_in<<" server: "<<m_handshake_check<<std::endl;
                                        if(m_handshake_in == m_handshake_check)
                                        {
                                            std::cout<<"Client Validated \n";
                                            server->onClientValidated(this->shared_from_this());
                                            // prime for the next job

                                            readHeader();
                                        }
                                        else
                                        {
                                            std::cout<<"Client Rejected (Failed Validation): "<<ec.message()<<std::endl;
                                            m_socket.close();
                                        }
									}
									else
                                    {
                                        // Connection is client try is solve the handshake
                                        m_handshake_out=scramble(m_handshake_in);
                                        writeValidation();
                                    }
								}
								else
								{
									std::cout<<"Client Disconnected (Read Validation): "<<ec.message()<<std::endl;
									m_socket.close();
								}
							});                
            }
            void readHeader()
            {
                asio::async_read(m_socket,asio::buffer(&m_msgTempIn.header,sizeof(message_header<T>)),
                            [this](std::error_code ec,std::size_t length)
                            {
                                if(!ec)
                                {
                                    if(m_msgTempIn.header.size > 8)
                                    {
                                        m_msgTempIn.body.resize(m_msgTempIn.header.size-8);
                                        readBody();
                                    }
                                    else
                                    {
                                        addToIncomingMsgQ();
                                    }
                                }
                                else
                                {
                                    std::cout<<"FATAL: Failed to read header ["<<id<<"]"<<ec.message()<<"\n";
                                    //closing the socket will be detected by the server/client when communicating in future since something like                                    client->isConnected will return false and in that condition clean up is handled
                                    m_socket.close();
                                }
                            });
            }
            void readBody()
            {
                asio::async_read(m_socket,asio::buffer(m_msgTempIn.body.data(),m_msgTempIn.body.size()),
                        [this](std::error_code ec,std::size_t length)
                        {
                            if(!ec)
                            {
                                addToIncomingMsgQ();
                            }
                            else
                            {
                                    std::cout<<"FATAL: Failed to read body ["<<id<<"]"<<ec.message()<<"\n";
                                    m_socket.close();
                            }
                        });
            }
            //ASYNC, these wont sit and wait for something to happen
            void writeValidation()
            {
                asio::async_write(m_socket,asio::buffer(&m_handshake_out,sizeof(uint64_t)),
                            [this](std::error_code ec,std::size_t length)
                                {
                                    if(!ec)
                                    {
                                        if(m_owner == owner::client)
                                            readHeader();
                                            
                                    }
                                    else
                                    {
                                        std::cout<<"Server rejected (Write Validation): "<<ec.message()<<std::endl;
                                        m_socket.close();
                                    }
                                });
            }
            void writeHeader()
            {
                asio::async_write(m_socket,asio::buffer(&m_messages_out.front().header,sizeof(message_header<T>)),
                        [this](std::error_code ec,std::size_t length)
                        {
                            if(!ec)
                            {
                               if(m_messages_out.front().body.size()>0)
                               {
                                    writeBody();
                               }
                               else
                               {
                                    m_messages_out.pop_front();
                                    if(!m_messages_out.empty())
                                        writeBody();
                               }
                            }
                            else
                            {
                                 std::cout<<"FATAL: Failed to write header ["<<id<<"]"<<ec.message()<<"\n";
                                 m_socket.close();
                            }
                        });
            }
            void writeBody()
            {
                asio::async_write(m_socket,asio::buffer(&m_messages_out.front().header,sizeof(message_header<T>)),
                            [this](std::error_code ec,std::size_t length)                                                                                                               {
                                if(!ec)
                                {
                                    m_messages_out.pop_front();
                                    //if the queue still has messages left begin writing again from header
                                    if(!m_messages_out.empty())
                                        writeHeader();
                                }
                                else
                                {
                                    std::cout<<"FATAL: Failed to write body ["<<id<<"]"<<ec.message()<<"\n";
                                    m_socket.close();
                                }
                            });                
            }
            void addToIncomingMsgQ()
            {
                if(m_owner==owner::server)
                {
                    m_messages_in.push_back({this->shared_from_this(),m_msgTempIn});//this constructs a owned_message struct
                }
                else
                    m_messages_in.push_back({nullptr,m_msgTempIn});//client need not have a tagged connection since it will only have one 
                                                                   //connection
                readHeader();
            }

            uint64_t scramble(uint64_t nInput)
            {
                uint64_t out = nInput ^ 0xDEADBEEFC0DECAFE; //constant
                out = (out & 0xF0F0F0F0F0F0F0) >> 4 | (out & 0xF0F0F0F0F0F0F0) << 4;
                return out ^ 0xC0DEFACE12345678; // some constant can be a version so old client wont communicate 
            }
            uint32_t getID()const{return id;}
        protected:
            asio::ip::tcp::socket m_socket;
            //this context will be shared with the whole asio instance 
            asio::io_context &m_context;
            //Will hold all the outgoing messages
            Queue<message<T>> m_messages_out;
            //This queue holds all the messages that have been recieved from the remote side of this connection.its a reference this will be                provided by the server or the client
            Queue<owned_message<T>> &m_messages_in;
            //temporary message read from client via socket
            message<T> m_msgTempIn;
            owner m_owner;
            uint32_t id;

            //handshake validation

            uint64_t m_handshake_out = 0; //what the connection will be sending outwards
            uint64_t m_handshake_in = 0; //what the connection has recieved 
            uint64_t m_handshake_check = 0;// will be used by the server to perform the comparison 
    };
}
