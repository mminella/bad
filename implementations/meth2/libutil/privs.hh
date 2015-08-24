#ifndef PRIVS_HH
#define PRIVS_HH

#include <cstring>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

/* Clear user environ. Should be done when running as root. */
char ** clear_environ( void );

/* Drop root privileges */
void drop_privileges( void );

/* Verify running as euid root, but not ruid root */
void check_suid_root( const std::string & prog );

/* Assert currently not running with root priveleges */
void assert_not_root( void );

/* Cause a lexical scope to run without root privileges. Values of this class
 * should be allocated on the stack or with a controlled lifetime (i.e., stack)
 * as when the class is deallocated, privileges are restored. */
class TemporarilyUnprivileged
{
private:
  const uid_t orig_euid;
  const gid_t orig_egid;

public:
  TemporarilyUnprivileged();
  ~TemporarilyUnprivileged();
};

#endif /* PRIVS_HH */
