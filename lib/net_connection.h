#include "net_common.h"
#include "net_queue.h"
#include "net_message.h"
namespace net
{
    template<typename T>
        class connection : public std::enable_shared_from_this<connection<T>>
    {
        public:
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
            }
            virtual ~connection()
            {}

        public:
            bool connectToServer();
            bool dissconnect();
            bool isConnected()const
            {
                return m_socket.is_open();
            }
            void connectToClient(uint32_t id = 0)
            {
                if(m_owner == owner::server)
                {
                    if(isConnected())
                    {
                        this->id = id;
                    }
                }
            }
        public:
            bool send(const message<T>& msg);
            uint32_t getID()const{return id;}
        protected:
            asio::ip::tcp::socket m_socket;
            //this context will be shared with the whole asio instance 
            asio::io_context &m_context;
            //Will hold all the outgoing messages
            Queue<message<T>> m_messages_out;
            //This queue holds all the messages that have been recieved from the remote side of this connection.its a reference this will be                provided by the server or the client
            Queue<owned_message<T>> &m_messages_in;
            owner m_owner;
            uint32_t id;
    };
}
