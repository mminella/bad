#include <iomanip>
#include <iostream>

#include "record_common.hh"
#include "record_t.hh"

using namespace std;

ostream & operator<<( ostream & o, const Record & r )
{
  o << hex;
  for ( unsigned int i = 0; i < Rec::KEY_LEN; ++i ) {
    o << setfill('0') << setw(2) << (int) r.key()[i];
  }
  o << dec;
  return o;
}
