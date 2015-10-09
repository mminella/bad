#ifndef CHANNEL_HH
#define CHANNEL_HH

#include <algorithm>
#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <thread>

#include <condition_variable>
#include <mutex>

template<typename T> class Channel;

/* We hide the inner implementation of channel that shouldn't be moved */
namespace _internal {

  /* A channel provides a buffered communication pipe.
   */
  template <typename T>
  class Channel_
  {
  private:
    friend class Channel<T>;

    class closed_error : public std::exception
    {
      const char * what( void ) const noexcept override
      {
        return "Channel closed!";
      }
    };

    std::mutex mtx_;
    std::condition_variable send_cv_;
    std::condition_variable recv_cv_;
    std::unique_ptr<T[]> slots_;
    const size_t size_;
    size_t used_;
    size_t wptr_;
    size_t rptr_;
    bool closed_;
    size_t send_wait_; // only for synchronous channels
    size_t recv_wait_; // only for synchronous channels

    explicit Channel_( const size_t buf = 1 )
      : mtx_{}
      , send_cv_{}
      , recv_cv_{}
      , slots_{new T[buf > 1 ? buf : 1]}
      , size_{buf}
      , used_{0}
      , wptr_{0}
      , rptr_{0}
      , closed_{false}
      , send_wait_{0}
      , recv_wait_{0}
    {}

    /* No move or copy */
    Channel_( const Channel_ & c ) = delete;
    Channel_ & operator=( const Channel_ & c ) = delete;
    Channel_( Channel_ && c ) = delete;
    Channel_ & operator=( Channel_ && c ) = delete;

    void waitEmpty()
    {
      std::unique_lock<std::mutex> lck( mtx_ );
      while ( used_ > 0 or send_wait_ > 0 ) {
        if ( closed_ )  {
          throw closed_error();
        }
        send_wait_++;
        send_cv_.wait( lck );
        send_wait_--;
        if ( send_wait_ > 0 ) {
          // we consumed one we don't need, so pass onwards
          send_cv_.notify_one();
        }
      }
    }

    void close()
    {
      {
        std::unique_lock<std::mutex> lck( mtx_ );
        closed_ = true;
      }
      send_cv_.notify_all();
      recv_cv_.notify_all();
    }

    template <typename U>
    void send_sync( U && u )
    {
      {
        std::unique_lock<std::mutex> lck( mtx_ );
        send_wait_++;
        send_cv_.wait( lck, [this] {
          return ( recv_wait_ > 0 and used_ == 0 ) or closed_;
        } );
        send_wait_--;
        if ( closed_ ) {
          throw closed_error();
        }
        slots_[0] = std::forward<U>( u );
        used_ = 1;
      }
      recv_cv_.notify_one();
    }

    template <typename U>
    void send_async( U && u )
    {
      {
        std::unique_lock<std::mutex> lck( mtx_ );
        send_cv_.wait( lck, [this] {
          return used_ < size_ or closed_;
        } );
        if ( closed_ ) {
          throw closed_error();
        }
        if ( used_ == size_ ) {
          throw std::runtime_error( "channel is full" );
        } else {
          used_++;
          slots_[wptr_] = std::forward<U>( u );
          wptr_ = (wptr_ + 1) % size_;
        }
      }
      recv_cv_.notify_one();
    }

    template <typename U>
    void send( U && u )
    {
      if ( size_ == 0 ) {
        send_sync<U>( std::forward<U>( u ) );
      } else {
        send_async<U>( std::forward<U>( u ) );
      }
    }

    T recv_sync( void )
    {
      T t;
      bool swait = false;
      {
        std::unique_lock<std::mutex> lck( mtx_ );
        recv_wait_++;
        if ( send_wait_ > 0 ) {
          lck.unlock();
          send_cv_.notify_one();
          lck.lock();
        }
        recv_cv_.wait( lck, [this]{ return used_ > 0 or closed_; } );
        recv_wait_--;
        if ( closed_ ) {
          throw closed_error();
        }
        used_ = 0;
        t = std::move( slots_[0] );
        swait = send_wait_ > 0;
      }
      if ( swait ) { send_cv_.notify_one(); }
      return t;
    }

    T recv_async( void )
    {
      T t;
      {
        std::unique_lock<std::mutex> lck( mtx_ );
        recv_cv_.wait( lck, [this] {
          return used_ > 0 or closed_;
        } );
        if ( closed_ ) {
          throw closed_error();
        }
        if ( used_ == size_t( 0 ) ) {
          throw std::runtime_error( "channel is empty" );
        } else {
          used_--;
          size_t i = rptr_;
          rptr_ = (rptr_ + 1) % size_;
          t = std::move( slots_[i] );
        }
      }
      send_cv_.notify_one();
      return t;
    }

    T recv( void )
    {
      if ( size_ == 0 ) {
        return recv_sync();
      } else {
        return recv_async();
      }
    }
  };
}

template<typename T>
class Channel
{
private:
  std::shared_ptr<_internal::Channel_<T>> chn;

public:
    using closed_error = typename _internal::Channel_<T>::closed_error;

    explicit Channel( size_t buf = 1 ) : chn{new _internal::Channel_<T>{buf}} {}

    void close( void ) { chn->close(); }
    void waitEmpty( void ) { chn->waitEmpty(); }
    void send( const T & t ) { chn->send( t ); }
    void send( T && t ) { chn->send( std::move( t ) ); }
    T recv( void ) { return chn->recv(); }
};

#endif /* CHANNEL_HH */
