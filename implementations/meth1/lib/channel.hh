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

/* We hide the inner implementation of channel that shouldn't be moved */
namespace internal {

  /* A channel provides a buffered communication pipe.
   */ 
  template <typename T>
  class Channel_
  {
  public:
    class closed_error : public std::exception
    {
      const char * what( void ) const noexcept override
      {
        return "Channel closed!";
      }
    };

  private:
    std::mutex mtx_;
    std::condition_variable send_cv_;
    std::condition_variable recv_cv_;
    std::unique_ptr<T[]> slots_;
    size_t size_;
    size_t used_;
    size_t wptr_;
    size_t rptr_;
    bool closed_;
    size_t send_wait_; // only for synchronous channels
    size_t recv_wait_; // only for synchronous channels

  public:

    Channel_( size_t buf = 1 )
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

    inline void close()
    {
      std::unique_lock<std::mutex> lck( mtx_ );
      closed_ = true;
      send_cv_.notify_all();
      recv_cv_.notify_all();
    }

    template <typename U>
    inline void send( U && u )
    {
      std::unique_lock<std::mutex> lck( mtx_ );
      if ( closed_ ) {
        throw closed_error();
      }

      if ( size_ == 0 ) {
        // synchronous
        while ( true ) {
          if ( recv_wait_ == 0 || used_ == 1 ) {
            send_wait_++;
            send_cv_.wait( lck );
            if ( closed_ ) {
              throw closed_error();
            }
            send_wait_--;
          } else {
            slots_[0] = std::forward<U>( u );
            used_ = 1;
            recv_cv_.notify_one();
            return;
          }
        }
      } else {
        // asynchronous
        if ( used_ == size_ ) {
          send_cv_.wait( lck );
          if ( closed_ ) {
            throw closed_error();
          }
        }
        recv_cv_.notify_one();

        used_++;
        slots_[wptr_] = std::forward<U>( u );
        wptr_ = (wptr_ + 1) % size_;
      }
    }

    inline T recv( void )
    {
      std::unique_lock<std::mutex> lck( mtx_ );
      if ( closed_ ) {
        throw closed_error();
      }

      if ( size_ == 0 ) {
        // synchronous
        while ( true ) {
          if ( used_ == 0 ) {
            recv_wait_++;
            if ( send_wait_ > 0 ) {
              send_cv_.notify_one();
            }
            recv_cv_.wait( lck );
            if ( closed_ ) {
              throw closed_error();
            }
            recv_wait_--;
          } else {
            used_ = 0;
            return slots_[0];
          }
        }
      } else {
        // asynchronous
        if ( used_ == 0 ) {
          recv_cv_.wait( lck );
          if ( closed_ ) {
            throw closed_error();
          }
        }
        send_cv_.notify_one();
        
        used_--;
        T t = slots_[rptr_];
        rptr_ = (rptr_ + 1) % size_;
        return t;
      }
    }
  };
}

template<typename T>
class Channel
{
private:
  std::shared_ptr<internal::Channel_<T>> chn;

public:
    using closed_error = typename internal::Channel_<T>::closed_error;

    Channel( size_t buf = 1 ) : chn{new internal::Channel_<T>{buf}} {}

    inline void close() { chn->close(); }
    inline void send( T && t ) { chn->send( std::move( t ) ); }
    inline void send( const T & t ) { chn->send( t ); }
    inline T recv( void ) { return chn->recv(); }
};

#endif /* CHANNEL_HH */
