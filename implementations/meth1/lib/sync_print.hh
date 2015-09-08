#ifndef SYNC_PRINT_HH
#define SYNC_PRINT_HH

/* Wrapper around cout using a mutex to allow printing from multiple threads.
 */

#include <iostream>
#include <mutex>

inline std::ostream & print_one( std::ostream & os )
{
  return os << std::endl;
}

template <class A0, class ...Args>
std::ostream &
print_one( std::ostream & os, const A0 & a0, const Args & ...args )
{
  os << ", " << a0;
  return print_one( os, args... );
}

template <class A0, class ...Args>
std::ostream &
print( std::ostream & os, const A0 a0, const Args & ...args )
{
  static std::mutex m;
  std::lock_guard<std::mutex> _(m);
  return print_one( os << a0, args... );
}

template <class A0, class ...Args>
std::ostream &
print( const A0 a0, const Args & ...args )
{
  return print( std::cout, a0, args... );
}

#endif /* SYNC_PRINT_HH */
