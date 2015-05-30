#ifndef EVENT_LOOP_HH
#define EVENT_LOOP_HH

#include <vector>
#include <functional>

#include "child_process.hh"
#include "iodevice.hh"
#include "poller.hh"
#include "signalfd.hh"
#include "util.hh"

/* EventLoop provides a event loop implementation. It abstracts over some of
 * the details of 'Poller', providing a simple way to fire events when file
 * descriptors become read-ready. It also allows child processes to be
 * associated with the event loop, which means that the children and parent
 * process share the same fate (i.e., exit together).
 */
class EventLoop
{
private:
  SignalMask signals_;
  Poller poller_;
  std::vector<ChildProcess> child_processes_;
  PollerShortNames::Result handle_signal( const signalfd_siginfo & sig );

public:
  /* Construct a new event loop. */
  EventLoop();

  /* Add an input event handler to the event loop for the file descriptor
   * specified. */
  void
  add_simple_input_handler( const IODevice & io,
                            const Poller::Action::CallbackType & callback )
  {
    poller_.add_action(
      Poller::Action( io, Poller::Action::PollDirection::In, callback ) );
  }

  /* Add a child process to the event loop. */
  template <typename... Targs> void add_child_process( Targs &&... Fargs )
  {
    child_processes_.emplace_back( std::forward<Targs>( Fargs )... );
  }

  /* Run the event loop, looping until an an event handler returns 'Exit'. */
  int loop( void );

  /* Destroy the event loop */
  virtual ~EventLoop() {}
};

#endif /* EVENT_LOOP_HH */
