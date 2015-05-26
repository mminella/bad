#include <fcntl.h>

#include <algorithm>
#include <vector>

#include "exception.hh"
#include "file.hh"
#include "util.hh"

using namespace std;

constexpr auto KEY_LEN = 10;
constexpr auto VAL_LEN = 90;

struct record {
  record( const char * s );
  string str( void );
  uint8_t key[KEY_LEN];
  uint8_t value[VAL_LEN];
};

record::record( const char * s )
{
  memcpy( key, s, KEY_LEN );
  memcpy( value, s, VAL_LEN );
}

string record::str( void )
{
  char buf[KEY_LEN + VAL_LEN];
  memcpy( buf, key, KEY_LEN );
  memcpy( buf + KEY_LEN, value, VAL_LEN );
  return string( buf, KEY_LEN + VAL_LEN );
}

bool operator<( const record & a, const record & b )
{
  return memcmp( a.key, b.key, KEY_LEN ) < 0 ? true : false;
}

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

  vector<record> recs{};

  for ( ;; ) {
    string r = fdi.read( sizeof( record ) );
    if ( fdi.eof() ) {
      break;
    }
    recs.push_back( r.c_str() );
  }

  sort( recs.begin(), recs.end() );

  for ( auto & r : recs ) {
    fdo.write( r.str() );
  }

  cout << "Read " << recs.size() << " records off disk." << endl;

  return EXIT_SUCCESS;
}
