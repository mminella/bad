#ifndef CHILD_PROCESS_HH
#define CHILD_PROCESS_HH

#include <functional>
#include <unistd.h>
#include <cassert>
#include <csignal>

/* object-oriented wrapper for handling Unix child processes */
class ChildProcess
{
private:
  std::string name_;
  pid_t pid_;
  bool running_, terminated_;
  int exit_status_;
  bool died_on_signal_;
  int graceful_termination_signal_;

  /* has this child process been moved (C++ move constructor)? */
  bool moved_away_;

public:
  /* Create a new UNIX process that runs the supplied lambda. The return value
   * of the lambda is the child's exit status */
  ChildProcess( const std::string & name,
                std::function<int()> && child_procedure,
                const bool new_namespace = false,
                const int termination_signal = SIGHUP );

  /* is process in a waitable state? */
  bool waitable( void ) const;

  /* wait for process to change state */
  void wait( const bool nonblocking = false );

  /* send signal */
  void signal( const int sig );

  /* send SIGCONT */
  void resume( void );

  /* Return the processes name */
  const std::string & name( void ) const
  {
    assert( not moved_away_ );
    return name_;
  }

  /* Return the processes pid */
  pid_t pid( void ) const
  {
    assert( not moved_away_ );
    return pid_;
  }

  /* Is the process still running? */
  bool running( void ) const
  {
    assert( not moved_away_ );
    return running_;
  }

  /* Has the process terminated? */
  bool terminated( void ) const
  {
    assert( not moved_away_ );
    return terminated_;
  }

  /* Return exit status or signal that killed process */
  bool died_on_signal( void ) const
  {
    assert( not moved_away_ );
    assert( terminated_ );
    return died_on_signal_;
  }

  /* Return process exit status. Should only be called after checking the
   * process has indeed terminated! */
  int exit_status( void ) const
  {
    assert( not moved_away_ );
    assert( terminated_ );
    return exit_status_;
  }

  void throw_exception( void ) const;

  /* destructor */
  ~ChildProcess();

  /* ban copying */
  ChildProcess( const ChildProcess & other ) = delete;
  ChildProcess & operator=( const ChildProcess & other ) = delete;

  /* allow move constructor */
  ChildProcess( ChildProcess && other );

  /* ... but not move assignment operator */
  ChildProcess & operator=( ChildProcess && other ) = delete;
};

#endif /* CHILD_PROCESS_HH */
