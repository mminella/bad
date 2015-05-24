#ifndef POLLER_HH
#define POLLER_HH

#include <functional>
#include <vector>

#include <poll.h>

#include "iodevice.hh"

class Poller
{
public:
  struct Action {
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

private:
  std::vector<Action> actions_;
  std::vector<pollfd> pollfds_;

public:
  struct Result {
    enum class Type { Success, Timeout, Exit } result;
    unsigned int exit_status;
    Result( const Type & s_result,
            const unsigned int & s_status = EXIT_SUCCESS )
      : result( s_result )
      , exit_status( s_status )
    {
    }
  };

  Poller()
    : actions_()
    , pollfds_()
  {
  }
  void add_action( Action action );
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
