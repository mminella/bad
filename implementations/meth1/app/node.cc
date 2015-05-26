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
  if ( argc != 3 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) + " [file] [out]" );
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

int run( int argc, char * argv[] )
{
  sanity_check_env( argc );
  check_usage( argc, argv );

  File fdi( argv[1], O_RDONLY );
  File fdo( argv[2], O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR );

  vector<Record> recs{};

  for ( ;; ) {
    string r = fdi.read( Record::SIZE );
    if ( fdi.eof() ) {
      break;
    }
    recs.push_back( Record( r ) );
  }

  sort( recs.begin(), recs.end() );

  for ( auto & r : recs ) {
    fdo.write( r.str() );
  }

  cout << "Read " << recs.size() << " records off disk." << endl;

  return EXIT_SUCCESS;
}
