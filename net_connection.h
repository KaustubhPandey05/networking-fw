#include "net_common.h"
#include "net_queue.h"
#include "net_message.h"
namespace olc
{
    namespace net
    {
        template<typename T>
        class connection : public std::enable_shared_from_this<connection<T>>
        {
         public:
            connection()
            {}
            virtual ~connection()
            {}
            
         public:
            bool connectToServer();
            bool dissconnect();
            bool isConnected()const;

         public:
            bool send(const message<T>& msg);
            
         protected:
            asio::ip::tcp::socket m_socket;
            //this context will be shared with the whole asio instance 
            asio::io_context &m_context;
            //Will hold all the outgoing messages
            Queue<message<T>> &m_messages_out;
            //This queue holds all the messages that have been recieved from the remote side of this connection.its a reference this will be provided by the              server or the client
            Queue<message<T>> &m_messages_in;
        }
    }
}

