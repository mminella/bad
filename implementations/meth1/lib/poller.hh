#ifndef POLLER_HH
#define POLLER_HH

#include <functional>
#include <vector>

#include <poll.h>

#include "iodevice.hh"

/**
 * Poller provides a poll mechanism for implementing event loops. Event
 * handlers (called an 'Action') for particular file descriptors can be added
 * to a poller (with 'add_action') and then the event loop run in a step-wise
 * fashion (with 'poll').
 */
class Poller
{
public:
  /* An action is a event handler associated with a file descriptor that
   * 'Poller' should run when the file descriptor becomes available. */
  struct Action {

    /* The result of running an action. */
    struct Result {
      enum class Type { Continue, Exit, Cancel } result;
      unsigned int exit_status;
      Result( const Type & s_result = Type::Continue,
              const unsigned int & s_status = EXIT_SUCCESS )
        : result( s_result )
        , exit_status( s_status )
      {
      }
    };

    typedef std::function<Result(void)> CallbackType;

    const IODevice & io;
    enum PollDirection : short { In = POLLIN, Out = POLLOUT } direction;
    CallbackType callback;
    std::function<bool(void)> when_interested;
    bool active;

    /* An action to run when a file descriptor is ready. */
    Action( const IODevice & s_io, const PollDirection & s_direction,
            const CallbackType & s_callback,
            const std::function<bool(void)> & s_when_interested =
              []() { return true; } )
      : io( s_io )
      , direction( s_direction )
      , callback( s_callback )
      , when_interested( s_when_interested )
      , active( true )
    {
    }

    unsigned int service_count( void ) const;
  };

  /* The result of a poll event. */
  struct Result {
    /* Exit type of the action */
    enum class Type { Success, Timeout, Exit } result;

    /* Exit status of the action */
    unsigned int exit_status;

    /* Construct a new result */
    Result( const Type & s_result,
            const unsigned int & s_status = EXIT_SUCCESS )
      : result( s_result )
      , exit_status( s_status )
    {
    }
  };

private:
  std::vector<Action> actions_;
  std::vector<pollfd> pollfds_;

public:
  /* Construct a new poller */
  Poller()
    : actions_()
    , pollfds_()
  {
  }

  /* Add an event driven action to the poller. */
  void add_action( Action action );

  /* Run the poller for a single step. This causes the poller to wait for the
   * next event for any of the file descriptors associated with poller actions.
   * When such an event occurs, the action is run and it's result returned. If
   * a timeout occurs before any event occurs, a 'Timeout' result is returned.
   */
  Result poll( const int & timeout_ms );
};

namespace PollerShortNames
{
typedef Poller::Action::Result Result;
typedef Poller::Action::Result::Type ResultType;
typedef Poller::Action::PollDirection Direction;
typedef Poller::Action Action;
typedef Poller::Result::Type PollResult;
}

#endif /* POLLER_HH */
