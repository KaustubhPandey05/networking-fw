#pragma once
#include <cassert>
#include <cstdlib>
#include <memory>


template <typename T, typename Alloc=std::allocator<T>>

class CQ
{
public:
    using value_type=T;
    using allocator_traits=std::allocator_traits<Alloc>;
    using size_type=typename allocator_traits::size_type;
    explicit CQ( size_type capacity, Alloc const& alloc=Alloc{})
        :Alloc{alloc},
        capacity_{capacity},
        ring{allocator_traits::allocate(*this,capacity_)}
    {}
    CQ(const CQ&)=delete;
    CQ(CQ&&)=delete;
    CQ& operator=(const CQ&)=delete;
    CQ& operator=(CQ&&)=delete;


    ~CQ()
    {
        while (not empty())
        {
            ring[popcursor%capacity_].~T();
            ++popcursor;
        }
        allocator_traits::deallocate(*this,ring,capacity_);
    }

    auto size() const noexcept
    {   
        assert(popcursor <= pushcursor);
        return pushcursor - popcursor;
    }
    
    auto empty() const noexcept {return size()==0; }
    auto full() const noexcept { return size()==capacity(); }
    auto capacity() const noexcept { return capacity_; }


    //pushing object into the queue
    auto push(T const &value)
    {
        if(full())
        {
            return false;
        }
        new (&ring[pushcursor%capacity_]) T(value);
        ++pushcursor;
        return true;
    }

    //poping object out of queue

    auto pop (T &value)
    {
        if(empty())
            return false;
        value=ring[popcursor%capacity_];
        ring[popcursor%capacity_].~T();
        ++popcursor;
        return true;
    }
    
private:
    size_type capacity_;
    T* ring;
    using CursorType = std::atomic<size_type>;
    static_assert(CursorType::is_always_lock_free);
    CursorType pushcursor{};
    CursorType popcursor{};
};
