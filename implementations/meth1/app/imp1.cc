#include <fcntl.h>

#include <algorithm>
#include <vector>

#include "exception.hh"
#include "file.hh"
#include "util.hh"

#include "record.hh"

using namespace std;

int run( int argc, char * argv[] );

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc != 2 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) + " [file]" );
  }
}

int main( int argc, char * argv[] )
{
  try {
    run( argc, argv );
  } catch ( const exception & e ) {
    print_exception( e );
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

Record linear_scan( File & in, Record & after )
{
  Record min { Record::MAX };

  for ( uint64_t i = 0; ; i++ ) {
    Record next{ i, in.read( Record::SIZE ), false };
    if (in.eof()) {
      break;
    } else if ( after <= next && next < min ) {
      // we check the offset to ensure we don't pick up same key, but can still
      // handle duplicate keys.
      if ( after.offset() != next.offset() ) {
        min = next.clone();
      }
    }
  }


  return min;
}

Record next_record( File & in, Record & after )
{
  Record r = linear_scan( in, after );
  cout << "Min was: " << r.offset() << endl;
  in.rewind();
  return r;
}

int run( int argc, char * argv[] )
{
  sanity_check_env( argc );
  check_usage( argc, argv );

  File fdi( argv[1], O_RDONLY );
  Record after { Record::MIN };

  after = next_record( fdi, after );
  after = next_record( fdi, after );
  after = next_record( fdi, after );
  after = next_record( fdi, after );
  after = next_record( fdi, after );
  after = next_record( fdi, after );

  return EXIT_SUCCESS;
}
