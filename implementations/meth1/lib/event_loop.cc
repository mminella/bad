#include "event_loop.hh"
#include "exception.hh"
#include "privs.hh"

using namespace std;
using namespace PollerShortNames;

/* Construct a new event loop. */
EventLoop::EventLoop()
  : poller_()
{
}

/* Run the event loop, looping until an an event handler returns 'Exit'. */
int EventLoop::loop( void )
{
  TemporarilyUnprivileged tu;

  while ( true ) {
    const auto poll_result = poller_.poll( -1 );
    if ( poll_result.result == Poller::Result::Type::Exit ) {
      return poll_result.exit_status;
    }
  }
}
