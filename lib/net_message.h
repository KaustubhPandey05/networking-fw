#pragma once
#include "net_common.h"
    namespace net
    {
        template <typename T>
        struct message_header
        {
            T id{};
            uint32_t size = 0;
        };
        template <typename T>
        struct message
        {
            message_header<T> header{};
            std::vector<uint8_t> body;//since its of 8bits we are always working with bytes i.e

            size_t size() const       //size = size bytes
            {
                return sizeof(header)+body.size();//header can be replaced with message_header<T>
            }
            
            friend std::ostream& operator<<(std::ostream &out,const message &msg)
            {
                out<<"ID: "<<static_cast<int>(msg.header.id)<<" Size: "<<msg.header.size;
                return out;
            }
            template<typename DataType>
            friend message<T> &operator<<(message<T> &msg,const DataType &data)
            {
                static_assert(std::is_standard_layout<DataType>::value,"Data is too complex to be pushed into vector");
                size_t i = msg.body.size();
                //resize the vector by the size of data being pushed
                msg.body.resize(msg.body.size()+sizeof(DataType));
                //physicall copy the data into newly allocated vector mem
                std::memcpy(msg.body.data()+i,&data,sizeof(DataType));
                msg.header.size = msg.size();
                return msg;
            }
            template<typename DataType>
            friend message<T>& operator>>(message& msg,DataType& data)
            {
                static_assert(std::is_standard_layout<DataType>::value,"Data is too complex to be poped out of vector");
                //this is assuming vector is behaving like a stack
                size_t i = msg.body.size()-sizeof(DataType);
                std::memcpy(&data,msg.body.data()+i,sizeof(DataType));
                msg.body.resize(i);
                return msg;
            }
        };
		// An "owned" message is identical to a regular message, but it is associated with
		// a connection. On a server, the owner would be the client that sent the message, 
		// on a client the owner would be the server.

		// Forward declare the connection
		template <typename T>
		class connection;

		template <typename T>
		struct owned_message
		{
			std::shared_ptr<connection<T>> remote = nullptr;
			message<T> msg;

			// Again, a friendly string maker
			friend std::ostream& operator<<(std::ostream& os, const owned_message<T>& msg)
			{
				os << msg.msg;
				return os;
			}
		};        
    }
