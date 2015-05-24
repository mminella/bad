#ifndef EVENT_LOOP_HH
#define EVENT_LOOP_HH

#include <vector>
#include <functional>

#include "child_process.hh"
#include "iodevice.hh"
#include "poller.hh"
#include "signalfd.hh"
#include "util.hh"

class EventLoop
{
private:
  SignalMask signals_;
  Poller poller_;
  std::vector<ChildProcess> child_processes_;
  PollerShortNames::Result handle_signal( const signalfd_siginfo & sig );

protected:
  void add_action( Poller::Action action ) { poller_.add_action( action ); }

  int internal_loop( const std::function<int(void)> & wait_time );

public:
  EventLoop();

  void
  add_simple_input_handler( const IODevice & io,
                            const Poller::Action::CallbackType & callback );

  template <typename... Targs> void add_child_process( Targs &&... Fargs )
  {
    child_processes_.emplace_back( std::forward<Targs>( Fargs )... );
  }

  int loop( void )
  {
    return internal_loop( []() { return -1; } );
  } /* no timeout */

  virtual ~EventLoop() {}
};

#endif /* EVENT_LOOP_HH */
