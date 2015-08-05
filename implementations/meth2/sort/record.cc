#include <iomanip>
#include <iostream>

#include "record.hh"

using namespace std;

ostream & operator<<( ostream & o, const Record & r )
{
  o << hex;
  for ( unsigned int i = 0; i < Record::KEY_LEN; ++i ) {
    o << setfill('0') << setw(2) << (int) r.key()[i];
  }
  o << dec;
  return o;
}
