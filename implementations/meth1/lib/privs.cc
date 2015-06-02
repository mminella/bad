#include <fcntl.h>
#include <pwd.h>
#include <unistd.h>

#include <numeric>
#include <vector>

#include "exception.hh"
#include "file_descriptor.hh"
#include "privs.hh"
#include "util.hh"

using namespace std;

/* Clear user environ. Should be done when running as root. */
char ** clear_environ( void )
{
  char ** const user_environment = environ;
  environ = nullptr;
  return user_environment;
}

/* Drop root privileges.
 *
 * Adapted from "Secure Programming Cookbook for C and C++: Recipes for
 * Cryptography, Authentication, Input Validation & More" - John Viega and Matt
 * Messier */
void drop_privileges( void )
{
  gid_t real_gid = getgid(), eff_gid = getegid();
  uid_t real_uid = getuid(), eff_uid = geteuid();

  /* change real group id if necessary */
  if ( real_gid != eff_gid ) {
    SystemCall( "setregid", setregid( real_gid, real_gid ) );
  }

  /* change real user id if necessary */
  if ( real_uid != eff_uid ) {
    SystemCall( "setreuid", setreuid( real_uid, real_uid ) );
  }

  /* verify that the changes were successful. if not, abort */
  if ( real_gid != eff_gid and
       ( setegid( eff_gid ) != -1 or getegid() != real_gid ) ) {
    cerr << "BUG: dropping privileged gid failed" << endl;
    _exit( EXIT_FAILURE );
  }

  if ( real_uid != eff_uid and
       ( seteuid( eff_uid ) != -1 or geteuid() != real_uid ) ) {
    cerr << "BUG: dropping privileged uid failed" << endl;
    _exit( EXIT_FAILURE );
  }
}

/* verify running as euid root, but not ruid root */
void check_suid_root( const string & prog )
{
  if ( geteuid() != 0 ) {
    throw runtime_error( prog + ": needs to be installed setuid root" );
  }

  if ( getuid() == 0 or getgid() == 0 ) {
    throw runtime_error( prog + ": please run as non-root" );
  }

  /* verify environment has been cleared */
  if ( environ ) {
    throw runtime_error( "BUG: environment not cleared in sensitive region" );
  }
}

/* Assert currently not running with root priveleges */
void assert_not_root( void )
{
  if ( ( geteuid() == 0 ) or ( getegid() == 0 ) ) {
    throw runtime_error( "BUG: privileges not dropped in sensitive region" );
  }
}

/* Cause a lexical scope to run without root privileges. Values of this class
 * should be allocated on the stack or with a controlled lifetime (i.e., stack)
 * as when the class is deallocated, privileges are restored. */
TemporarilyUnprivileged::TemporarilyUnprivileged()
  : orig_euid( geteuid() )
  , orig_egid( getegid() )
{
  SystemCall( "setegid", setegid( getgid() ) );
  SystemCall( "seteuid", seteuid( getuid() ) );

  assert_not_root();
}

TemporarilyUnprivileged::~TemporarilyUnprivileged()
{
  SystemCall( "seteuid", seteuid( orig_euid ) );
  SystemCall( "setegid", setegid( orig_egid ) );
}
