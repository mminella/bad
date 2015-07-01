#ifndef EVENT_LOOP_HH
#define EVENT_LOOP_HH

#include <vector>
#include <functional>

#include "file_descriptor.hh"
#include "poller.hh"
#include "util.hh"

/* EventLoop provides a event loop implementation. It abstracts over some of
 * the details of 'Poller', providing a simple way to fire events when file
 * descriptors become read-ready.
 */
class EventLoop
{
private:
  Poller poller_;

public:
  /* Construct a new event loop. */
  EventLoop();

  /* Add an input event handler to the event loop for the file descriptor
   * specified. */
  void
  add_simple_input_handler( const FileDescriptor & fd,
                            const Poller::Action::CallbackType & callback )
  {
    poller_.add_action(
      Poller::Action( fd, Poller::Action::PollDirection::In, callback ) );
  }

  /* Run the event loop, looping until an an event handler returns 'Exit'. */
  int loop( void );
};

#endif /* EVENT_LOOP_HH */
