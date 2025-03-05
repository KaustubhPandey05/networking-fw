#pragma once
#include <net_common.h>
namespace olc
{
    namespace net
    {
        template<typename T>
        class Queue
        {
         public:
            Queue()=default;
            Queue(const Queue<T>&)=delete;
         public:
            const T& front()
            {
                std::scoped_lock lock(muxQueue);
                return deqQueue.front();
            }
            const T& back()
            {
                std::scoped_lock lock(muxQueue);
                return deqQueue.back();                
            }
            size_t count()
            {
                std::scoped_lock lock(muxQueue);
                return deqQueue.size();
            }
            void clear()
            {
                std::scoped_lock lock(muxQueue);
                return deqQueue.clear();    
            }
         protected:
             std::mutex muxQueue;
             std::deque<T> deqQueue;
        }
    }
}
